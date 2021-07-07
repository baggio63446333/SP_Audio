#ifndef __SP_AUDIO_BEEPER_H__
#define __SP_AUDIO_BEEPER_H__

#include <SP_Audio.h>

class SP_AudioBeeper : public SP_Audio {
public:
  SP_AudioBeeper()
   : SP_Audio(PLAYER_BEEP, beeper_thread) {}
  ~SP_AudioBeeper() {}

  int tone(short frequency, uint32_t duration = 0); // freq: 94~4085
  int noTone();

private:
  static const int volume_min = -90;
  static const int volume_max = 0;
  static int beeper_thread(int argc, FAR char *argv[]);
  static err_t eventBeep(msg_s *pmsg);
  static err_t eventVolume(msg_s *pmsg);
};

#endif
