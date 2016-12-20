# carpal2
Carpal2 project using AT&amp;T IoT Starter Kit

The Pubnub queue watcher "pubnub_mongo_attiot.js", to load MongoDB, will need your MongoDB account in the mongodbURI (my example has mqtt:password) and your Pubnub subscribeKey. 

The mylocation.html eon-map - just follow the instructions at https://github.com/pubnub/eon-map to setup a mapbox account and map, etc. 

FlowFunctions - function blocks used in AT&T Flow for preparing data for eon-map, m2x or request for dtc reply. 

clear_m2x_carpal.bash - utility for deleting data from device streams
