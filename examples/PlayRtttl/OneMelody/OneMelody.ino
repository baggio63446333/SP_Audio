/*
 * OneMelody.ino
 *
 * Plays a melody from FLASH.
 *
 * More RTTTL songs can be found under http://www.picaxe.com/RTTTL-Ringtones-for-Tune-Command/
 *
 *  Copyright (C) 2019  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of PlayRttl https://github.com/ArminJo/PlayRtttl.
 *
 *  PlayRttl is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>
#include <SP_AudioBeeper.h>

SP_AudioBeeper beeper;

void tone(uint8_t pin, unsigned int frequency, unsigned long duration)
{
  beeper.tone(frequency, duration);
}

void noTone(uint8_t pin)
{
  beeper.noTone();
}

#include <PlayRtttl.h>

const int TONE_PIN = 11; // dummy

char StarWarsInRam[] =
        "StarWars:d=32,o=5,b=45,l=2,s=N:p,f#,f#,f#,8b.,8f#.6,e6,d#6,c#6,8b.6,16f#.6,e6,d#6,c#6,8b.6,16f#.6,e6,d#6,e6,8c#6";

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_PLAY_RTTTL));

    /* Initialize beeper */
    beeper.begin();
    beeper.volume(-20);

    /*
     * Play one melody
     */
    playRtttlBlocking(TONE_PIN, StarWarsInRam);
    delay(5000);
}

void loop() {
    /*
     * And all the other melodies, but use now the non blocking functions
     */
    for (uint8_t i = 1; i < ARRAY_SIZE_MELODIES_SMALL; ++i) {
        const char* tSongPtr;
        tSongPtr = (char*) RTTTLMelodiesSmall[i];
        Serial.println(F("Play next melody"));
        startPlayRtttlPGM(TONE_PIN, tSongPtr);
        while (updatePlayRtttl()) {
            /*
             * your own code here...
             */
            //delay(1);
            usleep(1 * 1000);
        }
        delay(2000);
    }
    delay(20000);
}

