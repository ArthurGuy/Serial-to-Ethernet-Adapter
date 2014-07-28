#include "SerialConfig.h"

//Setup the web request
SerialConfig::SerialConfig(Stream &serial) {
	_serial = &serial;
}


void SerialConfig::readConfigString() {
	char readChar;
	uint8_t configStringIndex = 0;	//Which string are we capturing
	uint8_t characterIndex = 0;		//Which character of the string are we on
	//char configStrings[numConfigStringsExpected][maxConfigStringLength];
	uint8_t finished = false;
	uint32_t lastCharacterTime = millis();
	
	while (!finished) {
		while (_serial->available()) {
			readChar = (char)_serial->read();
		
			if (readChar == '\n') {
				//End of the setup string
	
				//Terminate the last string
				configStrings[configStringIndex][characterIndex] = '\0';
	
				//Save the new details
				//saveConfig();
	
				//_serial->println(F("Config Fetched"));
				//for(configStringIndex = 0; configStringIndex < 2; configStringIndex++) {
				//	_serial->println(configStrings[configStringIndex]);
				//}
	
				//Reset
				characterIndex = 0;
				configStringIndex = 0;
				finished = true;
				break;
			} else if (readChar == '\r') {
				//Ignore
			} else if (readChar == ':') {
				
				//Terminate the last string
				configStrings[configStringIndex][characterIndex] = '\0';
				
				//Increment to the next string
				configStringIndex++;
				//Make sure we dont go past the set limit
				if (configStringIndex >= 3) {
					//_serial->println(F("To many strings"));
					return;
				}
				
				//Reset the index count
				characterIndex = 0;
			} else {
				configStrings[configStringIndex][characterIndex] = readChar;
				characterIndex++;
			}
			//When was the last character received. - timeout detection
			lastCharacterTime = millis();
		}
		//Timeout check
		//Keep track of the time incase something gets stuck and we need to timeout
		if ((millis() - lastCharacterTime) > 250) {
			//Stop receiving and return what we have got
			finished = true;
			//_serial->println(F("Timeout"));
		}
	}
}

void SerialConfig::getConfig(char targetArray[], uint8_t configStringIndex) {
	//targetArray = configStrings[configStringIndex];
	//_serial->println(configStrings[configStringIndex]);
	//strcpy(configStrings[configStringIndex], targetArray);
	//The methods above dont work!
	for (int i = 0; i < sizeof(configStrings[configStringIndex]); i++) {
		targetArray[i] = configStrings[configStringIndex][i];
	}
}

boolean SerialConfig::getBooleanConfig(uint8_t configStringIndex) {
	return (configStrings[configStringIndex][0] - 48);
}

uint8_t SerialConfig::getIntConfig(uint8_t configStringIndex) {
	return (configStrings[configStringIndex][0] - 48);
}

/*
void SerialConfig::saveToEEPROM(char httpResponse[][]) {

	for (int i = 0; i < 50; i++) {
		EEPROM.write(i, HOST[i]);
	}
	for (int i = 0; i < 50; i++) {
		EEPROM.write((i+50), URL[i]);
	}
	
}
*/


