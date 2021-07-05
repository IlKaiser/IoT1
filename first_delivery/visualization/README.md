# Visualization Code explained

The main code is in [app.js](./app.js) file. It is simple and pretty straightforward; it is divided in three logical blocks:
- [Start http socket and dashboard](#http-socket)
- [Scan DynamoDB](#scan-dynamodb)
- [Publish on MQTT](#publish-on-mqtt)

in the beginning are initialized all modules used:
```js
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
```
# Http socket
Using express is start dashboard in localhost at specified port (defualt 8001):
```js

app.get('/', function(req, res) {
    res.sendFile(path.join(__dirname + '/index.html'));
});
// [...]
// blocking call at the bottom
app.listen(port);

```
# Scan DynamoDB
Connect to AWS and retrieve all data from DynamoDB:
```js
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

// Process data and send to dashboard which periodically does get requests
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

```
Necessary data processing is made from dashboard in [index.html](views/index.html):
```js
// analyze parsed data every three slices since we have:
// first  slice  -> Meausrements
// second slice  -> DB timestamp
// third  slice  -> Device id
for(i=0;i<parsed_data.length-2;i+=3){
    // try parsing JSON
    var json = "";
    try {
        json = JSON.parse(parsed_data[i]);
    } catch (error) {
        continue;
    }
    
    // display only data from selected timestamp
    var date = new Date(parseInt(parsed_data[i+1]))
    var ONE_HOUR = 60 * 60 * 1000*24*10; /* ms */
    var myDate = new Date();
    var one_hour_ago = new Date(myDate.getTime() - ONE_HOUR);
    
    
    if(date - one_hour_ago > 0 ){
        // sorting stuff
        timestamp_only.push(parsed_data[i+1]);
        // get device_id
        let device_id = parsed_data[i+2];
        // average stuff
        total_hum+=json.humidity;
        total_temp+=json.temperature;
        cnt ++;
        // minmax
        if(json.temperature>max_temp){
            max_temp = json.temperature
        }
        else if(json.temperature<min_temp){
            min_temp = json.temperature
        }

        if(json.humidity>max_hum){
            max_hum = json.humidity
        }
        if(json.humidity<min_hum){
            min_hum = json.humidity
        }
        //[...]
    }
    //[...]
}
```

# Publish on MQTT
When a post request is made by pressing button, data are parsed and sent via mqtt:
```js
/* Mqtt listener */
app.post("/mqtt", function (req, res) {
    console.log(req.body.turn + req.body.id);
    device.publish('both_directions', req.body.turn + req.body.id);
});
```
