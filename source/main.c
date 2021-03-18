#include <stdio.h>

#include "board.h"
#include "xtimer.h"
#include "thread.h"
#include "shell.h"
#include "led.h"

#include "periph/gpio.h"

int i = 0;

void cb_f(void* args){
    (void) args;
    
    printf("MVNT detected %i\n",++i);
    
    //r
    gpio_write(GPIO_PIN(PORT_B,3),1024);
    //g
    gpio_write(GPIO_PIN(PORT_B,5),1024);
    //b
    gpio_write(GPIO_PIN(PORT_B,4),1024);
    
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

int main(void)
{
    //gpio_init(GPIO_PIN(PORT_A, 10),GPIO_IN);
    gpio_init(GPIO_PIN(PORT_B, 3),GPIO_OD_PU);
    gpio_init(GPIO_PIN(PORT_B, 5),GPIO_OD_PU);
    gpio_init(GPIO_PIN(PORT_B, 4),GPIO_OD_PU);
    
    gpio_init_int(GPIO_PIN(PORT_A, 10),GPIO_IN,GPIO_RISING,cb_f,NULL);
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    

    return 0;
}
