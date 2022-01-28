#include <SP_AudioRecorder.h>
#include <Storage.h>

extern int playerStatus[SP_Audio::PLAYER_MAX];
extern int playerVolume[SP_Audio::PLAYER_MAX];

err_t SP_AudioRecorder::setRecorder(msg_s *pmsg)
{
  err_t     err = AUDIOLIB_ECODE_OK;
  AsClkMode clkmode;

  theAudio->setReadyMode();

  /* Set audio clock 48kHz/192kHz */
  clkmode = (pmsg->fs <= 48000) ? AS_CLKMODE_NORMAL : AS_CLKMODE_HIRES;
  theAudio->setRenderingClockMode(clkmode);

  /* Select input device as analog microphone */
  theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC, 100);
  _mode = RECORDER_MODE;

  theAudio->initRecorder(pmsg->codec,
                         "/mnt/sd0/BIN",
                         pmsg->fs,
                         pmsg->bitlen,
                         pmsg->channel);

  return err;
}

err_t SP_AudioRecorder::eventRecorder(msg_s *pmsg, File &myFile)
{
  err_t   err       = AUDIOLIB_ECODE_OK;
  cmd_e   cmd       = pmsg->cmd;
  int     id        = pmsg->id;
  char    *filename = pmsg->filename;
  uint8_t codec     = pmsg->codec;

  switch (cmd) {
    case CMD_REC:
      playerStatus[id] = RECORDING;
      setRecorder(pmsg);
      ////printf("%s\n", msg.filename);
      myFile = Storage.open(filename, FILE_WRITE);
      free(filename);
      if (!myFile) {
        //printf("Doesn't exist\n");
        myFile.close();
        playerStatus[id] = STOPPED;
        break;
      }
      if (codec == AS_CODECTYPE_WAV) {
        theAudio->writeWavHeader(myFile);
      }
      theAudio->startRecorder();
      /* FALLTHRU */
    case CMD_RECCONTINUE:
      err = theAudio->readFrames(myFile);
      if (err != AUDIOLIB_ECODE_OK) {
        goto recstop;
      }
      break;
    case CMD_RECSTOP:
    recstop:
      theAudio->stopRecorder();
      theAudio->readFrames(myFile);
      theAudio->closeOutputFile(myFile);
      playerStatus[id] = STOPPED;
      break;
    default:
      break;
  }
  return err;
}

err_t SP_AudioRecorder::eventVolume(msg_s *pmsg)
{
  err_t err  = AUDIOLIB_ECODE_OK;
  cmd_e cmd  = pmsg->cmd;
  int id     = pmsg->id;
  int volume = pmsg->volume;

  switch (cmd) {
    case CMD_VOLUME:  playerVolume[id]  = volume; break;
    case CMD_VOLUP:   playerVolume[id] += volume; break;
    case CMD_VOLDOWN: playerVolume[id] -= volume; break;
    default:
      break;
  }

  if (volume_max < playerVolume[id]) { playerVolume[id] = volume_max; }
  if (playerVolume[id] < volume_min) { playerVolume[id] = volume_min; }

  return err;
}

int SP_AudioRecorder::recorder_thread(int argc, FAR char *argv[])
{
  struct msg_s msg;
  mqd_t        rmqd;
  File         myFile;

  /* Open a message queue */
  int  id = atoi(argv[1]);
  char mqname[16];
  sprintf(mqname, "playmq%d", id);
  rmqd = mq_open(mqname, O_RDONLY, 0666, NULL);
  if (rmqd == (mqd_t)-1) {
    printf("ERROR: mq_open %s failed errno=%d\n", mqname, errno);
  }

  while (1) {
    /* Receive command from message queue */
    receiveCommand(rmqd, &msg, (state_e)playerStatus[id]);
    ////printf("msg.cmd=%d\n", msg.cmd);

    switch (msg.cmd) {
    case CMD_REC:
    case CMD_RECCONTINUE:
    case CMD_RECSTOP:
      eventRecorder(&msg, myFile);
      break;
    case CMD_VOLUME:
    case CMD_VOLUP:
    case CMD_VOLDOWN:
      eventVolume(&msg);
      break;
    case CMD_INVALID:
    default:
      break;
    }
  }

  return 0;
}

int SP_AudioRecorder::recorder(const char *filename,
                                   uint8_t codec, uint32_t fs,
                                   uint8_t bitlen, uint8_t channel)
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  if ((playerStatus[PLAYER_MAIN] != STOPPED) ||
      (playerStatus[PLAYER_SUB] != STOPPED) ||
      (playerStatus[PLAYER_BEEP] != STOPPED)) {
        return -EPERM;  
  }

  msg.filename = (char *)zalloc(128);
  strncpy(msg.filename, filename, 128);
  msg.id      = _id;
  msg.cmd     = CMD_REC;
  msg.codec   = codec;
  msg.fs      = fs;
  msg.bitlen  = bitlen;
  msg.channel = channel;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  return ret;
}

int SP_AudioRecorder::mp3recorder(const char *filename)
{
  if (_fs == AS_SAMPLINGRATE_AUTO) {
    /* MP3ENC doesn't supported AUTO, then set 48kHz by default */
    _fs = AS_SAMPLINGRATE_48000;
  }
  return recorder(filename, AS_CODECTYPE_MP3, _fs, _bitlen, _channel);
}

int SP_AudioRecorder::wavrecorder(const char *filename)
{
  if (_fs == AS_SAMPLINGRATE_AUTO) {
    /* WAV doesn't supported AUTO, then set 48kHz by default */
    _fs = AS_SAMPLINGRATE_48000;
  }
  return recorder(filename, AS_CODECTYPE_WAV, _fs, _bitlen, _channel);
}

int SP_AudioRecorder::stop()
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  msg.id  = _id;
  msg.cmd = CMD_RECSTOP;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  do {
    usleep(100 * 1000);
  } while (!isStopped());
  return ret;
}
