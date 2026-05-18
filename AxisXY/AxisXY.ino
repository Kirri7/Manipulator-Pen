#include "GyverPlanner.h"
#include <Arduino.h>

// Выбор режима получения данных:
#define USE_WIFI_MODE  - пакеты приходят по WiFi
// без #define            - пакеты приходят по Serial

#ifdef USE_WIFI_MODE
#include <WiFi.h>
#endif

// A2-5
#define button1Pin 13
#define button2Pin 12
#define button3Pin 14
#define button4Pin 27
const int Z_Pin = 17;
#define LED_BUILTIN 2

const int pointAm = 57;  // кол-во точек в массиве
int dist = 100;

const int32_t path1[][1] = {
{-3000},{-2900},{-2800},{-2700},{-2600},{-2500},{-2400},{-2300},{-2200},{-2100},{-2000},{-1900},{-1800},{-1700},{-1600},{-1500},{-1400},{-1300},{-1200},{-1100},{-1000},{-900},{-800},{-700},{-600},{-500},{-400},{-300},{-200},{-100},{0},{100},{200},{300},{400},{500},{600},{700},{800},{900},{1000},{1100},{1200},{1300},{1400},{1500},{1600},{1700},{1800},{1900},{2000},{2100},{2200},{2300},{2400},{2500},{2600}};

const int32_t path2[][1] = {
{-3000},{-2900},{-2800},{-2700},{-2600},{-2500},{-2400},{-2300},{-2200},{-2100},{-2000},{-1900},{-1800},{-1700},{-1600},{-1500},{-1400},{-1300},{-1200},{-1100},{-1000},{-900},{-800},{-700},{-600},{-500},{-400},{-300},{-200},{-100},{0},{100},{200},{300},{400},{500},{600},{700},{800},{900},{1000},{1100},{1200},{1300},{1400},{1500},{1600},{1700},{1800},{1900},{2000},{2100},{2200},{2300},{2400},{2500},{2600}};



/*
const int32_t path1[][1] = {
{-300},{-290},{-280},{-270},{-260},{-250},{-240},{-230},{-220},{-210},{-200},{-190},{-180},{-170},{-160},{-150},{-140},{-130},{-120},{-110},{-100},{-90},{-80},{-70},{-60},{-50},{-40},{-30},{-20},{-10},{0},{10},{20},{30},{40},{50},{60},{70},{80},{90},{100},{110},{120},{130},{140},{150},{160},{170},{180},{190},{200},{210},{220},{230},{240},{250},{260}};

const int32_t path2[][1] = {
{-300},{-290},{-280},{-270},{-260},{-250},{-240},{-230},{-220},{-210},{-200},{-190},{-180},{-170},{-160},{-150},{-140},{-130},{-120},{-110},{-100},{-90},{-80},{-70},{-60},{-50},{-40},{-30},{-20},{-10},{0},{10},{20},{30},{40},{50},{60},{70},{80},{90},{100},{110},{120},{130},{140},{150},{160},{170},{180},{190},{200},{210},{220},{230},{240},{250},{260}};
*/
#define LIMSW_X 16
#define BTN_DEB 50
#define BTN_HOLD 500

Stepper<STEPPER2WIRE> MTR2(5, 18);
Stepper<STEPPER2WIRE> MTR4(4, 15);
// Stepper<STEPPER2WIRE> stepper2_2(4, 15);
Stepper<STEPPER2WIRE> MTR1(19, 21);
GPlanner<STEPPER2WIRE, 1> planner_mtr1;
GPlanner<STEPPER2WIRE, 1> planner_mtr2;

bool pState_0 = false;
bool pStateSwitch_0 = false;
bool pState_1 = false;
bool state_switch_0 = 0;
bool state_switch_1 = 0;
bool hold_0 = 0;
bool hold_1 = 0;


bool Z1State = 0;
bool State = false;
//int count = (pointAm) / 2 + 2;
//int count2 = (pointAm) / 2 + 2;
int count = 31;
int count2 = 31;
int gstep = 1;  // шаг перемещения по массиву точек

bool state_home_0 = 1;
bool state_home_1 = 1;
bool state_home_encoder_0 = 0;
bool state_home_encoder_1 = 0;
bool Status1 = 0;
bool Status2 = 0;

bool Status3 = 0;
bool Status4 = 0;
bool state_target = 0;
bool last_target = 0;

#ifdef USE_WIFI_MODE
const char* ssid = "xxx";
const char* password = "xxx";

WiFiServer server(10000);  // server port to listen on

void printWifiStatus();
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(Z_Pin, INPUT_PULLUP);
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);
  pinMode(button4Pin, INPUT);
  pinMode(LIMSW_X, INPUT_PULLUP);

  Serial.begin(115200);
  //Serial.println(L);
  // добавляем шаговики на оси
  planner_mtr2.addStepper(0, MTR2);  // ось 0
  // planner_mtr2.addStepper(1, stepper2_2);  // ось 1

  planner_mtr1.addStepper(0, MTR1);  // ось 0
  //planner_mtr1.addStepper(1, MTR4);  // ось 1
  // устанавливаем ускорение и скорость
  planner_mtr2.setAcceleration(600);
  planner_mtr2.setMaxSpeed(1500);

  planner_mtr1.setAcceleration(600);
  planner_mtr1.setMaxSpeed(1500);  //400


  // L = L/1.44;



  //Serial.println(path[0][0]);
  //Serial.println(path[1][0]);

  planner_mtr2.setSpeed(0,500);
  planner_mtr1.setSpeed(0, 500);  //300

  // setup Wi-Fi network with SSID and password
#ifdef USE_WIFI_MODE
  Serial.printf("Connecting to %s\n", ssid);
  Serial.printf("\nattempting to connect to WiFi network SSID '%s' password '%s' \n", ssid, password);
  // attempt to connect to Wifi network:
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(500);
  }
  server.begin();
  // you're connected now, so print out the status:
  digitalWrite(LED_BUILTIN, HIGH);
  printWifiStatus();
  Serial.println(" listening on port 10000");
  Serial.flush();
#endif
      planner_mtr2.stop();
      planner_mtr2.reset();
      planner_mtr2.resume();
  
        planner_mtr1.stop();
      planner_mtr1.reset();
      planner_mtr1.resume();

}
char readc='f'; 
char read_buf='f'; 
static uint32_t s=0;
bool alreadyConnected = false;  // whether or not the client was connected previously

void loop() {
  //delay(10);
  // здесь происходит движение моторов, вызывать как можно чаще
  planner_mtr2.tick();
  planner_mtr1.tick();

// if (Serial.available()!=0){
// readc=(char)Serial.read();
// }

  state_switch_0 = !digitalRead(LIMSW_X);
  state_switch_1 = !digitalRead(Z_Pin);


  Status1 = digitalRead(button1Pin);
  Status2 = digitalRead(button2Pin);

  Status3 = digitalRead(button3Pin);
  Status4 = digitalRead(button4Pin);


  static int16_t seqExpected = 0;
  bool left;
  bool right;
  bool up;
  bool down;

#ifdef USE_WIFI_MODE
  static WiFiClient client;
  if (!client) {
      client = server.available();  // Listen for incoming clients
      digitalWrite(LED_BUILTIN, LOW);
  }
  if (client) {                   // if client connected
    if (!alreadyConnected) {
        // clead out the input buffer:
        client.flush();
        alreadyConnected = true;
        digitalWrite(LED_BUILTIN, HIGH);
    }
    // if data available from client read and display it
    int length;
    int32_t value;
    if ((length = client.available()) > 0) {
        //str = client.readStringUntil('\n');  // read entire response
        // if data is correct length read and display it
        if (length == sizeof(value)) {
            client.readBytes((char*)&value, sizeof(value));
            right = (value & (1 << 0)) != 0;
            left = (value & (1 << 8)) != 0;
            up = (value & (1 << 16)) != 0;
            down = (value & (1 << 24)) != 0;
            // left = 1;
        } else
        while (client.available()) {};  // discard corrupt packet
    }
  }
#else
  // Serial mode - read commands from Serial
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    // Ожидаем 4 байта для формирования int32_t
    static uint8_t serial_buf[4];
    static uint8_t buf_idx = 0;
    
    serial_buf[buf_idx++] = cmd;
    
    if (buf_idx == 4) {
      buf_idx = 0;
      uint32_t value = ((uint32_t)serial_buf[3] << 24) | 
                       ((uint32_t)serial_buf[2] << 16) | 
                       ((uint32_t)serial_buf[1] << 8) | 
                       ((uint32_t)serial_buf[0]);
      right = (value & (1 << 0)) != 0;
      left = (value & (1 << 8)) != 0;
      up = (value & (1 << 16)) != 0;
      down = (value & (1 << 24)) != 0;
    }
  }
#endif

/*
  //Обработка концевика 0
  static uint32_t tmr4;
  if (pState_0 != state_switch_0 && millis() - tmr4 >= BTN_DEB) {
    tmr4 = millis();
    pState_0 = state_switch_0;
    hold_0 = false;  // сброс флага удержания
    if (state_switch_0) {
      planner_mtr2.setSpeed(0,0);
      planner_mtr2.stop();
      planner_mtr2.reset();
      planner_mtr2.resume();
      if (planner_mtr2.ready()) {
        planner_mtr2.setTarget(path2[0]);
        state_home_0 = 1;
        state_home_encoder_0 = 1;
      }
    }
  }


  //Обработка концевика 1
  static uint32_t tmr5;
  if (pState_1 != state_switch_1 && state_home_encoder_1 == 0 && millis() - tmr5 >= BTN_DEB) {
    tmr5 = millis();
    pState_1 = state_switch_1;
    hold_1 = false;  // сброс флага удержания
    if (state_switch_1) {
      planner_mtr1.setSpeed(0,0);
      planner_mtr1.stop();
      planner_mtr1.reset();
      planner_mtr1.resume();
      if (planner_mtr1.ready()) {
        planner_mtr1.setTarget(path1[0]);
        state_home_1 = 1;
        state_home_encoder_1 = 1;
      }
    }
  }
*/


  // вперёд-назад
  if (Status3 or Status4 or state_home_0 or readc == 'a' or readc == 'b' or up or down) {
  Status1 = 0;
  Status2 = 0;
    if (planner_mtr2.ready()) {
      last_target = state_target;
      if (state_home_0 == 1) {
        state_home_0 = 0;
        // planner_mtr2.reset();
        planner_mtr2.setTarget(path2[(pointAm/2)+2]);
        Serial.println("up/down");
        Serial.println("middle:");
        Serial.println((pointAm/2)+2);
      } else if (state_home_0 == 0) {

        Serial.println("up/down");
        Serial.println("count:");
        Serial.println(count);
        planner_mtr2.setTarget(path2[count]);

        if (Status4 == true or readc == 'a' or up) {

          //planner_mtr2.setTarget(path[count+1]);  // загружаем новую точку (начнётся с 0)
          if (count < pointAm-1) {
            count += gstep;
          } else count = pointAm-1;
        }

        if (Status3 == true or readc == 'b' or down) {
          if (count > 3) count -= gstep;
          else count = 3;  //0
          //planner_mtr2.setTarget(path[count]);  // загружаем новую точку (начнётся с 0)
        }
      }
    }
  }



  // по бокам - забвно, но при отключении крутится подъём инструмента
  if (Status1 or Status2 or readc == 'c' or readc == 'd' or left or right) {
  Status3 = 0;
  Status4 = 0;
   
    if (planner_mtr1.ready()) {
      if (state_home_1 == 1) {
        state_home_1 = 0;
        // planner_mtr1.reset();
        planner_mtr1.setTarget(path1[count2]);
        Serial.println("left/right");
        Serial.println("count2:");
        Serial.println(count2);
      } else if (state_home_1 == 0) {
        planner_mtr1.setTarget(path1[count2]);
        Serial.println("left/right");
        Serial.println("count2:");
        Serial.println(count2);


        if (Status2 == true or readc=='c' or left) {
          if (count2 < pointAm) {
            count2 += gstep;
          } else count2 = pointAm;
        }

        //возвращает обратно
        if (Status1 == true or readc=='d' or right) {
          if (count2 > 4) count2 -= gstep;
          else count2 = 4;  //0
        }
      }
    }
 }
}

#ifdef USE_WIFI_MODE
void printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("\nSSID: ");
    Serial.println(WiFi.SSID());
    
    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}
#endif
