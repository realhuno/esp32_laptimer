var udp = require('dgram');

// --------------------creating a udp server --------------------
// Kleiner HTTP-Server auf Port 8080
var app = require('http').createServer(serveStaticIndex).listen(5000);
 
// Laden von Socket.io
//(Gibt für die Demo nur Fehler/Warnungen auf der Konsole aus)
var io = require('socket.io').listen(app).set('log level', 1);
 
// Zugriff auf das Dateisystem (zum Ausliefern der index.html)
var fs = require('fs');
 
// Liefert die Startseite aus
function serveStaticIndex(req, res) {
  var fileStream = fs.createReadStream(__dirname + '/index.html');
      res.writeHead(200);
      fileStream.pipe(res);
}
// creating a udp server
var server = udp.createSocket('udp4');

// emits when any error occurs
server.on('error',function(error){
  console.log('Error: REUP' + error);
 server.close();

});

// emits on new datagram msg
server.on('message',function(msg,info){
  console.log('Data received from client : ' + msg.toString());
  console.log('Received %d bytes from %s:%d\n',msg.length, info.address, info.port);

  // Socket.io-Events
io.sockets.on('connection', function (socket) {
  console.log('[socket.io] Ein neuer Client (Browser) hat sich verbunden.\n');
 
  console.log('[socket.io] SENDE "welcome"-Event an den Client.\n');
  socket.emit('pass_record', {'node': 1, 'frequency': 5808, 'timestamp': 111554455});
 
  socket.on('user agent', function (data) {
    console.log('[socket.io] EMPFANGE "user agent"-Event vom Client:');
    console.log(data, '\n');
	msg=NULL;
  });
});
  
  
  
//sending msg
server.send(msg,info.port,'localhost',function(error){
  if(error){
  client.close();
	
  }else{
    console.log('Data sent !!!');
  }

});

});

//emits when socket is ready and listening for datagram msgs
server.on('listening',function(){
  var address = server.address();
  var port = address.port;
  var family = address.family;
  var ipaddr = address.address;
  console.log('Server is listening at port' + port);
  console.log('Server ip :' + ipaddr);
  console.log('Server is IP4/IP6 : ' + family);
});

//emits after the socket is closed using socket.close();
server.on('close',function(){
  console.log('Socket is closed !');
});

server.bind(1236);

//setTimeout(function(){
//server.close();
//},8000);


 
