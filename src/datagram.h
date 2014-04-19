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
  protected:
    //header
    uint8_t  headerMessageType;
    unsigned short headerProtocol;
    unsigned int   headerAckSequenceNumber;

    //packet flags
    bool isReliable;

    static unsigned short headerPlayerID;
    static unsigned int  headerSequenceNumber;

    uint8_t* BuildHeader();
    uint8_t* InitDatagram(int size);
    unsigned short GetDatagramFlags();
    virtual void SetDatagram(uint8_t MessageType);
    virtual void SetDatagram(uint8_t* fromByteArray);

};

/***********************************************************/

class Move : public Datagram
{
  public:
    Move();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint8_t x, uint8_t y, uint8_t d);
    virtual uint8_t* GetDatagram();

  private:
    uint8_t _x;
    uint8_t _y;
    uint8_t _d;
};

/***********************************************************/

class SyncRequest : public Datagram
{
  public:
    SyncRequest();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint8_t x, uint8_t y, uint8_t d, string name, uint16_t score);
    virtual uint8_t* GetDatagram();

  private:
    uint8_t _x;
    uint8_t _y;
    uint8_t _d;
    uint16_t _score;
    string _playerName;
};

/***********************************************************/

class SyncResponse : public Datagram
{
  public:
    SyncResponse();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint8_t x, uint8_t y, uint8_t d, string name, uint16_t score);
    virtual uint8_t* GetDatagram();

  private:
    uint8_t _x;
    uint8_t _y;
    uint8_t _d;
    uint16_t _score;
    string _playerName;
};

/***********************************************************/

class KeepAlive : public Datagram
{
  public:
    KeepAlive();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint16_t score);
    virtual uint8_t* GetDatagram();

  private:
    uint16_t _score;
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
    virtual void SetDatagram(uint8_t x, uint8_t y, uint16_t killedPlayerId);
    virtual uint8_t* GetDatagram();

  private:
    uint8_t _x;
    uint8_t _y;
    uint8_t _killedPlayerId;
};

class KillResponse : public Datagram
{
  public:
    KillResponse();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint8_t x, uint8_t y, uint16_t killerPlayerId);
    virtual uint8_t* GetDatagram();

  private:
    uint8_t _x;
    uint8_t _y;
    uint8_t _killerPlayerId;

};

/***********************************************************/

class KillDenied : public Datagram
{
  public:
    KillDenied();
    virtual void SetDatagram(uint8_t* fromByteArray);
    virtual void SetDatagram(uint8_t x, uint8_t y, uint16_t _killerPlayerId);
    virtual uint8_t* GetDatagram();

  private:
    uint8_t _x;
    uint8_t _y;
    uint8_t _killerPlayerId;
};
#endif
