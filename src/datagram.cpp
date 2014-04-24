/*
 *   FILE: datagram.cpp
 * AUTHOR: Ivan Marcin (ivmarcin@stanford.edu)
 *   DATE: April 14 23:59:59 PST 2014
 *  DESCR: Datagram abstraction - describe protocol datagram
 */

#include <stdio.h>
#include <time.h>
#include <cstdlib>
#include "main.h"
#include "datagram.h"
#include "constants.h"
#include "mazewar.h"

using namespace std;

// statics
unsigned short Datagram::headerPlayerID;
unsigned int  Datagram::headerSequenceNumber;

// DEBUGGING
void log(char* str)
{
  if(LOGGING)
  {
    printf("\n %s \n", str);
  }
}

/**
* Pretty print an incoming datagram
**/
void log_bytearr(uint8_t* buffer, int size)
{
  if(LOGGING)
  {
    int i = 0;
    printf("________________________\n");
    for (i = 0; i < size; i++)
    {
      // split datagram in 8 byte chunks
      // so prints look like in the protocol doc
      if(i % 8 == 0)
      {
        printf("\n");
      }
      printf("%2X", buffer[i]);
      printf(" ");
    }
    printf("\n________________________\n\n\n");
  }
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

  // Assign a player id randomly for the first time it hasn't been assigned yet.
  // TODO: fix player ID collisions.
  if (headerPlayerID == 0)
  {
    srand(time(NULL));
    headerPlayerID = rand() % 65500;
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
  //unsigned short value = 0;
  //value &= (isReliable & MESSAGE_FLAG_RELIABILITY);
  return isReliable;
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

  uint16_t proto = htons(headerProtocol);
  uint32_t playID = htonl(headerPlayerID);
  uint32_t seqNum = htonl(headerSequenceNumber);
  uint32_t ackNum = htonl(headerAckSequenceNumber);

  memcpy(buffer     , &proto,              2); // Shows up as 254:202
  memcpy(buffer + 2 , &headerMessageType,  1);
  memcpy(buffer + 3 , &(flags),            1);
  memcpy(buffer + 4 , &playID,             4);
  memcpy(buffer + 8 , &seqNum,             4);
  memcpy(buffer + 16, &ackNum,             4);

  log("----- Datagram::BuildHeader -----");
  cout << "headerMessageType: " << headerMessageType << " - SequenceNumber: " << headerSequenceNumber << " - PlayerId: " << headerPlayerID << " - headerAckSequenceNumber: " << headerAckSequenceNumber << "\n" ;
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
  printf("Datagram::GetDatagram");
  return BuildHeader();
};

/**
* Sets the type of datagram
**/
void Datagram::SetDatagram(uint8_t messageType)
{
  headerMessageType = messageType;
};

void Datagram::SetDatagram(uint8_t* byteArray)
{
  //no-op
};

/**
* Parses the headers fields from an incoming byte array
**/
void Datagram::SetHeaderFromDatagram(uint8_t* bytes)
{
  //byte 0 has proto version

  //byte 2 has type
  headerMessageType = bytes[2];

  //byte 3 starts with reliability flag
  int position = 3;

  memcpy(&(isReliable), bytes + position, 1); position += 1;
  memcpy(&(playerID), bytes + position, 4); position += 4;
  memcpy(&(sequenceNumber), bytes + position, 4); position += 4;
  memcpy(&(headerAckSequenceNumber), bytes + position, 4); position += 4;

  playerID          = ntohl(playerID);
  sequenceNumber    = ntohl(sequenceNumber);
  headerAckSequenceNumber = ntohl(headerAckSequenceNumber);
}

/**
* initialized a new instance of a Datagram
**/
uint8_t* Datagram::InitDatagram(int size)
{
  headerSequenceNumber += 1;
  uint8_t* buffer = new uint8_t[size];
  memset(buffer, 0, size);

  uint8_t* header = BuildHeader();
  memcpy(buffer, header, DATAGRAM_SIZE_HEADER);

  //log(" --- InitDatagram Header --- ");
  //log_bytearr(header, DATAGRAM_SIZE_HEADER);
  delete header;

  //log(" --- InitDatagram Buffer --- ");
  //log_bytearr(buffer, size);
  return buffer;
}

/**
* Parses an incoming packet as byte arrays into a
* a corresponding Datagram object
* It casts itself into the correct Object type
**/
Datagram* Datagram::ByteArrayToDatagram(uint8_t* data)
{
  Datagram* val = NULL;

  //first check the header for 0xCAFE
  if (!(data[0]==0xCA && data[1]==0xFE))
  {
    log("Not a 0xCAFE datagram");
    val = new Datagram(); //no-op datagram
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
      log("KILL_RES received");
      val = new KillResponse();
      break;
    case KILL_DEN:
      log("KILL_DEN received");
      val = new KillDenied();
      break;
    case REL_ACK:
      log("KILL_ACK received");
      val = new KillAck();
      break;
    default:
      log("UNDEFINED packet received");
      return val; //we don't want to set a datagram on a nil.
      break;
  }

  log_bytearr(data, 40);
  val->SetHeaderFromDatagram(data);
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
void Move::SetDatagram(uint16_t x, uint16_t y, uint16_t d)
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
  _y = ntohs(_y);
  _d = ntohs(_d);
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

  log("----- Move::GetDatagram buffer -----");
  log_bytearr(buffer, DATAGRAM_SIZE_MOVE);

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
void SyncRequest::SetDatagram(uint16_t x, uint16_t y, uint16_t d, char* name, int score)
{
  _x = x;
  _y = y;
  _d = d;
  _score = score;
  _playerName = name;
};

/**
* Parse values from byte array - received from a datagram
**/
void SyncRequest::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;

  //initialize the name array
  int score = 0;
  _playerName = new char[MAX_NAME_LENGTH];
  memset(_playerName, 0, MAX_NAME_LENGTH);

  memcpy(_playerName   , bytes + position ,16); position += 16;
  memcpy(&(_x)         , bytes + position , 2); position += 2;
  memcpy(&(_y)         , bytes + position , 2); position += 2;
  memcpy(&(_d)         , bytes + position , 2); position += 2;
  memcpy(&(score)     , bytes + position , 2);

  _x = ntohs(_x);
  _y = ntohs(_y);
  _d = ntohs(_d);
  _score = ntohs(score) - SCORE_OFFSET;

  log("----- SyncRequest::SetDatagram buffer -----");
  log_bytearr(bytes, DATAGRAM_SIZE_SYNC_REQ);
  cout << "SyncRequest Parsed from bytes\n" << "PlayerName " << _playerName << "- X:" << _x << "- Y:" << _y << "- D:" << _d << "- Score:"<< _score;
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
  uint16_t tscore = htons(_score + SCORE_OFFSET);

  memcpy(buffer + DATAGRAM_SIZE_HEADER     , _playerName, 16);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 16, &(tx), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 18, &(ty), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 20, &(td), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 22, &(tscore), 2);

  cout << "SyncRequest -- PlayerName:" << _playerName << " - score:" << _score;
  log("----- SyncRequest::GetDatagram buffer -----");
  log_bytearr(buffer, DATAGRAM_SIZE_SYNC_REQ);
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
void SyncResponse::SetDatagram(uint16_t x, uint16_t y, uint16_t d, char* name, int score)
{
  _x = x;
  _y = y;
  _d = d;
  _score = score;
  _playerName = name;
};

/**
* Parse values from byte array - received from a datagram
**/
void SyncResponse::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;
  int score;

   //initialize the name array
  _playerName = new char[MAX_NAME_LENGTH];
  memset(_playerName, 0, MAX_NAME_LENGTH);

  memcpy(_playerName   , bytes + position ,16); position += 16;
  memcpy(&(_x)         , bytes + position , 2); position += 2;
  memcpy(&(_y)         , bytes + position , 2); position += 2;
  memcpy(&(_d)         , bytes + position , 2); position += 2;
  memcpy(&(score)     , bytes + position , 2);

  _x = ntohs(_x);
  _y = ntohs(_y);
  _d = ntohs(_d);
  _score = ntohs(score) - SCORE_OFFSET;

  cout << "SyncResponse: " << " PlayerName:" << _playerName << "- X:" << _x << "- Y:" << _y << "- D:" << _d << " -Score: " << _score;
  log("----- SyncResponse::SetDatagram buffer -----");
  log_bytearr(bytes, DATAGRAM_SIZE_SYNC_ACK);
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
  uint16_t tscore = htons(_score + SCORE_OFFSET);

  memcpy(buffer + DATAGRAM_SIZE_HEADER     , _playerName, 15);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 16, &(tx), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 18, &(ty), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 20, &(td), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 22, &(tscore), 2);

  log("----- SyncResponse::GetDatagram buffer -----");
  log_bytearr(buffer, DATAGRAM_SIZE_SYNC_ACK);
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
void KeepAlive::SetDatagram(int score)
{
  _score = score;
};


/**
* Parse values from byte array - received from a datagram
**/
void KeepAlive::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;
  int tscore = 0;
  memcpy(&(tscore), bytes + position , 2);
  _score = ntohs(tscore) - SCORE_OFFSET;
}


/**
* Returns a Datagram of type KeepAlive
**/
uint8_t* KeepAlive::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_KEEPALIVE);
  uint16_t tscore = (_score + SCORE_OFFSET);
  tscore = htons(tscore);
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
void Leave::SetDatagram(uint8_t* bytes){}

/**
* Returns a Datagram of type KeepAlive
**/
uint8_t* Leave::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_HEADER);

  log("----- LEAVE::GetDatagram buffer -----");
  return NULL;
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
void KillRequest::SetDatagram(uint16_t x, uint16_t y, uint32_t killedPlayerId)
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

  memcpy(&(_killedPlayerId), bytes + position, 4); position += 4;
  memcpy(&(_x)             , bytes + position, 2); position += 2;
  memcpy(&(_y)             , bytes + position, 2);

  _x = ntohs(_x);
  _y = ntohs(_y);
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
  uint32_t tkilledId = htonl(_killedPlayerId);

  memcpy(buffer + DATAGRAM_SIZE_HEADER    , &(tkilledId), 4);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 4, &(tx)       , 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 6, &(ty)       , 2);

  log("----- KillRequest::GetDatagram buffer -----");
  log_bytearr(buffer, DATAGRAM_SIZE_KILLS);
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
void KillResponse::SetDatagram(uint16_t x, uint16_t y, uint32_t killerPlayerId)
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

  memcpy(&(_killerPlayerId), bytes + position, 4); position += 4;
  memcpy(&(_x)             , bytes + position, 2); position += 2;
  memcpy(&(_y)             , bytes + position, 2);

  _x = ntohs(_x);
  _y = ntohs(_y);
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
  uint32_t tkillerId = htonl(_killerPlayerId);

  memcpy(buffer + DATAGRAM_SIZE_HEADER    , &(tkillerId), 4);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 4, &(tx)       , 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 6, &(ty)       , 2);

  log("----- KillResponse::GetDatagram buffer -----");
  log_bytearr(buffer, DATAGRAM_SIZE_KILLS);

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
void KillDenied::SetDatagram(uint16_t x, uint16_t y, uint32_t killerPlayerId)
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

  memcpy(&(_killerPlayerId), bytes + position, 4); position += 4;
  memcpy(&(_x)             , bytes + position, 2); position += 2;
  memcpy(&(_y)             , bytes + position, 2);

  _x = ntohs(_x);
  _y = ntohs(_y);
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
  uint32_t tkillerId = htonl(_killerPlayerId);

  memcpy(buffer + DATAGRAM_SIZE_HEADER    , &(tkillerId), 4);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 4, &(tx), 2);
  memcpy(buffer + DATAGRAM_SIZE_HEADER + 6, &(ty), 2);

  log("----- KillDenied::GetDatagram buffer -----");
  log_bytearr(buffer, DATAGRAM_SIZE_KILLS);
  return buffer;
};


/***********************************************************/
/***** Kill Ack *****/
/***********************************************************/

/**
* Initializes an instance of a packet containing a Kill Denied command
**/
KillAck::KillAck() : Datagram()
{
  headerMessageType = REL_ACK;
  isReliable        = false;
};

/**
* Build a KillDenied packet
**/
void KillAck::SetDatagram(uint32_t killerPlayerId, uint32_t sequenceNum)
{
  _killerPlayerId = killerPlayerId;
  headerAckSequenceNumber = sequenceNum;
};

/**
* Parse values from byte array - received from a datagram
**/
void KillAck::SetDatagram(uint8_t* bytes)
{
  int position = DATAGRAM_SIZE_HEADER;
  memcpy(&(_killerPlayerId), bytes + position, 4); position += 4;
  _killerPlayerId = ntohl(_killerPlayerId);
}

/**
* Returns a Datagram of type KillAck
**/
uint8_t* KillAck::GetDatagram()
{
  uint8_t* buffer = InitDatagram(DATAGRAM_SIZE_KILLS);
  uint32_t tkillerId = htonl(_killerPlayerId);
  memcpy(buffer + DATAGRAM_SIZE_HEADER    , &(tkillerId), 4);
  return buffer;
};


