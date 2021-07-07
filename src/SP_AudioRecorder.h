#ifndef __SP_AUDIO_RECORDER_H__
#define __SP_AUDIO_RECORDER_H__

#include <SP_Audio.h>
#include <File.h>

class SP_AudioRecorder : public SP_Audio {
public:
  SP_AudioRecorder()
   : SP_Audio(RECORDER, recorder_thread) {}
  ~SP_AudioRecorder() {}

  int recorder(const char *filename,
               uint8_t codec,    // AS_CODECTYPE_{MP3,WAV}
               uint32_t fs,      // AS_SAMPLINGRATE_xxxxxx
               uint8_t bitlen,   // AS_BITLENGTH_xx
               uint8_t channel); // AS_CHANNEL_{MONO,STEREO,4CH,6CH,8CH} 
  int mp3recorder(const char *filename);
  int wavrecorder(const char *filename);
  int stop();

private:
  static const int volume_min = 0;
  static const int volume_max = 120;
  static int recorder_thread(int argc, FAR char *argv[]);
  static err_t setRecorder(msg_s *pmsg);
  static err_t eventRecorder(msg_s *pmsg, File &myFile);
  static err_t eventVolume(msg_s *pmsg);
};

#endif
