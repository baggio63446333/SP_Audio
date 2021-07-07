#ifndef __SP_AUDIO_H__
#define __SP_AUDIO_H__

#include <Arduino.h>
#include <Audio.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>

class SP_Audio {
public:
  enum {
    PLAYER_MAIN = 0,
    PLAYER_SUB,
    PLAYER_BEEP,
    RECORDER,
    PLAYER_MAX,
  };
  SP_Audio(int id, main_t func)
   : _id(id), _mqd((mqd_t)-1), _pid(-1),
     _fs(AS_SAMPLINGRATE_AUTO),
     _bitlen(AS_BITLENGTH_16),
     _channel(AS_CHANNEL_STEREO),
     _threadfunc(func)
  {
    assert((PLAYER_MAIN <= id) && (id < PLAYER_MAX));
    assert(func != NULL);
  }
  ~SP_Audio() {}

  // Initialize
  int begin();
  // Parameter
  void setFs(uint32_t fs) { _fs = fs; }
  void setBitlen(uint8_t bitlen) { _bitlen = bitlen; }
  void setChannel(uint8_t channel) { _channel = channel; }
  uint32_t getFs() { return _fs; }
  uint8_t getBitlen() { return _bitlen; }
  uint8_t getChannel() { return _channel; }
  // State
  bool isStopped();
  bool isPlaying();
  bool isPaused();
  bool isRecording();
  int readState();
  // Volume
  int volume(int vol); /* player: -1020~120, beep: -90~0, recorder: 0~120 */
  int volumeUp(int incvol = 10);
  int volumeDown(int decvol = 10);
  int readVolume();
  // Finalize
  int end() { /* not support */ return 0; }

protected:
  enum mode_e {
    PLAYER_MODE = 0,
    RECORDER_MODE,
  };
  enum state_e {
    STOPPED = 0,
    PLAYING,
    PAUSED,
    BEEPING,
    RECORDING,
  };
  enum cmd_e {
    CMD_PLAY,
    CMD_PLAYCONTINUE,
    CMD_STOP,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_VOLUME,
    CMD_VOLUP,
    CMD_VOLDOWN,
    CMD_TONE,
    CMD_NOTONE,
    CMD_REC,
    CMD_RECSTOP,
    CMD_RECCONTINUE,
    CMD_INVALID = 0xffffffff,
  };
  struct msg_s {
    cmd_e    cmd;
    int      id;
    int      volume;
    union {
      struct {
        char     *filename;
        uint32_t fs;      /* AS_SAMPLINGRATE_XXXXX */
        uint8_t  codec;   /* AS_CODECTYPE_MP3 or AS_CODECTYPE_WAV */
        uint8_t  bitlen;  /* AS_BITLENGTH_XX */
        uint8_t  channel; /* AS_CHANNEL_XXX */
      };
      struct {
        short    frequency;
        uint32_t duration;
      };
    };
  };
  int      _id;
  mqd_t    _mqd;
  pid_t    _pid;
  uint32_t _fs;
  uint8_t  _bitlen;
  uint8_t  _channel;
  main_t   _threadfunc;
  static AudioClass *theAudio;
  static mode_e _mode;
  static int receiveCommand(mqd_t mqdes, msg_s *msg, state_e state);
};

#endif
