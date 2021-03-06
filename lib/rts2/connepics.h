/* 
 * EPICS interface.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __RTS2_CONNEPICS__
#define __RTS2_CONNEPICS__

#include "connnosend.h"
#include "error.h"

#include <cadef.h>
#include <map>

namespace rts2core
{

/**
 * EPICS connection error.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnEpicsError:public Error
{
	public:
		ConnEpicsError (const char *_message, int _result): Error ()
		{
			setMsg (std::string (_message) + ca_message (_result));
		}
};


/**
 * Error class for channles.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnEpicsErrorChannel:public ConnEpicsError
{
	public:
		ConnEpicsErrorChannel (const char *_message, const char *_pvname, int _result)
		:ConnEpicsError (_message, _result)
		{
		 	setMsg (std::string (_message) + ", channel " + _pvname + " error " + ca_message (_result));
		}
};


/**
 * Interface between class and channel value.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class EpicsVal
{
	public:
		rts2core::Value *value;
		chid vchid;
		void *storage;

		EpicsVal (rts2core::Value *_value, chid _vchid)
		{
			value = _value;
			vchid = _vchid;
			storage = NULL;
		}

		~EpicsVal ()
		{
			if (storage != NULL)
				free (storage);
		}
};


/**
 * Connection for EPICS clients.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnEpics: public ConnNoSend
{
	private:
		std::list <EpicsVal> channels;

		EpicsVal * findValue (rts2core::Value *value);
	public:
		ConnEpics (rts2core::Block *_master);

		virtual ~ConnEpics ();

		virtual int init ();

		/**
		 * Associate Value with channel.
		 */
		void addValue (rts2core::Value *value, const char *pvname);

		void addValue (rts2core::Value *value, const char *prefix, const char *pvname)
		{
			addValue (value, (std::string (prefix) + std::string (pvname)).c_str ());
		}

		chid createChannel (const char *pvname);

		chid createChannel (const char *prefix, const char *pvname)
		{
			return createChannel ((std::string (prefix) + std::string (pvname)).c_str ());
		}

		/**
		 * Deletes channel.
		 */
		void deleteChannel (chid _ch);

		void get (chid _ch, double *val);

		/**
		 * This function request value update. You are responsible to
		 * call ConnEpics::callPendIO() to actually perform the
		 * request. Value will not be update before you call
		 * ConnEpics::callPendIO().
		 *
		 * @param value Value which is requested.
		 */
		void queueGetValue (rts2core::Value *value);

		/**
		 * Queue value for set operation.
		 */
		void queueSetValue (rts2core::Value *value);

		/**
		 * Set integer value.
		 */
		void queueSet (chid _vchid, int value);
		void queueSet (chid _vchid, double value);

		void queueSetEnum (chid _vchid, int value);

		void callPendIO ();
};

};

#endif /* !__RTS2_CONNEPICS__ */
