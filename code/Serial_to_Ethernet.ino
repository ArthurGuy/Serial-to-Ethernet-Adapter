
/* Serial to Ethernet Adapter (arduino comptible code)
 * Copyright (C) 2014 by Arthur Guy
 *
 * This is released under the GNU General Public License
 */
#include <avr/wdt.h>
#include <avr/power.h> //http://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <EtherCard.h>
#include "SerialConfig.h"


#include <EEPROM.h>


//MAC address EEPROM settings
#define MAC_I2C_ADDRESS 0xA0
#define SDA_PIN A4
#define SCL_PIN A5
#include <I2cMaster.h>
SoftI2cMaster rtc(SDA_PIN, SCL_PIN);

//Array to hold the mac address
uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
const static uint8_t dns_ip[] = {8,8,8,8};



//Network variables
Stash stash;                 //Handle data stashes on the ethernet module
static byte session_id;      //Hold the ID of the current inprogress session
boolean waitingForResponse;  //Are we waiting for a reply to come back?
unsigned long sendTime;      //When did we send the message
byte Ethernet::buffer[600];

//The url and host name
char HOST[50] = "iot.arthurguy.co.uk";
char URL[50] = "/ping_back";
uint8_t postRequest = true;  //0:GET,1:POST,2:POST-BODY
//char postParam[10] = "data";



//Configure using this line
//EthernetOut.println("+iot.arthurguy.co.uk:/ping_back:1:data");
//+members.buildbrighton.com:/validate_key.asp:1
//+iot.arthurguy.co.uk:/ping_back:1
//+iot.arthurguy.co.uk:/art_light_data:0
//+iot.arthurguy.co.uk:/sensor_network/save:1
//+arthurguy.co.uk:/sensor_network/save:1
//+data.arthurguy.co.uk:/stream/XRdO9uGzIG/data:2



SerialConfig serialConfig = SerialConfig(Serial);



//Pin assignment
uint8_t ethernetReset = 9;
uint8_t systemBusy = 8;
uint8_t systemReady = 7;


//Serial input character handling
uint8_t stringFetched, setupString, fetchingHost, fetchingUrl;
char readChar, lastChar;


unsigned int failureCount = 0;



void setup() {
  
  //Setup the outputs, this needs to happen as soon as possible
  pinMode(ethernetReset, OUTPUT);
  pinMode(systemBusy, OUTPUT);
  pinMode(systemReady, OUTPUT);
  
  digitalWrite(systemBusy, true);
  digitalWrite(systemReady, false);
  
  //watchdogOn();
  //power_adc_disable();
  
  Serial.begin(9600);
  Serial.println(F("\nStarting..."));
  

  if (!readMACAddress()) {
      //The process failed, wait a bit and give it one more chance
      delay(250);
      if (!readMACAddress()) {
          //Serial.println(F("Error fetching hardware MAC address"));
          //It should fallback to the hardcoded value at the top
      }
  }
  
  Serial.print(mac[0], HEX);
  Serial.print(F(":"));
  Serial.print(mac[1], HEX);
  Serial.print(F(":"));
  Serial.print(mac[2], HEX);
  Serial.print(F(":"));
  Serial.print(mac[3], HEX);
  Serial.print(F(":"));
  Serial.print(mac[4], HEX);
  Serial.print(F(":"));
  Serial.println(mac[5], HEX);
  
  
  
  
  //Read the host name, url and request type out of the eeprom
  readConfig();
  Serial.print(F("Host:"));
  Serial.println(HOST);
  Serial.print(F("URL:"));
  Serial.println(URL);
  if (postRequest == 0) {
      Serial.println(F("GET"));
  } else if (postRequest == 1) {
      Serial.println(F("POST"));
  } else if (postRequest == 2) {
      Serial.println(F("POST-BODY"));
  } else {
      Serial.println(F("Unknown"));
  }
  
  
  //Start and configure the ethernet device
  if (!restartEthernet()) {
      Serial.println(F("Error Starting Ethernet Module"));
      //Wait and timeout
      //while(1) {}
  }
  
  
  //Startup is complete, the module is now ready
  Serial.println(F("Ready..."));
  digitalWrite(systemReady, true);
  digitalWrite(systemBusy, false);

}



char stringToSend[100];
char stringToSendEncoded[100];
char serverResponse[100];
unsigned int i = 0;
void loop() {
    ether.packetLoop(ether.packetReceive());
    wdt_reset();
  
  
    //See if a new character is available
    while (Serial.available()) {
        readChar = (char)Serial.read();
        
        
        if (readChar == '+') {
          
            digitalWrite(systemBusy, true);
          
            memset(&HOST[0], 0, sizeof(HOST));
            memset(&URL[0], 0, sizeof(URL));
            //Fetch the incomming config string
            serialConfig.readConfigString();
            serialConfig.getConfig(HOST, 0);
            serialConfig.getConfig(URL, 1);
            postRequest = serialConfig.getIntConfig(2);
            //Serial.println(postRequest);
            Serial.println(F("OK"));
            //Serial.print(F("Host:"));
            //Serial.println(HOST);
            //Serial.print(F("URL:"));
            //Serial.println(URL);
            
            //Save the new URL and HOST in the eeprom
            saveConfig();
            //restartEthernet();
            setHost();
            
            digitalWrite(systemBusy, false);
          
        } else {
            if (readChar == '\n') {
                //The string has been fetched - we can now check it
                stringFetched = true;
                stringToSend[i] = '\0';
                i = 0;
                continue;
            }
            stringToSend[i] = readChar;
            i++;
        }
    }
  
    //A complete string has been fetched      
    if (stringFetched) {
        digitalWrite(systemBusy, true);
      
        //Serial.println(stringToSend);
        
        //Create the request, POST or GET
        if (postRequest == 0)
        {
            Stash::prepare(PSTR("GET $S HTTP/1.1" "\r\n" "Host: $S" "\r\n" "\r\n"), URL, HOST);
        }
        else if (postRequest == 1)
        {
            ether.urlEncode(stringToSend, stringToSendEncoded);
            //Serial.println(stringToSendEncoded);
            Stash::prepare(PSTR("POST $S HTTP/1.1" "\r\n" 
                                "Content-Type: application/x-www-form-urlencoded" "\r\n" 
                                "Host: $S" "\r\n" 
                                "Connection: close" "\r\n" 
                                "Content-Length: $D" "\r\n" 
                                "\r\n"
                                "data=$S" "\r\n"
                                "\r\n"), &URL, &HOST, (strlen(stringToSendEncoded) + 5), stringToSendEncoded);
        }
        else if (postRequest == 2)
        {
            ether.urlEncode(stringToSend, stringToSendEncoded);
            //Serial.println(stringToSendEncoded);
            Stash::prepare(PSTR("POST $S HTTP/1.1\r\n" 
                                "Content-Type: application/json\r\n" 
                                "Host: $S\r\n" 
                                "Content-Length: $D\r\n" 
                                "\r\n"
                                "$S"), &URL, &HOST, (strlen(stringToSendEncoded)), stringToSendEncoded);
        }
        else
        {
            
        }
                            
                            

        // send the packet - this also releases all stash buffers once done
        ether.tcpSend();
        session_id = ether.tcpSend();
        waitingForResponse = true;
        sendTime = millis();
        
                
        //Reset
        memset(&stringToSend[0], 0, sizeof(stringToSend));
        stringFetched = false;
        //Serial.println("Request sent");
    }


    if (waitingForResponse) {
        //Check for a response
        const char* reply = ether.tcpReply(session_id);
        
        if(reply != 0) {
            //Serial.println("Reply Received");
            //Serial.println(reply);
            
            waitingForResponse = false;
            digitalWrite(systemBusy, false);
            
            memset(&serverResponse[0], 0, sizeof(serverResponse));  //Clear the array for the new packet
            
            //Parse the response
            if (parseResponse(reply, serverResponse)) {
              failureCount = 0;
              //Outout the result over the serial line
              Serial.println(serverResponse);
            } else {
              handleFailedResponse();
              /*
              Serial.println("ERROR");
              failureCount++;
              if (failureCount == 3) {
                restartEthernet();
                failureCount = 0;
              }
              */
            }
        }
        
        //Check how long we have been waiting and timeout
        if (waitingForResponse && ((millis() - sendTime) > 4000))
        {
            //Serial.println("ERROR");
            //failureCount++;
            handleFailedResponse();
            waitingForResponse = false;
            digitalWrite(systemBusy, false);
        }
    }
}


void handleFailedResponse()
{
    Serial.println("ERROR");
    failureCount++;
    if (failureCount == 3) {
        restartEthernet();
        failureCount = 0;
    }
}

boolean restartEthernet() {
    //Reset the ethernet module to ensure a clean start
    digitalWrite(ethernetReset, false);
    delay(50);
    digitalWrite(ethernetReset, true);
    delay(200);
    
    //Start the ethercard process, pass in the buffer size, mac address and CS port
    if (ether.begin(sizeof Ethernet::buffer, mac, 10) == 0) {
        return false;
    }
    Serial.println("Ethernet Started");
  
    if (!ether.dhcpSetup()) {
        delay(1000);
        if (!ether.dhcpSetup()) {
            return false;
        }
    }
    Serial.println("DHCP Setup");
    
    ether.printIp("IP: ", ether.myip);
    ether.printIp("Netmask: ", ether.netmask);
    //ether.printIp("GW IP: ", ether.gwip);
    ether.printIp("DNS IP: ", ether.dnsip);
    
    //Overide DNS With a decent one 
    ether.copyIp(ether.dnsip, dns_ip);
    ether.printIp("DNS IP: ", ether.dnsip);
  
    if (!setHost()) {
        delay(1000);
        if (!setHost()) {
            return false;
        }
    }
    
    Serial.println("DNS Lookup Complete");
    //ether.printIp("Server: ", ether.hisip);
    return true;
}


boolean setHost()
{
    if (!ether.dnsLookup(HOST, true)) {
        return false;
    }
    return true;
}


void saveConfig()
{
    for (int i = 0; i < 50; i++)
        EEPROM.write(i, HOST[i]);
    for (int i = 0; i < 50; i++)
        EEPROM.write((i+50), URL[i]);
    EEPROM.write(100, postRequest);
}

void readConfig()
{
    if (EEPROM.read(0) == 0xFF) {
        //FF implies the config is empty so leave it for the defaults
        return;
    }
    for (int i = 0; i < 50; i++) {
        HOST[i] = EEPROM.read(i);
    }
    for (int i = 0; i < 50; i++) {
        URL[i] = EEPROM.read(i+50);
    }
    postRequest = EEPROM.read(100);
}

//Read the mac address from the eeprom and load it into the local mac variable
boolean readMACAddress()
{
  //Start command
  if (!rtc.start(MAC_I2C_ADDRESS | I2C_WRITE)) {
    return false;
  }
  //Write the register we want o read
  if (!rtc.write(0xFA)) {
    return false;
  }
  //Start the read command
  if (!rtc.restart(MAC_I2C_ADDRESS | I2C_READ)) {
    return false;
  }
  //Read of the data from the 6 registers
  for (uint8_t i = 0; i < 6; i++) {
    // send NAK until last byte then send ACK
    mac[i] = rtc.read(i == 5);
  }
  rtc.stop();
  return true;
}



uint8_t parseResponse(const char receivedResponse[], char httpResponse[]) {
	
	uint8_t lastCharacterEOL = true;
	uint8_t httpBody = false;
	uint8_t statusLine = false;
	//uint8_t receiving = true;
	char statusCode[4];
	uint8_t i = 0;
	uint8_t n = 0;
        uint16_t x = 0;
	//uint32_t lastCharacterTime = millis();
	char c;

        //Serial.println("\n-- Parsing Response --");
        //Serial.print("Response Size ");
        //Serial.println(strlen(receivedResponse));

	//while (receiving) {
	  while(x < strlen(receivedResponse)) {

		c = receivedResponse[x];
		//Serial.print(c);

		//Detect the start of the status line
		if (c == ' ' && !statusLine) {
			//Status code is starting soon
			statusLine = true;

			//Clear the status code array ready for the incomming value
			memset(&statusCode[0], 0, sizeof(statusCode));
		}
		
		//Collect the status code
		if (statusLine && i < 3 && c != ' ') {
			statusCode[i] = c;
			i++;
		}
		
		//a 3 digit status code has been received, make sure its a 200
		if(i == 3) {
			if (strcmp(statusCode, "200") != 0) {
				Serial.println(statusCode);
				return false;
			} else {
				//Move along so we dont hit this statement again
				i++;
			}
		}
        
		//New line character
		if (c == '\n') {

			//If we are in the body a new line indicates the end
			if (httpBody) {
				//receiving = false;
                            break;
			}

			//If we have two new line characters the body is starting
			if (lastCharacterEOL && !httpBody) {
				httpBody = true;
			}

		}

		//Keep track of new line characters
		if (c == '\n') {
			lastCharacterEOL = true;
		} else if (c != '\r') {
			//Normal character received
			lastCharacterEOL = false;
		}

		//Start collecting the body message
		if (!lastCharacterEOL && httpBody){
			//_serial->print(c);
			httpResponse[n] = c;
			n++;
		}

		//When was the last character received. - timeout detection
		//lastCharacterTime = millis();

              x++;
	  }
	  /*
	  //Keep track of the time incase something gets stuck and we need to timeout
	  if ((millis() - lastCharacterTime) > 250) {
		//Stop receiving and return what we have got
		receiving = false;
		//_serial->println("Timeout");
	  }
	//}
	*/
	//Serial.println(httpResponse);
	
	return true;
}


void watchdogOn() {
  
  // Clear the reset flag, the WDRF bit (bit 3) of MCUSR.
  MCUSR = MCUSR & B11110111;
    
  // Set the WDCE bit (bit 4) and the WDE bit (bit 3) 
  // of WDTCSR. The WDCE bit must be set in order to 
  // change WDE or the watchdog prescalers. Setting the 
  // WDCE bit will allow updtaes to the prescalers and 
  // WDE for 4 clock cycles then it will be reset by 
  // hardware.
  WDTCSR = WDTCSR | B00011000; 
  
  // Set the watchdog timeout prescaler value to 1024 K 
  // which will yeild a time-out interval of about 8.0 s.
  WDTCSR = B00100001;
  
  // Enable the watchdog timer interupt.
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;

}

