#include <SP_AudioBeeper.h>
#include "note.h"

SP_AudioBeeper beeper;

void setup()
{
  Serial.begin(115200);
  Serial.println("Spresense Simple Beep Player");

  /* Initialize Beeper */
  beeper.begin();
  beeper.volume(-30);

  Serial.println("Beep start");
  beeper.tone(NOTE_C6, 500);
  beeper.tone(NOTE_D6, 500);
  beeper.tone(NOTE_E6, 500);
  beeper.tone(NOTE_F6, 500);
  beeper.tone(NOTE_G6, 500);
  beeper.tone(NOTE_A6, 500);
  beeper.tone(NOTE_B6, 500);
  beeper.tone(NOTE_C7, 500);
  Serial.println("Beep end");
}

void loop()
{
  // REVISIT: This thread has the same priority as an audio manager,
  // threfore it must wait in order to dispatch to other threads.
  usleep(1);
}
