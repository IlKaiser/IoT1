var AWS = require('aws-sdk');
const server  = require('http').createServer(app); 
const port    = 8001;
const fs = require('fs');
const path = require('path');
var express = require('express');


var app = express();

app.use(express.static(__dirname+"/views"));
// Parse JSON bodies (as sent by API clients)
app.use(express.json());

app.get('/', function(req, res) {
    res.sendFile(path.join(__dirname + '/index.html'));
});

AWS.config.update({
    region: "us-east-1",
    endpoint: "dynamodb.us-east-1.amazonaws.com"
});


var awsIot = require('aws-iot-device-sdk');

var device = awsIot.device({
   keyPath:  path.resolve(__dirname, "./secret/Nucleo.private.key"),
  certPath:  path.resolve(__dirname, "./secret/Nucleo.cert.pem"),
    caPath:  path.resolve(__dirname, "./secret/rootCA.pem"),
  clientId: "ASIA2K4LFQ3KFGLYGZKZ",
    host: "a1gznbp4fjkpqr-ats.iot.us-east-1.amazonaws.com"
});

//
// Device is an instance returned by mqtt.Client(), see mqtt.js for full
// documentation.
//
device
  .on('connect', function() {
    console.log('connect');
    device.subscribe('topic_1');
    
  });

device
  .on('message', function(topic, payload) {
    console.log('message', topic, payload.toString());
  });


var docClient = new AWS.DynamoDB.DocumentClient();


var params = {
    TableName: "wx_data"
}

/*              

var query_params = {
  TableName: 'wx_data',
  KeyConditionExpression: 'sample_time > :t',
  Limit: 1,
  ScanIndexForward: false,    // true = ascending, false = descending
  ExpressionAttributeValues: {
      ':t': one_week_ago.toString()
  }
};

docClient.query(query_params, function(err, data) {
  if (err) {
      console.log(JSON.stringify(err, null, 2));
  }  data.Items.forEach(function(element, index, array) {
    console.log(element.Title.S + " (" + element.Subtitle.S + ")");
  });
});
*/
console.log("Scanning...");
docClient.scan(params, onScan);
var final = "";
function onScan(err, data) {
    if (err) {
        console.error("Unable to scan the table. Error JSON:", JSON.stringify(err, null, 2));
    } else {
        // print all the movies
        console.log("Scan succeeded.");
        data.Items.forEach(function(measurement) {
            //console.log(measurement.device_data);
            if(measurement.device_data !== undefined){
              var date = new Date(measurement.sample_time);
              var mid = ""
              mid += JSON.stringify(measurement.device_data)+",\n";
              mid+= JSON.stringify(measurement.sample_time)+",\n";
              console.log(mid)
              final+=mid;
            }
            
        });
        app.get("/data",function(req, res){
            res.send(final); //replace with your data here
        })
    }
}

app.post("/mqtt", function (req, res) {
    console.log(req.body.turn);
    device.publish('both_directions', JSON.stringify(req.body.turn));
});
app.listen(port);



