#!/usr/bin/node
// version 8 - for pubnub logging into mongo from cardata channel
//

var PubNub = require('/usr/lib/node_modules/pubnub');
var mongodb=require('/usr/lib/node_modules/mongodb');

var mongodbClient=mongodb.MongoClient;
var mongodbURI='mongodb://mqtt:password@127.0.0.1:27017/data'

function showit(payload) {
  //obj = JSON.parse(payload['message']);
  //console.log(obj)
  console.log(payload['message']);
  }

function insertEvent(payload) {
      mongodbClient.connect(mongodbURI, function(err,db) {
          if(err) { console.log(err); return; }
          else {
              console.log(payload['message']['message']);
              var coll = 'carpal2';
              collection = db.collection(coll);
              obj = payload['message'];
              if (obj['ax'] != null & obj['ay'] != null) {  // channel is used for led control also so ignore
                 collection.insert(
                    //{ data:payload['message'], when:new Date() },
                    {ax:obj['ax'],ay:obj['ay'],lo:obj['lo'],la:obj['la'],when:new Date() },
                    function(err,docs) {
                       if(err) { console.log("Insert fail" + err); } // Improve error handling
                       else { console.log("Update callback - closing db");
                              db.close() }
              });  // end of insert block
             }   // end of test for nulls
          }      // end of mongo  connect else block
      });       // end of mongo connect block
    }          // end of insertEvent


var pubnub = new PubNub({
    subscribeKey: "sub-c-xxxxxxxx-yyyy-zzzz-aaaa-bbbbbbbbbbbb"
});

pubnub.addListener({
    message: function(message) {
       //showit(message);
       insertEvent(message);
    }
});

pubnub.subscribe({
    channels: ['carpal2'],
    withPresence: false
});

