#include <M5Stack.h>
#include "M5StackUpdater.h"
#include "ip5306.h"

#include "clappyavator.h"

Avator *avator;

#define chgDET

int pos1 = 0;
int pos2 = 0;

/********************************
* for Servo control
*********************************/
const uint8_t motorInterval = 30;  //[ms]

#define LEDC_CHANNEL_0  0  // channel of 16 channels (started from zero)
#define LEDC_TIMER_BIT 12  // use 12 bit precission for LEDC timer
#define LEDC_BASE_FREQ 50  // use 50 Hz as a LEDC base frequency

// Servo Range
// int srvMin = 103; // (103/4096)*20ms ≒ 0.5 ms (-90°)
// int srvMax = 491; // (491/4096)*20ms ≒ 2.4 ms (+90°)
#define srvMin 105
#define srvMax 475

// Servo assign to each channel
#define pinFootR 2   // foot
#define channelFootR 0
#define pinFootL 12
#define channelFootL 1

#define pinlegR 0   // leg
#define channelLegR 2
#define pinLegL 15
#define channelLegL 3

#define pinBallast 13 // battery
#define channelBallast 4

// center position calibration(0-180 deg)
const int centFootR = 110;
const int centFootL = 100;

const int centLegR = 80;
const int centLegL = 95;

const int centBallast = 83;

const int rotFoot = 15;
const int rotLeg = 30;
const int rotBallast = 75;

int smooth = 5;
int stepInterval = 100;

void initServo(int pinServo, int ledc_channel) { 
  // init Servo
  // Setup timer and attach timer to a servo pin
  // ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
  ledcSetup(ledc_channel, LEDC_BASE_FREQ, LEDC_TIMER_BIT);

  // ledcAttachPin(Servo_PIN1, LEDC_CHANNEL_0);
  ledcAttachPin(pinServo, ledc_channel);
  ledcWrite(ledc_channel, 0);
}

// Angle 0 - 180 degree
void setSrvAngle(int channelServo, int degree){
  float stepDegree = (srvMax - srvMin) / 180;
  ledcWrite(channelServo, srvMin + ((int)stepDegree * degree));
}

/*
 * Update Avator
 */
void avatorUpdate(void* arg) {
  int cnt = 0;
  while (1) {
    if(cnt == 0){
      //avator->openMouth(15);
    }
    if (cnt % 100 == 97){
      avator->openEye(false);
    }
    if (cnt % 100 == 0){
      avator->openEye(true);
      cnt = random(100);
    }
    cnt++;
    vTaskDelay(50);
  }
}

void setup() {

// M5Stack::begin(LCDEnable, SDEnable, SerialEnable, I2CEnable);
  M5.begin(true, true, true, true);

  M5.Power.begin();
  M5.Power.setVinMaxCurrent(CURRENT_100MA);

  M5.Speaker.begin();
  M5.Speaker.mute();
  M5.Speaker.end();

  if(digitalRead(BUTTON_A_PIN) == 0) {
    Serial.println("Will Load menu binary");
    updateFromFS(SD);
    ESP.restart();
  }

  Serial.println("M5Walk-chan direct servo version");

  M5.Lcd.setBrightness(64);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);

  avator = new Avator();
  M5.Lcd.fillScreen(SECONDARY_COLOR);

  // Servo initialization
  initServo(pinFootR, channelFootR);
  initServo(pinFootL, channelFootL);
  initServo(pinlegR, channelLegR);
  initServo(pinLegL, channelLegL);
  initServo(pinBallast, channelBallast);

 /*
 * Demo action
 */
  setHomePosi();
  
  delay(1000);

  #ifdef chgDET
    if(checkI2C(0x75)){
      Serial.print("IP5306-I2C found!");
      if(getChargeEnable()){
        adjMode();
      }
    }else{
        Serial.print("IP5306-I2C not found!");
    }
  #endif

  avator->init();

  // xTaskCreatePinnedToCore(タスクの関数名,"タスク名",スタックメモリサイズ,NULL,タスク優先順位,タスクハンドルポインタ,Core ID);
  // xTaskCreatePinnedToCore(func,"name",Stuck size,NULL,priority,Task pointer,Core ID)
  // Core ID: 0 or 1 or tskNO_AFFINITY
  xTaskCreatePinnedToCore(avatorUpdate, "Task0", 4096, NULL, 1, NULL, 0);

//  checkServo();

// Walking test
//--- 左ステップ ---
// バラストを左へ移動
  for(int j = 0; j < rotBallast; j++){
    setBallast(j);
    delay(smooth);
  }

  delay(stepInterval);

// 左側外へ倒す
  for(int j = 0; j < rotFoot; j++){
    setFootL(j);
    delay(10);
    setFootR(-1 * j);
    delay(smooth);
  }

  delay(stepInterval);

// 右足を前に出す
  for(int j = 0; j < rotLeg; j++){
    setLegL(-1 * j);
    delay(10);
    setLegR(j);
    delay(smooth);
  }

  delay(stepInterval);

// 右足を下ろす
  for(int j = rotFoot; j >= 0; j--){
    setFootL(j);
    delay(10);
    setFootR(-1 * j);
    delay(smooth);
  }

  delay(stepInterval);

//****** ここから繰り返し *****
//****** 右ステップ *****
// バラストを右へ移動
  for(int j = rotBallast; j >= -1 * rotBallast; j--){
    setBallast(j);
    delay(smooth);
  }

  delay(stepInterval);

// 右足を外へ倒す
  for(int j = 0; j < rotFoot; j++){
    setFootR(j);
    delay(10);
    setFootL(-1 * j);
    delay(smooth);
  }

  delay(stepInterval);

// 左足を前に出す
    for(int j = rotLeg; j >= -1 * rotLeg; j--){
      setLegL(-1 * j);
      delay(10);
      setLegR(j);
      delay(smooth);
    }

    delay(stepInterval);

// 左足を下ろす
    for(int j = rotFoot; j >= 0; j--){
      setFootR(j);
      delay(10);
      setFootL(-1 * j);
      delay(smooth);
    }

    delay(stepInterval);

//****** 左ステップ *****
// バラストを左へ移動
  for(int j = -1 * rotBallast; j < rotBallast; j++){
    setBallast(j);
    delay(smooth);
  }

  delay(stepInterval);

// 左足を外へ倒す
  for(int j = 0; j < rotFoot; j++){
    setFootL(j);
    delay(10);
    setFootR(-1 * j);
    delay(smooth);
  }

  delay(stepInterval);

// 右足を前に出す
  for(int j = -1 * rotLeg; j < rotLeg; j++){
    setLegL(-1 * j);
    delay(10);
    setLegR(j);
    delay(smooth);
  }

    delay(stepInterval);

// 右足を下ろす
  for(int j = rotFoot; j >= 0; j--){
    setFootL(j);
    delay(10);
    setFootR(-1 * j);
    delay(smooth);
  }

  delay(stepInterval);

}

void loop(){

//****** ここから繰り返し *****
//****** 右ステップし *****
// バラストを右へ移動
  for(int j = rotBallast; j >= -1 * rotBallast; j--){
    setBallast(j);
    delay(smooth);
  }

  delay(stepInterval);

// 右側外へ倒す
  for(int j = 0; j < rotFoot; j++){
    setFootR(j);
    delay(10);
    setFootL(-1 * j);
    delay(smooth);
  }

  delay(stepInterval);

// 左足を前に出す
  for(int j = rotLeg; j >= -1 * rotLeg; j--){
     setLegL(-1 * j);
    delay(10);
    setLegR(j);
    delay(smooth);
  }

  delay(stepInterval);

// 左足を下ろす
  for(int j = rotFoot; j >= 0; j--){
    setFootR(j);
    delay(10);
    setFootL(-1 * j);
    delay(smooth);
  }

  delay(stepInterval);

//****** 左ステップ *****
// バラストを左へ移動
  for(int j = -1 * rotBallast; j < rotBallast; j++){
    setBallast(j);
    delay(smooth);
  }

  delay(stepInterval);

// 左足を外へ倒す
  for(int j = 0; j < rotFoot; j++){
    setFootL(j);
    delay(10);
    setFootR(-1 * j);
    delay(smooth);
  }

  delay(stepInterval);

// 右足を前に出す
  for(int j = -1 * rotLeg; j < rotLeg; j++){
    setLegL(-1 * j);
    delay(10);
    setLegR(j);
    delay(smooth);
  }

  delay(stepInterval);

// 右足を下ろす
  for(int j = rotFoot; j >= 0; j--){
    setFootL(j);
    delay(10);
    setFootR(-1 * j);
    delay(smooth);
  }

  delay(stepInterval);
}

// Home position
void setHomePosi(){
  setBallast(0);
  delay(stepInterval);
  setFootR(0);
  delay(stepInterval);
  setFootL(0);
  delay(stepInterval);
  setLegR(0);
  delay(stepInterval);
  setLegL(0);
}

// Servo Check
void checkServo(){
  Serial.println("Ballast test");
  setBallast(0);
  delay(500);
  setBallast(30);   // Left
  delay(500);
  setBallast(-30);  // Right
  delay(500);
  setBallast(0);
  delay(500);

  Serial.println("Right foot test");
  setFootR(0);
  delay(500);
  setFootR(10);   // Out
  delay(500);
  setFootR(-10);  // In
  delay(500);
  setFootR(0);
  delay(500);

  Serial.println("Lest foot test");
  setFootL(0);
  delay(500);
  setFootL(10);   // Out
  delay(500);
  setFootL(-10);  // In
  delay(500);
  setFootL(0);
  delay(500);

  Serial.println("Right leg test");
  setLegR(0);
  delay(500);
  setLegR(10);   // Out
  delay(500);
  setLegR(-10);  // In
  delay(500);
  setLegR(0);
  delay(500);

  Serial.println("Left leg test");
  setLegL(0);
  delay(500);
  setLegL(10);   // Out
  delay(500);
  setLegL(-10);  // In
  delay(500);
  setLegL(0);
  delay(500);
}

/*************************************************** 
 Motion Control
 画面正面から見た状態で左右を定義
 **************************************************/

// Ballast +:Left, -: Right
void setBallast(int angle){
  setSrvAngle(channelBallast, centBallast + angle);
}

// Right Foot +:Out, -: In
void setFootR(int angle){
  setSrvAngle(channelFootR, centFootR + angle);
}

// Left Foot +:Out, -: In
void setFootL(int angle){
  setSrvAngle(channelFootL, centFootL - angle);
}

// Right Leg +:Out, -: In
void setLegR(int angle){
  setSrvAngle(channelLegR, centLegR - angle);
}

// Left Leg +:Out, -: In
void setLegL(int angle){
  setSrvAngle(channelLegL, centLegL + angle);
}

/*************************************************** 
 status of charging
 **************************************************/
void adjMode(){
    delay(300);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(50, 0);
    M5.Lcd.print("HOME POSITION");
    setHomePosi();

    while(1){
      M5.update();
      M5.Lcd.setCursor(10, 150);
      if(getChargeEnable()){
        switch(getChargeMode()){
          case 1:
            M5.Lcd.print("  Full Charge     ");
            break;
          case 2:
            M5.Lcd.print("Charging(CV mode) ");
            break;
          case 3:
            M5.Lcd.print("Charging(CC mode) ");
            break;
          default:
            M5.Lcd.print("  Unknown mode    ");
            break;
        }
      }else{
        M5.Lcd.print("                     ");
      }

      uint8_t Level =  M5.Power.getBatteryLevel();
      if(Level > 100){
        Level = 100;
      }else if(Level < 0){
        Level = 0;
      }
      M5.Lcd.progressBar(30, 200, 260, 20, Level);
      M5.Lcd.drawRect(29, 199, 262, 22, WHITE);

      if(M5.BtnB.wasPressed()){
        M5.Lcd.fillScreen(SECONDARY_COLOR);
        avator->init();
        break;
      }
      delay(100);
   }
}

boolean checkI2C(byte address){
  Wire.beginTransmission(address);
  byte error;
  error = Wire.endTransmission();
  if(error == 0){
    return true;
  }else{
    return false;
  }
}

boolean getChargeEnable(){
  uint8_t data;
  M5.I2C.readByte(IP5306_ADDR, IP5306_REG_READ0, &data);
  return((data & CHARGE_ENABLE_BIT) == 0x08);
}

uint8_t getChargeMode(){
  uint8_t data;
  M5.I2C.readByte(IP5306_ADDR, IP5306_REG_READ1, &data);

  if((data & CHARGE_FULL_BIT)){
//    Serial.println("CHARGE_FULL");
    return 1;
  }else if((data & 0xE0) == CHARGE_MODE_CV){
//    Serial.println("CHARGE_MODE_CV");
    return 2;
  }else if((data & 0xE0) == CHARGE_MODE_CC){
//    Serial.println("CHARGE_MODE_CC");
    return 3;
  }else{
//    Serial.println("CHARGE_MODE_Unknown");
    return 0;
  }
}
