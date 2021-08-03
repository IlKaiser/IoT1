#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "net/loramac.h"
#include "semtech_loramac.h"

#include "msg.h"
#include "random.h"
#include "shell.h"
#include "thread.h"
#include "xtimer.h"


#define RECV_MSG_QUEUE             (4U)

#define MAIN_QUEUE_SIZE            (8)

static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];



static msg_t _recv_queue[RECV_MSG_QUEUE];
static char _recv_stack[THREAD_STACKSIZE_DEFAULT];

static semtech_loramac_t loramac;

int device_id = -1;

/*
 * 
 *  Put your secret data here ;)
 *           |
 *           V
 */
const unsigned char* deveui = (const unsigned char*) "";
const unsigned char* appeui = (const unsigned char*) "";
const unsigned char* appkey = (const unsigned char*) "";

//message arrays
uint32_t th[2] = {200,600};
char msg[32];
char json[32];


static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static int mod_table[] = {0, 2, 1};

char *base64_encode(const char *data, size_t input_length, size_t *output_length){
    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(*output_length);
    if(encoded_data == NULL) return NULL;

    for(size_t i = 0, j = 0; i < input_length;){
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++){
        encoded_data[*output_length - 1 - i] = '=';
    }
    return encoded_data;
}




/* Fake DHT */
void dht_temp_read(void) {
    
    th[0] = random_uint32_range (100, 300);
    th[1] = random_uint32_range (400, 1000);
    
    return;
}

static void *_recv(void *arg) {
    msg_init_queue(_recv_queue, RECV_MSG_QUEUE);
    (void)arg;
    while (1) {

        semtech_loramac_recv(&loramac);
        loramac.rx_data.payload[loramac.rx_data.payload_len] = 0;
        printf("Data received: %s, port: %d", (char *)loramac.rx_data.payload, loramac.rx_data.port);
        char * in = (char *)loramac.rx_data.payload;
		printf("received: %s\n",in);
        
        if(in[0]=='o' && in[1]=='n' && (in[2]-'0') == device_id){
            printf("Light it up");
                
        }else if(in[0]=='o' && in[1]=='f'&& in[2]=='f'
            && (in[2]-'0') == device_id){
            
            printf("Turn it off");

        }else{
            printf("Command not recognized\n");
            
        }
    
       
    }
    return NULL;
}

int lora_init(void){
    printf("device id: %d\n", device_id);

    semtech_loramac_init(&loramac);
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);

    semtech_loramac_set_dr(&loramac, 5);

    if(semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED){
        puts("Join procedure failed");
        return 1;
    }
    puts("Join procedure succeeded");

    puts("Starting recv thread");
    thread_create(_recv_stack, sizeof(_recv_stack), THREAD_PRIORITY_MAIN - 1, 0, _recv, NULL, "recv thread");
    return 0;
}



void send(const char* message){
    printf("Sending message '%s'\n", message);

    size_t inl = strlen(message);
    size_t output;

    char *encoded = base64_encode(message, inl, &output);
    
    printf("Encoded: %s\n",encoded );
   
    uint8_t status = semtech_loramac_send(&loramac, (uint8_t *)message, strlen(message));
  
    if (status != SEMTECH_LORAMAC_TX_DONE) {
        printf("Cannot send message '%s'\n", message);
    } else {
        printf("Successfully sent message: %s\n", message);
    }
}


int cmd_handler(int argc, char **argv){
    
    if(argc < 2){
        printf("Usage start [id]\n");
        return -1;
    }
	//getting device id
	device_id = atoi(argv[1]);
    
    
    //init random 
    random_init(0);
    
    //init lora
	printf("Initializing Lora...");
    
    lora_init();
    xtimer_sleep(2);
	
    printf("done!\n");
    
    //main loop
    while(1){
        
       dht_temp_read();
       
       printf("Advertising...\n");
       
       sprintf(json,"{\n \"temperature\":%.1f,\n \"humidity\":%.1f,\n \"last_awake\":%.1f\n}",
               th[0]/10.0f,th[1]/10.0f,random_uint32_range (0, 1000)/10.0f );
    
            
       send(json);
        
       xtimer_sleep(30);
    }
    
    /* never reached */
    return 0;
}

static const shell_command_t shell_commands[] = {
	{ "start", "initialize LoraWAN and starts sending data to TTN", cmd_handler},
    { NULL, NULL, NULL}
};

int main(void){
	
	msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
	
    return 0;
}
