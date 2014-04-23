#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

/** Log all communication **/
#define LOGGING                   1
/***************************/


/** Datagram constants**/
#define PROTOCOL_HEADER           51966 // 0xCAFE
#define MESSAGE_FLAG_RELIABILITY  0x1

/** Message Types **/
#define REL_ACK                   0x00
#define SYNC_REQ                  0x01
#define SYNC_ACK                  0x02
#define KILL_REQ                  0x30
#define KILL_ACK                  0x31
#define KILL_DEN                  0x32
#define KEEPLIVE                  0x40
#define MOVE                      0x20
#define QUIT                      0x10

/** Move Directions **/
#define DIRECTION_UP              0x01
#define DIRECTION_DOWN            0x02
#define DIRECTION_LEFT            0x03
#define DIRECTION_RIGHT           0x04

/** Game Values **/
#define MAX_NAME_LENGTH           16
#define FIRE_REFRESH_MILLIS       100
#define MISSILE_SPEED_MILLIS      50
#define DATAGRAM_SIZE_HEADER      16
#define DATAGRAM_SIZE_MOVE        22
#define DATAGRAM_SIZE_SYNC_REQ    40
#define DATAGRAM_SIZE_SYNC_ACK    40
#define DATAGRAM_SIZE_KEEPALIVE   18
#define DATAGRAM_SIZE_KILLS       24
#define RAT_ID_NOT_FOUND          999
#define SCORE_OFFSET              32768

#endif
