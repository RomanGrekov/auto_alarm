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
#include <eeprom_cli.h>

// Set up nRF24L01 radio on SPI-1 bus (MOSI-PA7, MISO-PA6, SCLK-PA5) ... IRQ not used?
NRFClient radio(PA4,PA3, Log);

unsigned char syn[] = "SYN";
unsigned char ack[] = "ACk";
unsigned char synack[] = "SYNACK";
unsigned char new_key_req[] = "NEW_KEY_REQ";

#define B_PIN PA15
#define LED1_PIN PC13

unsigned char session_id;

void blinker(unsigned int times, unsigned int t_high, unsigned int t_low);
void seans(void);

uint8_t new_key[KEY_SIZE];
unsigned char data[DATA_SIZE];

EepromCli eeprom(Log, PB6, PB7);

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
    radio.init_tx();


    pinMode(B_PIN, INPUT);
    pinMode(LED1_PIN, OUTPUT);
    digitalWrite(LED1_PIN, 1);
    
    randomSeed(analogRead(PA0));
     
    uint8_t status;
    bool res = false;
    //eeprom.set_key_unsaved();
    status = eeprom.is_key_saved(&res);
    if (status == 0 && res == true){
      status = eeprom.read_key(new_key);
      if (status == 0)radio.change_key(new_key);
    }
    else Log.trace("Key is absent in eeprom"CR);
    
    
}

void loop(){
    unsigned int button_state = digitalRead(B_PIN);
    if (button_state == LOW){
        seans();
        while (button_state != HIGH)
        {
            button_state = digitalRead(B_PIN);
            delay(100);
        }
    }
}

void seans(void){
    bool res;
    session_id = random(255);
     
    blinker(1, 100, 1);
    //---------------------------------------------------------Handshake----------------------------------------//
    
    Log.trace("--%s--"CR, syn);
    res = radio.send(syn, sizeof(syn)); 
    if (res != true){
      Log.error("Failed to send SYN"CR);
      blinker(1, 500, 1);
      return;
    }
   
    Log.trace("--Receiving %s..."CR, ack);  
    res = radio.receive_expected(ack, sizeof(ack), 1000);
    if ( res != true){
        Log.error("Failed to receive ACK"CR);
        blinker(1, 500, 1);
        return;
    }

    Log.trace("--%s--"CR, synack);
    res = radio.send(synack, sizeof(synack)); 
    if (res != true){
      Log.error("Failed to send SYN"CR);
      blinker(1, 500, 1);
      return;
    }
    //---------------------------------------------------------Handshake----------------------------------------//

    Log.trace("Receiving new key..."CR);  
    res = radio.receive(new_key, 1500);
    if ( res != true){
        Log.error("Failed to receive new key"CR);
        blinker(1, 500, 1);
        return;
    }

    Log.trace("--%s--"CR, synack);
    res = radio.send(synack, sizeof(synack)); 
    if (res != true){
      Log.error("Failed to send SYN"CR);
      blinker(1, 500, 1);
      return;
    }
    
    Log.notice("Change key!!!"CR);
    radio.change_key(new_key);

    for (unsigned char i=0; i<KEY_SIZE; i++) data[i] = random(255);
    Log.notice("Session id: %y"CR, data);
    Log.trace("Sending session id..."CR);  
    res = radio.send(data, sizeof(data)); 
    if (res != true){
      Log.error("Failed to send session id"CR);
      blinker(1, 500, 1);
      return;
    }

    Log.trace("Waiting for %s..."CR, synack);  
    res = radio.receive_expected(synack, sizeof(synack), 2500);
    if ( res != true){
        Log.error("Failed to receive %s"CR, synack);
        blinker(1, 500, 1);
        return;
    }

    uint8_t status;
    Log.notice("Save new key to FLASH"CR);
    status = eeprom.write_key(new_key);
    if (status != 0){
        Log.error("Impossible to save new key"CR);
        return;
    }
    status = eeprom.set_key_saved();
    if (status != 0){
        Log.error("Impossible to save key changed status flag"CR);
        return;
    }

    Log.notice("Successfully!"CR);
    blinker(6, 75, 75);
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




