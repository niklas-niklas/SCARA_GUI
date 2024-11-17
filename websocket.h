#include <WiFi.h>
#include <WebServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <MoveBuffer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>

// #include <varibles.h>

void XY_AngDis(float x,float y);


void home();
void moveServo(int i);
void quad_check();
void flip();
void serial();
void coords();

void forwardkinematics();
void presets();

//Preset shapes
void preset1();
void preset2();
void preset3();
void preset4();
bool valid();
MoveBuffer xy_positions;
MoveBuffer ang_positions;

String status = "blank";
float add_move_x  = 1000;
float add_move_y  = 1000;

float ratio_1 = 1, ratio_2 = 1;

float speed = 0.25;
float speed_new = 0.25;
float jog_new = 10;
float jog = 10;

bool stop = 0;

bool moving = 0;

bool hoome = true;
String toggleState = "Up";

int preset;

int up = 90,down = 180;

unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

int chunksize=0;

// Replace with your network credentials
// const char* ssid = "AV-WIFI";
// const char* password = "password";
const char* ssid = "ALEX_PC";
const char* password = "NotPassword";

float new_x,new_y;   
  float xValue ,
  yValue ,
  ang1 ,
  ang2 ; 

String  line,  line_new;
int num,line_num_new;

int g,new_g = 100;
float g_x, new_g_x = 1000, g_y, new_g_y = 1000;

String sendCoordinatesToWeb();//float xValue, float yValue, float xRobot, float yRobot
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void initWebSocket();
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len);
void initWiFi();
void initLittleFS();
void notifyClients(String sensorReadings);


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len);

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initLittleFS(){
    if (!LittleFS.begin(true)){
        Serial.println("An error has occurred while mounting LittleFS");
    }
    Serial.println("LittleFS mounted successfully");
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

    if(!MDNS.begin("SCARA")){
    Serial.println(WiFi.localIP());
}
Serial.println("Address: " + (String)MDNS.hostname(0));
}



void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char*)data;
        // Check if the message is "getReadings"
        if (strcmp((char*)data, "getXY") == 0) {
        //if it is, send current sensor readings
            String sending = sendCoordinatesToWeb();
            Serial.print(sending);
            notifyClients(sending);

        }
        if (strcmp((char*)data, "home") == 0) {hoome = true;}
        if (strcmp((char*)data, "servoup") == 0) {moveServo(0);}
        if (strcmp((char*)data, "servodown") == 0) {moveServo(180);}
        if (strcmp((char*)data, "preset1") == 0) {preset=1;}
        if (strcmp((char*)data, "preset2") == 0) {preset=2;}
        if (strcmp((char*)data, "preset3") == 0) {preset=3;}
        if (strcmp((char*)data, "preset4") == 0) {preset=4;}
        if (strcmp((char*)data, "clear") == 0) {preset=5;}
        if (strcmp((char*)data, "e_stop") == 0) {preset=6;}


        if (strcmp((char*)data, "+X") == 0) {
          add_move_x = xValue+jog;
          add_move_y = yValue;
        }
        if (strcmp((char*)data, "-X") == 0) {
          add_move_x = xValue-jog;
          add_move_y = yValue;
        }
        if (strcmp((char*)data, "+Y") == 0) {
          add_move_x = xValue;
          add_move_y = yValue+jog;
        }
        if (strcmp((char*)data, "-Y") == 0) {
          add_move_x = xValue;
          add_move_y = yValue-jog;
        }

        if (message.startsWith("values")) {
            // Extract X and Y values from the message
            
            // Serial.println(message);
            int parsedItems = sscanf(message.c_str(), "values %f,%f", &new_x, &new_y);
            if ( parsedItems== 2) {
                // Successfully parsed X and Y values
                Serial.print("Received X and Y values: ");
                Serial.print(new_x);
                Serial.print(", ");
                Serial.println(new_y);
                add_move_x = new_x;
                add_move_y = new_y;
                // XY_AngDis(new_x,new_y);
                // Do something with the X and Y values here
            } else {
                // Parsing failed
                Serial.println("Failed to parse X and Y values");
            }
        }
        if (message.startsWith("varibles")) {
            // Extract X and Y values from the message
            
            Serial.println(message);
            int parsedItems = sscanf(message.c_str(), "varibles %f,%f", &jog, &speed);
            if ( parsedItems== 2) {
                // Successfully parsed X and Y values
                jog_new = jog;
                speed_new = speed;

            } else {
                // Parsing failed
                Serial.println("Failed to parse X and Y values");
            }
        }
        if (message.startsWith("file")) {
            // Extract X and Y values from the message
            
            // Serial.println(message);
            int parsedItems = sscanf(message.c_str(), "file[%d] G%d X%f Y %f", &num, &g, &g_x, &g_y );
            if ( parsedItems>= 1) {
                // Successfully parsed line number and data
                switch (parsedItems)
                {
                case 1:

                  Serial.print("Received G-code: file[");
                  Serial.println(num);
                  Serial.print("]");
                  break;
                case 2:

                  Serial.print("Received G-code: file[");
                  Serial.print(num);
                  Serial.print("] G");
                  Serial.println(g);
                  if(g==0){
                    add_move_x = 0;
                    add_move_y = 280;
                  }
                  else if(g==9){
                    moveServo(0);
                  }
                  else if(g==8){
                    moveServo(180);
                  }
                  break;
                case 4:

                  Serial.print("Received G-code: file[");
                  Serial.print(num);
                  Serial.print("] G");
                  Serial.print(g);
                  Serial.print(" X");
                  Serial.print(g_x);
                  Serial.print(" Y");
                  Serial.println(g_y); // Print the entire G-code command
                  new_g = g;
                  new_g_x = g_x;
                  new_g_y = g_y;
                  break;
                default:
                  break;
                }

                chunksize++;
                if(chunksize==49){
                  Serial.println("waiting");
                  delay(150);
                  notifyClients("chunk_received");
                  
                  chunksize=0;
                  
                }
                delay(100);

            } 
            else {
                // Parsing failed
                Serial.println("Failed to parse X and Y gcode");
            }
        }
    }
}

String sendCoordinatesToWeb()//float xValue, float yValue, float ang1, float ang2
{

  // Create a message in the format "x,y"
  String message = String(xValue) + "," + String(yValue) + "," + String(ang1) + "," + String(ang2) + "," + toggleState+","+ String(status);

  // Send the message to all connected clients
  return message;

  // Serial.println("Sent coordinates: " + message);
}

void notifyClients(String sending) {
  ws.textAll(sending);
}