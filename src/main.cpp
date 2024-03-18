#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

#define UUU 111
#define UUR 112
#define UUD 113
#define UUL 114
#define URU 121
#define URR 122
#define URD 123
#define URL 124
#define UDU 131
#define UDR 132
#define UDD 133
#define UDL 134
#define ULU 141
#define ULR 142
#define ULD 143
#define ULL 144
#define RUU 211
#define RUR 212
#define RUD 213
#define RUL 214
#define RRU 221
#define RRR 222
#define RRD 223
#define RRL 224
#define RDU 231
#define RDR 232
#define RDD 233
#define RDL 234
#define RLU 241
#define RLR 242
#define RLD 243
#define RLL 244
#define DUU 311
#define DUR 312
#define DUD 313
#define DUL 314
#define DRU 321
#define DRR 322
#define DRD 323
#define DRL 324
#define DDU 331
#define DDR 332
#define DDD 333
#define DDL 334
#define DLU 341
#define DLR 342
#define DLD 343
#define DLL 344
#define LUU 411
#define LUR 412
#define LUD 413
#define LUL 414
#define LRU 421
#define LRR 422
#define LRD 423
#define LRL 424
#define LDU 431
#define LDR 432
#define LDD 433
#define LDL 434
#define LLU 441
#define LLR 442
#define LLD 443
#define LLL 444

#define cmdTreeSize 21

const uint16_t irPin = 4;

IRsend irsend(irPin);

// WiFi configs
const char* ssid = "FAMILIA MEDEIROS";
const char* password = "sl23jo316";

WiFiClient espClient;
ESP8266WebServer server(3000);

//Default Command tree
unsigned short int commandTree[cmdTreeSize][3] = {{UUU, 0x0, 0x21},
                                                  {UUR, 0x0, 0x22},
                                                  {UUD, 0x0, 0x23},
                                                  {UUL, 0x0, 0x24},
                                                  {URU, 0x0, 0x25},
                                                  {URR, 0x0, 0x26},
                                                  {URD, 0x0, 0x27},
                                                  {URL, 0x0, 0x28},
                                                  {UDU, 0x0, 0x29},
                                                  {UDR, 0x0, 0x30},
                                                  {UDD, 0x0, 0x31},
                                                  {UDL, 0x0, 0x32},
                                                  {ULU, 0x0, 0x33},
                                                  {ULR, 0x0, 0x34},
                                                  {ULD, 0x0, 0x35},
                                                  {ULL, 0x0, 0x36},
                                                  {RUU, 0x0, 0x37},
                                                  {RUR, 0x0, 0x38},
                                                  {RUD, 0x0, 0x39},
                                                  {RUL, 0x0, 0x40},
                                                  {DUU, 0x0, 0x41}};


JsonObject lastCommand;

DynamicJsonDocument jsonCmdTree(1536);

void setCmdTreeJson(){
  for(uint i = 0; i<cmdTreeSize; i++){
    JsonObject cmd  = jsonCmdTree.createNestedObject(i);
    cmd["movement"] = commandTree[i][0];
    char irAddressStr[8];
    snprintf(irAddressStr, sizeof(irAddressStr), "0x%02X", commandTree[i][1]);
    char irDataStr[8];
    snprintf(irDataStr, sizeof(irDataStr), "0x%02X", commandTree[i][2]);
    cmd["irAddress"] = irAddressStr;
    cmd["irData"] = irDataStr;
  }
  lastCommand["movement"] = 0;
  lastCommand["irAddress"] = 0x00;
  lastCommand["irData"] = 0x00;
}

void reconnectWiFi(){
  // Starting the conection
  WiFi.begin(ssid, password);
  // Waiting conection been established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void deserialErroMsg(DeserializationError error){
  Serial.print(F("Error parsing JSON "));
  Serial.println(error.c_str());
  String msg = error.c_str();
  server.send(400, F("text/html"),
    "Error in parsin json body! <br>" + msg);
}

void dataErrorMsg(){
  DynamicJsonDocument doc(512);
  doc["status"] = "KO";
  doc["message"] = F("No data found, or incorrect!");
  String buf;
  serializeJson(doc, buf);
  server.send(400, F("aplication/json"), buf);
  Serial.print(F("done"));
  Serial.println("");
}

void postDoneOk(){
  DynamicJsonDocument doc(512);
  doc["status"] = "OK";
  String buf;
  serializeJson(doc, buf);
  server.send(201, F("application/json"), buf);
  Serial.print(F("done."));
  Serial.println("");
}

void sendIRbyMoveSequence(unsigned short int movement){
  for (int row = 0; row < cmdTreeSize; row++) {
    if(movement == jsonCmdTree[row]["movement"]){
      JsonObject tempCmd = jsonCmdTree[row];
      irsend.sendNEC(irsend.encodeNEC(jsonCmdTree[row]["irAddress"], jsonCmdTree[row]["irData"]));
      Serial.println("Comando enviado!");
      lastCommand = tempCmd;
      serializeJson(lastCommand, Serial);
      Serial.println();
      break;
    }
  }
}

void changeCommandByMoveSequence(JsonObject body){
  boolean finded = false;
  for(uint i =0; i<cmdTreeSize; i++){
    if(jsonCmdTree[i]["movement"] == body["movement"]){
      finded = true;
      jsonCmdTree[i]["irAddress"] = body["newIrAddress"];
      jsonCmdTree[i]["irData"] = body["newIrData"];
    }
  }
  if (finded){
    postDoneOk();
  } else {
    dataErrorMsg();
  }
}

void getCommands(){
  if(server.method() == HTTP_GET){
    String buf;
    serializeJson(jsonCmdTree, buf);
    server.send(200, "application/json", buf);
    Serial.print(F("done."));
    Serial.println("");
  } else {
    dataErrorMsg();
  }
}

void changeCommand(){
  String postBody = server.arg("plain");
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postBody);
  if(error){
    deserialErroMsg(error);
  } else {
    JsonObject objBody = doc.as<JsonObject>();
    if(server.method() == HTTP_POST){
      if(objBody.containsKey("movement") && objBody.containsKey("newIrAddress") && objBody.containsKey("newIrData")){
        changeCommandByMoveSequence(objBody);
      }
    }
  }
}

void getLastCommand(){
  String buf;
  serializeJson(lastCommand, buf);
  server.send(200, "application/json", buf);
  Serial.print(F("done."));
  Serial.println("");
}

void makeMovement(){
  String postBody = server.arg("plain");
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postBody);
  if(error){
    deserialErroMsg(error);
  } else {
    JsonObject objBody = doc.as<JsonObject>();
    if(server.method() == HTTP_POST){
      if(objBody.containsKey("movement")){
        // ###
        // code for IRsend and change lastVar
        sendIRbyMoveSequence(objBody["movement"]);
        postDoneOk();
      } else {
        dataErrorMsg();
      }
    }
  }
}

void setupWifi(){
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // Seting the ESP8266 as a client not an AP 
  WiFi.mode(WIFI_STA);
  reconnectWiFi();
  // Visual confirmation
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
  
}

void restServerRouting() {
  server.on("/", HTTP_GET, []() {
      server.send(200, F("text/html"),
          F("<h1>Welcome to the REST Web Server</h1><h2>Routes:</h2><ul><li>GET/var = get all possibles moviments and what they return</li><li>POST/var = change a variable</li><li>GET/lastVar = get the last movement made</li><li>POST/makeMove = movemente tied with the comand: {'movement' = int}</li></ul>"));
  });
  server.on(F("/command"), HTTP_GET, getCommands);
  server.on(F("/command"), HTTP_POST, changeCommand);
  server.on(F("/lastCommand"), HTTP_GET, getLastCommand);
  server.on(F("/makeMove"), HTTP_POST, makeMovement);
}

void setup() {
  irsend.begin();
  Serial.begin(9600);
  setCmdTreeJson();
  setupWifi();
  restServerRouting();
  server.begin();
}

void loop() {
  if(WiFi.status() != WL_CONNECTED){
    reconnectWiFi();
  }
  server.handleClient();
}