#include <stdio.h>

#include "xtimer.h"
#include "thread.h"
#include "shell.h"
#include "led.h"



int my_command1(int argc, char **argv) {
	(void) argc;
	(void) argv;

	printf("Toggling LED\n");

	LED1_ON;
	xtimer_sleep(1);
	LED1_OFF;
	for(int i = 0; i < 10; i++){
		xtimer_usleep(50000);
		LED1_ON;
		xtimer_usleep(50000);
		LED1_OFF;
	}

	printf("This is our custom command\n");
	return 0;
}

int my_command(int argc, char **argv) {
	(void) argc;
	(void) argv;

	printf("Toggling LED\n");

	LED0_ON;
	xtimer_sleep(1);
	LED0_OFF;
	for(int i = 0; i < 10; i++){
		xtimer_usleep(50000);
		LED0_ON;
		xtimer_usleep(50000);
		LED0_OFF;
	}

	printf("This is our custom command\n");
	return 0;
}

static const shell_command_t shell_commands[] = {
    { "cmd", "our custom command", my_command },
    { "laser", "our custom command", my_command1 },
    { NULL, NULL, NULL }
};

int main(void)
{
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
