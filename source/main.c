#include <stdio.h>

#include "board.h"
#include "xtimer.h"
#include "thread.h"
#include "shell.h"
#include "led.h"
//#include "pir.h"

#include "periph/gpio.h"


#include "dht.h"
#include "dht_params.h"
#include "saul.h"
#include "hd44780.h"
#include "thread.h"

//char rcv_thread_stack[THREAD_STACKSIZE_MAIN];

dht_t dht_dev;
int th[2];
char msg[32];




    

/*dht read*/
void dht_temp_read(void) {
    
    int16_t temp,hum;
    dht_read(&dht_dev,&temp,&hum);
    
    th[0] = temp/10;
    th[1] = hum/10;
    
    return;
}

/*lcd*/
int lcd_write(char msg[]){
    
    hd44780_t dev;
    hd44780_params_t params = {
    
    
        .cols=(16U),
        .rows=(2U),
        
        .rs=GPIO_PIN(PORT_A,9),
        .rw=GPIO_UNDEF,
        .enable=GPIO_PIN(PORT_C, 7),
        .data={GPIO_PIN(PORT_B, 6),GPIO_PIN(PORT_A, 7),GPIO_PIN(PORT_A, 6),
               GPIO_PIN(PORT_A, 5),GPIO_UNDEF,GPIO_UNDEF,GPIO_UNDEF,GPIO_UNDEF}
    };
    
    // init display 
    //puts("[START]");
    if (hd44780_init(&dev, &params) != 0) {
        puts("[FAILED]");
        return 1;
    }
    // clear screen, reset cursor 
   
    hd44780_clear(&dev);
    hd44780_home(&dev);
    // write first line 
    hd44780_print(&dev,msg);

    return 0;
}
/*
void *rcv_thread(void *arg)
{
    (void) arg;
    //sprintf(msg,"T %d H %d%%",th[0],th[1]);
    //printf("msg: %s\n",msg);
    lcd_write("Ciao");
    return NULL;
    
}
*/
/* main interrupt handler */
void cb_f(void* args){
    
    (void) args;
    
    
    printf("MVNT detected \n");
    
    printf("LGHT on \n");
    
    gpio_write(GPIO_PIN(PORT_B,5),1);
    
    dht_temp_read();
    
    printf("T %d H %d%%\n",th[0],th[1]);
    
    /*thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
                  THREAD_PRIORITY_MAIN-1, THREAD_CREATE_STACKTEST,
                  rcv_thread, NULL, "rcv_thread");*/
    
    lcd_write("Ciao");

    xtimer_sleep(5);
    
    gpio_write(GPIO_PIN(PORT_B,5),0);
        
    printf("LGHT off \n");
    
}

int main(void){
    
    printf("Start SETUP...\n");
    
    // init pir pin
    gpio_init(GPIO_PIN(PORT_B, 5),GPIO_OUT);
    
     
    // init dht  
    dht_params_t params ={0};
    
    params.pin=GPIO_PIN(PORT_B, 3);
    params.type=DHT11;
    params.in_mode=GPIO_IN;
    
    dht_init(&dht_dev,&params);	
    
    
    //init pir int
    gpio_init_int(GPIO_PIN(PORT_A, 10),GPIO_IN,GPIO_RISING,cb_f,NULL);
    
    printf("SETUP done!\n");
    
    return 0;
}
