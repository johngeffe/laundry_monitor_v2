# laundry_montitor_v2

General stratagy:
 * Feather uses MQTT PubSubClient to publish change of modes while Node-red handles notification (email etc).
 * to raspberry pi running MQTT.
https://learn.adafruit.com/diy-esp8266-home-security-with-lua-and-mqtt/configuring-mqtt-on-the-raspberry-pi

 * Node-red subscribes and publishes to MQTT server.
 * Node-red can do other IoT things like email or twitter.
>> I am running them in a Rasberry PI so they are almost always on
http://nodered.org/docs/hardware/raspberrypi.html
 
Adafruit Feather Huzzah ESP8266
 * https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/overview
 * #1 (washer)LIS3DH I2C Address default 0x18
 * https://learn.adafruit.com/adafruit-lis3dh-triple-axis-accelerometer-breakout/overview
 * #2 (dryer) LIS3DH > Wiring 3.3v to SDO pin to change address to 0x19
 * SDO - When in I2C mode, this pin can be used for address selection. 
 --> When connected to GND or left open, the address is 0x18
 --> it can also be connected to 3.3V to set the address to 0x19 <--
 * Wire the following from the Feather {3v, GND, SLC, SLA} to both LIS3DH modules in parallel.

I used doublesided tap to attach magnets to the LIS3DH sensors making this a completely portable and non-intrusive project.
NOTE: This code is configured for I2C only.


Inspired by
https://github.com/domiflichi/Launitor-Laundry-Monitor

Import this node-red for an example.

[{"id":"ebfcffbd.07c9b","type":"mqtt out","z":"4b14c49.a27843c","name":"","topic":"laundry","qos":"","retain":"","broker":"e606f447.8145b8","x":520,"y":120,"wires":[]},{"id":"decd3d6f.a67a6","type":"inject","z":"4b14c49.a27843c","name":"","topic":"","payload":"status","payloadType":"str","repeat":"","crontab":"","once":true,"x":130,"y":60,"wires":[["ebfcffbd.07c9b"]]},{"id":"21b1aca6.479164","type":"mqtt in","z":"4b14c49.a27843c","name":"","topic":"laundryStatus","qos":"0","broker":"e606f447.8145b8","x":130,"y":220,"wires":[["5ead724a.8e06cc"]]},{"id":"5ead724a.8e06cc","type":"debug","z":"4b14c49.a27843c","name":"","active":true,"console":"false","complete":"false","x":530,"y":220,"wires":[]},{"id":"26a44db8.15ac12","type":"inject","z":"4b14c49.a27843c","name":"","topic":"","payload":"washer","payloadType":"str","repeat":"","crontab":"","once":false,"x":130,"y":100,"wires":[["ebfcffbd.07c9b"]]},{"id":"e036f82c.8aea68","type":"inject","z":"4b14c49.a27843c","name":"","topic":"","payload":"dryer","payloadType":"str","repeat":"","crontab":"","once":false,"x":130,"y":140,"wires":[["ebfcffbd.07c9b"]]},{"id":"955ce5b6.d38f38","type":"mqtt in","z":"4b14c49.a27843c","name":"","topic":"washerStatus","qos":"0","broker":"e606f447.8145b8","x":90,"y":320,"wires":[["58607ba0.50d414"]]},{"id":"60a35747.967a98","type":"mqtt in","z":"4b14c49.a27843c","name":"","topic":"dryerStatus","qos":"0","broker":"e606f447.8145b8","x":90,"y":460,"wires":[["995d301b.6bba3"]]},{"id":"b736dd41.e8ee","type":"change","z":"4b14c49.a27843c","name":"Finished","rules":[{"t":"set","p":"payload","pt":"msg","to":"finished","tot":"str"}],"action":"","property":"","from":"","to":"","reg":false,"x":540,"y":400,"wires":[["6ce91e1b.4309b"]]},{"id":"20953072.043b2","type":"link out","z":"4b14c49.a27843c","name":"email","links":["41283473.3ffb6c"],"x":855,"y":340,"wires":[]},{"id":"58607ba0.50d414","type":"change","z":"4b14c49.a27843c","name":"washer","rules":[{"t":"set","p":"appliance","pt":"msg","to":"Washer","tot":"str"}],"action":"","property":"","from":"","to":"","reg":false,"x":260,"y":320,"wires":[["3fc2d84b.ebade8"]]},{"id":"995d301b.6bba3","type":"change","z":"4b14c49.a27843c","name":"Dryer","rules":[{"t":"set","p":"appliance","pt":"msg","to":"Dryer","tot":"str"}],"action":"","property":"","from":"","to":"","reg":false,"x":270,"y":460,"wires":[["3fc2d84b.ebade8"]]},{"id":"6ce91e1b.4309b","type":"template","z":"4b14c49.a27843c","name":"","field":"payload","fieldType":"msg","format":"handlebars","syntax":"mustache","template":"{{appliance}} is {{payload}}","x":710,"y":400,"wires":[["20953072.043b2","ba30ec8f.75b0e"]]},{"id":"ba30ec8f.75b0e","type":"debug","z":"4b14c49.a27843c","name":"","active":true,"console":"false","complete":"false","x":910,"y":400,"wires":[]},{"id":"aaa9e6c3.bda4e8","type":"change","z":"4b14c49.a27843c","name":"Running","rules":[{"t":"set","p":"payload","pt":"msg","to":"running","tot":"str"}],"action":"","property":"","from":"","to":"","reg":false,"x":540,"y":340,"wires":[[]]},{"id":"3fc2d84b.ebade8","type":"switch","z":"4b14c49.a27843c","name":"","property":"payload","propertyType":"msg","rules":[{"t":"eq","v":"1","vt":"str"},{"t":"eq","v":"3","vt":"str"},{"t":"eq","v":"4","vt":"str"}],"checkall":"true","outputs":3,"x":390,"y":380,"wires":[[],["aaa9e6c3.bda4e8"],["b736dd41.e8ee"]]},{"id":"cb67e778.f365b8","type":"comment","z":"4b14c49.a27843c","name":"Email","info":"This could be any action from tweeting to changing\nthe color of your smartthings lights.","x":930,"y":340,"wires":[]},{"id":"e606f447.8145b8","type":"mqtt-broker","broker":"localhost","port":"1883","usetls":false,"verifyservercert":true,"compatmode":true,"keepalive":15,"cleansession":true,"willQos":"0","birthQos":"0"}]
