#define PT_USE_TIMER
#define PT_USE_SEM

#include "pt.h"
static struct pt thread1, thread2, thread3, thread_pia, thread_normal;
static struct pt_sem sem_LED;
//unsigned char i;

int Led_controll = 10;     //控制M1输出电流的大小
int led_direction = 12;

String comdata = "";
String blue_tooth_data = "";
int mode_controll = 0;  // 控制模式,0为关闭模式

// 以下为夜间模式下与人体感应模块结合控制LED的变量
String condition = "no_one_moved"; //默认condition,客厅没有移动的热量
boolean islockedSitLed = false;  //  判断是否用声音锁住灯的开启状态,默认0为不锁住

int normal_led_intensity;

void setup() {
  //Set Hardware
  normal_led_intensity = 255;
  pinMode(13,OUTPUT);
  Serial.begin(115200);
  //Initialize Semaphore
  PT_SEM_INIT(&sem_LED,1);
  //Initialize the Threads
  PT_INIT(&thread1);
  PT_INIT(&thread2);
  PT_INIT(&thread3);
  PT_INIT(&thread_pia);
  PT_INIT(&thread_normal);
} 

// 用PT_BEGIN和PT_END来标记线程的开始和结束
static int thread1_entry(struct pt *pt)
{
  // proto函数中的宏定义,必须在代码的最前面
  PT_BEGIN(pt);

    while (Serial.available() > 0)  
    {
        comdata += char(Serial.read());
        delay(2);
    }
    if (comdata.length() > 0)
    {
      if(comdata == "high" || comdata == "blink" ||  comdata == "ktv"){
        Serial.println("high起来啦");
        mode_controll = 1;  // 1代表high起来
        digitalWrite(led_direction, HIGH); 
       }
       else if(comdata == "sleep"){
        Serial.println("睡眠模式1");
        mode_controll = 2;  // 2代表夜间模式
        digitalWrite(led_direction, HIGH); 
       }else if(comdata == "off"){
        mode_controll = 0;  // 0代表关闭模式
        analogWrite(Led_controll, 0);
       }
       else if(comdata == "normal"){
        digitalWrite(led_direction, HIGH);  
        mode_controll = 3;    // 3代表正常模式 可以调节灯的亮度
        }
      Serial.println(comdata);
      blue_tooth_data = comdata;
      comdata = "";
    }

  // 与PT_BEGIN对应
  PT_END(pt);
}


// ktv mode
static int thread2_entry(struct pt *pt){
  PT_BEGIN(pt);
  if(mode_controll == 1){
     ktv_mode();
  }
  PT_END(pt);
}

// evening mode
static int thread3_entry(struct pt *pt)
{
  PT_BEGIN(pt);
  if(mode_controll == 2){
    if(islockedSitLed == true){
      int soundVol;
      soundVol = soundSensor();
      if(soundSensor() > 45){         // 第一次到传感器的声波大于一定值
        Serial.println(soundVol);
        PT_TIMER_DELAY(pt,60);         
    
        if(soundSensor() > 2){        // 延时五十毫秒后重新获取声波大小值,并判断是否大于一定值
          exchange_locked_state();            // 若都满足条件,则改变声音环绕状态值
          led_lock_by_sound(islockedSitLed);
          PT_TIMER_DELAY(pt,4500);                         //让灯泡暗一会儿
        }
    
//      blink();
      

      }

      
    }
    else{
      if(humanSensor() > 500){
        turn_on_light(60);   
        condition = "someone_moved";
        PT_TIMER_DELAY(pt,15000); 
        condition = "no_one_moved";
      }  
      else{
        turn_on_light(0);   
      }
      
    }
  
  }

  PT_END(pt);
}


// ktv mode
static int thread_pia_entry(struct pt *pt){
  PT_BEGIN(pt);
  if(condition == "someone_moved"){
    pia(soundSensor(), 40, 2);                       //监听声音

    if(soundSensor() > 40){         // 第一次到传感器的声波大于一定值
      delay(50);              
      if(soundSensor() > 2){        // 延时五十毫秒后重新获取声波大小值,并判断是否大于一定值
        exchange_locked_state();            // 若都满足条件,则改变声音环绕状态值
        led_lock_by_sound(islockedSitLed); 
        PT_TIMER_DELAY(pt,900);   
      }
    
    }

  }
  PT_END(pt);
}


// normal mode
static int thread_normal_entry(struct pt *pt){
  PT_BEGIN(pt);
  if(mode_controll == 3){
    if (blue_tooth_data.length() > 0)
    {
     if(blue_tooth_data.toInt() != 0 && blue_tooth_data.toInt() <= 255){
       normal_led_intensity = blue_tooth_data.toInt();
     }

    }
    
     analogWrite(Led_controll,normal_led_intensity);
    

   }
  PT_END(pt);
}


// 读取arduino uno上A0引脚的值,为人体传感器的输出值. 人体感应模块接5V电压
int humanSensor(){
  int sensorValue = analogRead(A0); 
  return sensorValue;
}

// 读取arduino uno上A1引脚的值,为声音传感器的输出值
int soundSensor(){
  int soundSensor = analogRead(A1); 
  return soundSensor;
}

// ktv mode
void ktv_mode(){
  turn_on_light(soundSensor());
//  turn_on_light(255);
  digitalWrite(led_direction, HIGH); 
  Serial.print(soundSensor());
  Serial.print("   is soundSensor result\n");
}

void blink(){
  turn_on_light(255);
  delay(500);
}

void turn_on_light(int intensity){
  if(intensity > 10){
    analogWrite(Led_controll,intensity);
  }else{
    analogWrite(Led_controll,0);
  }
  
//  analogWrite(Led_controll,intensity);
}

// 夜间模式下调用
// 监听没有发出较大的声音
void pia(int soundVol, int first_wave, int next_wave){
  if(soundVol > first_wave){         // 第一次到传感器的声波大于一定值
    Serial.println(soundVol);
    delay(50);              

//    Serial.println(lightVol);
    if(soundSensor() > next_wave){        // 延时五十毫秒后重新获取声波大小值,并判断是否大于一定值
      exchange_locked_state();            // 若都满足条件,则改变声音环绕状态值
    }

  blink();
  led_lock_by_sound(islockedSitLed);
  }
}

void exchange_locked_state(){
   if(islockedSitLed == false){       
    islockedSitLed = true;
   }
   else{
    islockedSitLed = false;
   } 
}

// 夜间模式下调用
// 改变LED是否被声音锁住状态
void led_lock_by_sound(boolean islocked){
  if(islocked == true){
    turn_on_light(255);
  }else{
    turn_on_light(0);
  }
}


void loop(){
  //Check each thread by priority
  thread1_entry(&thread1);
  thread2_entry(&thread2);
  thread3_entry(&thread3);
  thread_pia_entry(&thread_pia);
  thread_normal_entry(&thread_normal);
//          Serial.println(humanSensor());
//        Serial.print("is  humanSensor \n");
}





