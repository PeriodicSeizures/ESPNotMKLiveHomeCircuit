# ESPNotMKLiveHomeCircuit
Little racecar prototype code, includes websocket communication to a Nodejs server in 'node-server'. Uses webcam to stream. I'll eventually refine the STL model to include it here.

## Usage
Start the Node server, start the ESP (make sure you've configured router/password, host/password. When "controller" pc connects to host server, input the address of the ESP you want to connect to as it appears in the node server log.

Can control servo rotation with A/D, and motor with forward/backward arrow on HTML page.
