#include <SP_Audio.h>

#define VOLUME_MAX 120
#define VOLUME_MIN -1020
#define BEEP_VOLUME_MAX 0
#define BEEP_VOLUME_MIN -90
#define AMIC_VOLUME_MAX 0
#define AMIC_VOLUME_MIN 120

int playerStatus[SP_Audio::PLAYER_MAX] = {0, 0, 0, 0};
int playerVolume[SP_Audio::PLAYER_MAX] = {0, 0, BEEP_VOLUME_MIN, 0};

AudioClass *SP_Audio::theAudio = NULL;
SP_Audio::mode_e SP_Audio::_mode = PLAYER_MODE;

int SP_Audio::receiveCommand(mqd_t mqdes, msg_s *msg, state_e state)
{
  ssize_t ret;
  struct timespec ts;

  if ((state == PLAYING) || (state == RECORDING)) {
    /* wait until receiving any message with 40msec timeout */
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += 40000;
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_sec++;
      ts.tv_nsec -= 1000000000;
    }
    ret = mq_timedreceive(mqdes, (char*)msg, sizeof(msg_s), NULL, &ts);
  } else {
    /* wait until receiving any message without timeout */
    ret = mq_receive(mqdes, (char*)msg, sizeof(msg_s), NULL);
  }
  if (ret < 0) {
    if ((errno == ETIMEDOUT) && (state == PLAYING)) {
      msg->cmd = CMD_PLAYCONTINUE;
    } else if ((errno == ETIMEDOUT) && (state == RECORDING)) {
      msg->cmd = CMD_RECCONTINUE;
    } else {
      msg->cmd = CMD_INVALID;
    }
  }
  return ret;
}

int SP_Audio::begin()
{
  int ret = 0;
  char mqname[16];

  if (!theAudio) {
    theAudio = AudioClass::getInstance();
    theAudio->begin();
    playerVolume[PLAYER_MAIN] = 0;
    playerVolume[PLAYER_SUB]  = 0;
    playerVolume[PLAYER_BEEP] = BEEP_VOLUME_MIN;
    // Set player mode for beep player
    theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
    theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);
    theAudio->setVolume(-2, playerVolume[PLAYER_MAIN], playerVolume[PLAYER_SUB]);
  }

  /* Create audio player message queue */
  struct mq_attr mq_attr;
  mq_attr.mq_maxmsg  = 1;
  mq_attr.mq_msgsize = sizeof(struct msg_s);
  mq_attr.mq_flags   = 0;

  sprintf(mqname, "playmq%d", _id);
  _mqd = mq_open(mqname, O_CREAT | O_WRONLY, 0666, &mq_attr);
  if (_mqd == (mqd_t)-1) {
    printf("ERROR: create mq_open %s failed errno=%d\n", mqname, errno);
  }

  /* Create audio player task */
  const char *argv[2];
  switch (_id) {
    case PLAYER_MAIN: argv[0] = "0"; break;
    case PLAYER_SUB:  argv[0] = "1"; break;
    case PLAYER_BEEP: argv[0] = "2"; break;
    case RECORDER:    argv[0] = "3"; break;
    default:          argv[0] = "0"; break;
  }
  argv[1] = 0;

  _pid = task_create("player", 101, 2048, _threadfunc, (char * const *)argv);
  return ret;
}

bool SP_Audio::isStopped()
{
  return (playerStatus[_id] == STOPPED);
}

bool SP_Audio::isPlaying()
{
  if (_id == PLAYER_BEEP) {
    return (playerStatus[_id] == BEEPING);
  }
  return (playerStatus[_id] == PLAYING);
}

bool SP_Audio::isPaused()
{
  return (playerStatus[_id] == PAUSED);
}

bool SP_Audio::isRecording()
{
  return (playerStatus[_id] == RECORDING);
}

int SP_Audio::readState()
{
  return playerStatus[_id];
}

int SP_Audio::volume(int vol)
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  switch (_id) {
    case PLAYER_MAIN:
    case PLAYER_SUB:
      if (vol < VOLUME_MIN || VOLUME_MAX < vol) {
        return -EINVAL;
      }
      break;
    case PLAYER_BEEP:
      if (vol < BEEP_VOLUME_MIN || BEEP_VOLUME_MAX < vol) {
        return -EINVAL;
      }
      break;
    case RECORDER:
      if (vol < AMIC_VOLUME_MIN || AMIC_VOLUME_MAX < vol) {
        return -EINVAL;
      }
      break;
    default:
      return -EINVAL;
  }
  msg.id  = _id;
  msg.cmd = CMD_VOLUME;
  msg.volume = vol;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  return ret;
}

int SP_Audio::volumeUp(int vol)
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  msg.id  = _id;
  msg.cmd = CMD_VOLUP;
  msg.volume = vol;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  return ret;
}

int SP_Audio::volumeDown(int vol)
{
  int ret;
  struct msg_s msg;
  int msg_prio = 0;

  msg.id  = _id;
  msg.cmd = CMD_VOLDOWN;
  msg.volume = vol;
  ret = mq_send(_mqd, (const char*)&msg, sizeof(msg), msg_prio);
  return ret;
}

int SP_Audio::readVolume()
{
  return playerVolume[_id];
}
