#ifndef __RTS2_SHOOTERCONN__
#define __RTS2_SHOOTERCONN__

#include "../utils/rts2connnosend.h"

#include "augershooter.h"

#define AUGER_BUF_SIZE	200

class Rts2DevAugerShooter;

class Rts2ConnShooter:public Rts2ConnNoSend
{
private:
  Rts2DevAugerShooter * master;

  struct timeval last_packet;

  double last_target_time;

  // init server socket
  int init_listen ();

  void getTimeTfromGPS (long GPSsec, long GPSusec, double &out_time);

  int port;

  int auger_listen_socket;
  char nbuf[AUGER_BUF_SIZE];

  int processAuger ();

public:
    Rts2ConnShooter (int in_port, Rts2DevAugerShooter * in_master);
    virtual ~ Rts2ConnShooter (void);

  virtual int idle ();
  virtual int init ();

  virtual int add (fd_set * set);

  virtual int connectionError (int last_data_size);
  virtual int receive (fd_set * set);

  int lastPacket ();
  double lastTargetTime ();
};

#endif /* !__RTS2_SHOOTERCONN__ */
