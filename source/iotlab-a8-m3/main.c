/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example application for demonstrating the RIOT network stack
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "msg.h"
#include "mutex.h"
#include "shell.h"

#include "net/emcute.h"
#include "periph/gpio.h"



#ifndef EMCUTE_ID
#define EMCUTE_ID           ("laser")
#endif
#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN - 1)

#define NUMOFSUBS           (1U)
#define TOPIC_MAXLEN        (64U)

#define MQTT_TOPIC_TO_AWS   "iot/2/data"
#define MQTT_TOPIC_FROM_AWS "both_directions"
#define MAIN_QUEUE_SIZE     (8)


/* Global Variables */

//thread stuff
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
static char mqtt_handler_stack[THREAD_STACKSIZE_DEFAULT];

//mutex
mutex_t mutex;

//mqtt
static emcute_sub_t subscriptions[NUMOFSUBS];
static char topics[NUMOFSUBS][TOPIC_MAXLEN];

/*sycnchronized resources*/

//message arrays
int th[2];
char msg[32];
char json[32];

float pir_last_awake_s = 0;

int auto_mode = 1;



/* Fake DHT */
void dht_temp_read(void) {
    
    
    th[0] = 20.0; //TBR with random
    th[1] = 50.0; //TBR with random
    
    return;
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






extern int udp_cmd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "udp", "send data over UDP and listen on UDP ports", udp_cmd },
    { NULL, NULL, NULL }
};

int main(void)
{
    
    int ret = 0;
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT network stack example application");

    //init mutex
    mutex_init(&mutex);
    
    //init mqtt client
    printf("[Start MQTT connection]\n");
    ret|=init_mqtt();
    printf("[MQTT connected]\n");
    
    /* Main Loop */
     while(1){
        //critical section start
        mutex_lock(&mutex);
                    
       // dht_temp_read();
        
        printf("Advertising...\n");
        
        sprintf(json,"{\n \"temperature\":%.1f,\n \"humidity\":%.1f,\n \"last_awake\":%.1f\n}",
        th[0]/10.0f,th[1]/10.0f,pir_last_awake_s == 0 ? 0.0f : (xtimer_now_usec() - pir_last_awake_s)/1000000 );
        
        mqtt_pub(MQTT_TOPIC_TO_AWS,json);
        
        mutex_unlock(&mutex);
        //critical section end
        
        xtimer_sleep(30);
    }
    
    /* start shell ?*/
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    /* should be never reached */
    return 0;
}
