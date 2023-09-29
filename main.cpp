/*参考サイト
倒立振子 https://qiita.com/Google_Homer/items/3897e7ffef9d247e2f56
M5pin　　https://nouka-it.hatenablog.com/entry/2022/02/19/131143
PID制御　https://controlabo.com/pid-control-introduction/
BLYNK　　https://homemadegarbage.com/bala03
　　　　 blynkとマイコン接続のコードはblynk始めるときに作ってくれるから自分で変えて。
*/

//見ての通りM5stack用。M5シリーズならできるが、M5stackGrayなどのMPU6886搭載モデルじゃないと角度が測れないので気を付けよう。
#define M5STACK_MPU6886 
/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "TMPL6LDgwnJgw"
#define BLYNK_TEMPLATE_NAME         "Quickstart Template"
#define BLYNK_AUTH_TOKEN            "gXbkVIaPjefLa_n5GN7zu8fZUIc29MHT"
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
//↑のdefineたちはincludeの前におかないとダメっぽい。
int ONOFF = 0;
int cal = 0;

#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

//PID係数
#define TARGETp           -2.5
#define KPp               5

float TARGETr             =40.1;//セグウェイが水平になるときM5は傾いているので後でこの値を引いて0度にする
float KPr                 =10  ;//roll用のKPゲイン
float KIr                 =1.8 ;//roll用のKIゲイン
float KDr                 =1.5 ;//roll用のKDゲイン
float oldDuty1            =0   ;
float oldDuty2            =0   ;

bool Button = true;

//Motor
#define MOTOR_PIN_F1        2   //白 to DC Motor Driver FIN
//#define MOTOR_PIN_R1        16   // to DC Motor Driver RIN
#define MOTOR_PWM_F1        0   // PWM CHANNEL
//#define MOTOR_PWM_R1        1   // PWM CHANNEL

#define MOTOR_PIN_F2        5   //青 to DC Motor Driver FIN
//#define MOTOR_PIN_R2        17   // to DC Motor Driver RIN
#define MOTOR_PWM_F2        2   // PWM CHANNEL
//#define MOTOR_PWM_R2        3   // PWM CHANNEL

#define Motor_Dir_1        16//緑
#define Motor_Dir_2        17//灰

#define MOTOR_POWER_MIN    50
#define MOTOR_POWER_MAX    255
//PID
float power1=0,power2=0,I=0,preP=0,preTime,oldpower1=0,oldpower2=0;
//powet1が向かって右モーター 2が左

float pitch0, roll0, yaw0;

//自分の使えるWIFIのssid、passwordを""に入れて
//char ssid[] = "aterm-0a7faf-5p";//俺の家の
//char pass[] = "8bed045d64442";//俺の家の
//char ssid[] = "";//君の
//char pass[] = "";//君の

char ssid[] = "Namagusa";//俺のパソコンの
char pass[] = "Oimooimo";//俺のパソコンの


//ヴァーチャルピンデータ受信
BLYNK_WRITE(V0) {
  KPr = param.asFloat()/10;//blynkで送った値の1/10をKPrに代入(スマホでゲイン調整可能)。
  //使いたかったらまずblynkのデータストリームでヴァーチャルピンのV0pinを作り、MIN,MAXは0,100。1/10されるためKPrを0-10の間で操作可能。defaultvalueはコードのKPrとおなじ。ここでは10。
  //次にスマホのblynkの編集画面でsliderを出し、sliderのdatastreamをv0にする。よくわからなかったらblynk使い方とかでググれ。V0,V1,V2もやることおなじ。
  Serial.println("Kp!!!!");
}

BLYNK_WRITE(V1) {
  KIr = param.asFloat()/10;//blynkで送った値の1/10をKIrに代入(スマホでゲイン調整可能)
  Serial.println("Ki!!!!!");
}

BLYNK_WRITE(V2) {
  KDr = param.asFloat()/10;//blynkで送った値の1/10をKDrに代入(スマホでゲイン調整可能)
  Serial.println("Kd!!!!");
}
  
BLYNK_WRITE(V4){
  ONOFF = param.asInt();
  if(ONOFF == 1){//blynkでボタンをONにすると止まる。blynkでV4を作ってMIN,MAXを0,1、defaultvalueを0にし、スマホblynkでButtonをだし、datastreamをV4にし、SettingsのModeはSwitchにする。
    Button = false ;   
  }
  else{
    Button = true;
  }
}

BLYNK_WRITE(V5) {
 TARGETr = param.asFloat();//blynkで送った値を代入(スマホで角度調整可能)。V5作ってスマホでSlider出してdatastreamをV5にする。
}  
void setup() {
  M5.begin();

  //Motor設定
  pinMode(MOTOR_PIN_F1, OUTPUT);
  //pinMode(MOTOR_PIN_R1, OUTPUT);
  ledcSetup(MOTOR_PWM_F1, 1000, 8); //CHANNEL, FREQ, BIT
  //ledcSetup(MOTOR_PWM_R1, 1000, 8);
  ledcAttachPin(MOTOR_PIN_F1, MOTOR_PWM_F1);
  //ledcAttachPin(MOTOR_PIN_R1, MOTOR_PWM_R1);

  pinMode(MOTOR_PIN_F2, OUTPUT);
  //pinMode(MOTOR_PIN_R2, OUTPUT);
  ledcSetup(MOTOR_PWM_F2, 1000, 8); //CHANNEL, FREQ, BIT
  //ledcSetup(MOTOR_PWM_R2, 1000, 8);
  ledcAttachPin(MOTOR_PIN_F2, MOTOR_PWM_F2);
  //ledcAttachPin(MOTOR_PIN_R2, MOTOR_PWM_R2);

  pinMode(Motor_Dir_1, OUTPUT);
  pinMode(Motor_Dir_2, OUTPUT);

  //MPU設定
  M5.MPU6886.Init();
  M5.MPU6886.getAhrsData(&pitch0,&roll0,&yaw0);
  //PID初期化
  preTime = micros(); 

  Serial.begin(115200);
  //これのせいで君が設定したWIFIとマイコンがつながんないとマイコンが動かない。
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {

  Blynk.run();
  //timer.run();
  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!
  float pitch,roll,yaw,Duty1,Duty2,deDuty1,deDuty2,P,D,nowr,nowp,dt,Time;
  M5.update();
  //ロール角取得
  M5.MPU6886.getAhrsData(&pitch,&roll,&yaw);
  //PID計算
  nowr     = TARGETr - roll                       ; // 目標角度から現在の角度を引いて偏差を求める
  nowp     = TARGETp - pitch                      ; // 目標角度から現在の角度を引いて偏差を求める

  float Degrees = nowr;
  Blynk.virtualWrite(V3, Degrees);
  if(M5.BtnB.isPressed()){
    Button = false ;
  }
  if(M5.BtnB.pressedFor(1000)){
    Button = true;
  }
  if(Button == true){
  if (-10 < nowr && nowr < 10 /*&! M5.BtnB.isPressed()*/) { 
    Time    = micros()                          ;
    dt      = (Time - preTime) / 1000000        ; // 処理時間を求める
    preTime = Time                              ; // 処理時間を記録
    P       = nowr / 90                         ; // -90~90→-1.0~1.0
    I      += P * dt                            ; // 偏差を積分する
    D       = (P - preP) / dt                   ; // 偏差微分するを
    preP    = P                                 ; // 偏差を記録する
    power1   = (KPr * P + KIr * I + KDr * D )   ; // 出力を計算する
    power2   = (KPr * P + KIr * I + KDr * D )   ; // 出力を計算する
    if (power1 < -1) power1 = -1                ; // →-1.0~1.0
    if (1 < power1)  power1 =  1                ; 
    if (power2 < -1) power2 = -1                ; // →-1.0~1.0
    if (1 < power2)  power2 =  1                ; 
    if (I > 1) I = 1                            ;
    oldpower1 = power1                          ;
    oldpower2 = power2                          ;
    
   //nowpが正だとM5が右に傾いている、負だと左
    if (power1 >0 && power2 >0) {

    if (nowp >5 && nowp <12){
    power2 -= nowp * KPp /90;
      if (power2<0.3)power2 = 0.3;
    }
    else if(nowp >12){
    power2 -= nowp * KPp /90;
      if(0.3>oldpower2)power2 = oldpower2;
      if(0.3<oldpower2)power2 = 0.3;
    }

    if (nowp <-5 && nowp >-12){
    power1 -= abs(nowp) * KPp /90;
      if (power1<0.3)power1 = 0.3;
    }
    else if(nowp <-12){
    power1 -= abs(nowp) * KPp /90;
    if(0.3<oldpower1)power1 = oldpower1;
    if(0.3>oldpower1)power1 = 0.3;
    }

    else {
      power1 = power1;
      power2 = power2;
    }
    }

    if (power1 <0 && power2< 0){

    if (nowp >5 && nowp <12){
    power2 += nowp * KPp /90;  
      if (power2 >-0.3)power2 = -0.3; 
    }  
    else if(nowp >12){
    power2 += nowp * KPp /90; 
    if(-0.3<oldpower2)power2 = oldpower2;
    if(-0.3>oldpower2)power2 = -0.3;
    }

    if (nowp <5 && nowp >12){
    power1 += abs(nowp) * KPp /90;
      if (power1 >-0.3)power1 = -0.3;
    }
    else if(nowp <-12){
    power1 += abs(nowp) * KPp /90;
    if(-0.3<oldpower1)power1 = power1 = oldpower1;
    if(-0.3>oldpower1)power1 = -0.3;
    }

    else {
      power1 = power1;
      power2 = power2;
    }
    }

    if (power1 >0) digitalWrite(Motor_Dir_1, HIGH);
    if (power1 <0) digitalWrite(Motor_Dir_1, LOW) ;
    if (power2 >0) digitalWrite(Motor_Dir_2, HIGH);
    if (power2 <0) digitalWrite(Motor_Dir_2, LOW) ;

    //Motor駆動
    Duty1 = (int)((MOTOR_POWER_MAX - MOTOR_POWER_MIN)* abs(power1) + MOTOR_POWER_MIN);  
    Duty2 = (int)((MOTOR_POWER_MAX - MOTOR_POWER_MIN)* abs(power2) + MOTOR_POWER_MIN);  
    
    deDuty1 = oldDuty1 - Duty1;
    deDuty2 = oldDuty2 - Duty2;

    if(deDuty1 > 2)Duty1 = deDuty1 -2;
    else if(deDuty1 <-2)Duty1 =deDuty1 +2;
    else if(deDuty2 > 2)Duty2 =deDuty2 -2;
    else if(deDuty2 <-2)Duty2 =deDuty2 +2;
    else {
      Duty1 =Duty1;
      Duty2 =Duty2;
    }

    ledcWrite( MOTOR_PWM_F1, Duty1);
    ledcWrite( MOTOR_PWM_F2, Duty2);

    oldDuty1 = Duty1;
    oldDuty2 = Duty2;
    
    /*ledcWrite( MOTOR_PWM_F1,(power1 < 0 ?    0 : Duty1) );  
    ledcWrite( MOTOR_PWM_R1,(power1 < 0 ? Duty1 :    0) );
    ledcWrite( MOTOR_PWM_F2,(power2 < 0 ?    0 : Duty2) );  
    ledcWrite( MOTOR_PWM_R2,(power2 < 0 ? Duty2 :    0) );*/
    }  

 /* else if(M5.BtnB.isPressed() && -13 < nowr && nowr < 13){
    ledcWrite( MOTOR_PWM_F1,0 );
    //ledcWrite( MOTOR_PWM_R1,0 );
    ledcWrite( MOTOR_PWM_F2,0 );
    //ledcWrite( MOTOR_PWM_R2,0 );
    power1 = 0;
    power2 = 0;
    Duty1 = 0 ;
    Duty2 = 0 ;
    I = 0     ;
  }*/

  else {  // +-20度を越えたら倒れたとみなす
    ledcWrite( MOTOR_PWM_F1,0 );
    //ledcWrite( MOTOR_PWM_R1,0 );
    ledcWrite( MOTOR_PWM_F2,0 );
    //ledcWrite( MOTOR_PWM_R2,0 );
    power1 = 0;
    power2 = 0;
    Duty1 = 0 ;
    Duty2 = 0 ;
    I = 0     ;
  }
  }

  else {
    ledcWrite( MOTOR_PWM_F1,0 );
    //ledcWrite( MOTOR_PWM_R1,0 );
    ledcWrite( MOTOR_PWM_F2,0 );
    //ledcWrite( MOTOR_PWM_R2,0 );
    power1 = 0;
    power2 = 0;
    Duty1 = 0 ;
    Duty2 = 0 ;
    P = 0     ;
    I = 0     ;
    D = 0     ;
  }
  M5.Lcd.clear(BLACK);  
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(150, 0);
  M5.Lcd.printf("power1:%6.1f",power1);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("power2:%6.1f",power2);
  M5.Lcd.setCursor(150,30);
  M5.Lcd.printf("Duty1:%6.1f",Duty1);
  M5.Lcd.setCursor(0, 30);
  M5.Lcd.printf("Duty2:%6.1f",Duty2);
  M5.Lcd.setCursor(0, 60);
  M5.Lcd.printf("nowr:%6.4f",nowr);
  M5.Lcd.setCursor(150,60);
  M5.Lcd.printf("nowp:%6.4f",nowp);
  M5.Lcd.setCursor(0, 90);
  M5.Lcd.printf("P:%6.4f",P);
  M5.Lcd.setCursor(90, 90);
  M5.Lcd.printf("I:%6.4f",I);
  M5.Lcd.setCursor(180, 90);
  M5.Lcd.printf("D:%6.4f",D);
  M5.Lcd.setCursor(0, 120);
  M5.Lcd.printf("KP:%6.2f",KPr);
  M5.Lcd.setCursor(90, 120);
  M5.Lcd.printf("KI:%6.2f",KIr);
  M5.Lcd.setCursor(180, 120);
  M5.Lcd.printf("KD:%6.2f",KDr);
  delay(10);
}