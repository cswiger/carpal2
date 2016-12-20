# carpal2
Carpal2 project using AT&amp;T IoT Starter Kit

The Pubnub queue watcher "pubnub_mongo_attiot.js", to load MongoDB, will need your MongoDB account in the mongodbURI (my example has mqtt:password) and your Pubnub subscribeKey. 

The mylocation.html eon-map - just follow the instructions at https://github.com/pubnub/eon-map to setup a mapbox account and map, etc. 

main.cpp/main.h is my example code from the mbed.org development environment. It also needs libraries:  <br/>
FXOS8700CQ   for the accelerometer<br/>
mbed-rtos   for the timer<br/>
Pubnub_mbed2_sync    for Pubnub support<br/>
WNCInterface      for the cell modem shield<br/>

Also needs the files:  <br/>
hts221_driver.cpp HTS221.h<br/>
pubnub_config.h<br/>
sensors.cpp sensors.h<br/>
xadow_gps.cpp  xadow_gps.h<br/>
