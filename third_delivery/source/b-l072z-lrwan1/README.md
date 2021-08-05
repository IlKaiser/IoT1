# Code for b-l072z-lrwan1 explained

## About

This is a test application for the Semtech LoRaMAC package. This package
provides the MAC primitives for sending and receiving data to/from a
LoRaWAN network.

See [LoRa Alliance](https://www.lora-alliance.org/) for more information on LoRa.
See [Semtech LoRamac-node repository](https://github.com/Lora-net/LoRaMac-node)
to have a look at the original package code.

This application can only be used with Semtech
[SX1272](https://semtech.my.salesforce.com/sfc/p/#E0000000JelG/a/440000001NCE/v_VBhk1IolDgxwwnOpcS_vTFxPfSEPQbuneK3mWsXlU) or
[SX1276](https://semtech.my.salesforce.com/sfc/p/#E0000000JelG/a/2R0000001OKs/Bs97dmPXeatnbdoJNVMIDaKDlQz8q1N_gxDcgqi7g2o) radio devices.

## Application configuration

Before building the application and joining a LoRaWAN network, you need an
account on a LoRaWAN backend provider. Then create a LoRaWAN application and
register your device.
Since this application has been heavily tested with the backend provided by
[TheThingsNetwork](https://www.thethingsnetwork.org/) (TTN), we recommend that
you use this one.

Once your application and device are created and registered, you'll have
several information (provided by the LoRaWAN provider):

* The type of join procedure: ABP (Activation by personnalization) or OTAA (
  Over The Air Activation)
* The device EUI: an 8 bytes array
* The application EUI: an 8 bytes array
* The application key: a 16 bytes array
* The device address: a 4 bytes array, only required with ABP join procedure
* The application session key: a 16 bytes array, only required with ABP join procedure
* The network session key: a 16 bytes array, only required with ABP join procedure

Once you have this information, either edit the `Makefile` accordingly or
use the `set`/`get` commands in test application shell.

## Joining with Over The Air Activation

In order to correct setup LoraWAN with TNN it is necessry to edit the [main.c](main.c) and to add your datas found on your TTN dashboard:
```c
/*
 * 
 *  Put your secret data here ;)
 *           |
 *           V
 */
const unsigned char* deveui = (const unsigned char*) "";
const unsigned char* appeui = (const unsigned char*) "";
const unsigned char* appkey = (const unsigned char*) "";

```
## Application Structure:
The code is divided in two main logical blocks:
- [Receive Module](#receive-module)
- [Send Module](#send-module)

When the code is run it automatically register the board on the LoRa network, wiht lora_init function:
```c
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

```
Then the two modules are started in separated threads.

### Receive Module
Receives messages from TTN via LoRa and if necessary emulates the actuators:
```c
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
```

### Send Module
Every 30s the send function is triggered in order to send emulated datas to TNN via LoRa:
```c
    //main loop
    while(1){
        
       dht_temp_read();
       
       printf("Advertising...\n");
       
       sprintf(json,"{\n \"temperature\":%.1f,\n \"humidity\":%.1f,\n \"last_awake\":%.1f\n}",
               th[0]/10.0f,th[1]/10.0f,random_uint32_range (0, 1000)/10.0f );
    
            
       send(json);
        
       xtimer_sleep(30);
    }
```
