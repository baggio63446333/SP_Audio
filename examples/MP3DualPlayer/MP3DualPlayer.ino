#include <SP_AudioPlayer.h>
#include <dirent.h>

SP_AudioPlayer player(SP_Audio::PLAYER_MAIN);
SP_AudioPlayer player2(SP_Audio::PLAYER_SUB);

void setup()
{
  Serial.begin(115200);
  Serial.println("Spresense MP3 Dual Player");

  /* Initialize Player */
  player.begin();
  player.volume(10);
  player.setBitlen(16); /* same as the default */
  player.setChannel(2); /* same as the default */

  player2.begin();
  player2.volume(10);
  player2.setBitlen(16); /* same as the default */
  player2.setChannel(2); /* same as the default */
}

void loop()
{
  DIR           *dir;
  struct dirent *entry;
  char          track[128];
  const char    *folder = "/mnt/sd0/mp3";

  /* Dual Play the same track in order from a folder */
  if ((dir = opendir(folder)) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      sprintf(track, "%s/%s", folder, entry->d_name);
      Serial.println(track);
      player.mp3play(track);
      delay(500); // dual play after a little delay
      player2.mp3play(track, true);
    }
    closedir(dir);
  }
}
