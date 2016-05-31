# Amazon-Echo-Switch
========

Build a cheap swich that can be controlled by Amazon Alexa. 

Part you need - 1.SONOFF switch 2.USB-UART board for download the code. 

******Process******

1. Download the code.
2. Copy WiFiSwitchManager into your Arduino library. 
3. Compile Switch.ino from Arduino IDE and download into device.
4. Reset the device power, and it will enter AP mode.
5. Connect AP named "ESP ID: xxxxxxx"
6. Input IP: 192.168.4.1 on your cell phone.
7. Setup you SSID/PASSWORD/DEVICE NAME through your cell phone.
8. Say "Discover my new device" to Alexa.
9. After Alexa find the device, say "turn on ***Device Name***" or "turn off ***Device Name***"
10. ***Press on switch more than 3 seconds will force deivce into AP mode***

Write me if you have any issue: leo6346@hotmail.com
