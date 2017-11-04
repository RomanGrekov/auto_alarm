/*

 Sensor Receiver.
 Each sensor modue has a unique ID.
 
 TMRh20 2014 - Updates to the library allow sleeping both in TX and RX modes:
      TX Mode: The radio can be powered down (.9uA current) and the Arduino slept using the watchdog timer
      RX Mode: The radio can be left in standby mode (22uA current) and the Arduino slept using an interrupt pin
 */

#include <SPI.h>
#include <nrf_client.h>
#include <ArduinoLog.h>
#include <eeprom_cli.h>

// Set up nRF24L01 radio on SPI-1 bus (MOSI-PA2, MISO-PA0, SCLK-PA4) ... IRQ not used?
NRFClient radio(PA4,PA3, Log);

unsigned char syn[] = "SYN";
unsigned char ack[] = "ACk";
unsigned char synack[] = "SYNACK";
unsigned char new_key_req[] = "NEW_KEY_REQ";

#define B_PIN PA15
#define LED1_PIN PC13

void blinker(unsigned int times, unsigned int t_high, unsigned int t_low);
void seans(void);

unsigned int delay_i = 3;

unsigned char new_key[KEY_SIZE];
EepromCli flash(Log);

void setup(){
   
    Serial1.begin(9600);
    while(!Serial1 && !Serial1.available()){}
    Log.begin   (LOG_LEVEL_NOTICE, &Serial1);
    Log.notice(F("RF24 Receive start" CR));
  
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);

    // Setup and configure rf radio
    radio.initialise();
    
    radio.init_rx();

    // Dump the configuration of the rf unit for debugging
    //radio.printDetails();

    pinMode(LED1_PIN, OUTPUT);

    randomSeed(analogRead(PA0));

    uint16_t status;
    flash.init();
    flash.print_conf();
    status = flash.init_key(new_key, KEY_SIZE);
    if (status == 0) radio.change_key(new_key);

}

void loop(){

    seans();
}

void seans(void){
    bool res = false;
    NRFClient::package package;

    //---------------------------------------------------------Handshake----------------------------------------//
    Log.trace("Waiting for %s..."CR, syn);  
    res = radio.receive_data(package, syn, sizeof(syn), 0);
    if ( res != true){
        Log.error("Failed to receive SYN"CR);
        blinker(1, 500, 1);
        return;
    }

    Log.trace("--%s--"CR, ack);
    res = radio.send_data(package, ack, sizeof(ack)); 
    if (res != true){
      Log.error("Failed to send ACK"CR);
      blinker(1, 500, 1);
      return;
    }

    Log.trace("Waiting for %s..."CR, synack);  
    res = radio.receive_data(package, synack, sizeof(synack), 800);
    if ( res != true){
        Log.error("Failed to receive %s"CR, synack);
        blinker(1, 500, 1);
        return;
    }

    Log.notice("Handshake successfull"CR);
    //---------------------------------------------------------Handshake----------------------------------------//
    
    for (int i=0; i<KEY_SIZE; i++) new_key[i] = random(255);
    //for (unsigned char i=0; i<KEY_SIZE; i++){
    //  new_key[i] = (i+30);
    //}
    Log.trace("Sending new key..."CR);  
    res = radio.send_data(package, new_key, KEY_SIZE); 
    if (res != true){
      Log.error("Failed to send new key"CR);
      blinker(1, 500, 1);
      return;
    }

    Log.trace("Waiting for %s..."CR, synack);  
    res = radio.receive_data(package, synack, sizeof(synack), 1200);
    if ( res != true){
        Log.error("Failed to receive %s"CR, synack);
        blinker(1, 500, 1);
        return;
    }

    Log.notice("Changing key!!!"CR);
    radio.change_key(new_key);

    Log.trace("Receiving session id..."CR);  
    res = radio.receive(package, 1500);
    if ( res != true){
        Log.error("Failed to receive session id"CR);
        blinker(1, 500, 1);
        return;
    }
    Log.notice("Session id: %y"CR, package.data);

    Log.trace("--%s--"CR, synack);
    res = radio.send_data(package, synack, sizeof(synack)); 
    if (res != true){
      Log.error("Failed to send SYN"CR);
      blinker(1, 500, 1);
      return;
    }
    
    uint16_t status;
    Log.notice("Save new key to FLASH"CR);
    status = flash.write_key(new_key, KEY_SIZE);
    if (status != 0){
        Log.error("Impossible to save new key"CR);
        return;
    }
    status = flash.set_key_saved();
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


