# Code for iotlab-m3 explained
The main challenge of the second assignment was adapting the code of the previous one to run on iotlab-m3 nodes where none of the sensors i chose were available. I decided to emulate sensor readings with random numbers within a reasonable range. Of course all GPIO and LCD related code was removed and/or emulated. PIR related code was also removed due to the lack of hardware.
The main structure of the code however is the same, despite having shell running for conneting to mqtt_sn broker:
- [Mqtt Module](#mqtt-module)
- [Sensor Emulation](#sensor-emulation)

## MQTT Module
Here below we can see all mqtt_cmd executed by shell; IPv6 address of the broker and topic device id need to be manually passed by the user in the shell:
```c
/* mqtt new routine */
int mqtt_cmd(int argc, char **argv){
    if(argc < 3){
        printf("Usage mqtt [addr] [id]\n");
        return -1;
    }
    
    /* setup mqtt with proper address */
    init_mqtt(argv[1]);
    
    
    /* create correct topic and set devide_id*/
    char mqtt_topic[11] = {'i','o','t','/','\0','\0','\0','\0','\0','\0','\0' };
    
    strcat(mqtt_topic,argv[2]);
    strcat(mqtt_topic,"/data");
    
    
    printf("Selected topic is: %s\n",mqtt_topic);
    
    device_id = atoi(argv[2]);
    
    printf("Device id is: %d\n",device_id);
    
    /* Advertise every 30s */
    
     while(1){
        //critical section start
        mutex_lock(&mutex);
                    
        dht_temp_read();
        
        printf("Advertising...\n");
        
        /* fake sensors polled */
        sprintf(json,"{\n \"temperature\":%.1f,\n \"humidity\":%.1f,\n \"last_awake\":%.1f\n}",
        th[0]/10.0f,th[1]/10.0f,random_uint32_range (0, 1000)/10.0f );
        
        
        mqtt_pub(mqtt_topic,json);
        
        mutex_unlock(&mutex);
        //critical section end
        
        xtimer_sleep(30);
    }
    /* never reached */
    return 0;

}
```
## Sensor Emulation
DHT sensor and PIR last wake are emulated with random numbers (for pir code see above):
```c
/* Fake DHT */
void dht_temp_read(void) {
    /* temp range 10-30C, humidity range 40-100% */
    th[0] = random_uint32_range (100, 300);
    th[1] = random_uint32_range (400, 1000);
    
    return;
}

```
