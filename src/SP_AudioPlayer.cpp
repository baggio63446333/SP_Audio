#include <SP_AudioPlayer.h>
#include <Storage.h>

extern int playerStatus[SP_Audio::PLAYER_MAX];
extern int playerVolume[SP_Audio::PLAYER_MAX];

err_t SP_AudioPlayer::setPlayer(msg_s *pmsg)
{
  static struct param_s {
    uint8_t  codec;
    uint32_t fs;
    uint8_t  bitlen;
    uint8_t  channel;
  } s_param[2] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
  static AsClkMode s_clkmode = AS_CLKMODE_NORMAL;
  err_t            err       = AUDIOLIB_ECODE_OK;
  AsClkMode        clkmode;

  AudioClass::PlayerId id = (pmsg->id == 0) ? AudioClass::Player0 : AudioClass::Player1;
  struct param_s *p_param = &s_param[pmsg->id];

  if ((p_param->codec   != pmsg->codec) ||
      (p_param->fs      != pmsg->fs) ||
      (p_param->bitlen  != pmsg->bitlen) ||
      (p_param->channel != pmsg->channel) ||
      (_mode != PLAYER_MODE)) {

    /* Set audio clock 48kHz/192kHz */
    clkmode = (pmsg->fs <= 48000) ? AS_CLKMODE_NORMAL : AS_CLKMODE_HIRES;

    if ((s_clkmode != clkmode) || (_mode != PLAYER_MODE)) {
      /* When the audio master clock will be changed, it should change the clock
       * mode once after returning the ready state.
       */
      theAudio->setReadyMode();

      s_clkmode = clkmode;
      theAudio->setRenderingClockMode(clkmode);
      theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);
      _mode = PLAYER_MODE;
    }

    /* Initialize player */

    err = theAudio->initPlayer(id,
                               pmsg->codec,
                               "/mnt/sd0/BIN",
                               pmsg->fs,
                               pmsg->bitlen,
                               pmsg->channel);
    if (err != AUDIOLIB_ECODE_OK) {
      printf("Player initialize error\n");
      return err;
    }

    /* Save the current setting for each channel */
    p_param->codec   = pmsg->codec;
    p_param->fs      = pmsg->fs;
    p_param->bitlen  = pmsg->bitlen;
    p_param->channel = pmsg->channel;
  }

  return err;
}

err_t SP_AudioPlayer::eventPlayer(msg_s *pmsg, File &myFile)
{
  err_t   err       = AUDIOLIB_ECODE_OK;
  cmd_e   cmd       = pmsg->cmd;
  int     id        = pmsg->id;
  char    *filename = pmsg->filename;
  state_e state     = (state_e)playerStatus[id];

  AudioClass::PlayerId playerid =
    (id == PLAYER_MAIN) ? AudioClass::Player0 : AudioClass::Player1;

  switch (cmd) {
    case CMD_PLAY:
      playerStatus[id] = PLAYING;
      if (state == PLAYING) {
        theAudio->stopPlayer(playerid, AS_STOPPLAYER_NORMAL);
        myFile.close();
      }
      if (state == PAUSED) {
        // TBD: Should call stopPlayer() only with CMN_SimpleFifoClear
        //theAudio->stopPlayer(playerid, AS_STOPPLAYER_FORCIBLY);
        myFile.close();
      }
      setPlayer(pmsg);
      ////printf("%s\n", msg.filename);
      myFile = Storage.open(filename);
      free(filename);
      if (!myFile) {
        //printf("Doesn't exist\n");
        myFile.close();
        playerStatus[id] = STOPPED;
        break;
      }
      /* FALLTHRU */
    case CMD_RESUME:
      err = theAudio->writeFrames(playerid, myFile);
      theAudio->startPlayer(playerid);
      state = PLAYING;
      playerStatus[id] = PLAYING;
      /* FALLTHRU */
    case CMD_PLAYCONTINUE:
      if (err != AUDIOLIB_ECODE_FILEEND) {
        err = theAudio->writeFrames(playerid, myFile);
      }
      if (err == AUDIOLIB_ECODE_FILEEND) {
        goto stop;
      }
      break;
    case CMD_PAUSE:
      if (state == PLAYING) {
        // TBD: Should call stopPlayer() without CMN_SimpleFifoClear
        theAudio->stopPlayer(playerid, AS_STOPPLAYER_NORMAL);
        state = PAUSED;
        playerStatus[id] = PAUSED;
      }
      break;
    case CMD_STOP:
    stop:
      if (state == PLAYING) {
        if (err == AUDIOLIB_ECODE_FILEEND) {
          theAudio->stopPlayer(playerid, AS_STOPPLAYER_ESEND);
        } else {
          theAudio->stopPlayer(playerid, AS_STOPPLAYER_NORMAL);
        }
        myFile.close();
      }
      if (state == PAUSED) {
        // TBD: Should call stopPlayer() only with CMN_SimpleFifoClear
        //theAudio->stopPlayer(playerid, AS_STOPPLAYER_FORCIBLY);
        myFile.close();
      }
      playerStatus[id] = STOPPED;
      break;
    default:
      break;
  }

  return err;
}

err_t SP_AudioPlayer::eventVolume(msg_s *pmsg)
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

  err = theAudio->setVolume(-2, playerVolume[PLAYER_MAIN], playerVolume[PLAYER_SUB]);
  return err;
}

int SP_AudioPlayer::player_thread(int argc, FAR char *argv[])
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
    case CMD_PLAY:
    case CMD_RESUME:
    case CMD_PLAYCONTINUE:
    case CMD_PAUSE:
    case CMD_STOP:
      eventPlayer(&msg, myFile);
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

int SP_AudioPlayer::play(const char *filename,
                               uint8_t codec, uint32_t fs,
                               uint8_t bitlen, uint8_t channel,
                               bool sync)
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  assert((_id == PLAYER_MAIN) || (_id == PLAYER_SUB));

  if (playerStatus[RECORDER] != STOPPED) {
    return -EPERM;  
  }

  msg.filename = (char *)zalloc(128);
  strncpy(msg.filename, filename, 128);
  msg.id      = _id;
  msg.cmd     = CMD_PLAY;
  msg.codec   = codec;
  msg.fs      = fs;
  msg.bitlen  = bitlen;
  msg.channel = channel;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  if (sync) {
    do {
      usleep(100 * 1000);
    } while (!isStopped());
  }
  return ret;
}

int SP_AudioPlayer::mp3play(const char *filename, bool sync)
{
  return play(filename, AS_CODECTYPE_MP3, AS_SAMPLINGRATE_AUTO, _bitlen, _channel, sync);
}

int SP_AudioPlayer::wavplay(const char *filename, bool sync)
{
  if (_fs == AS_SAMPLINGRATE_AUTO) {
    /* WAV doesn't supported AUTO, then set 48kHz by default */
    _fs = AS_SAMPLINGRATE_48000;
  }
  return play(filename, AS_CODECTYPE_WAV, _fs, _bitlen, _channel, sync);
}

int SP_AudioPlayer::stop()
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  assert((_id == PLAYER_MAIN) || (_id == PLAYER_SUB));

  msg.id  = _id;
  msg.cmd = CMD_STOP;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  return ret;
}

int SP_AudioPlayer::pause()
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  assert((_id == PLAYER_MAIN) || (_id == PLAYER_SUB));

  msg.id  = _id;
  msg.cmd = CMD_PAUSE;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  return ret;
}

int SP_AudioPlayer::resume()
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  assert((_id == PLAYER_MAIN) || (_id == PLAYER_SUB));

  if (playerStatus[RECORDER] != STOPPED) {
    return -EPERM;  
  }

  msg.id  = _id;
  msg.cmd = CMD_RESUME;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  return ret;
}
