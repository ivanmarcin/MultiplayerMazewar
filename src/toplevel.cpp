/*
 *   FILE: toplevel.c
 * AUTHOR: Ivan Marcin (ivmarcin@stanford.edu)
 *   DATE: April 14 23:59:59 PST 2014
 *  DESCR: Mazewar multiplayer implementation.
 */

#define DEBUG

#include "main.h"
#include <string>
#include "mazewar.h"
#include "constants.h"
#include "datagram.h"

static bool		updateView;	/* true if update needed */

// Allow Only 1 live missile per player
static bool       missileIsLive; /*There's a live missile being shot*/
static Loc        missileX = 0;
static Loc        missileY = 0;
static Direction  missileDir = 0;

static int keepAliveCounter; /* keepalive cloock */
MazewarInstance::Ptr M;

/* Use this socket address to send packets to the multi-cast group. */
static Sockaddr         groupAddr;
#define MAX_OTHER_RATS  (MAX_RATS - 1)


int main(int argc, char *argv[])
{
    Loc x(1);
    Loc y(5);
    Direction dir(0);
    char *ratName;
    keepAliveCounter = 0;

    signal(SIGHUP, quit);
    signal(SIGINT, quit);
    signal(SIGTERM, quit);

    getName("Welcome to MazeWar!\n\n Your Name", &ratName);
    ratName[strlen(ratName)-1] = 0;

    M = MazewarInstance::mazewarInstanceNew(string(ratName));
    MazewarInstance* a = M.ptr();
    strncpy(M->myName_, ratName, NAMESIZE);

    free(ratName);

    MazeInit(argc, argv);

    NewPosition(M);


    /* So you can see what a Rat is supposed to look like, we create
    one rat in the single player mode Mazewar.
    It doesn't move, you can't shoot it, you can just walk around it */

    play();

    return 0;
}


/* ----------------------------------------------------------------------- */

void
play(void)
{
	MWEvent		event;
	MW244BPacket	incoming;

	event.eventDetail = &incoming;

	while (TRUE) {
		NextEvent(&event, M->theSocket());



		if (!M->peeking())
			switch(event.eventType) {
			case EVENT_A:
				aboutFace();
				break;

			case EVENT_S:
				leftTurn();
				break;

			case EVENT_D:
				forward();
				break;

			case EVENT_F:
				rightTurn();
				break;

			case EVENT_BAR:
				backward();
				break;

			case EVENT_LEFT_D:
				peekLeft();
				break;

			case EVENT_MIDDLE_D:
				shoot();
				break;

			case EVENT_RIGHT_D:
				peekRight();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;

			case EVENT_INT:
				quit(0);
				break;

			}
		else
			switch (event.eventType) {
			case EVENT_RIGHT_U:
			case EVENT_LEFT_U:
				peekStop();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;
			}

		ratStates();		/* clean house */

		manageMissiles();

    NetSendKeepAlive();

		DoViewUpdate();

    MoveMissile();

		/* Any info to send over network? */
	}
}

/* ----------------------------------------------------------------------- */

static	Direction	_aboutFace[NDIRECTION] ={SOUTH, NORTH, WEST, EAST};
static	Direction	_leftTurn[NDIRECTION] =	{WEST, EAST, NORTH, SOUTH};
static	Direction	_rightTurn[NDIRECTION] ={EAST, WEST, SOUTH, NORTH};

void
aboutFace(void)
{
	M->dirIs(_aboutFace[MY_DIR]);
	NetUpdateDirection(999, 999);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
leftTurn(void)
{
	M->dirIs(_leftTurn[MY_DIR]);
	NetUpdateDirection(999, 999);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
rightTurn(void)
{
	M->dirIs(_rightTurn[MY_DIR]);
	NetUpdateDirection(999, 999);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

/* remember ... "North" is to the right ... positive X motion */

void
forward(void)
{
  int lastX = MY_X_LOC;
  int lastY = MY_Y_LOC;

	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case SOUTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case EAST:	if (!M->maze_[tx][ty+1])	ty++; break;
	case WEST:	if (!M->maze_[tx][ty-1])	ty--; break;
	default:
		MWError("bad direction in Forward");
	}
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		NetUpdateDirection(lastX, lastY);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void backward()
{
  int lastX = MY_X_LOC;
  int lastY = MY_Y_LOC;

	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case SOUTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case EAST:	if (!M->maze_[tx][ty-1])	ty--; break;
	case WEST:	if (!M->maze_[tx][ty+1])	ty++; break;
	default:
		MWError("bad direction in Backward");
	}
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		NetUpdateDirection(lastX, lastY);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekLeft()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(WEST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(EAST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(NORTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	default:
			MWError("bad direction in PeekLeft");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekRight()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(EAST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(WEST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(NORTH);
			}
			break;

	default:
			MWError("bad direction in PeekRight");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekStop()
{
	M->peekingIs(FALSE);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void MoveMissile()
{
  Loc oldX = missileX;
  Loc oldY = missileY;
  bool oldMissileState = missileIsLive;

  if(missileIsLive)
  {
    switch(missileDir.value())
    {
    case NORTH:
        if (!M->maze_[missileX.value() + 1][missileY.value()])
        {
          missileX = missileX.value() + 1;
        }
        break;
    case SOUTH:
        if (!M->maze_[missileX.value() - 1][missileY.value()])
        {
          missileX = missileX.value() - 1;
        }
        break;
    case EAST:
        if (!M->maze_[missileX.value()][missileY.value() + 1])
        {
          missileY = missileY.value() + 1;
        }
        break;
    case WEST:
        if (!M->maze_[missileX.value()][missileY.value() - 1])
        {
          missileY = missileY.value() - 1;
        }
        break;
    }

    //Missile moved, now check if we hit a rat
    int ratHit = RatOccupiesXY(missileX.value(), missileY.value(), 0);

    if (ratHit)
    {
      NetSendKillRequest(ratHit);
      DissappearRat(ratHit);
    }
  }

  // if missile didn't move...It hit something! remove.
  if ( oldY.value() == missileY.value() && oldX.value() == missileX.value())
  {
    missileIsLive = false;
  }

  // Draw
  if(missileIsLive)
  {
    showMissile(missileX, missileY, missileDir, oldX, oldY, true);
  }else
  {
    // clear the position bu only if a missle exploded to avoid flickering
    // when own player is on top of missile
    if(missileIsLive != oldMissileState)
    {
      clearSquare(oldX, oldY);
    }
  }
}

/**
* If we shoot - jst check there's only 1 live missle per game
* refresh rate is player can shoot as soon as old missle is destroyed
**/
void shoot()
{
	M->scoreIs( M->score().value()-1 );
	UpdateScoreCard(M->myRatId().value());

  if(!missileIsLive)
  {
    missileIsLive = true;
    missileX = MY_X_LOC;
    missileY = MY_Y_LOC;
    missileDir = MY_DIR;
  }

  MoveMissile();
  updateView = TRUE;
}



/* ----------------------------------------------------------------------- */

/*
 * Exit from game, clean up window
 */

void quit(int sig)
{
	Leave* lv = new Leave();
	sendPacket(lv);
	delete lv;

	StopWindow();
	exit(0);
}


/**
* Get the current time in milliseconds
**/
double CurrentTimeInMillis()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double millis =(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
  return millis;
}


/* ----------------------------------------------------------------------- */

void NewPosition(MazewarInstance::Ptr m)
{
	Loc newX(0);
	Loc newY(0);
	Direction dir(0); /* start on occupied square */

	while (M->maze_[newX.value()][newY.value()]) {
	  /* MAZE[XY]MAX is a power of 2 */

	  /* In real game, also check that square is
       unoccupied by another rat */
    do
    {
      newX = Loc(random() & (MAZEXMAX - 1));
      newY = Loc(random() & (MAZEYMAX - 1));
    } while (RatOccupiesXY(newX, newY, 0));

	}

	/* prevent a blank wall at first glimpse */

	if (!m->maze_[(newX.value())+1][(newY.value())]) dir = Direction(NORTH);
	if (!m->maze_[(newX.value())-1][(newY.value())]) dir = Direction(SOUTH);
	if (!m->maze_[(newX.value())][(newY.value())+1]) dir = Direction(EAST);
	if (!m->maze_[(newX.value())][(newY.value())-1]) dir = Direction(WEST);

	m->xlocIs(newX);
	m->ylocIs(newY);
	m->dirIs(dir);
	NetUpdateDirection(999, 999);
  updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void MWError(char *s)

{
	StopWindow();
	fprintf(stderr, "CS244BMazeWar: %s\n", s);
	perror("CS244BMazeWar");
	//exit(-1);
}

/* ----------------------------------------------------------------------- */

/* Get the score of a rat */
Score GetRatScore(RatIndexType ratId)
{
  if (ratId.value() == M->myRatId().value())
  {
    return(M->score());
  }
  else
  {
    return (Score(M->rat(ratId).score));
  }
}

/* ----------------------------------------------------------------------- */

/**
* Return the Rat name set from the protocol
**/
char *GetRatName(RatIndexType ratId)
{
  if (ratId.value() == M->myRatId().value())
  {
    return M->myName_;
  }
  else
  {
    return (M->rat(ratId).name);
  }
}


/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void ratStates()
{
  /* In our sample version, we don't know about the state of any rats over
     the net, so this is a no-op */
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void manageMissiles()
{
  /* Leave this up to you. */
  // SEE: Shoot and movemissile.
}

/* ----------------------------------------------------------------------- */

void DoViewUpdate()
{
	if (updateView) {	/* paint the screen */
		ShowPosition(MY_X_LOC, MY_Y_LOC, MY_DIR);
		if (M->peeking())
			ShowView(M->xPeek(), M->yPeek(), M->dirPeek());
		else
			ShowView(MY_X_LOC, MY_Y_LOC, MY_DIR);
		updateView = FALSE;
	}
}

/* ----------------------------------------------------------------------- */

/*
 * Sample code to send a packet to a specific destination
 */

/**
* Send a UDP packet
**/
void sendPacket(Datagram* data)
{
  //TODO: drop random packets
  //TODO: mess with packet order
  uint8_t* dt = data->GetDatagram();

  if (sendto((int)M->theSocket(), dt, 100, 0,
               (sockaddr *) &groupAddr, sizeof(Sockaddr)) < 0) {
        MWError("Error Sending Packet to the Network");
    }

  delete dt;
}

// This is a hack
// We move the rat to wall to flash it out of the screen when shot
// this provides instant gratification to the shooter despite network lag.
void DissappearRat(int ratHit)
{
  M->rat(ratHit).x = 0;
  M->rat(ratHit).y = 0;
  updateView = TRUE;
}

/**
* Respawn a player by moving to a random position
* send the new pos to the network
* NOTE: NewPosition is altered to check for colliding rats
**/
void Respawn()
{
  NewPosition(M);
  NetUpdateDirection(999, 999);
  updateView = TRUE;
}

/** Send a kill request to the network
*
**/
void NetSendKillRequest(int ratId)
{
  KillRequest* kr = new KillRequest();

  kr->SetDatagram(M->rat(ratId).x.value(), M->rat(ratId).y.value(), M->rat(ratId).netPlayerId);
  sendPacket(kr);
}

/**
* Process a Kill Request received.
**/
void NetReceiveKillRequest(uint8_t* payload)
{
  Datagram *dg = new Datagram();
  KillRequest* kr = dynamic_cast<KillRequest*>(dg->ByteArrayToDatagram(payload));

  // if this packet was destineed to me:
  if (kr->_killedPlayerId == M->rat(0).netPlayerId)
  {
    cout << "Kill request to me at: X: " << kr->_x << "  ,y: " << kr->_y << "  when I'm at X: " << MY_X_LOC << " , y: " << MY_Y_LOC << "\n";
    if(kr->_x == MY_X_LOC && kr->_y == MY_Y_LOC)
    {
      // loose 5 points
      M->scoreIs( M->score().value()-5);
      Respawn();
      NetSendKillAck(payload);
      updateView = TRUE;
      UpdateScoreCard(M->myRatId().value());
    }else
    {
      NetSendKillDenied(payload);
    }
  }
}

void NetReceiveKillAck(uint8_t* payload)
{
  Datagram *dg = new Datagram();
  KillResponse* kr = dynamic_cast<KillResponse*>(dg->ByteArrayToDatagram(payload));

  // if this packet was destineed to me:
  if (kr->_killerPlayerId == M->rat(0).netPlayerId)
  {
    // 5 points of the kill, plus restoring the point lost from shooting a missile.
    M->scoreIs( M->score().value()+11);
    updateView = TRUE;
    UpdateScoreCard(M->myRatId().value());
    //send ack
    //sendPacket(dg);
  }
}

void NetReceiveKillDenied(uint8_t* payload)
{
  Datagram *dg = new Datagram();
  KillDenied* kd = dynamic_cast<KillDenied*>(kd->ByteArrayToDatagram(payload));
  if (kd->_killerPlayerId == M->rat(0).netPlayerId)
  {
    //send ack
    //sendPacket(dg);
  }
}

void NetSendKillAck(uint8_t* payload)
{
  Datagram *dg = new Datagram();
  KillRequest* kr = dynamic_cast<KillRequest*>(dg->ByteArrayToDatagram(payload));
  KillResponse* kd = new KillResponse();
  kd->SetDatagram(kr->_x, kr->_y, kr->playerID);
  sendPacket(kd);
}


void NetSendKillDenied(uint8_t* payload)
{
  Datagram *dg = new Datagram();
  KillRequest* kr = dynamic_cast<KillRequest*>(dg->ByteArrayToDatagram(payload));
  KillDenied* kd = new KillDenied();

  kd->SetDatagram(kr->_x, kr->_y, kr->playerID);
  sendPacket(kd);
}

/**
* Send a network update with the corresponding change of direction.
* backtracks the rat if the move send conflicts with another rat
**/
void NetUpdateDirection(int lastX, int lastY)
{
  if (RatOccupiesXY(MY_X_LOC, MY_Y_LOC, 0))
  {
      M->xlocIs(Loc(lastX));
      M->ylocIs(Loc(lastY));
      return;
  }

	Move* mov = new Move();
	mov->SetDatagram(MY_X_LOC, MY_Y_LOC, MY_DIR);
	sendPacket(mov);
}

/**
* Per protocol:
* if a score exceeds 9999 on either directions
* cap score to prevent overflows.
**/
void CapScore()
{
  if ( M->score().value() > 9999)
    {
      M->scoreIs(9999);
    }
    if (M->score().value() < -9999)
    {
      M->scoreIs(-9999);
    }
}

/**
* Send a network keep alive every X seconds
* Per protocol specification:
* Offset the score to represent negative values.
* 0 is midpoint of an unsigned short.
**/
void NetSendKeepAlive()
{
  CapScore();

	if (keepAliveCounter < 1)
	{
		keepAliveCounter = 500;
		KeepAlive* ka = new KeepAlive();
		ka->SetDatagram( (M->score().value() + SCORE_OFFSET) );
		//log("Sending out KeepAlive");
    //cout << "Sending with keepalive SCORE offsetted :" << M->score().value() + SCORE_OFFSET << " and Score in datagram:" << ka->_score;
		sendPacket(ka);
		delete ka;
	}
  keepAliveCounter--;
}


//Update a player stats from an ack
void UpdateOrRegisterPlayerFromSyncAck(uint8_t* payload)
{
  Datagram *dg = new Datagram();
  SyncResponse* sr = dynamic_cast<SyncResponse*>(dg->ByteArrayToDatagram(payload));
  SyncRequest* tmp = new SyncRequest();

  tmp->_playerName = sr->_playerName;
  tmp->playerID = sr->playerID;
  tmp->_score = tmp->_score;
  tmp->_x = sr->_x;
  tmp->_y = sr->_y;
  tmp->_d = sr->_d;

  RegisterPlayer( tmp );

  delete tmp;
  delete sr;
}

/**
* Register a new rat in the game
* based on the data received from a sync
* Datagram
**/
void RegisterPlayer(SyncRequest* sr)
{
  short unsigned int freeRatId = RatIdFromPlayerId(sr->playerID);
  log("Registering New Player");

  if(freeRatId == 0)
  {
    // Duplicate Player id received from network.
    // Protocol doesn't allow this - ignore
    return;
  }

  // never seen this rat before or it's a dup player id! let's open up a spot.
  if(freeRatId == RAT_ID_NOT_FOUND)
  {
    freeRatId = NextFreeRatId();
    cout << "NextFreeRatId: " << freeRatId ;
  }

  // No free slots left
  if (freeRatId == RAT_ID_NOT_FOUND)
  {
    log("Receive Registration request but max rats already playing. Skipped!");
    return;
  }

  // New rat
  cout << "Grabbing rat to make it:" << freeRatId << "\n";
  Rat r = M->rat(freeRatId);

  r.playing = true;
  r.x = sr->_x;
  r.y = sr->_y;
  r.dir = sr->_d;
  r.name = sr->_playerName;
  int tmpscore = sr->_score;
  tmpscore = (SCORE_OFFSET - tmpscore) * (-1);
  r.score = tmpscore;
  r.netPlayerId = sr->playerID;
  r.lastNetPacketReceivedTimeStamp = CurrentTimeInMillis();
  r.lastSequenceNumberProcessed = 0;
  r.LastKillRequestSeqNum = 0;
  r.LastKillAckSeqNum = 0;

  M->ratIs(r, freeRatId);
  updateView = TRUE;
  UpdateScoreCard(1);
}


/**
* Get the next free slot in the mazewar rat data structure
* A free slot is a non playing rat
* returns minus 1 if no slots are left.
**/
short unsigned int NextFreeRatId()
{
  short unsigned int i = 1;
  for( i = 1; i < MAX_RATS; i++)
  {
    if (!M->rat(i).playing)
    {
      return i;
    }
  }
  return -1;
}


/**
* Broadcast a sync request in the network
**/
void NetSendSyncRequest()
{
  SyncRequest* sr = new SyncRequest();
  sr->SetDatagram(MY_X_LOC, MY_Y_LOC, MY_DIR, M->myName_, (M->score().value() + SCORE_OFFSET));
  sendPacket(sr);
  delete sr;
}


/**
* Respond to a Sync Response
**/
void NetSendSyncResponse(uint8_t* payload)
{
  SyncResponse* sr = new SyncResponse();
  sr->SetDatagram(MY_X_LOC, MY_Y_LOC, MY_DIR, M->myName_, (M->score().value() + SCORE_OFFSET));
  sendPacket(sr);
  delete sr;
}


/**
* Update player score from keep alive
**/
void NetUpdateKeepAlive(Datagram* dg)
{
  KeepAlive* ka = dynamic_cast<KeepAlive*>(dg);
  short unsigned int ratId = RatIdFromPlayerId(dg->playerID);

  Rat r = M->rat(ratId);
  // do the offset calculation
  int tmpscore = ka->_score;
  tmpscore = (SCORE_OFFSET - tmpscore) * (-1);
  r.score = tmpscore;
  //cout << "Updating score via keepalive RatID:" << ratId << "  score:" << r.score;
  M->ratIs(r, ratId);
  updateView = TRUE;
  UpdateScoreCard(1);
}

int RatOccupiesXY(Loc x, Loc y,int ratId)
{
  short unsigned int i = 1;
  for( i = 1; i < MAX_RATS; i++)
  {
    if (ratId == i)
    {
      //skip
      continue;
    }else
    {
      //rat playing in that location
      if (M->rat(i).playing && M->rat(i).x == x && M->rat(i).y == y)
      {
        return i;
      }
    }
  }
  return 0;
}

/**
* Update player score from keep alive
**/
void NetUpdatePosition(Datagram* dg)
{
  Move* m = dynamic_cast<Move*>(dg);
  short unsigned int ratId = RatIdFromPlayerId(dg->playerID);

  if (ratId == RAT_ID_NOT_FOUND)
  {
    log("Skipping move for rad not found");
    return;
  }

  // Skip this update if it is conflicting
  if(RatOccupiesXY(m->_x, m->_y, ratId))
  {
    log("Ignoring move - position is occupied");
    return;
  }

  Rat r = M->rat(ratId);

  r.x   = m->_x;
  r.y   = m->_y;
  r.dir = m->_d;

  M->ratIs(r, ratId);
}

/**
* Update player score from keep alive
**/
void NetUpdateQuit(Datagram* dg)
{
  short unsigned int ratId = RatIdFromPlayerId(dg->playerID);
  Rat r = M->rat(ratId);

  r.playing = false;
  M->ratIs(r, ratId);
}


/**
* Returns the Rat ID from a playerID
**/
short unsigned int RatIdFromPlayerId(uint16_t playerID)
{
  short unsigned int i = 0;
  cout << "RatIdFromPlayerId Get for player: " << playerID << "\n";
  for( i = 1; i < MAX_RATS; i++)
  {
    Rat tmp = M->rat(i);
    if( tmp.netPlayerId == playerID)
    {
      return i;
    }
  }

  log("Unexpected Error: Rat Not found!");
  return RAT_ID_NOT_FOUND;
}
/* ----------------------------------------------------------------------- */

/**
* Register a player locally from a sync request packet.
**/
void RegisterPlayerFromSyncRequest(uint8_t* payload)
{
  // Register player locally
  Datagram *dg = new Datagram();
  SyncRequest* sr = dynamic_cast<SyncRequest*>(dg->ByteArrayToDatagram(payload));

  RegisterPlayer(sr);
  delete sr;
  delete dg;
}

/**
* Receives incoming network events.
*
**/
void processPacket (MWEvent *eventPacket)
{
	log("Processing incoming Packet");

		MW244BPacket    *pack = eventPacket->eventDetail;
    uint8_t *payload = reinterpret_cast<uint8_t *>(pack);

    Datagram* tmp = new Datagram();
    Datagram* command = tmp->ByteArrayToDatagram(payload);
    delete tmp;

    /*  Extensions from Rat class in mazewar.h.**
    *   here for quick reference
    *********************************************
    *   int netPlayerId;
    *   int lastSequenceNumberProcessed;
    *   int LastKillRequestSeqNum;
    *   int LastKillAckSeqNum;
    *   int lastNetPacketReceivedTimeStamp;
    *********************************************
    **/

    // If player ID from packet not registered and NOT sync:
    if( RatIdFromPlayerId(command->playerID) == RAT_ID_NOT_FOUND)
    {
      if(command->GetType() != SYNC_REQ && command->GetType() != SYNC_ACK)
      {
        log("toplevel::processPacket - incoming packet from player not registered and Not a sync command");
        NetSendSyncRequest();
        return;
      }
    }

    // Handle each packet Type;
    switch (command->GetType())
    {
      case SYNC_REQ:
        RegisterPlayerFromSyncRequest(payload);
        NetSendSyncResponse(payload);
        break;
      case SYNC_ACK:
        UpdateOrRegisterPlayerFromSyncAck(payload);
        break;

      case MOVE:
        NetUpdatePosition(command);
        break;

      case KEEPLIVE:
        NetUpdateKeepAlive(command);
        break;

      case QUIT:
        NetUpdateQuit(command);
        break;

      /** Message Types **/
      case KILL_REQ:
        NetReceiveKillRequest(payload);
        break;
      case KILL_ACK:
        NetReceiveKillAck(payload);
        break;
      case KILL_DEN:
        NetReceiveKillDenied(payload);
        break;

      default:
        log("TopLevel::ProcessPacket -- Received unknown packet" );
        log_bytearr(command->GetDatagram(), 256);
        break;
    }



    delete command;

  // Redraw and redfresh Scoring
  updateView = TRUE;
  UpdateScoreCard(1);
}

void ConvertIncoming(MW244BPacket*)
{

}

/* ----------------------------------------------------------------------- */

/* This will presumably be modified by you.
   It is here to provide an example of how to open a UDP port.
   You might choose to use a different strategy
 */
void
netInit()
{
	Sockaddr		nullAddr;
	Sockaddr		*thisHost;
	char			buf[128];
	int				reuse;
	u_char          ttl;
	struct ip_mreq  mreq;

	/* MAZEPORT will be assigned by the TA to each team */
	M->mazePortIs(htons(MAZEPORT));

	gethostname(buf, sizeof(buf));
	if ((thisHost = resolveHost(buf)) == (Sockaddr *) NULL)
	  MWError("who am I?");
	bcopy((caddr_t) thisHost, (caddr_t) (M->myAddr()), sizeof(Sockaddr));

	M->theSocketIs(socket(AF_INET, SOCK_DGRAM, 0));
	if (M->theSocket() < 0)
	  MWError("can't get socket");

	/* SO_REUSEADDR allows more than one binding to the same
	   socket - you cannot have more than one player on one
	   machine without this */
	reuse = 1;
	if (setsockopt(M->theSocket(), SOL_SOCKET, SO_REUSEADDR, &reuse,
		   sizeof(reuse)) < 0) {
		MWError("setsockopt failed (SO_REUSEADDR)");
	}

	nullAddr.sin_family = AF_INET;
	nullAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nullAddr.sin_port = M->mazePort();
	if (bind(M->theSocket(), (struct sockaddr *)&nullAddr,
		 sizeof(nullAddr)) < 0)
	  MWError("netInit binding");

	/* Multicast TTL:
	   0 restricted to the same host
	   1 restricted to the same subnet
	   32 restricted to the same site
	   64 restricted to the same region
	   128 restricted to the same continent
	   255 unrestricted

	   DO NOT use a value > 32. If possible, use a value of 1 when
	   testing.
	*/

	ttl = 1;
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
		   sizeof(ttl)) < 0) {
		MWError("setsockopt failed (IP_MULTICAST_TTL)");
	}

	/* join the multicast group */
	mreq.imr_multiaddr.s_addr = htonl(MAZEGROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)
		   &mreq, sizeof(mreq)) < 0) {
		MWError("setsockopt failed (IP_ADD_MEMBERSHIP)");
	}

	/*SNIPPET FROM GOBIND: testing to listen to my own packets, remove this before submitting*/
	int loop = 0;
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
		printf("%s\n", "Failed to setup self loop" );
	}

	/*
	 * Now we can try to find a game to join; if none, start one.
	 */

	printf("\n");

	/* set up some stuff strictly for this local sample */
	M->myRatIdIs(0);
	M->scoreIs(0);

	/* Get the multi-cast address ready to use in SendData()
           calls. */
	memcpy(&groupAddr, &nullAddr, sizeof(Sockaddr));
	groupAddr.sin_addr.s_addr = htonl(MAZEGROUP);

}


/* ----------------------------------------------------------------------- */
