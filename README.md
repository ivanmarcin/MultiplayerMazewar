MultiplayerMazewar
==================

Mod of the original Xerox Alto Mazewar game - converted to UDP multiplayer



Todo
==================
* Packet Data structures
	- (done)

* Logging
	- (done)

* Add ahtonl to all outgoing packets(make everything network order on the way out)
	- (todo)

* Receive and deserialize packets
	- Serialize all  (done)

* deserialize
	- all(done)

* Fix the killPlayerID in killResponse header
	- (todo)

* Fix all outgoing packets to be in network order
	- (done)

* Send packets on ALL actions
	-move done
	-leave done
	-keepalive done
	-shoot(in progress)
	-sync(todo)

*Add misile shoot
	- (in progress)
	
* Add conflict resolution rules
	- (conflict on receiving conflicting move)

* Add other rats to map and display updates
	- (todo)


