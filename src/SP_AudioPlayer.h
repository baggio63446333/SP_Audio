#ifndef __SP_AUDIO_PLAYER_H__
#define __SP_AUDIO_PLAYER_H__

#include <SP_Audio.h>
#include <File.h>

class SP_AudioPlayer : public SP_Audio {
public:
  SP_AudioPlayer(int id = PLAYER_MAIN)
   : SP_Audio(id, player_thread)
  {
    assert((PLAYER_MAIN <= id) && (id <= PLAYER_SUB));
  }
  ~SP_AudioPlayer() {}

  int play(const char *filename,
           uint8_t codec,   // AS_CODECTYPE_{MP3,WAV}
           uint32_t fs,     // AS_SAMPLINGRATE_xxxxxx
           uint8_t bitlen,  // AS_BITLENGTH_xx
           uint8_t channel, // AS_CHANNEL_{MONO,STEREO,4CH,6CH,8CH} 
           bool sync = false);
  int mp3play(const char *filename, bool sync = false);
  int wavplay(const char *filename, bool sync = false);
  int stop();
  int pause();
  int resume();

private:
  static const int volume_min = -1020;
  static const int volume_max = 120;
  static int player_thread(int argc, FAR char *argv[]);
  static err_t setPlayer(msg_s *pmsg);
  static err_t eventPlayer(msg_s *pmsg, File &myFile);
  static err_t eventVolume(msg_s *pmsg);
};

#endif
