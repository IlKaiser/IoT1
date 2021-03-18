#include <stdio.h>

#include "board.h"
#include "xtimer.h"
#include "thread.h"
#include "shell.h"
#include "led.h"

#include "periph/gpio.h"

/*
#include "dht.h"
#include "dht_params.h"
#include "saul.h"
#include "hd44780.h"
*/

int i = 0;

void cb_f(void* args){
    (void) args;
    
    printf("MVNT detected %i\n",++i);
    
    //r
    gpio_write(GPIO_PIN(PORT_B,3),1024);
    //g
    gpio_write(GPIO_PIN(PORT_B,5),0);
    //b
    gpio_write(GPIO_PIN(PORT_B,4),0);
    
    printf("LGHT on \n");
    
    
    xtimer_sleep(5);
    
    //r
    gpio_write(GPIO_PIN(PORT_B,3),0);
    //g
    gpio_write(GPIO_PIN(PORT_B,5),0);
    //b
    gpio_write(GPIO_PIN(PORT_B,4),0);
    
    printf("LGHT off \n");
    
    
    
}


int my_command1(int argc, char **argv) {
	(void) argc;
	(void) argv;
    
    while(1){
        printf("Read: %d\n",gpio_read(GPIO_PIN(PORT_A,10)));
        xtimer_sleep(2);
    }
	return 0;
}

static const shell_command_t shell_commands[] = {
    //{ "cmd", "our custom command", my_command },
    { "detect", "our custom command", my_command1 },
    { NULL, NULL, NULL }
};


/*dht read
int my_command1(int argc, char **argv) {
	(void) argc;
	(void) argv;

	printf("Read some s***\n");
    int16_t temp,hum;
    dht_read(&dev,&temp,&hum );
	printf("This is it:\nTemp -> %f°C\nHum  -> %f%% \n",temp/10.0f,hum/10.0f);
	return 0;
}*/


/*lcd 
int main(void)
{
    
    hd44780_t dev;
    hd44780_params_t params = {
    
    
        .cols=(16U),
        .rows=(2U),
        
        .rs=GPIO_PIN(PORT_A,10),
        .rw=GPIO_UNDEF,
        .enable=GPIO_PIN(PORT_B, 3),
        .data={GPIO_PIN(PORT_B, 5),GPIO_PIN(PORT_B, 4),GPIO_PIN(PORT_B, 10),
               GPIO_PIN(PORT_A, 8),GPIO_UNDEF,GPIO_UNDEF,GPIO_UNDEF,GPIO_UNDEF}
    };
    
    // init display 
    puts("[START]");
    if (hd44780_init(&dev, &params) != 0) {
        puts("[FAILED]");
        return 1;
    }
    // clear screen, reset cursor 
   
    hd44780_clear(&dev);
    hd44780_home(&dev);
    // write first line 
    hd44780_print(&dev, "Hello World ...");
    xtimer_sleep(1);
    // set cursor to second line and write 
    hd44780_set_cursor(&dev, 0, 1);
    hd44780_print(&dev, "   RIOT is here!");
    xtimer_sleep(3);
    // clear screen, reset cursor 
    hd44780_clear(&dev);
    hd44780_home(&dev);
    // write first line 
    hd44780_print(&dev, "The friendly IoT");
    // set cursor to second line and write 
    hd44780_set_cursor(&dev, 0, 1);
    hd44780_print(&dev, "Operating System");

    puts("[SUCCESS]");

    return 0;
}*/


int main(void)
{
    //gpio_init(GPIO_PIN(PORT_A, 10),GPIO_IN);
    gpio_init(GPIO_PIN(PORT_B, 3),GPIO_OD_PU);
    gpio_init(GPIO_PIN(PORT_B, 5),GPIO_OD_PU);
    gpio_init(GPIO_PIN(PORT_B, 4),GPIO_OD_PU);
    
    /*dht
     
       
    dht_params_t params ={0};
    
    params.pin=GPIO_PIN(PORT_A, 10);
    params.type=DHT11;
    params.in_mode=GPIO_IN;
    
    
    dht_init(&dev,&params);	
     */
    
    gpio_init_int(GPIO_PIN(PORT_A, 10),GPIO_IN,GPIO_RISING,cb_f,NULL);
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    

    return 0;
}
