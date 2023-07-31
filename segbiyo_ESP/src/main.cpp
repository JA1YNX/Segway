#include <Arduino.h>
//#include <SoftwareSerial.h>
#include "BluetoothSerial.h"

/*const int PWM_pin_L = 2;//L motor driver in2
const int PWM_pin_R = 5;//R motor driver in2

const int IO_pin_L = 16;//L motor driver in1
const int IO_pin_R = 17;//R motor driver in1

const double PWM_Hz = 1000;// PWM周波数
const uint8_t PWM_level = 8;// PWM分解能 8bit(1～256)

const uint8_t PWM_CH_L = 0;//L motorDriverに入れるpwmを扱うチャンネルを0
const uint8_t PWM_CH_R = 1;//R motorDriverに入れるpwmを扱うチャンネルを1*/

#define MOTOR_PIN_F1        2   // to DC Motor Driver1 FIN
#define MOTOR_PIN_R1        16   // to DC Motor Driver1 RIN
#define MOTOR_PWM_F1        0   // PWM CHANNEL
#define MOTOR_PWM_R1        1   // PWM CHANNEL

#define MOTOR_PIN_F2        5   // to DC Motor Driver2 FIN
#define MOTOR_PIN_R2        17   // to DC Motor Driver2 RIN
#define MOTOR_PWM_F2        2   // PWM CHANNEL
#define MOTOR_PWM_R2        3   // PWM CHANNEL


#define MOTOR_POWER_MIN    50
#define MOTOR_POWER_MAX    255

/*#define rxPin 5
#define txPin 4

SoftwareSerial mySerial =  SoftwareSerial(rxPin, txPin);*/

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;



void setup() {
  
  /*pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);*/
  Serial.begin(125000); 
  SerialBT.begin("ESP32test"); 
  Serial.println("The device started, now you can pair it with bluetooth!");
 
  /*pinMode(PWM_pin_L, OUTPUT); 
  pinMode(PWM_pin_R, OUTPUT); 
  pinMode(IO_pin_L, OUTPUT);
  pinMode(IO_pin_R, OUTPUT);
  

  // チャンネルと周波数の分解能を設定
  ledcSetup(PWM_CH_L, PWM_Hz, PWM_level);
  ledcSetup(PWM_CH_R, PWM_Hz, PWM_level);
  // モータのピンとチャンネルの紐づけ
  ledcAttachPin(PWM_pin_L, PWM_CH_L);
  ledcAttachPin(PWM_pin_R, PWM_CH_R);

  //M5stackの起動時に回り続けないようにする。
  ledcWrite(PWM_CH_L,0);
  ledcWrite(PWM_CH_R,0);*/
  
  //Motor設定
  pinMode(MOTOR_PIN_F1, OUTPUT);
  pinMode(MOTOR_PIN_R1, OUTPUT);
  pinMode(MOTOR_PIN_F2, OUTPUT);
  pinMode(MOTOR_PIN_R2, OUTPUT);
  
  ledcSetup(MOTOR_PWM_F1, 1000, 8); //CHANNEL, FREQ, BIT
  ledcSetup(MOTOR_PWM_R1, 1000, 8);
  ledcSetup(MOTOR_PWM_F2, 1000, 8); //CHANNEL, FREQ, BIT
  ledcSetup(MOTOR_PWM_R2, 1000, 8);

  ledcAttachPin(MOTOR_PIN_F1, MOTOR_PWM_F1);
  ledcAttachPin(MOTOR_PIN_R1, MOTOR_PWM_R1);
  ledcAttachPin(MOTOR_PIN_F2, MOTOR_PWM_F2);
  ledcAttachPin(MOTOR_PIN_R2, MOTOR_PWM_R2);

}

void loop() {
float Duty;
 if (SerialBT.available()){
    uint8_t power = Serial.read();
    Duty = (int)((MOTOR_POWER_MAX - MOTOR_POWER_MIN)* abs(power) + MOTOR_POWER_MIN); 
  ledcWrite( MOTOR_PIN_F1,(power < 0 ?    0 : Duty) );  
  ledcWrite( MOTOR_PIN_R1 ,(power < 0 ? Duty :    0) ); 
  ledcWrite( MOTOR_PIN_F2,(power < 0 ?    0 : Duty) );  
  ledcWrite( MOTOR_PIN_R2 ,(power < 0 ? Duty :    0) ); 
 }
 else {  // +-20度を越えたら倒れたとみなす
    ledcWrite( MOTOR_PWM_F1,0 );
    ledcWrite( MOTOR_PWM_R1,0 );
    ledcWrite( MOTOR_PWM_F2,0 );
    ledcWrite( MOTOR_PWM_R2,0 );
  }
}

