#!/bin/bash
# script to clear values from M2X device

M2XKEY=____your_m2x_key_here___________
DEVICEID=____your_device_id_here_________

for STREAM in load speed rpm coolant ax ay
do
  curl -X DELETE -H "Content-Type: application/json" -H "X-M2X-KEY: $M2XKEY" -d '{"from": "2016-12-04T00:00:00.000Z","end": "2016-12-25T18:30:00.000Z" }' http://api-m2x.att.com/v2/devices/$DEVICEID/streams/$STREAM/values
  echo
done

