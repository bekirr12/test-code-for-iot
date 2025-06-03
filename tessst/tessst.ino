#include "src/Twin/Twin.h"
#include <WiFi.h>
// #include <HTTPClient.h>
// #include <UrlEncode.h>


// define ble channel
channel_st bleChannel;
// Global Variables
QueueHandle_t bleQueue;
QueueHandle_t responseQueue;

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;

bool deviceConnected =false ;

// String phoneNumber = "+905312892096";
// String apiKey = "2542756";

const char* ssid = "FiberHGW_TPB4AE";
const char* password = "bekiradamerkanadam";

// Command structure example (modify as needed)
struct Command {
  uint8_t data[ITEM_SIZE];
  //unsigned long timestamp;

};

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    pCharacteristic->setValue("0");
    pServer->startAdvertising();  // Need to restart advertising when disconnected
  }
};




// BLE Characteristic Callback
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) {
      Command cmd;
      String value = pChar->getValue();
      if(value.length() > 0) {
        // Veriyi güvenli şekilde kopyala
        //size_t copySize = min(value.length(), sizeof(cmd.data) - 1);
        memcpy(cmd.data, value.c_str(), value.length());
        //cmd.data[copySize] = '\0';  // Null terminator ekle
      
        if (xQueueSendToBack(bleQueue, &cmd, 0) != pdTRUE) {
          Serial.println("queue error");
        }
         vTaskDelay(pdMS_TO_TICKS(1)); 
      }    
  }  
  
};





void setup() {
  Serial.begin(115200);
  pinMode(15, OUTPUT); 
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi bağlantısı başarılı!");
    Serial.print("IP adresi: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi bağlantısı başarısız.");
  }
 
  // Create BLE queue
  bleQueue = xQueueCreate(QUEUE_LENGTH, sizeof(Command));
  responseQueue = xQueueCreate(QUEUE_LENGTH, sizeof(Command));

  setupBLE();

  xTaskCreate(processingTask, "ProcessingTask", 8192, NULL, 1, NULL);
  xTaskCreate(responseTask, "response", 4096, NULL, 1, NULL);
  
  Serial.println("IoT is Ready!");
}

void loop() {
  vTaskDelete(NULL);  
}

void processingTask(void *pvParameters) {
  Command receivedCmd;
  
  while(1) {
    if (xQueueReceive(bleQueue, &receivedCmd, pdMS_TO_TICKS(100)) == pdTRUE) {
      // Process the command
      
      handleCommand(receivedCmd);
    }
      vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}



void responseTask(void *pvParameters) {
  uint8_t response[20];
  
  while(1) {
    if (xQueueReceive(responseQueue, &response, pdMS_TO_TICKS(100)) == pdTRUE) {
     
      pCharacteristic->setValue(response, header_length + bleChannel.sent.message_length);
      pCharacteristic->notify();
      
    }
      vTaskDelay(pdMS_TO_TICKS(2)); 
  }
}



void getResponse(){


  uint8_t response[40] = {0};
    response[0] = SYNC_BYTE1;
    response[1] = SYNC_BYTE2;
    response[2] = SYNC_BYTE3;
    response[3] = bleChannel.sent.message_id;
    response[4] = bleChannel.sent.message_length;
    for(uint8_t i = 0; i < bleChannel.sent.message_length; i++)
    response[header_length+i] = bleChannel.sent.message_data[i];

    Serial.print("Sending Data: ");
    for (int i = 0; i < header_length + bleChannel.sent.message_length; i++) {
      Serial.print(response[i],HEX);
      Serial.print(" ");
    }
    Serial.println();

    if (xQueueSendToBack(responseQueue, &response, 0) != pdTRUE) {
          Serial.println("response queue error");
        }
}


void digitalWrite_F(){
    uint8_t pin = 35;         
    pinMode(pin, OUTPUT);
    uint8_t value = bleChannel.received.message_data[1];
    digitalWrite(pin, value);
    /*
    if (value == 0x00){
      sendMessage("Led off!");
    } else if (value == 0XFF){
      sendMessage("Led on");
    } else {
      sendMessage("unknown characters");
    }
    */
    bleChannel.sent.message_length = 1;
    uint8_t response_data[1] = {1};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
    memcpy(bleChannel.sent.message_data, response_data, bleChannel.sent.message_length);
    bleChannel.sent.message_id = DIGITAL_WRITE;

}




void digitalRead_F(){

  digitalWrite(15, HIGH); 
    
  uint8_t pin = 12;         
  pinMode(pin, INPUT); 

  uint8_t response_data[2] = {bleChannel.received.message_data[0],digitalRead(pin)};

  bleChannel.sent.message_length = 2;
  memcpy(bleChannel.sent.message_data, response_data, bleChannel.sent.message_length);                 
  bleChannel.sent.message_id = DIGITAL_READ;


}


void initTwin_F(){
  pCharacteristic->setValue("0");


  pinMode(35, OUTPUT);  // D6
  digitalWrite(35, LOW);


   

  pinMode(12, INPUT);   // A6
  digitalWrite(12, LOW);


   
  digitalWrite(15, LOW);



 uint8_t response_data[1] = {1};
 bleChannel.sent.message_length = 1;   
  memcpy(bleChannel.sent.message_data, response_data, bleChannel.sent.message_length);             
  bleChannel.sent.message_id = INIT_TWIN;
                

}



void getfirmwareVersion_F(){

 uint8_t response_data[2] = {FIRMWARE_VERSION_MINOR,FIRMWARE_VERSION_MAJOR};
 bleChannel.sent.message_length = 2;   
 memcpy(bleChannel.sent.message_data, response_data, bleChannel.sent.message_length);             
 bleChannel.sent.message_id = GETFIRMWAREVERSION;

 
                   
} 





// Example command handler (replace with your logic)
void handleCommand(Command cmd) {
  
  if (cmd.data[0] == SYNC_BYTE1 && cmd.data[1] == SYNC_BYTE2 && cmd.data[2] == SYNC_BYTE3 ){
  

    bleChannel.received.message_id = cmd.data[3];
    bleChannel.received.message_length = cmd.data[4];
    for (int i = 0; i < bleChannel.received.message_length; i++) 
      bleChannel.received.message_data[i] = cmd.data[header_length + i] ;


      Serial.print("Received command: ");
    for (int i = 0; i < bleChannel.received.message_length + header_length; i++) 
      Serial.printf("%02X ", cmd.data[i]);
    //Serial.print(cmd.timestamp);
    Serial.println();

    applyCommands();
    
    getResponse();
  }
  else {
      Serial.println("Invalid sync bytes");
  }     

}
/*
void wifiConfig_F() {
  uint8_t* buf = bleChannel.received.message_data;
  uint8_t ssidLen = buf[0];
  String ssid; for(int i=0;i<ssidLen;i++) ssid+=char(buf[1+i]);
  uint8_t pwdLen = buf[1+ssidLen];
  String pwd; for(int i=0;i<pwdLen;i++) pwd+=char(buf[2+ssidLen+i]);
  Serial.printf("Configuring Wi-Fi: SSID='%s' PWD='%s'\n",ssid.c_str(),pwd.c_str());

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pwd.c_str());
  unsigned long start=millis(); while(WiFi.status()!=WL_CONNECTED && millis()-start<5000){delay(200);Serial.print('.');}
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("wifi connection failed!");

    uint8_t failPayload[1] = {0x00};
    bleChannel.sent.message_id = CMD_WIFI_CONFIG;
    bleChannel.sent.message_length = 1;
    memcpy(bleChannel.sent.message_data, failPayload, 1);
    return;
  }

  IPAddress ip = WiFi.localIP();
  Serial.printf("Connected! IP=%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

  uint8_t payload[4];
  payload[0] = ip[0];
  payload[1] = ip[1];
  payload[2] = ip[2];
  payload[3] = ip[3];

  bleChannel.sent.message_id = CMD_WIFI_CONFIG;
  bleChannel.sent.message_length = 4;
  memcpy(bleChannel.sent.message_data, payload, 4);
}
*/
/*
void sendMessage(String message){

  // Data to send with HTTP POST
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(message);    
  HTTPClient http;
  http.begin(url);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Send HTTP POST request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    Serial.print("Message sent successfully");
  }
  else{
    Serial.println("Error sending the message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
}
*/

void applyCommands(){  
    switch (bleChannel.received.message_id)
    {
      case DIGITAL_WRITE:
        digitalWrite_F();
        break;

      case DIGITAL_READ:
        digitalRead_F();
        break;
 

      case INIT_TWIN:
        initTwin_F();        
        break;
        
      case GETFIRMWAREVERSION:
        getfirmwareVersion_F();        
        break;
      
      /*case CMD_WIFI_CONFIG:
        wifiConfig_F();        
        break; */

    }
}



void setupBLE() {
  // Initialize BLE
  BLEDevice::init("Twin");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE_NR |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
  BLEDevice::setMTU(64);


  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("0"); 
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

}

