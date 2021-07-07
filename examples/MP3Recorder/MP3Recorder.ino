#include <Arduino.h>
#include <SP_AudioPlayer.h>
#include <SP_AudioRecorder.h>

SP_AudioPlayer   player;
SP_AudioRecorder recorder;

void setup()
{
  Serial.begin(115200);
  Serial.println("Spresense Simple MP3 Recorder");

  /* Wait a moment to prevent a recording file from being overwritten
   * immediately after a reset.
   */
  sleep(5);

  /* Initialize Recorder */
  recorder.begin();
  recorder.volume(50);
  recorder.setFs(48000);  /* same as the default */
  recorder.setBitlen(16); /* same as the default */
  recorder.setChannel(2); /* same as the default */

  /* Initialize Player */
  player.begin();
  player.volume(10);
  player.setFs(48000);  /* same as the default */
  player.setBitlen(16); /* same as the default */
  player.setChannel(2); /* same as the default */
}

void loop()
{
  const char *fname = "/mnt/sd0/sample.mp3";

  /* Remove a file in advance */
  unlink(fname);

  /* Play the file after recording */

  Serial.print(String("Recording (10sec): ") + fname);
  recorder.mp3recorder(fname);
  sleep(10); /* 10sec recording */
  recorder.stop();
  Serial.println("Done");

  Serial.print(String("Playing: ") + fname);
  player.mp3play(fname, true);
  Serial.println("Done");
}
