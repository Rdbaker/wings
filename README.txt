SAUCER SHOOT -- 2 player

Ryan Baker (WPI)
rdbaker


*****************************************
HOW TO BUILD
*****************************************


I've been building on an OSX machine (El Capitan). To make all the necessary files, simply use the command:

"make"

This will build the executable.

There's also a new sprite for the client ship in the sprites directory.


*****************************************
HOW TO RUN
*****************************************

to run the server, use:

"./game"

everything will spin up and, if everything works, it should "printf" a message indicating that it's waiting for the client

to run the client, use:

"./game -c [IP ADDR]"

if you're running on (and hoping to connect to) "127.0.0.1", you don't need to specify an IP address, otherwise it is required

Once the client connects, both screens should appear. The client will be waiting for the host to send a message to indicate that the game should start.

Once the host starts the game, both games will start.
