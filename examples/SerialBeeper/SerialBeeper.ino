#include <SP_AudioBeeper.h>

SP_AudioBeeper beeper;

void setup()
{
  Serial.begin(115200);
  Serial.println("Spresense Serial Beep Player");
  
  /* Initialize Beeper */
  beeper.begin();
  beeper.volume(-40);

  /* Usage */
  Serial.println("Input command:");
  Serial.println(" +: volume up");
  Serial.println(" -: volume down");
  Serial.println(" f#: set frequency");
  Serial.println(" u#: frequency up");
  Serial.println(" n#: frequency down");
  Serial.println(" s: stop");
}

void loop()
{
  static short freq = 523;

  if (Serial.available() > 0) {
    String str = Serial.readString();
    Serial.println(str);  
    char cmd = str[0];
    str = str.substring(1);
    int i = str.toInt();
    switch (cmd) {
      case '+': beeper.volumeUp(5); goto setvol;
      case '-': beeper.volumeDown(5);
      setvol:
        Serial.println(String("vol=") + beeper.readVolume());
        if (beeper.isPlaying())
          beeper.tone(freq);
        break;
      case 'u': freq += i; goto setfreq;
      case 'n': freq -= i; goto setfreq;
      case 'f': freq = i;
      setfreq:
        Serial.println(String("freq=") + freq);
        beeper.tone(freq);
        break;
      case 's': beeper.noTone(); break;
      default:
        break;
    }
  }
  usleep(1);
}
