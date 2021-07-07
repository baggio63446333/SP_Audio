#include <Audio.h>
#include <SDHCI.h>
#include <EEPROM.h>
#include <audio/utilities/playlist.h>
#include <SP_AudioPlayer.h>
#include <SP_AudioBeeper.h>

//#define FOLDER "/mnt/sd0/The Beatles/Revolver"
//#define FOLDER "/mnt/sd0/The Beatles/Abbey Road"
//#define FOLDER "/mnt/sd0/The Beatles/Beatles For Sale"
//#define FOLDER "/mnt/sd0/The Beatles/Help!"
//#define FOLDER "/mnt/sd0/The Beatles/A Hard Day's Night"
#define FOLDER "/mnt/sd0/mp3"

static void play();
static void list();

SP_AudioPlayer player;
SP_AudioBeeper beeper;
SDClass  theSD;
Playlist thePlaylist("TRACK_DB.CSV");
Track    currentTrack;

int eeprom_idx = 0;
struct SavedObject {
  int saved;
  int volume;
  int random;
  int repeat;
  int autoplay;
} preset;

void setup()
{
  Serial.begin(115200);
  Serial.println("Spresense Serial Playlist Player");

  /* Load preset data */
  EEPROM.get(eeprom_idx, preset);
  if (!preset.saved) {
    /* If no preset data, come here */
    preset.saved = 1;
    preset.volume = -160; /* default */
    preset.random = 0;
    preset.repeat = 0;
    preset.autoplay = 0;
    EEPROM.put(eeprom_idx, preset);
  }
  printf("Volume=%d\n", preset.volume);
  printf("Random=%s\n", (preset.random) ? "On" : "Off");
  printf("Repeat=%s\n", (preset.repeat) ? "On" : "Off");
  printf("Auto=%s\n", (preset.autoplay) ? "On" : "Off");

  /* Initialize SD */
  while (!theSD.begin()) {
      /* wait until SD card is mounted. */
      Serial.println("Insert SD card.");
  }

  /* Initialize playlist */
  const char *playlist_dirname = FOLDER;
  bool success = thePlaylist.init(playlist_dirname);
  if (!success) {
    printf("ERROR: no exist playlist file %s/TRACK_DB.CSV\n",
           playlist_dirname);
    while (1);
  }

  /* Set random seed to use shuffle mode */
  struct timespec ts;
  clock_systime_timespec(&ts);
  srand((unsigned int)ts.tv_nsec);

  /* Restore preset data */
  if (preset.random) {
    thePlaylist.setPlayMode(Playlist::PlayModeShuffle);
  }
  if (preset.repeat) {
    thePlaylist.setRepeatMode(Playlist::RepeatModeOn);
  }
  thePlaylist.getNextTrack(&currentTrack);

  /* Initialize Player */
  player.begin();
  player.volume(preset.volume);

  /* Initialize Beeper */
  beeper.begin();
  beeper.volume(-20);

  if (preset.autoplay) {
    play();
  }
}

enum {
  EVENT_NONE = 0,
  EVENT_PLAY,
  EVENT_STOP,
  EVENT_NEXT,
  EVENT_BACK,
  EVENT_VOLUP,
  EVENT_VOLDOWN,
  EVENT_LIST,
  EVENT_AUTOPLAY,
  EVENT_REPEAT,
  EVENT_RANDOM,
  EVENT_HELP,
};

int InputSerial()
{
  if (Serial.available() > 0) {
    switch (Serial.read()) {
      case 'p': return EVENT_PLAY;
      case 's': return EVENT_STOP;
      case 'n': return EVENT_NEXT;
      case 'b': return EVENT_BACK;
      case '+': return EVENT_VOLUP;
      case '-': return EVENT_VOLDOWN;
      case 'l': return EVENT_LIST;
      case 'a': return EVENT_AUTOPLAY;
      case 'r': return EVENT_REPEAT;
      case 'R': return EVENT_RANDOM;
      case 'm':
      case 'h':
      case '?':
        printf("=== MENU (input key ?) ==============\n");
        printf("p: play  s: stop  +/-: volume up/down\n");
        printf("l: list  n: next  b: back\n");
        printf("r: repeat on/off  R: random on/off\n");
        printf("a: auto play      m,h,?: menu\n");
        printf("=====================================\n");
        break;
      default:  return EVENT_NONE;
    }
  }
  return EVENT_NONE;
}

void loop()
{
  static enum State {
    Stopped,
    Active
  } s_state = preset.autoplay ? Active : Stopped;

  /* Menu operation */
  int ev = InputSerial();

  switch (ev) {
    case EVENT_PLAY:
      printf("play\n");
      play();
      s_state = Active;
      break;
    case EVENT_STOP:
      printf("stop\n");
      if (player.isPlaying()) {
        player.stop();
        s_state = Stopped;
      }
      break;
    case EVENT_NEXT:
      printf("next\n");
      if (thePlaylist.getNextTrack(&currentTrack)) {
        if (player.isPlaying()) {
          play();
        }
      }
      break;
    case EVENT_BACK:
      printf("back\n");
      if (thePlaylist.getPrevTrack(&currentTrack)) {
        if (player.isPlaying()) {
          play();
        }
      }
      break;
    case EVENT_VOLUP:
      player.volumeUp();
      preset.volume = player.readVolume();
      printf("Volume=%d\n", preset.volume);
      EEPROM.put(eeprom_idx, preset);
      break;
    case EVENT_VOLDOWN:
      player.volumeDown();
      preset.volume = player.readVolume();
      printf("Volume=%d\n", preset.volume);
      EEPROM.put(eeprom_idx, preset);
      break;
    case EVENT_LIST:
      if (preset.repeat) {
        thePlaylist.setRepeatMode(Playlist::RepeatModeOff);
        list();
        thePlaylist.setRepeatMode(Playlist::RepeatModeOn);
      } else {
        list();
      }
      break;
    case EVENT_AUTOPLAY:
      preset.autoplay = (preset.autoplay) ? 0 : 1;
      printf("Auto=%s\n", (preset.autoplay) ? "On" : "Off");
      EEPROM.put(eeprom_idx, preset);
      break;
    case EVENT_REPEAT:
      preset.repeat = (preset.repeat) ? 0 : 1;
      thePlaylist.setRepeatMode((preset.repeat) ? Playlist::RepeatModeOn :
                                                  Playlist::RepeatModeOff);
      printf("Repeat=%s\n", (preset.repeat) ? "On" : "Off");
      EEPROM.put(eeprom_idx, preset);
      break;
    case EVENT_RANDOM:
      preset.random = (preset.random) ? 0 : 1;
      thePlaylist.setPlayMode((preset.random) ? Playlist::PlayModeShuffle :
                                                Playlist::PlayModeNormal);
      printf("Random=%s\n", (preset.random) ? "On" : "Off");
      EEPROM.put(eeprom_idx, preset);
      break;
    case EVENT_NONE:
    default:
      break;
  }

  if ((s_state == Active) && (player.isStopped())) {
    if (thePlaylist.getNextTrack(&currentTrack)) {
      play();
    } else {
      player.stop();
      s_state = Stopped;
    }
  }

  usleep(1000);
  return;
}

static void play()
{
  Track *t = &currentTrack;

  printf("-> %s\n", t->title);

  char track[128];
  sprintf(track, "%s/%s", FOLDER, t->title);
  player.play(track,
              t->codec_type,
              t->sampling_rate,
              t->bit_length,
              t->channel_number);
  return;
}

static void list()
{
  Track t;
  thePlaylist.restart();
  printf("-----------------------------\n");
  while (thePlaylist.getNextTrack(&t)) {
    if (0 == strncmp(currentTrack.title, t.title, 64)) {
      printf("-> ");
    }
    printf("\t%s | %s | %s\n", t.author, t.album, t.title);
  }
  printf("-----------------------------\n");

  /* restore the current track */
  thePlaylist.restart();
  while (thePlaylist.getNextTrack(&t)) {
    if (0 == strncmp(currentTrack.title, t.title, 64)) {
      break;
    }
  }
}
