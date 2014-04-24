#ifndef _DATAGRAM_H_
#define _DATAGRAM_H_

#include <string>

using namespace std;

class Datagram
{

  public:
    Datagram();
    uint8_t GetType();
    virtual uint8_t* GetDatagram();
    virtual Datagram* ByteArrayToDatagram(uint8_t* array);

    unsigned int   sequenceNumber;
    unsigned int   playerID;
    unsigned int   headerAckSequenceNumber;

    //header
    uint8_t  headerMessageType;
    unsigned short headerProtocol;

    //packet flags
    bool isReliable;
    static unsigned short headerPlayerID;
    static unsigned int  headerSequenceNumber;

    uint8_t* BuildHeader();
    uint8_t* InitDatagram(int size);
    unsigned short GetDatagramFlags();
    virtual void SetHeaderFromDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint8_t MessageType);
    virtual void SetDatagram(uint8_t* fromByteArray);


};

/***********************************************************/

class Move : public Datagram
{
  public:
    Move();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint16_t x, uint16_t y, uint16_t d);
    virtual uint8_t* GetDatagram();
    uint16_t _x;
    uint16_t _y;
    uint16_t _d;
};

/***********************************************************/

class SyncRequest : public Datagram
{
  public:
    SyncRequest();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint16_t x, uint16_t y, uint16_t d, char* name, int score);
    virtual uint8_t* GetDatagram();
    uint16_t _x;
    uint16_t _y;
    uint16_t _d;
    int _score;
    char* _playerName;
};

/***********************************************************/

class SyncResponse : public Datagram
{
  public:
    SyncResponse();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint16_t x, uint16_t y, uint16_t d, char* name, int score);
    virtual uint8_t* GetDatagram();
    uint16_t _x;
    uint16_t _y;
    uint16_t _d;
    int _score;
    char* _playerName;
};

/***********************************************************/

class KeepAlive : public Datagram
{
  public:
    KeepAlive();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(int score);
    virtual uint8_t* GetDatagram();
    int _score;
};

/***********************************************************/

class Leave : public Datagram
{
  public:
    Leave();
    virtual uint8_t* GetDatagram();
    virtual void SetDatagram(uint8_t* fromByteArray);

  private:
    virtual void SetDatagram();
};

/***********************************************************/

class KillRequest : public Datagram
{
  public:
    KillRequest();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint16_t x, uint16_t y, uint32_t killedPlayerId);
    virtual uint8_t* GetDatagram();
    uint16_t _x;
    uint16_t _y;
    uint32_t _killedPlayerId;
};

class KillResponse : public Datagram
{
  public:
    KillResponse();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint16_t x, uint16_t y, uint32_t killerPlayerId);
    virtual uint8_t* GetDatagram();
    uint16_t _x;
    uint16_t _y;
    uint32_t _killerPlayerId;

};

/***********************************************************/

class KillDenied : public Datagram
{
  public:
    KillDenied();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint16_t x, uint16_t y, uint32_t _killerPlayerId);
    virtual uint8_t* GetDatagram();
    uint16_t _x;
    uint16_t _y;
    uint32_t _killerPlayerId;
};

/***********************************************************/

class KillAck : public Datagram
{
  public:
    KillAck();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint32_t _killerPlayerId, unsigned int sequence);
    virtual uint8_t* GetDatagram();
    uint32_t _killerPlayerId;
};
#endif
