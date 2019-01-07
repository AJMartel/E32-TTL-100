/**
 * E32-TTL-100 Transceiver Interface
 *
 * @author Bob Chen (bob-0505@gotmail.com)
 * @date 1 November 2017
 * https://github.com/Bob0505/E32-TTL-100
 */
#include <SoftwareSerial.h>

#include "E32TTL100.h"

/*
 need series a 4.7k Ohm resistor between .
 UNO/NANO(5V mode)                E32-TTL-100
    *--------*                      *------*
    | D7     | <------------------> | M0   |
    | D8     | <------------------> | M1   |
    | A0     | <------------------> | AUX  |
    | D10(Rx)| <---> 4.7k Ohm <---> | Tx   |
    | D11(Tx)| <---> 4.7k Ohm <---> | Rx   |
    *--------*                      *------*
*/
#define M0  8
#define M1  9
#define AUX 14
#define SOFT_RX 11
#define SOFT_TX 10
uint8_t DEVICE_ADDR_H  = 0x05;
uint8_t DEVICE_ADDR_L  = 0x01;

//SoftwareSerial soft(SOFT_RX, SOFT_TX);  // RX, TX
#define Device_B
#include "SoftwareSerial.h"

SoftwareSerial soft(SOFT_RX, SOFT_TX);
E32TTL100 radio( M0, M1, AUX, DEVICE_ADDR_H,DEVICE_ADDR_L);

//The setup function is called once at startup of the sketch
void setup()
{
  
  Serial.begin(9600);
  while (!Serial);
  soft.begin(9600); 
  while (!soft) ;
  
  Serial.print("started");
  radio.begin(&soft,&Serial);

}

// The loop function is called in an endless loop
void loop()
{
  char data_buf[100], data_len;

#ifdef Device_A
  if(radio.ReceiveMsg(data_buf, &data_len)==RET_SUCCESS)
  {
    Serial.write(data_buf,data_len);
    Serial.println(data_len);
  }
#else
  if(radio.SendMsg("Maine Kr Dhikaya")==RET_SUCCESS)
  {
    Serial.println('sent');
  }
#endif

  delay(random(400, 600));
}
