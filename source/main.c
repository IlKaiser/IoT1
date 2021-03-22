#include <stdio.h>

#include "board.h"
#include "xtimer.h"
#include "thread.h"
#include "shell.h"
#include "led.h"
#include "pir.h"

#include "periph/gpio.h"


#include "dht.h"
#include "dht_params.h"
#include "saul.h"
#include "hd44780.h"
#include "thread.h"

char pir_handler_stack[THREAD_STACKSIZE_MAIN];

dht_t dht_dev;
pir_t pir_dev;
hd44780_t display_dev;
int th[2];
char msg[32];


    

/*dht read*/
void dht_temp_read(void) {
    
    int16_t temp,hum;
    dht_read(&dht_dev,&temp,&hum);
    
    th[0] = temp;
    th[1] = hum;
    
    return;
}

/*lcd*/
int lcd_write(char msg[]){
    
    // clear screen, reset cursor 
   
    hd44780_clear(&display_dev);
    hd44780_home(&display_dev);
    // write first line 
    hd44780_print(&display_dev,msg);
    
    /* set cursor to second line and write */
    hd44780_set_cursor(&display_dev, 0, 1);
    hd44780_print(&display_dev, "Ciao");

    return 0;
}


/* main interrupt handler */


void* pir_handler(void *arg)
{
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
                puts("something started moving.\n");
                gpio_write(GPIO_PIN(PORT_B,5),1);
                dht_temp_read();
                sprintf(msg,"T:%.1f C H:%.1f%%",th[0]/10.0f,th[1]/10.0f);
                printf("msg: %s\n",msg);
                lcd_write(msg);
                break;
            }
            case PIR_STATUS_INACTIVE:{
                puts("the movement has ceased.");
                gpio_write(GPIO_PIN(PORT_B,5),0);
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
    
    // init relay pin
    gpio_init(GPIO_PIN(PORT_B, 5),GPIO_OUT);
    
    // init pir sensor
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
    
     
    // init dht  
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
    
    // init display 
    
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
    
    //init pir interrupt routine
    thread_create(
           pir_handler_stack, sizeof(pir_handler_stack), THREAD_PRIORITY_MAIN - 1,
           THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
           pir_handler, NULL, "pir_handler");
    
    printf("[SETUP done]!\n");
    
    return 0;
}
