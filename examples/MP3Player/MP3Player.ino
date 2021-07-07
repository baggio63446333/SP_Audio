#include <SP_AudioPlayer.h>
#include <dirent.h>

SP_AudioPlayer player;

void setup()
{
  Serial.begin(115200);
  Serial.println("Spresense Simple MP3 Player");

  /* Initialize Player */
  player.begin();
  player.volume(10);
  player.setBitlen(16); /* same as the default */
  player.setChannel(2); /* same as the default */
}

void loop()
{
  DIR           *dir;
  struct dirent *entry;
  char          track[128];
  const char    *folder = "/mnt/sd0/mp3";

  /* Play tracks in order from a folder */

  if ((dir = opendir(folder)) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      sprintf(track, "%s/%s", folder, entry->d_name);
      Serial.println(track);
      player.mp3play(track, true);
    }
    closedir(dir);
  }
}
