#include <stdio.h>
#include <string.h>

#include "board.h"
#include "led.h"
#include "mutex.h"
#include "pir.h"
#include "shell.h"
#include "thread.h"
#include "xtimer.h"

#include "dht.h"
#include "dht_params.h"
#include "saul.h"
#include "hd44780.h"
#include "thread.h"
#include "timex.h"
#include "tm.h"

#include "net/emcute.h"
#include "periph/gpio.h"


#ifndef EMCUTE_ID
#define EMCUTE_ID           ("gertrud")
#endif
#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN - 1)

#define NUMOFSUBS           (1U)
#define TOPIC_MAXLEN        (64U)

#define MQTT_TOPIC_TO_AWS   "iot/1/data"
#define MQTT_TOPIC_FROM_AWS "both_directions"


/*global variables*/

//thread stuff
static char mqtt_handler_stack[THREAD_STACKSIZE_DEFAULT];
static char pir_handler_stack[THREAD_STACKSIZE_MAIN];

//mutex
mutex_t mutex;

//mqtt
static emcute_sub_t subscriptions[NUMOFSUBS];
static char topics[NUMOFSUBS][TOPIC_MAXLEN];

//device resources
dht_t dht_dev;
pir_t pir_dev;
hd44780_t display_dev;

/*sycnchronized resources*/

//message arrays
int th[2];
char msg[32];
char json[32];

float pir_last_awake_s = 0;

int auto_mode = 1;







/* init devices */

/*
 * lcd *
       */ 
int init_lcd(void){ 
    hd44780_params_t display_params = {
    .cols=(16U),
    .rows=(2U),
    .rs=GPIO_PIN(PORT_A,9),
    .rw=GPIO_UNDEF,
    .enable=GPIO_PIN(PORT_C, 7),
    .data={GPIO_PIN(PORT_B, 6),GPIO_PIN(PORT_A, 7),GPIO_PIN(PORT_A, 6),
        GPIO_PIN(PORT_A, 5),GPIO_UNDEF,GPIO_UNDEF,GPIO_UNDEF,GPIO_UNDEF}
    };

    if (hd44780_init(&display_dev, &display_params) == 0) {
        puts("[DISPLAY OK]");
    }else{
        puts("[Failed DISPLAY]");
        return 1;
    }
    return 0;
}

int lcd_write(char msg[]){
    
    // clear screen, reset cursor 
   
    hd44780_clear(&display_dev);
    hd44780_home(&display_dev);
    // write first line 
    
    hd44780_print(&display_dev,msg);
    
    //set cursor to second line and write 
    hd44780_set_cursor(&display_dev, 0, 1);
    // greetings :)
    hd44780_print(&display_dev,"Welcome!");

    return 0;
}

/*
 * dht *
       */
int init_dht(void){
    dht_params_t dht_params ={0};
    
    dht_params.pin=GPIO_PIN(PORT_B, 3);
    dht_params.type=DHT11;
    dht_params.in_mode=GPIO_IN;
    
     if (dht_init(&dht_dev,&dht_params) == 0) {
        puts("[DHT OK]\n");
    }
    else {
        puts("[Failed DHT]");
        return 1;
    }
    return 0;
}

void dht_temp_read(void) {
    
    int16_t temp,hum;
    dht_read(&dht_dev,&temp,&hum);
    
    th[0] = temp;
    th[1] = hum;
    
    return;
}


/*
 * pir *
       */
int init_pir(void){
    pir_params_t pir_params = {0};
    pir_params.gpio = GPIO_PIN(PORT_A,10);
    pir_params.active_high=1;
    if (pir_init(&pir_dev, &pir_params) == 0) {
        puts("[PIR OK]\n");
    }
    else {
        puts("[Failed PIR]");
        return 1;
    }
    return 0;
}


/* mqtt routine*/
static void *emcute_thread(void *arg){
    
    (void)arg;
    emcute_run(CONFIG_EMCUTE_DEFAULT_PORT, EMCUTE_ID);
    return NULL;    //should never be reached 
}

static void on_pub(const emcute_topic_t *topic, void *data, size_t len){
    (void) len;
    char *in = (char *)data;

    printf("### got publication for topic '%s' [%i] ###\n",
           topic->name, (int)topic->id);
    
    if(in[0]=='o' && in[1]=='n'){
        printf("Light it up");
        //critical section start
        mutex_lock(&mutex);
        
        //auto off
        auto_mode=0;
        /// bulb on
        gpio_write(GPIO_PIN(PORT_B,5),1);
        
        mutex_unlock(&mutex);
        //critical section end
                
    }else if(in[0]=='o' && in[1]=='f'&& in[2]=='f'){
         printf("Turn it off");
         //critical section start
        mutex_lock(&mutex);
        
        /// ...and off for now
        gpio_write(GPIO_PIN(PORT_B,5),0);
        
        // auto on
        auto_mode=1;
        
        mutex_unlock(&mutex);
        //critical section end
        
    }else{
        printf("Command not recognized\n");
    }
}


static int mqtt_pub(char *topic,char *data){
    
    emcute_topic_t t;
    unsigned flags = EMCUTE_QOS_0;

    printf("pub with topic: %s and name %s and flags 0x%02x\n",topic,data, (int)flags);

    /* step 1: get topic id */
    t.name = topic;
    if (emcute_reg(&t) != EMCUTE_OK) {
        puts("error: unable to obtain topic ID");
        return 1;
    }

    /* step 2: publish data */
    if (emcute_pub(&t,data, strlen(data), flags) != EMCUTE_OK) {
        printf("error: unable to publish data to topic '%s [%i]'\n",
                t.name, (int)t.id);
        return 1;
    }
    printf("Published %i bytes to topic '%s [%i]'\n",
            (int)strlen(data), t.name, t.id);
    
    return 0;
}

int init_mqtt(void){
    
    /* initialize our subscription buffers */
    memset(subscriptions, 0, (NUMOFSUBS * sizeof(emcute_sub_t)));

    /* start the emcute thread */
    thread_create(mqtt_handler_stack, sizeof(mqtt_handler_stack), EMCUTE_PRIO, 0,
                  emcute_thread, NULL, "emcute");

    // connect to MQTT-SN broker
    printf("Connecting to MQTT-SN broker %s port %d.\n",
           SERVER_ADDR, SERVER_PORT);

    sock_udp_ep_t gw = { .family = AF_INET6, .port = SERVER_PORT };
    char *topic = MQTT_TOPIC_FROM_AWS;
    char *message = "connected";
    size_t len = strlen(message);

    /* parse address */
    if (ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, SERVER_ADDR) == NULL) {
        printf("error parsing IPv6 address\n");
        return 1;
    }

    if (emcute_con(&gw, true, topic, message, len, 0) != EMCUTE_OK) {
        printf("error: unable to connect to [%s]:%i\n", SERVER_ADDR,
               (int)gw.port);
        return 1;
    }

    printf("Successfully connected to gateway at [%s]:%i\n",
           SERVER_ADDR, (int)gw.port);

    /*setup subscription to topic*/
    unsigned flags = EMCUTE_QOS_0;
    subscriptions[0].cb = on_pub;
    strcpy(topics[0], MQTT_TOPIC_FROM_AWS);
    subscriptions[0].topic.name = MQTT_TOPIC_FROM_AWS;

    if (emcute_sub(&subscriptions[0], flags) != EMCUTE_OK) {
        printf("error: unable to subscribe to %s\n", MQTT_TOPIC_FROM_AWS);
        return 1;
    }

    printf("Now subscribed to %s\n", MQTT_TOPIC_FROM_AWS);
    
    return 0;
}





/* main interrupt handler */
void* pir_handler(void *arg){
    
    (void)arg;
    msg_t msg_q[1];
    msg_init_queue(msg_q, 1);

    printf("Registering PIR handler thread...     %s\n",
           pir_register_thread(&pir_dev) == 0 ? "[OK]" : "[Failed]");

    msg_t m;
    while (msg_receive(&m)) {
        printf("PIR handler got a message: ");
        switch (m.type) {
            case PIR_STATUS_ACTIVE: {
                
                //critical section start
                mutex_lock(&mutex);
                
                puts("[PIR]: Movement detected.\n");
                
                pir_last_awake_s=xtimer_now_usec();
                
                
                /* if bulb not lighten up from mqtt turn it on now */
                if(auto_mode){
                    /// bulb on
                    gpio_write(GPIO_PIN(PORT_B,5),1);
                }
                
                dht_temp_read();

                sprintf(msg,"T:%.1f C H:%.1f%%",th[0]/10.0f,th[1]/10.0f);
                printf("msg %s\n",msg);
              
                lcd_write(msg);
                
                mutex_unlock(&mutex);
                //critical section end
                break;
            }
            case PIR_STATUS_INACTIVE:{
                puts("[PIR]: Movement has ceased.");
                  
                //critical section start
                mutex_lock(&mutex);
               /* if bulb was not turned on from mqtt turn it off now */
                if(auto_mode){
                     /// bulb off
                    gpio_write(GPIO_PIN(PORT_B,5),0);
                }
                mutex_unlock(&mutex);
                //critical section end
                break;
            }
            default:
                puts("stray message.");
                break;
        }
    }
    puts("PIR handler: this should not have happened!");

    return NULL;
}


int main(void){
    
    printf("[Start SETUP]\n");
    
    int ret;
    
    // init relay pin
    ret=gpio_init(GPIO_PIN(PORT_B, 5),GPIO_OUT);
    
    // init pir sensor
    ret|=init_pir();
    
    // init dht sensor  
    ret|=init_dht();
    
   
    // init lcd display 
    ret|=init_lcd();
    
    //init mutex
    mutex_init(&mutex);
    
    //init mqtt client
    printf("[Start MQTT connection]\n");
    ret|=init_mqtt();
    printf("[MQTT connected]\n");
    
    // check everithing is okay thus far
    if(ret){
        printf("[INIT devices failed]\n");
        return -1;
    }
     printf("[INIT devices succeded]\n");
    
    //init pir interrupt routine
    thread_create(
           pir_handler_stack, sizeof(pir_handler_stack), THREAD_PRIORITY_MAIN - 1,
           THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
           pir_handler, NULL, "pir_handler");
    
    printf("[SETUP done!]\n");
    
    
    /* MQTT advertising every 30s */
    while(1){
        //critical section start
        mutex_lock(&mutex);
                    
        dht_temp_read();
        
        printf("Advertising...\n");
        
        sprintf(json,"{\n \"temperature\":%.1f,\n \"humidity\":%.1f,\n \"last_awake\":%.1f\n}",
        th[0]/10.0f,th[1]/10.0f,pir_last_awake_s == 0 ? 0.0f : (xtimer_now_usec() - pir_last_awake_s)/1000000 );
        
        mqtt_pub(MQTT_TOPIC_TO_AWS,json);
        
        mutex_unlock(&mutex);
        //critical section end
        
        xtimer_sleep(30);
    }
    puts("MQTT adv: this should not have happened!");
    
    return 0;
}
