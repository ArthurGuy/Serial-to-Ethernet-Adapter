#ifndef SerialConfig_h
#define SerialConfig_h

#include <Arduino.h>


class SerialConfig {
  public:
  
  	/**
	 * Constructor
	 *
	 * @param  Stream 		Serial port for debugging and data io
	 * @param  Client 		The web client, WiFi or Ethernet
	 * @param  array[char] 	Host to communicate with
	 * @param  array[char] 	The url to send data to
	 * @return void
	 */	
  	SerialConfig(Stream &serial);

	void readConfigString();
	
	//void SerialConfig::saveToEEPROM(char httpResponse[][]);
	
	void getConfig(char targetArray[], uint8_t configStringIndex);
	
	boolean getBooleanConfig(uint8_t configStringIndex);
	
	uint8_t getIntConfig(uint8_t configStringIndex);
    
  private:
  
  	Stream* _serial;
  	char configStrings[3][50];

};



#endif

