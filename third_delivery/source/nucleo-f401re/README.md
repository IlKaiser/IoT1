# Code for nucleo-f401re explained

All code necessary is in [main.c](./main.c) file. The code is organized in logical modules:
- [Pir Module](#pir-module)
- [Dht Module](#dht-module)
- [LCD Module](#lcd-module)
- [Mqtt Module](#mqtt-module)

All code is started in main function, including necessary pin setup and mutext initialization (for multithread reading of pir and dht values).
```c
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
```
## Pir Module
In this first part the is handled the message queue mantained by RIOT OS on PIR movement detection:
```c
void* pir_handler(void *arg){
    
    (void)arg;
    msg_t msg_q[1];
    msg_init_queue(msg_q, 1);

    printf("Registering PIR handler thread...     %s\n",
           pir_register_thread(&pir_dev) == 0 ? "[OK]" : "[Failed]");

    msg_t m;
    while (msg_receive(&m)) {
```
If a message is recieved a check is made to determine if a message refers to the start or the finish of the movement and actions are taken accordingly:
```c
        printf("PIR handler got a message: ");
        switch (m.type) {
            case PIR_STATUS_ACTIVE: {
                
                //critical section start
                mutex_lock(&mutex);
                
                puts("[PIR]: Movement detected.\n");
                
                pir_last_awake_s=xtimer_now_usec();
                
```
If the movement is started the bulb is turned on (if not already turned on from web interface), the dht sensor is polled and values are written on lcd: 
```c
                /* if bulb not lighten up from mqtt turn it on now */
                if(auto_mode){
                    /// bulb on
                    gpio_write(GPIO_PIN(PORT_B,5),1);
                }
                
                dht_temp_read();

                sprintf(msg,"T:%.1f C H:%.1f%%",th[0]/10.0f,th[1]/10.0f);
              
                lcd_write(msg);
                
                mutex_unlock(&mutex);
                //critical section end
                break;
            }
            
```
When the movement is no longer detected the bulb is turned off(if not controlled by web interface)
```c
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
```
## Dht Module
Simple function used to poll the sensor and update global variable th (for temperature and humidity):
```c
void dht_temp_read(void) {
    
    int16_t temp,hum;
    dht_read(&dht_dev,&temp,&hum);
    
    th[0] = temp;
    th[1] = hum;
    
    return;
}
```
## LCD Module
Simple function used to write a string on display:
```c
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
```
## Mqtt Module
This is the code for the main advertiser loop: every 30s are sent via mqtt_sn data read from sensors:
```c
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
```
