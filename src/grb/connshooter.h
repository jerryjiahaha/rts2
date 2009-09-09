#ifndef __RTS2_SHOOTERCONN__
#define __RTS2_SHOOTERCONN__

#include "../utils/rts2connnosend.h"

#include "augershooter.h"

#define AUGER_BUF_SIZE  5000

namespace rts2grbd
{

class DevAugerShooter;

class ConnShooter:public Rts2ConnNoSend
{
	private:
		DevAugerShooter * master;
		Rts2ValueDouble *minEnergy;
		Rts2ValueInteger *maxTime;

		struct timeval last_packet;

		double last_target_time;

		// init server socket
		int init_listen ();

		void getTimeTfromGPS (long GPSsec, long GPSusec, double &out_time);

		int port;

		char nbuf[AUGER_BUF_SIZE];

		int processAuger ();

	public:
		ConnShooter (int _port, DevAugerShooter * _master, Rts2ValueDouble *_minEnergy, Rts2ValueInteger *_maxTime);
		virtual ~ ConnShooter (void);

		virtual int idle ();
		virtual int init ();

		virtual void connectionError (int last_data_size);
		virtual int receive (fd_set * set);

		int lastPacket ();
		double lastTargetTime ();
};

}
#endif							 /* !__RTS2_SHOOTERCONN__ */
