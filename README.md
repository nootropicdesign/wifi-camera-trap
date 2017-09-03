Wi-Fi Camera Trap

This project has 2 parts: Arduino code that runs on the ESP8266 and a Node.js based image server.

Arduino code dependencies:
* [ESP8266 core for Arduino](https://github.com/esp8266/Arduino)
* [WiFiManager library](https://github.com/tzapu/WiFiManager)
* [ArduCAM library](https://github.com/ArduCAM/Arduino)

The Arduino code uploads images to the image server. Specify your image server hostname in the Arduino code.

For the image server, you need a host to run it on. Just run
```
npm install
node app.js
```

The server runs on port 8000 and the uploaded images are accessible at

```
http://yourserver:8000/images
```


[Full Project Description at nootropicdesign.com](https://nootropicdesign.com/projectlab/2017/09/09/wifi-camera-trap/)





