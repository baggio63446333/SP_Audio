#include <SP_AudioPlayer.h>
#include <dirent.h>

SP_AudioPlayer player(SP_Audio::PLAYER_MAIN);
SP_AudioPlayer player2(SP_Audio::PLAYER_SUB);

void setup()
{
  Serial.begin(115200);
  Serial.println("Spresense WAV Dual Player");

  /* Initialize Player */
  player.begin();
  player.volume(10);
  player.setFs(44100); /* Change from the default 48kHz to 44.1kHz */
  player.setBitlen(16); /* same as the default */
  player.setChannel(2); /* same as the default */

  player2.begin();
  player2.volume(10);
  player2.setFs(44100); /* Change from the default 48kHz to 44.1kHz */
  player2.setBitlen(16); /* same as the default */
  player2.setChannel(2); /* same as the default */
}

void loop()
{
  DIR           *dir;
  struct dirent *entry;
  char          track[128];
  const char    *folder = "/mnt/sd0/wav";

  /* Dual Play the same track in order from a folder */
  if ((dir = opendir(folder)) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      sprintf(track, "%s/%s", folder, entry->d_name);
      Serial.println(track);
      player.wavplay(track);
      delay(100); // dual play after a little delay
      player2.wavplay(track, true);
    }
    closedir(dir);
  }
}
