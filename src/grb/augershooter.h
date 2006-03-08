#ifndef __RTS2_AUGERSHOOTER__
#define __RTS2_AUGERSHOOTER__

#include "../utilsdb/rts2devicedb.h"
#include "rts2connshooter.h"

#define RTS2_EVENT_AUGER_SHOWER		RTS2_LOCAL_EVENT + 700

class Rts2ConnShooter;

class Rts2DevAugerShooter:public Rts2DeviceDb
{
private:
  Rts2ConnShooter * shootercnn;
  int port;
protected:
    virtual int processOption (int in_opt);
public:
    Rts2DevAugerShooter (int in_argc, char **in_argv);
    virtual ~ Rts2DevAugerShooter (void);

  virtual int ready ()
  {
    return 0;
  }

  virtual int init ();
};

#endif /*! __RTS2_AUGERSHOOTER__ */
