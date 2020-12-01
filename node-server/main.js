const path = require('path');
const express = require('express');
const WebSocket = require('ws');
const app = express();

const ESP_WS_PORT  = 8888; //esp 8888
const CONTROL_WS_PORT  = 8989; //esp 8888

const HTTP_DUMMY_PORT = 8000; // 8000

// ESP SERVER
const espWsServer = new WebSocket.Server({port: ESP_WS_PORT}, ()=> console.log(`Listening for ESP\'s on port ${ESP_WS_PORT}`));

// CONTROL DEVICE SERVER
const controlWsServer = new WebSocket.Server({port: CONTROL_WS_PORT}, ()=> console.log(`Listening for controllers on port ${CONTROL_WS_PORT}`));


// string-ip, object
var controlClients = new Map(); //[];
var espClients = new Map(); //[];

class Controller {
    constructor(ws, owned_esp_addr) {
        this.ws = ws; this.owned_esp_addr = owned_esp_addr;
        //this.alive = true;
    }
};

class ESP {
    constructor(ws, owned_controller_addr) {
        this.ws = ws; this.owned_controller_addr = owned_controller_addr;
        this.alive = true;
    }
}

// the controller will search for esp and then pair
function setAsPair(controller_ws, espAddr) {
    controlAddr = controller_ws._socket.remoteAddress
    controlClients.set(controlAddr, new Controller(controller_ws, espAddr));//_socket.remoteAddress

    esp_ws = espClients.get(espAddr).ws;
    
    espClients.set(espAddr, new ESP(esp_ws, controlAddr));//_socket.remoteAddress


    /*
        DEBUG
    */

    //console.log(`${controlClients.get(controlAddr).ws._socket.remoteAddress.toString()}, ${controlClients.get(controlAddr).owned_esp_addr}`);
    //console.log(`${espClients.get(espAddr).ws._socket.remoteAddress.toString()}, ${espClients.get(espAddr).owned_controller_addr}`);

}


espWsServer.on('connection', (ws, req) => { // when every connected ESP to connectedClients
    console.log(`ESP ${ws._socket.remoteAddress.toString()} connected`);
    espClients.set(ws._socket.remoteAddress.toString(), new ESP(ws, ""));
    
    // WHEN ESP SENDS DATA:
    ws.on('message', data => {
        // Send data to controller
        //console.log("Received data from ESP");



        espAddrStr = ws._socket.remoteAddress.toString();

        // get address of corresponding controller
        esp = espClients.get(espAddrStr);

        // send data to that controller
        if (esp != undefined) { // then get
            controllerAddrStr = esp.owned_controller_addr
            control = controlClients.get(controllerAddrStr);
            if (control == undefined) { // not linked
                //console.log("fail");
                return;
            }
            if (control.ws.readyState === ws.OPEN) {
                // then send
                control.ws.send(data);
                //console.log("sent data to controller");
                return;
            }


// else {
             //   espClients.delete(espAddrStr);
             //   controlClients.delete(controllerAddrStr);
            //}
            //console.log("nothing was sent");
        }
        //console.log("undefined");

        //espClients.forEach((esp_ws, i) => {
        //    esp_ws = car.espClient;
        //    // check whether ips are same
        //    if(esp_ws.readyState === ws.OPEN) { // === for strict equality
        //        if (car.controllerIndex = -1) continue;
        //
        //        if (ws === esp_ws) {
        //            controlClients[car.controllerIndex].send(data);
        //            break;
        //        }
        //    }
        //    else espCars.splice(i, 1);
        //})
    });

  //ws_client_stream.is_alive = true;
  //ws_client_stream.on('pong', () => { ws_client_stream.is_alive = true; });

});


controlWsServer.on('connection', (ws, req) => { // when every connected ESP to connectedClients
    console.log(`Controller ${ws._socket.remoteAddress.toString()} connected`);
    controlClients.set(ws._socket.remoteAddress.toString(), new Controller(ws, ""));
    
    ws.on('message', data => {
        
        if (data.startsWith("esp-ip=")) {
            otherAddrStr = data.toString().substring(7);
            //console.log(`SUBSTRING: ${otherAddrStr}, size: ${otherAddrStr.length}`);
            setAsPair(ws, otherAddrStr);
            return;
        }

        controllerAddrStr = ws._socket.remoteAddress.toString();

        // get address of corresponding esp
        controller = controlClients.get(controllerAddrStr);


        // send data to that controller
        if (controller != undefined) { // check is redundant
            espAddrStr = controller.owned_esp_addr
            esp = espClients.get(espAddrStr);
            if (esp == undefined) { // not linked
                return;
            }
            if (esp.ws.readyState === ws.OPEN) {
                // then send
                esp.ws.send(data);
                return;
            }// else console.log(`ESP ${espAddrStr} is disconnected`);

// else {
              //  controlClients.delete(controllerAddr);
             //   espClients.delete(espAddrStr);
            //}
        }
    });
});

//setInterval(function ping() {
//  Array.from(connected_clients.values()).forEach(function each(client_stream) {
//    if (!client_stream.is_alive) { client_stream.terminate(); return; }
//    client_stream.is_alive = false;
//    client_stream.ping();
//  });
//}, 1000);

app.get('/',(req,res) => {
    console.log(`HTTP client ${req.connection.remoteAddress} connected`);
    res.sendFile(path.resolve(__dirname, 'index.html'))
});

app.listen(HTTP_DUMMY_PORT, ()=> {
    console.log(`HTTP server listening on port ${HTTP_DUMMY_PORT}`)
});