#include <Arduino.h>
#include "BluetoothSerial.h"

#define PWM_pin_L 2//L motor driver in2
#define PWM_pin_R 5//R motor driver in2

#define IO_pin_L 16//L motor driver in1
#define IO_pin_R 17;//R motor driver in1

#define PWM_Hz 1000// PWM周波数
#define PWM_level 8// PWM分解能 8bit(0～255)

#define PWM_CH_L 0//L motorDriverに入れるpwmを扱うチャンネルを0
#define PWM_CH_R 1//R motorDriverに入れるpwmを扱うチャンネルを1

#define MOTOR_POWER_MIN    50
#define MOTOR_POWER_MAX    255

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
char data[2];// data[0] = duty; data[1] = dir;

void setup() {
  Serial.begin(115200); 
  SerialBT.begin("ESP32"); 
  Serial.println("The device started, now you can pair it with bluetooth!");
 
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
  ledcWrite(PWM_CH_R,0);
}

void loop() {
  if (SerialBT.available()){
    for(int i = 0; i < 2; i++){
      data[i] = SerialBT.read();
    }
  }

  digitalWrite(IO_pin_L, data[1]); ledcWrite(PWM_CH_L, data[0]);
  digitalWrite(IO_pin_R, data[1]); ledcWrite(PWM_CH_R, data[0]);
  
}
