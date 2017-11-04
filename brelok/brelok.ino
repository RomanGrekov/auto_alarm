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

unsigned char new_key[KEY_SIZE];
//EepromCli flash(0x801F000, 0x801F800, 0x400, Log);
EepromCli flash(Log);

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
 
    // Dump the configuration of the rf unit for debugging
    //radio.printDetails();

    pinMode(B_PIN, INPUT);
    pinMode(LED1_PIN, OUTPUT);
    digitalWrite(LED1_PIN, 1);
    
    randomSeed(analogRead(PA0));
    
    uint16_t status;
    flash.init();
    flash.print_conf();
    status = flash.init_key(new_key, KEY_SIZE);
    if (status == 0) radio.change_key(new_key);
    //EEPROM.PageBase0 = 0x801F000;
    //EEPROM.PageBase1 = 0x801F800;
    //EEPROM.PageSize  = 0x400;
    //EEPROM.format();
    //EEPROM.write(0x10, data);
    //status = EEPROM.read(0x10, &data);
    //Log.trace("Status: %X, Data: %x"CR, status, data);
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
    NRFClient::package package;
    session_id = random(255);
     
    blinker(1, 100, 1);
    //---------------------------------------------------------Handshake----------------------------------------//
    Log.trace("--%s--"CR, syn);
    res = radio.send_data(package, syn, sizeof(syn)); 
    if (res != true){
      Log.error("Failed to send SYN"CR);
      blinker(1, 500, 1);
      return;
    }
   
    Log.trace("--Receiving %s..."CR, ack);  
    res = radio.receive_data(package, ack, sizeof(ack), 500);
    if ( res != true){
        Log.error("Failed to receive ACK"CR);
        blinker(1, 500, 1);
        return;
    }

    Log.trace("--%s--"CR, synack);
    res = radio.send_data(package, synack, sizeof(synack)); 
    if (res != true){
      Log.error("Failed to send SYN"CR);
      blinker(1, 500, 1);
      return;
    }
    //---------------------------------------------------------Handshake----------------------------------------//

    Log.trace("Receiving new key..."CR);  
    res = radio.receive(package, 1000);
    if ( res != true){
        Log.error("Failed to receive new key"CR);
        blinker(1, 500, 1);
        return;
    }

    memcpy(new_key, package.data, KEY_SIZE);

    Log.trace("--%s--"CR, synack);
    res = radio.send_data(package, synack, sizeof(synack)); 
    if (res != true){
      Log.error("Failed to send SYN"CR);
      blinker(1, 500, 1);
      return;
    }
    
    Log.notice("Change key!!!"CR);
    radio.change_key(new_key);

    for (unsigned char i=0; i<KEY_SIZE; i++) package.data[i] = random(255);
    Log.notice("Session id: %y"CR, package.data);
    Log.trace("Sending session id..."CR);  
    res = radio.send(package); 
    if (res != true){
      Log.error("Failed to send session id"CR);
      blinker(1, 500, 1);
      return;
    }

    Log.trace("Waiting for %s..."CR, synack);  
    res = radio.receive_data(package, synack, sizeof(synack), 1800);
    if ( res != true){
        Log.error("Failed to receive %s"CR, synack);
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




