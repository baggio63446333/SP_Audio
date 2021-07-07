#include <SP_AudioBeeper.h>

extern int playerStatus[SP_Audio::PLAYER_MAX];
extern int playerVolume[SP_Audio::PLAYER_MAX];

err_t SP_AudioBeeper::eventBeep(msg_s *pmsg)
{
  err_t err         = AUDIOLIB_ECODE_OK;
  cmd_e cmd         = pmsg->cmd;
  int id            = pmsg->id;
  short frequency   = pmsg->frequency;
  uint32_t duration = pmsg->duration;

  switch (cmd) {
    case CMD_TONE:
      playerStatus[id] = BEEPING;
      if (_mode != PLAYER_MODE) {
        theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
        theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);
        _mode = PLAYER_MODE;
      }
      theAudio->setBeep(1, playerVolume[id], frequency);
      if (duration > 0) {
        usleep(duration * 1000);
        theAudio->setBeep(0, playerVolume[id], 0);
        playerStatus[id] = STOPPED;
      }
      break;
    case CMD_NOTONE:
      theAudio->setBeep(0, playerVolume[id], 0);
      playerStatus[id] = STOPPED;
      break;
    default:
      break;
  }

  return err;
}

err_t SP_AudioBeeper::eventVolume(msg_s *pmsg)
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

int SP_AudioBeeper::beeper_thread(int argc, FAR char *argv[])
{
  struct msg_s msg;
  mqd_t        rmqd;

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
    case CMD_TONE:
    case CMD_NOTONE:
      eventBeep(&msg);
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

int SP_AudioBeeper::tone(short frequency, uint32_t duration)
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  if (playerStatus[RECORDER] != STOPPED) {
    return -EPERM;  
  }

  msg.id  = _id;
  msg.cmd = CMD_TONE;
  msg.frequency = frequency;
  msg.duration = duration;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  return ret;
}

int SP_AudioBeeper::noTone()
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  msg.id  = _id;
  msg.cmd = CMD_NOTONE;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  return ret;
}
