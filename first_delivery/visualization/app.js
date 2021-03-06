
/* requirements */

var AWS = require('aws-sdk');
var awsIot = require('aws-iot-device-sdk');
const server  = require('http').createServer(app); 
const port    = 8001;
const fs = require('fs');
const path = require('path');
var express = require('express');

/* start express */
var app = express();

app.use(express.static(__dirname+"/views"));

// Parse JSON bodies (as sent by API clients)
app.use(express.json());

app.get('/', function(req, res) {
    res.sendFile(path.join(__dirname + '/index.html'));
});

/* start aws */
AWS.config.update({
    region: "us-east-1",
    endpoint: "dynamodb.us-east-1.amazonaws.com"
});

var device = awsIot.device({
   keyPath:  path.resolve(__dirname, "./secret/Nucleo.private.key"),
  certPath:  path.resolve(__dirname, "./secret/Nucleo.cert.pem"),
    caPath:  path.resolve(__dirname, "./secret/rootCA.pem"),
  clientId: "ASIA2K4LFQ3KFGLYGZKZ",
    host: "a1gznbp4fjkpqr-ats.iot.us-east-1.amazonaws.com"
});

/* scan DynamoDB */
var docClient = new AWS.DynamoDB.DocumentClient();


var params = {
    TableName: "wx_data"
}


console.log("Scanning...");
docClient.scan(params, onScan);

var final = "";
function onScan(err, data) {
    if (err) {
        console.error("Unable to scan the table. Error JSON:", JSON.stringify(err, null, 2));
    } else {
        // send all datas
        console.log("Scan succeeded.");
        data.Items.forEach(function(measurement) {    
          if(measurement.device_data !== undefined){
            final += JSON.stringify(measurement.device_data)+",\n";
            final += JSON.stringify(measurement.sample_time)+",\n";
            final += JSON.stringify(measurement.device_id)+",\n";
          }
        });
        app.get("/data",function(req, res){
            res.send(final); 
        })
    }
}

/* Mqtt listener */
app.post("/mqtt", function (req, res) {
    console.log(req.body.turn);
    device.publish('both_directions', req.body.turn);
});

setInterval(function scan(){ docClient.scan(params, onScan)},10000);
app.listen(port);



