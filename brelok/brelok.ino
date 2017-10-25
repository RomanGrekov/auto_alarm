/*

 Sensor Receiver.
 Each sensor modue has a unique ID.
 
 TMRh20 2014 - Updates to the library allow sleeping both in TX and RX modes:
      TX Mode: The radio can be powered down (.9uA current) and the Arduino slept using the watchdog timer
      RX Mode: The radio can be left in standby mode (22uA current) and the Arduino slept using an interrupt pin
 */


#include <SPI.h>
#include <ArduinoLog.h>
#include <nrf_client.h>

// Set up nRF24L01 radio on SPI-1 bus (MOSI-PA7, MISO-PA6, SCLK-PA5) ... IRQ not used?
NRFClient radio(PA4,PA3, Log);

// The various roles supported by this sketch
typedef enum { role_tx = 1, role_rx } role_e;

// The role of the current running sketch
role_e role;

unsigned char password[] = "120288";
unsigned char response[] = "Ok";

#define B_PIN PA15
#define LED1_PIN PC13

unsigned char session_id;

void blinker(unsigned int times, unsigned int t_high, unsigned int t_low);
bool is_same(unsigned char *arr1, unsigned char *arr2);
void print_packet(NRFClient::package &packet);

void setup(){ 
    Serial1.begin(9600);
    while(!Serial1 && !Serial1.available()){}
    Log.begin   (LOG_LEVEL_VERBOSE, &Serial1);
    Log.notice("\n\rRF24 Transiver start"CR);
  
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
  
    // Setup and configure rf radio

    radio.initialise();

    role = role_tx;

    // Open pipes to other nodes for communication

    // This simple sketch opens two pipes for these two nodes to communicate
    // back and forth.
    // Open 'our' pipe for writing
    // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

    if ( role == role_tx )
    {
        radio.init_tx();
    }
    else
    {
        radio.init_rx();
    }
 
    // Dump the configuration of the rf unit for debugging
    //radio.printDetails();

    pinMode(B_PIN, INPUT);
    pinMode(LED1_PIN, OUTPUT);
    digitalWrite(LED1_PIN, 1);

    //testCipher();
    
    randomSeed(analogRead(PA0));
}

void loop(){
    
    unsigned int button_state = digitalRead(B_PIN);
    if (button_state == LOW){
        session_id = random(255);
     
        blinker(1, 100, 1);
        if (role == role_tx){
            NRFClient::package package;
            memcpy(package.data, password, sizeof(password));
            bool res = radio.send(package); 
            if (res == true)
            {
                Log.trace("Data sent successfully"CR);
                role = role_rx;
            }
            else
            {
                blinker(1, 250, 1);
            }
            delay(100);
            while (button_state != HIGH)
            {
                button_state = digitalRead(B_PIN);
                delay(100);
            }
       }

       if (role == role_rx){
           Log.trace("Receiving..."CR);
           NRFClient::package package;
           bool res = radio.receive(package, 500);
           if ( res != true)
           {
               blinker(1, 500, 1);
           }
           else
           {
               if (is_same(package.data, response, sizeof(response)) == true){
                   Log.notice("Response is correct"CR);
                   blinker(6, 75, 75);
               }
           }
           role = role_tx;
       }

    } 

}


void blinker(unsigned int times, unsigned int t_high, unsigned int t_low){
    for(int i=0; i<times; i++)
    {
        digitalWrite(LED1_PIN, 0);
        delay(t_high);
        digitalWrite(LED1_PIN, 1);
        delay(t_low);
    }
}

bool is_same(unsigned char *arr1, unsigned char *arr2, unsigned int len){
    for (unsigned int i=0; i < len; i++){
        if (arr1[i] != arr2[i]) return false;
    }
    return true;
}



