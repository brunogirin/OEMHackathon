/*
The OpenTRV project licenses this file to you
under the Apache Licence, Version 2.0 (the "Licence");
you may not use this file except in compliance
with the Licence. You may obtain a copy of the Licence at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the Licence is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied. See the Licence for the
specific language governing permissions and limitations
under the Licence.

Author(s) / Copyright (s):  Milenko Alcin 2016
*/

/*Unit test routines for library code.
 */

//#define RECEIVER
// Arduino libraries imported here (even for use in other .cpp files).
#include <Arduino.h>
#include <Wire.h>

#define UNIT_TESTS

// Include the library under test.
#include <OTV0p2Base.h>
#include <OTRadioLink.h>
#include <OTRFM23BLink.h>
#include <OTRadValve.h>
#include <utility/OTRadioLink_JeelabsOemPacket.h>

#define RADIO_CONFIG_NAME "JEELABS/OEM TEST"

// Packet encoder and decoder object
OTRadioLink::JeelabsOemPacket JL;

// We'll configure 3 channels to show that we do not need to receive on channel 0
static const uint8_t nradioChannels = 3; 

static const OTRadioLink::OTRadioChannelConfig RFM23BConfigs[nradioChannels] =
  {
  // GFSK channel 0 full config, RX/TX, not in itself secure.
  OTRadioLink::OTRadioChannelConfig(OTRFM23BLink::StandardRegSettingsGFSK57600, true),
  // FS20/FHT8V compatible channel 1 full config, used for TX only, not secure, unframed.
  OTRadioLink::OTRadioChannelConfig(OTRFM23BLink::StandardRegSettingsOOK5000, true, false, true, false, false, true),
  // GFSK channel 0 full config, RX/TX, not  encrypted.
  OTRadioLink::OTRadioChannelConfig(OTRFM23BLink::StandardRegSettingsJeeLabs, true),
  };
  
// 

#define PIN_RFM_NIRQ 9 
#define PIN_SPI_nSS 10 
OTRFM23BLink::OTRFM23BLink<PIN_SPI_nSS, PIN_RFM_NIRQ, OTRFM23BLink::DEFAULT_RFM23B_RX_QUEUE_CAPACITY> RFM23B;

OTRadioLink::OTRadioLink &radio = RFM23B;


uint8_t prevStatePB = PINB;

#define RFM23B_INT_MASK 2
ISR(PCINT0_vect)
  {
//  ++intCountPB;
  const uint8_t pins = PINB;
  const uint8_t changes = pins ^ prevStatePB;
  prevStatePB = pins;

  if((changes & RFM23B_INT_MASK) && !(pins & RFM23B_INT_MASK))
    { radio.handleInterruptSimple(); }

  }


void panic()
  {
  Serial.println("Panic");
#ifdef ENABLE_RADIO_PRIMARY_MODULE
  // Reset radio and go into low-power mode.
  radio.panicShutdown();
#endif
  // Power down almost everything else...
  OTV0P2BASE::minimisePowerWithoutSleep();
#if 0
#ifdef LED_HEATCALL
  pinMode(LED_HEATCALL, OUTPUT);
#else
  pinMode(LED_HEATCALL_L, OUTPUT);
#endif
  for( ; ; )
    {
    LED_HEATCALL_ON();
    tinyPause();
    LED_HEATCALL_OFF();
    bigPause();
    }
#endif
  }

// Panic with fixed message.
void panic(const __FlashStringHelper *s)
  {
  OTV0P2BASE::serialPrintlnAndFlush(s); // May fail.
  panic();
  }


void setup()
  {
  // initialize serial communications at 4800 bps for typical use with V0p2 board.
  Serial.begin(4800);
  Serial.print(RADIO_CONFIG_NAME);
#ifdef RECEIVER
  Serial.println(" RECEIVER");
#else
  Serial.println(" TRANSMITTER");
#endif
  Serial.flush();

  OTV0P2BASE::powerSetup();
  PRR &= ~_BV(PRUSART0); // Enable UART. It can stay on for this test.

  radio.preinit(NULL);
  if(!radio.configure(nradioChannels, RFM23BConfigs) || !radio.begin()) { panic(F("r1")); }

#ifdef RECEIVER
  JL.setNodeAndGroupID(5,100);
#else
  JL.setNodeAndGroupID(10,100);
#endif

  // We can get always get node and group ID from library if we need it
  Serial.print("NodeID=");
  Serial.println(JL.getNodeID());
  Serial.print("GroupID=");
  Serial.println(JL.getGroupID());

  // Our driver does not know the length of packet, so we need to learn whole buffer for 
  // every packet, so it is good idea to let RFM23B interrupt routine to do first filtering
  // In this case we eliminate obviously wrong packets and if packets are short we can save a 
  // lot of buffer space.
  radio.setFilterRXISR(OTRadioLink::JeelabsOemPacket::filter);

  // Let radio listen on channel 2 (Jeelabs/OEM)
  radio.listen(true,2);


  // Enable interrupts
  pinMode(PB1, INPUT);           // set pin to input
  digitalWrite(PB1, HIGH);       // turn on pullup resistors
  cli();		// switch interrupts off while messing with their settings  
  PCICR  = 0b00000001;  // Enable pin change interrupts 0-7 
  PCMSK0 = 0b00000010;  // Interrupt on D9/PCINT1
  sei();		// turn interrupts back on

  }



uint8_t mtype;

// Tests generally flag an error and stop the test cycle with a call to panic() or error().
void loop()
  {
#ifdef RECEIVER
  // We can poll if we don't want to enable interrupt for some reason
  radio.poll();
  const volatile uint8_t *pb;
  while(NULL != (pb = radio.peekRXMsg()))
    {
    // If Jeelabs/OEM channel apply packet decoder
    if (radio.getListenChannel() == 2) {
    uint8_t len = pb[-1]; // Note that filter returns correct packet length, without filter it would be size of FIFO
    uint8_t nodeID;
    bool    dest;
    bool    ackReq;
    bool    ackConf;

    uint8_t result = JL.decode( (uint8_t *)pb,  len,  nodeID,  dest,  ackReq,  ackConf);
    switch (result)
       {
       case 255:
          Serial.print("Wrong group: ");
          break;
       case 254:
          Serial.print("CRC error: ");
          break;
       case 253:
          Serial.print("Wrong node: ");
          break;
       }
       // This is what we received 
       Serial.print("R("); Serial.print(JL.getGroupID()); Serial.print(','); Serial.print(nodeID); Serial.print(',');
       Serial.print(ackConf?"C":"-"); Serial.print(dest?"D":"-"); Serial.print(ackReq?"A":"-"); Serial.print(") ");
       Serial.print(len); Serial.print(":");
       for(uint8_t i = 0; i < len ; i++ ) {Serial.print(pb[i],HEX); Serial.print(' ');}
       Serial.println();
    }
    radio.removeRXMsg();
  }

#else

  uint8_t txbuf[5] = { mtype, mtype, mtype };  // Lets make packet 3 bytes long and and fill it with some info
  uint8_t len = 3; 
  // Set flags to se if they get accross OK
  bool    ackC = mtype & 1;
  bool    ack  = mtype & 2;
  bool    dest = mtype & 4;
  // Set node ( it will be used only if dest flag set, for broadcase we are using node id configured earlier)
  uint8_t nodeId = (mtype>>3) & 0x1f;
  uint8_t packlen = JL.encode(txbuf, len, nodeId,dest,ack,ackC );
  if(radio.sendRaw(txbuf, packlen, 2)) 
  { 
     // Here is packet we sent
     Serial.print(F("S(")); Serial.print(JL.getGroupID()); Serial.print(','); Serial.print(nodeId); Serial.print(',');
     Serial.print(ackC?"C":"-"); Serial.print(dest?"D":"-"); Serial.print(ack?"A":"-"); Serial.print(") ");
     Serial.print(packlen); Serial.print(":");
     for(uint8_t i = 0; i < packlen ; i++ ) {Serial.print(txbuf[i],HEX); Serial.print(' ');}
     Serial.println(); 
  }
  else
     Serial.println(F("Failed")); 
  delay(3000);
  mtype++;
#endif
  }
