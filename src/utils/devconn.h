/*! 
 * @file Constants for data connection.
 *
 * @author petr
 */

#ifndef __RTS_DEVCONN__
#define __RTS_DEVCONN__

#define MAXMSG  		512
#define MAXDATACONS		4
//! Data port range
#define MINDATAPORT		5556
#define MAXDATAPORT		5656
#define DATA_BLOCK_SIZE		1000

#define STATUSNAME		8

//! holds status informations
struct devconn_status
{
  char name[STATUSNAME + 1];
  int status;
};

#endif // __RTS_DEVCONN__
