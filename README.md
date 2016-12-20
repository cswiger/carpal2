# carpal2
Carpal2 project using AT&amp;T IoT Starter Kit

The Pubnub queue watcher to load MongoDB will need your MongoDB account in the mongodbURI (my example has mqtt:password) and your Pubnub subscribeKey. 

The mylocation.html eon-map - just follow the instructions at https://github.com/pubnub/eon-map to setup a mapbox account and map, etc. 

main.cpp/main.h is my example code from the mbed.org development environment. It also needs libraries:  
FXOS8700CQ   for the accelerometer
mbed-rtos   for the timer
Pubnub_mbed2_sync    for Pubnub support
WNCInterface      for the cell model shield

Also needs the files:  
hts221_driver.cpp HTS221.h
pubnub_config.h
sensors.cpp sensors.h
xadow_gps.cpp  xadow_gps.h
