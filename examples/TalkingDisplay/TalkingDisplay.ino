#include "SpresenseTalking_Display.h"

SpresenseTalking_Display td;

uint32_t tim;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Start");
  td.begin();
  td.volume(30);
  td.setChannel(AS_CHANNEL_MONO);
  
  tim = millis();
  Serial.println("Commands: E=english, G=german, I##=integer, F##=float,");
  Serial.println("W##=word, D=date1, d=date2, T=time1, t=time2");
}

void loop() {
  /*
  if ((millis()-tim) > 10000) {
    td.say(1,207);
    td.say(1,203);
    td.say(1,24);
    td.say(1,208);
    td.say(1,213);
    tim = millis();
  }
  */
  td.loop();
  if (Serial.available()) {
    time_t t1 = 607016004; //is Mon, 27 Mar 1989 15:33:24 
    time_t t2 = 1610602200; //is Thu, 14 Jan 2021 05:30:00 
    struct tm * s_time;
    String x = Serial.readString();
    Serial.println(x);  
    char cmd = x[0];
    x = x.substring(1);
    switch (cmd) {
      case 'E' : td.setEnglish(true); break;
      case 'G' : td.setEnglish(false); break;
      case 'F' : td.sayFloat(x.toFloat(),1); break;
      case 'f' : td.sayFloat(x.toFloat(),2); break;
      case 'I' : td.sayInt(x.toInt()); break;
      case 'T' :  
         s_time = localtime(&t1);
         Serial.println("15:33:24");
         td.sayTime(s_time,true,true);
         break;
      case 't' : 
         s_time = localtime(&t2);
         Serial.println("05:30:00");
         td.sayTime(s_time,false,false);
         break;
      case 'D' : 
         s_time = localtime(&t1);
         Serial.println("Mon, 27 Mar 1989");
         td.sayDate(s_time,true);
         break;
      case 'd' : 
         s_time = localtime(&t2);
         Serial.println("14 Jan 2021");
         td.sayDate(s_time,false);
         break;
      case 'W' : td.say(x.toInt()); break;
    }
     
  }

}
