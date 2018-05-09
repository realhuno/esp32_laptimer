//websocket gateway on 8070
var app = require('http').createServer(handler)
, io = require('socket.io').listen(app)
, fs = require('fs')
app.listen(5000);
var mysocket = 0;
function handler (req, res) {
fs.readFile(__dirname + '/index.html',
function (err, data) {
if (err) {
res.writeHead(500);
return res.end('Error loading index.html');
}
res.writeHead(200);
res.end(data);
});
}
io.sockets.on('connection', function (socket) {
console.log('index.html connected');
mysocket = socket;
});

//udp server on 41181
var dgram = require("dgram");
var server = dgram.createSocket("udp4");
server.on("message", function (msg, rinfo) {
console.log("msg: " + msg);
if (mysocket != 0){
mysocket.emit('pass_record', msg);
//mysocket.emit('pass_record', {'node': 1, 'frequency': 5808, 'timestamp': 111554455});
console.log("schicke weiter");
}
});
server.on("listening", function () {
var address = server.address();
console.log("udp server listening " + address.address + ":" + address.port);
});
server.bind(1236);