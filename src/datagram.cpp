/*
 *   FILE: datagram.cpp
 * AUTHOR: Ivan Marcin (ivmarcin@stanford.edu)
 *   DATE: April 14 23:59:59 PST 2014
 *  DESCR: Datagram abstraction - describe protocol datagram
 */

#include "main.h"
#include "datagram.h"
#include "constants.h"
#include <cstdlib>

using namespace std;

// statics
unsigned short Datagram::headerPlayerID;
unsigned int  Datagram::headerSequenceNumber;

// DEBUGGING
void log(char* str)
{
  if(LOGGING)
  {
    printf(str);
    printf("\n");
  }
}

void log_bytearr(uint8_t* buffer)
{

  int i = 0;
  for (i = 0; i < sizeof(buffer); i++)
  {
    printf("%X", buffer[i]);
    printf(":");
  }
  printf("\n");
}


/**
* Initializes an Instance of a generic datagram
**/
Datagram::Datagram()
{
  headerProtocol = PROTOCOL_HEADER;
  headerMessageType = 0x00;
  headerAckSequenceNumber = 0;
  isReliable = 0; //unreliable datagram by default

  // Assign a player id randomly if it hasn't been assigned yet.
  // TODO: fix player ID collisions.
  if (headerPlayerID == 0)
  {
      headerPlayerID = rand() % 65533;
  }

  // Create a sequencial sequence number
  headerSequenceNumber += 1;
  // Sequence number wrap around
  if (headerSequenceNumber > 65534)
  {
    headerSequenceNumber = 1;
  }
};


/**
* Returns the packet flags
* NOTE: can be extended to do bitmasking logic when protocol supports
* more than 1 flag.
**/
unsigned short Datagram::GetDatagramFlags()
{
  unsigned short value = 0;
  value &= (isReliable & MESSAGE_FLAG_RELIABILITY);
  return value;
};


/**
* Return the datagram header buffer - Packs all vaiable states in a buffer
**/
uint8_t* Datagram::BuildHeader()
{
  uint8_t* buffer = new uint8_t[DATAGRAM_SIZE_HEADER];

  //initialize buffer
	memset(buffer, 0, DATAGRAM_SIZE_HEADER);

  //Set values
  uint8_t flags = GetDatagramFlags();

  uint8_t proto = htons(headerProtocol);
  uint8_t seqNum = htons(headerSequenceNumber);
  uint8_t playID = htons(headerPlayerID);
  uint8_t ackNum = htons(headerAckSequenceNumber);

  memcpy(buffer     , &proto,              2); // Shows up as 254:202
  memcpy(buffer + 2 , &headerMessageType,  1);
  memcpy(buffer + 3 , &(flags),            1);
  memcpy(buffer + 4 , &seqNum,             4);
  memcpy(buffer + 8 , &playID,             4);
  memcpy(buffer + 16, &ackNum,             4);

  return buffer;
};


/**
* Returns the type of message
**/
uint8_t Datagram::GetType()
{
  return headerMessageType;
};

/**
* must be overriden by inherited classes
**/
uint8_t* Datagram::GetDatagram()
{
  return NULL;
};

void Datagram::SetDatagram(uint8_t messageType)
{
  headerMessageType = messageType;
};

void Datagram::SetDatagram(uint8_t* byteArray)
{
  //no-op
};

uint8_t* Datagram::InitDatagram(int size)
{
  headerSequenceNumber += 1;
  uint8_t* buffer = new uint8_t[size];
  uint8_t* header = BuildHeader();
  memset(buffer, 0, size);
  memcpy(buffer, &(header), DATAGRAM_SIZE_HEADER);

  delete header;
  return buffer;
}

Datagram* Datagram::ByteArrayToDatagram(uint8_t* data)
{
  Datagram* val = NULL;

  //first check the header for 0xCAFE
  if (!(data[0]==0xCA && data[1]==0xFE))
  {
    log("Not a 0xCAFE datagram");
    return val;
  }

  //check datagram Type
  switch (data[2])
  {
    case SYNC_REQ:
      log("SYNC_REQ received");
      val = new SyncRequest();
      break;
    case SYNC_ACK:
      log("SYNC_ACK received");
      val = new SyncResponse();
      break;
    case KEEPLIVE:
      log("KEEPALIVE received");
      val = new KeepAlive();
      break;
    case MOVE:
      log("MOVE received");
      val = new Move();
      break;
    case QUIT:
      log("QUIT received");
      val = new Leave();
      break;
    case KILL_REQ:
      log("KILL_REQ received");
      val = new KillRequest();
      break;
    case KILL_ACK:
      log("KILL_ACK received");
      val = new KillResponse();
      break;
    case KILL_DEN:
      log("KILL_DEN received");
      val = new KillDenied();
      break;
    default:
      log("UNDEFINED packet received");
      return val; //we don't want to set a datagram on a nil.
      break;
  }

  val->SetDatagram(data);
  return val;
}

/***********************************************************/
/***** MOVE *****/
/***********************************************************/

/**
* Initializes an instance of a packet containing a MOVE command
**/
Move::Move() : Datagram()
{
  headerMessageType = MOVE;
  isReliable        = false;
};

/**
* Build a move packet
* x = X coord
* y = Y coord
* d = facing direction
**/
void Move::SetDatagram(uint8_t x, uint8_t y, uint8_t d)
{
  _x = x;
  _y = y;
  _d = d;
};

/** Parse values from byte array - received from a datagram **/
void Move::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;

  memcpy(&(_x), bytes + position, 2); position += 2;
  memcpy(&(_y), bytes + position, 2); position += 2;
  memcpy(&(_d), bytes + position, 2); position += 2;

  _x = ntohs(_x);
  _y = ntohs(_x);
  _d = ntohs(_x);
}

/**
* Returns a Datagram of type MOVE
**/
uint8_t* Move::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_MOVE);

  uint16_t tx = htons(_x);
  uint16_t ty = htons(_y);
  uint16_t td = htons(_d);

  memcpy(buffer + DATAGRAM_SIZE_HEADER    , &(tx), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 2, &(ty), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 4, &(td), 2);

  return buffer;
};


/***********************************************************/
/***** SYNC REQUEST *****/
/***********************************************************/

/**
* Initializes an instance of a packet containing a SyncRequest command
**/
SyncRequest::SyncRequest() : Datagram()
{
  headerMessageType = SYNC_REQ;
  isReliable        = false;
};

/**
* Build a move packet
* x = X coord
* y = Y coord
* d = facing direction
**/
void SyncRequest::SetDatagram(uint8_t x, uint8_t y, uint8_t d, string name, uint16_t score)
{
  _x = x;
  _y = y;
  _d = d;
  _score = score;
  _playerName = name.length() > 15 ? name.substr(0,15) : name ;
};


/**
* Parse values from byte array - received from a datagram
**/
void SyncRequest::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;

  memcpy(&(_playerName), bytes + position ,15); position += 16;
  memcpy(&(_x)         , bytes + position , 2); position += 2;
  memcpy(&(_y)         , bytes + position , 2); position += 2;
  memcpy(&(_d)         , bytes + position , 2); position += 2;
  memcpy(&(_score)     , bytes + position , 2);

  _x = ntohs(_x);
  _y = ntohs(_x);
  _d = ntohs(_x);
  _score = ntohs(_score);
}

/**
* Returns a Datagram of type Sync_REQ
**/
uint8_t* SyncRequest::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_SYNC_REQ);

  uint16_t tx = htons(_x);
  uint16_t ty = htons(_y);
  uint16_t td = htons(_d);
  uint16_t tscore = htons(_score);

  memcpy(buffer + DATAGRAM_SIZE_HEADER     , &(_playerName), 15);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 16, &(tx), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 18, &(ty), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 20, &(td), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 22, &(tscore), 2);

  return buffer;
};

/***********************************************************/
/***** SYNC RESPONSE *****/
/***********************************************************/

/**
* Initializes an instance of a packet containing a Sync Ack command
**/
SyncResponse::SyncResponse() : Datagram()
{
  headerMessageType = SYNC_ACK;
  isReliable        = false;
};

/**
* Build a sync ack packet
**/
void SyncResponse::SetDatagram(uint8_t x, uint8_t y, uint8_t d, string name, uint16_t score)
{
  _x = x;
  _y = y;
  _d = d;
  _score = score;
  _playerName = name.length() > 15 ? name.substr(0,15) : name;
};

/**
* Parse values from byte array - received from a datagram
**/
void SyncResponse::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;

  memcpy(&(_playerName), bytes + position ,15); position += 16;
  memcpy(&(_x)         , bytes + position , 2); position += 2;
  memcpy(&(_y)         , bytes + position , 2); position += 2;
  memcpy(&(_d)         , bytes + position , 2); position += 2;
  memcpy(&(_score)     , bytes + position , 2);

  _x = ntohs(_x);
  _y = ntohs(_x);
  _d = ntohs(_x);
  _score = ntohs(_score);
}

/**
* Returns a Datagram of type Sync_ACK
**/
uint8_t* SyncResponse::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_SYNC_ACK);

  uint16_t tx = htons(_x);
  uint16_t ty = htons(_y);
  uint16_t td = htons(_d);
  uint16_t tscore = htons(_score);

  memcpy(buffer + DATAGRAM_SIZE_HEADER     , &(_playerName), 15);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 16, &(tx), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 18, &(ty), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 20, &(td), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 22, &(tscore), 2);

  return buffer;
};

/***********************************************************/
/***** KEEP ALIVE *****/
/***********************************************************/

/**
* Initializes an instance of a packet containing a SyncRequest command
**/
KeepAlive::KeepAlive() : Datagram()
{
  headerMessageType = KEEPLIVE;
  isReliable        = false;
};

/**
* Build a Keep Alive packet
**/
void KeepAlive::SetDatagram(uint16_t score)
{
  _score = score;
};


/**
* Parse values from byte array - received from a datagram
**/
void KeepAlive::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;

  memcpy(&(_score), bytes , 2);

  _score = ntohl(_score);
}


/**
* Returns a Datagram of type KeepAlive
**/
uint8_t* KeepAlive::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_KEEPALIVE);

  uint16_t tscore = htons(_score);

  memcpy(buffer + DATAGRAM_SIZE_HEADER , &(tscore), 2);

  return buffer;
};


/***********************************************************/
/***** LEAVE *****/
/***********************************************************/


/**
* Initializes an instance of a packet containing a leave command
**/
Leave::Leave() : Datagram()
{
  headerMessageType = QUIT;
  isReliable        = false;
};

/**
* Build a leave packet
**/
void Leave::SetDatagram(){};

/**
* Parse values from byte array - received from a datagram
**/
void Leave::SetDatagram(uint8_t* bytes)
{
 // no- op
}

/**
* Returns a Datagram of type KeepAlive
**/
uint8_t* Leave::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_HEADER);

  return buffer;
};


/***********************************************************/
/***** Kill Request *****/
/***********************************************************/


/**
* Initializes an instance of a packet containing a KillRequest command
**/
KillRequest::KillRequest() : Datagram()
{
  headerMessageType = KILL_REQ;
  isReliable        = true;
};

/**
* Build a KillReq packet
**/
void KillRequest::SetDatagram(uint8_t x, uint8_t y, uint16_t killedPlayerId)
{
  _x = x;
  _y = y;
  _killedPlayerId = killedPlayerId;
};

/**
* Parse values from byte array - received from a datagram
**/
void KillRequest::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;

  memcpy(&(_x)             , bytes + position, 2); position += 2;
  memcpy(&(_y)             , bytes + position, 2); position += 2;
  memcpy(&(_killedPlayerId), bytes + position, 4);

  _x = ntohs(_x);
  _y = ntohs(_x);
  _killedPlayerId = ntohl(_killedPlayerId);
}

/**
* Returns a Datagram of type KillReq
**/
uint8_t* KillRequest::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_KILLS);

  uint16_t tx = htons(_x);
  uint16_t ty = htons(_y);
  uint32_t tkilledId = htons(_killedPlayerId);

  memcpy(buffer + DATAGRAM_SIZE_HEADER    , &(tkilledId), 4);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 4, &(tx)       , 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 6, &(ty)       , 2);

  return buffer;
};

/***********************************************************/
/***** Kill Response *****/
/***********************************************************/


/**
* Initializes an instance of a packet containing a KillAck command
**/
KillResponse::KillResponse() : Datagram()
{
  headerMessageType = KILL_ACK;
  isReliable        = true;
};

/**
* Build a KillAck packet
**/
void KillResponse::SetDatagram(uint8_t x, uint8_t y, uint16_t killerPlayerId)
{
  _x = x;
  _y = y;
  _killerPlayerId = killerPlayerId;
};

/**
* Parse values from byte array - received from a datagram
**/
void KillResponse::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;

  memcpy(&(_x)             , bytes + position, 2); position += 2;
  memcpy(&(_y)             , bytes + position, 2); position += 2;
  memcpy(&(_killerPlayerId), bytes + position, 4);

  _x = ntohs(_x);
  _y = ntohs(_x);
  _killerPlayerId = ntohl(_killerPlayerId);
}

/**
* Returns a Datagram of type KillAck
**/
uint8_t* KillResponse::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_KILLS);

  uint16_t tx = htons(_x);
  uint16_t ty = htons(_y);
  uint32_t tkillerId = htons(_killerPlayerId);

  memcpy(buffer + DATAGRAM_SIZE_HEADER    , &(tkillerId), 4);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 4, &(tx)       , 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 6, &(ty)       , 2);

  return buffer;
};


/***********************************************************/
/***** Kill Denied *****/
/***********************************************************/

/**
* Initializes an instance of a packet containing a Kill Denied command
**/
KillDenied::KillDenied() : Datagram()
{
  headerMessageType = KILL_DEN;
  isReliable        = true;
};

/**
* Build a KillDenied packet
**/
void KillDenied::SetDatagram(uint8_t x, uint8_t y, uint16_t killerPlayerId)
{
  _x = x;
  _y = y;
  _killerPlayerId = killerPlayerId;
};

/**
* Parse values from byte array - received from a datagram
**/
void KillDenied::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;

  memcpy(&(_x)             , bytes + position, 2); position += 2;
  memcpy(&(_y)             , bytes + position, 2); position += 2;
  memcpy(&(_killerPlayerId), bytes + position, 4);

  _x = ntohs(_x);
  _y = ntohs(_x);
  _killerPlayerId = ntohl(_killerPlayerId);
}

/**
* Returns a Datagram of type KillAck
**/
uint8_t* KillDenied::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_KILLS);

  uint16_t tx = htons(_x);
  uint16_t ty = htons(_y);
  uint32_t tkillerId = htons(_killerPlayerId);

  memcpy(buffer + DATAGRAM_SIZE_HEADER    , &(tkillerId), 4);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 4, &(tx), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 6, &(ty), 2);

  return buffer;
};


