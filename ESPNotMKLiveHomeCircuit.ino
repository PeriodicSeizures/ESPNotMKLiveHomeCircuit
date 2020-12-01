#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <string>
#include <ESP32ServoRW.h>

#include "esp_camera.h"
#include "camera_pins.h"

using namespace websockets;

const int FORWARD = HIGH;
const int BACKWARD = LOW;

const char* ssid = "Linksys08286";
const char* password = "1pbdajb3i7";
const char* websocket_server_host = "192.168.1.200";
const uint16_t websocket_server_port = 8888;

// config (can be changed by client)
uint8_t cfg_m_speed = 195; // motor speed
int cfg_s_offset = 0;       // servo offset from center
uint8_t cfg_s_angle = 35;        // max servo turn angle (relative to center)

// Motor
const int m_in1 = 12;
const int m_in2 = 13;
const int m_pwm = 15;
uint8_t m_speed = 0; // current speed to go at
uint8_t m_direction = 0; // current forward/backward

// Servo
Servo myservo(14); // ledc channel 14
const int s_pin = 14;
uint8_t s_rotation = 90; // servo rotation

// pwm properties
const int __freq = 30000;
const int __motorChannel = 8; //0 err?
const int __resolution = 8; //Resolution 8, 10, 12, 15

// through research 0% duty does nothing
//const int MIN_DUTY = 155; // like 10% 
//const int MAX_DUTY = 240; // 100%

void operateMotor() {
  digitalWrite(m_in1, m_direction);
  digitalWrite(m_in2, !m_direction);
  
  ledcWrite(__motorChannel, m_speed);
}

void operateServo() {
  myservo.write(s_rotation + cfg_s_offset);
}

void onMessageCallback(WebsocketsMessage message) {

  //Serial.print("received: ");
  //Serial.println(message.data());
  
  const char *s = message.c_str();

  // steering
  switch (s[0]) {
    case 's': { // normal steering
      if (s[2] == 'a')
        s_rotation = 90 + cfg_s_angle;
      else if (s[2] == 'd')
        s_rotation = 90 - cfg_s_angle;
      else if (s[2] == '0')
        s_rotation = 90;
      break;
    }
    case 'v': { // normal speed (forward/backward)
      if (s[2] == 'f') {
        m_direction = FORWARD;
        m_speed = cfg_m_speed;
      } else if (s[2] == 'b') {
        m_direction = BACKWARD;
        m_speed = cfg_m_speed;
      } else if (s[2] == '0') {
        m_speed = 1;
      } break;
    }
    case 'm': { // actual motor speed
      cfg_m_speed = (uint8_t)atoi(s+2);
      //Serial.print("set speed to: ");
      //Serial.println(cfg_m_speed, DEC);
      break;
    }
    case 'o': { // steering offset
      cfg_s_offset = atoi(s+2);
      //Serial.print("set offset to: ");
      //Serial.println(cfg_s_offset, DEC);
      break;
    }
    case 'a': { // servo max turn angle
      cfg_s_angle = (uint8_t)atoi(s+2);
      //Serial.print("set max angle to: ");
      //Serial.println(cfg_s_angle, DEC);
      break;
    }
  }
  /*
  Serial.print("m_direction: ");
  Serial.println(m_direction, DEC);

  Serial.print("m_speed: ");
  Serial.println(m_speed, DEC);*/
}

// events are generally connection related
void onEventsCallback(WebsocketsEvent event, String data) {
  if(event == WebsocketsEvent::ConnectionClosed) {
      Serial.print("Connnection Closed. Reconnecting");
      tryConnectToWebsocketServer();
  }
}

WebsocketsClient client;
void tryConnectToWebsocketServer() {
  while(!client.connect(websocket_server_host, websocket_server_port, "/")){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Websocket Connected to host");
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  /*
    MOTOR INIT
  */
  pinMode(m_in1, OUTPUT);
  pinMode(m_in2, OUTPUT);
  pinMode(m_pwm, OUTPUT);

  ledcSetup(__motorChannel, __freq, __resolution);
  ledcAttachPin(m_pwm, __motorChannel);

  /********************************************************
   
                        CAMERA INIT
    
  *********************************************************/
  //Serial.println("Initializing camera");
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  
  Serial.println("Camera initialized");

  // servo init
  myservo.attach(s_pin);
  
  /********************************************************
   
                          WIFI INIT
    
  *********************************************************/
  //Serial.print("Connecting to WiFi");

  Serial.println("Delaying to avoid brownout (0)... (1s)");
  delay(1000);

  WiFi.enableSTA(true); // wifi debug to serial
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("WiFi connected");

  Serial.println("Delaying to avoid brownout (1)... (1s)");
  delay(1000);

  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);
  
  tryConnectToWebsocketServer();

  Serial.println("Delaying to avoid brownout (2)... (1s)");
  delay(1000);
}

void loop() {
  operateMotor();
  operateServo();
  
  client.poll();
  
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    //delay(500);
    return;
  }

  if(fb->format != PIXFORMAT_JPEG){
    Serial.println("Non-JPEG data not implemented");
    //delay(500);
    return;
  }

  client.sendBinary((const char*) fb->buf, fb->len);
  esp_camera_fb_return(fb);
}
