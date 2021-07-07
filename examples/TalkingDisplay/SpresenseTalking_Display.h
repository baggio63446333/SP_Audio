/*
|| Modified for Spresense Arduino Environment
|| Use Spresense Audio Player instead of DFPlayer
|| @author baggio63446333
||
|| @file Talking_Display.h
|| @version 1.4
|| @author Gerald Lechner
|| @contact lechge@gmail.com
||
|| The communication part was copied from library "DFPlayer_Mini_Mp3_by_Makuna"
|| Written by Michael C. Miller.
||
|| @description
|| | The library uses the MP3 player module DFPlayer Mini for output.
|| | The communication with module is handled in the library.
|| | No extra driver for DFPlayer is required. Only the pointer on
|| | a Serial Interface hardware or software is required.
|| | MP3 Files for English and German are supplied and need to be copied on a Micro-SD card.
|| #
||
|| @license
|| | This library is free software; you can redistribute it and/or
|| | modify it under the terms of the GNU Lesser General Public
|| | License as published by the Free Software Foundation; version
|| | 2.1 of the License.
|| |
|| | This library is distributed in the hope that it will be useful,
|| | but WITHOUT ANY WARRANTY; without even the implied warranty of
|| | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|| | Lesser General Public License for more details.
|| |
|| | You should have received a copy of the GNU Lesser General Public
|| | License along with this library; if not, write to the Free Software
|| | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
|| #
||
*/
#ifndef SpresenseTalking_Display_h
#define SpresenseTalking_Display_h

#include <SP_AudioPlayer.h>

//vocabulary
#define WORD_HUNDREDS 100
#define WORD_TOUSEND 110
#define WORD_MILLION 120
#define WORD_BILLION 130
#define WORD_WEEKDAY 300
#define WORD_MONTH 310
#define WORD_DAY 330

#define WORD_IT_IS 201
#define WORD_CLOCK 202
#define WORD_IS 203
#define WORD_SECOND 204
#define WORD_DOT 205
#define WORD_TEMPERATURE 206
#define WORD_OUTSIDE_TEMPERATURE 207
#define WORD_DEGREE 208
#define WORD_HUMIDITY 209
#define WORD_PERCENT 210
#define WORD_MINUS 211
#define WORD_AND 212
#define WORD_CELSIUS 213
#define WORD_AIR_PRESSURE 214
#define WORD_HECTOPASCAL 215
#define WORD_FAHRENHEIT 216
#define WORD_MINUTES 217
#define WORD_HOURS 218
#define WORD_BRILLIANCE 219
#define WORD_LUX 220
#define WORD_AM 221
#define WORD_PM 222

//class definition
class SpresenseTalking_Display : public SP_AudioPlayer {
public:
  //initializer
  SpresenseTalking_Display() : _english(true) {};

  //call this function within the main loop to reakt on
  //messages from modul
  void loop()
  {
    usleep(1);
  }

  //set the volume vakues from 0 to 30 are allowed
  void setVolume(uint8_t volume)
  {
  }

  //set or unset English for language
  void setEnglish(boolean english) {
    _english = english;
  }

  //say the word found on the given track
  //if english folder 00 will be used otherwise folder 01
  void say(uint16_t track) {
    uint8_t folder = _english?0:1;
    char filename[64];

    sprintf(filename, "/mnt/sd0/words/%02d/%04d.mp3", folder, track);
    mp3play(filename);
  }

  //speaks three digit numbers
  void sayHundreds(uint16_t number) {
    uint16_t h = number / 100;
    uint16_t e = number%100;
    if (h>0) say(WORD_HUNDREDS+h);
    if (e>0) say(e);
  }

  //speaks any 32 bit integer
  void sayInt(int32_t number) {
   if (number == 0) {
     say(0);
   } else {
     boolean minus = (number < 0);
     if (minus) number *= -1;
     uint16_t einer = number%1000;
     number /= 1000;
     uint16_t tausender = number%1000;
     number /= 1000;
     uint16_t millionen = number%1000;
     number /= 1000;
     uint16_t milliarden = number%1000;
     if (minus) say(WORD_MINUS);
     if (milliarden > 0) {
      if (milliarden == 1) {
        say(WORD_BILLION+1);
      } else  {
        sayHundreds(milliarden);
        say(WORD_BILLION);
      }
     }
     if (millionen > 0) {
      if (millionen == 1) {
        say(WORD_MILLION+1);
      } else  {
        sayHundreds(millionen);
        say(WORD_MILLION);
      }
     }
     if (tausender > 0) {
      if (tausender == 1) {
        say(WORD_TOUSEND+1);
      } else  {
        sayHundreds(tausender);
        say(WORD_TOUSEND);
      }
     }
     sayHundreds(einer);
   }
  }

  //speaks a float number with 1 or two decimals
  //max value +/- 2147483647.99999
  void sayFloat(float number, uint8_t decimals = 2) {
    int32_t num = (int)number;
    float n = number - (float)num;
    int16_t dez = 0;
    if (decimals < 2) {
      dez = round(n*10);
      if (dez==10) {
        dez = 0; num++;
      }
    } else {
      dez = round(n*100);
      if (dez==100) {
        dez = 0; num++;
      }
    }
    sayInt(num);
    say(WORD_DOT);
    say(dez);
  }

  //say a time using 24 hours and switching seconds off can be done by parameter
  void sayTime(struct tm * s_time, boolean h24 = true, boolean seconds = false) {
     uint16_t ampm = WORD_AM;
     uint8_t hour = s_time->tm_hour;
     if ((hour > 11) && (!h24)) {
       ampm = WORD_PM;
       hour = hour - 12;
     }
     if (hour == 0) hour = 12;
     say(hour);
     if (_english) {
       delay(200);
     } else {
       say(WORD_CLOCK);
     }
     say(s_time->tm_min);
     if(!h24) say(ampm);
     if (seconds) {
       say(WORD_AND);
       say(s_time->tm_sec);
       say(WORD_SECOND);
     }
  }

  //say a date switching weekday and year off can be done by parameter
  //for english month will be followed by day
  //for german day will be followed by month
  void sayDate(struct tm * s_time, boolean weekday = true, boolean sayYear = true) {
    //if (weekday) say(WORD_WEEKDAY + s_time->tm_wday);
    if (_english) {
      say(WORD_MONTH+s_time->tm_mon);
      say(s_time->tm_mday);
    } else {
      say(WORD_DAY+s_time->tm_mday);
      say(WORD_MONTH+s_time->tm_mon);
    }
    if (sayYear) {
      uint16_t year = s_time->tm_year + 1900;
      if (year < 2000) {
        say(year/100);
        say(WORD_HUNDREDS);
        say(year%100);
      } else {
        sayInt(year);
      }
    }
  }

private:
  boolean _english;

};
#endif
