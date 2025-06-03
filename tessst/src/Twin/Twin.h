#ifndef TWIN_H
#define TWIN_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>



#define QUEUE_LENGTH 256
#define ITEM_SIZE 64  // Adjust based on your command size

//Service and characteristic UUIDs
#define SERVICE_UUID        "0000FFE0-0000-1000-8000-00805F9B34FB" // FFE0 short format
#define CHARACTERISTIC_UUID "0000FFE1-0000-1000-8000-00805F9B34FB" // FFE1 

// Firmware version 2.3
#define FIRMWARE_VERSION_MAJOR 2
#define FIRMWARE_VERSION_MINOR 3


#define header_length  5 // SYNC_BYTE1, SYNC_BYTE2, SYNC_BYTE3, msgID, msglength



// Protocol constants
const uint8_t SYNC_BYTE1 = 0xAA;
const uint8_t SYNC_BYTE2 = 0x44;
const uint8_t SYNC_BYTE3 = 0x1C;


struct data_st
{
	uint8_t	  message_length = 0;
	uint8_t	  message_id = 0;
	uint8_t	  message_data[64] = {0};
};

struct channel_st
{
	data_st received;
	data_st sent;
};


void applyCommands();
void sendResponse(uint8_t msgId,uint8_t length, uint8_t* data);
void setupBLE();


// Message IDs
enum MessageID {
    DIGITAL_WRITE = 0x01,
    DIGITAL_READ = 0x02,
	INIT_TWIN = 0x0E,
    GETFIRMWAREVERSION = 0x14,
	CMD_WIFI_CONFIG    = 0x20,
 
};

#endif
