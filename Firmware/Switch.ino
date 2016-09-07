/*
Hardware-ESP8266:
GPIO0   SWITCH_PIN
GPIO12  RELAY
GPIO13  LED GREEN
PIN 
*/
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiSwitchManager.h>
#include <WiFiUdp.h>
#include <functional>

const int LED_GREEN = 13;
// LED_RED is not physically connected
const int RELAY 	 = 12;
const int SWITCH_PIN    = 0;

void prepareIds();
boolean connectWifi();
boolean connectUDP();
void startHttpServer();
void CheckWifiSwitch(void);
void ReadDataFromEEPROM();
unsigned int localPort = 1900;      // local port to listen on

WiFiUDP UDP;
boolean udpConnected = false;
IPAddress ipMulti(239, 255, 255, 250);
unsigned int portMulti = 1900;      // local port to listen on
ESP8266WebServer HTTP(80);
boolean wifiConnected = false;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,

String serial;
String persistent_uuid;
String DeviceName;
String SSID;
String Password;
WiFiManager wifiManager; 
//===========================     
void setup() 
//===========================
{
  Serial.begin(115200);
  prepareIds();
  pinMode(LED_GREEN, OUTPUT);
  pinMode(RELAY, OUTPUT); 	
  pinMode(SWITCH_PIN, INPUT);
  digitalWrite(RELAY, HIGH);   
	for(int i=0;i<6;i++)  
		{
  	digitalWrite(LED_GREEN, LOW);
  	delay(500);
  	digitalWrite(LED_GREEN, HIGH);
  	delay(500);  
		}  
  Serial.println("System Start");
//  wifiManager.resetSettings();	//test 	
  wifiManager.autoConnect();	
  while(!connectUDP())
        {
        }
  startHttpServer();
  ReadDataFromEEPROM();
}
//===========================
void loop() 
//===========================
{
CheckWifiSwitch(); 
HTTP.handleClient();
delay(1);
if(wifiConnected)
	{
    if(udpConnected)
    	{  
		int packetSize = UDP.parsePacket();      
		if(packetSize) 
      		{
        	Serial.println("");
        	Serial.print("Received UDP packet. Size: ");
        	Serial.println(packetSize);
        	Serial.print(". From ");
        	IPAddress remote = UDP.remoteIP();        
        	for (int i =0; i < 4; i++) 
          		{
          		Serial.print(remote[i], DEC);
          		if (i < 3) 
            		{
            		Serial.print(".");
            		}
          		}        
          	Serial.print(", port ");
          	Serial.println(UDP.remotePort());
          	int len = UDP.read(packetBuffer, 255);
          	if (len > 0) 
            	{
            	packetBuffer[len] = 0;
            	}
          	Serial.println("");    
          	Serial.println("Data:");
          	for(int i=0;i<packetSize;i++)
            	{   
            	Serial.print(packetBuffer[i]);  
            	} 
          	Serial.println("");      
        	String request = packetBuffer;
        	//Serial.println("Request:");
        	//Serial.println(request);
        	if(request.indexOf('M-SEARCH') > 0) 
          		{
            	if(request.indexOf("urn:Belkin:device:**") > 0) 
            		{
                	Serial.println("Responding to search request ...");
                	respondToSearch();
            		}
          		}    
      		}        
      delay(10);
    	}
    }
}
//=================
void ReadDataFromEEPROM()
//=================
{
	EEPROM.begin(512);
	int j=0;	
	for(int i=0;i<32;i++)		{SSID +=char(EEPROM.read(j)); j++;}
	for(int i=0;i<64;i++)		{Password +=char(EEPROM.read(j)); j++;}
	for(int i=0;i<64;i++)		{DeviceName +=char(EEPROM.read(j)); j++;}
  Serial.print("SSID = ");	
	Serial.println(SSID);	
	Serial.print("PASSWORD = ");	
	Serial.println(Password);
	Serial.print("DEVICE NAME = ");
	Serial.println(DeviceName);
}
//=================
void CheckWifiSwitch(void)
//=================
{
if(!digitalRead(SWITCH_PIN))
  {
    int i=0,j=0;
    for(i=0;i<30;i++)
    {
      delay(100);
      if(!digitalRead(SWITCH_PIN))
        j++;
      else
        i=30;
    }
    if(j>25)
    {
      Serial.println("RESETING WiFi SSID/PASSWORD/DEVICE NAME");
      wifiManager.resetSettings();
      wifiManager.autoConnect();      
   		while(!connectUDP())
        {
        }
      startHttpServer();
    }
  }
}
//=================
void prepareIds() 
//=================
{
  uint32_t chipId = ESP.getChipId();
  char uuid[64];
  sprintf_P(uuid, PSTR("38323636-4558-4dda-9188-cda0e6%02x%02x%02x"),
        (uint16_t) ((chipId >> 16) & 0xff),
        (uint16_t) ((chipId >>  8) & 0xff),
        (uint16_t)   chipId        & 0xff);

  serial = String(uuid);
  persistent_uuid = "Socket-1_0-" + serial;
}
//=================
void respondToSearch() 
//=================
{
Serial.println("");
Serial.print("Sending response to ");
Serial.println(UDP.remoteIP());
Serial.print("Port : ");
Serial.println(UDP.remotePort());
IPAddress localIP = WiFi.localIP();
char s[16];
sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
String response = 
         "HTTP/1.1 200 OK\r\n"
         "CACHE-CONTROL: max-age=86400\r\n"
         "DATE: Fri, 15 Apr 2016 04:56:29 GMT\r\n"
         "EXT:\r\n"
         "LOCATION: http://" + String(s) + ":80/setup.xml\r\n"
         "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
         "01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
         "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
         "ST: urn:Belkin:device:**\r\n"
         "USN: uuid:" + persistent_uuid + "::urn:Belkin:device:**\r\n"
         "X-User-Agent: redsonic\r\n\r\n";
UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
UDP.write(response.c_str());
UDP.endPacket();                    
Serial.println("Response sent !");
}
//=================
void startHttpServer() 
//=================
{
    HTTP.on("/index.html", HTTP_GET, [](){
      Serial.println("Got Request index.html ...\n");
      HTTP.send(200, "text/plain", "Hello World!");
    });

    HTTP.on("/upnp/control/basicevent1", HTTP_POST, []() {
      Serial.println("============ Responding to  /upnp/control/basicevent1 ... ============");      

      //for (int x=0; x <= HTTP.args(); x++) {
      //  Serial.println(HTTP.arg(x));
      //}
  
      String request = HTTP.arg(0);      
      Serial.print("Received request:");
      Serial.println(request);
 
      if(request.indexOf("<BinaryState>1</BinaryState>") > 0) {         //ON
          digitalWrite(LED_GREEN, LOW);
          digitalWrite(RELAY, HIGH);
          Serial.println("Got Turn on request");
      }

      if(request.indexOf("<BinaryState>0</BinaryState>") > 0) {         //OFF
          digitalWrite(LED_GREEN, HIGH);
          digitalWrite(RELAY, LOW);
          Serial.println("Got Turn off request");
      }
      
      HTTP.send(200, "text/plain", "");
    });

    HTTP.on("/eventservice.xml", HTTP_GET, [](){
      Serial.println("============ Responding to eventservice.xml ... ============\n");
      String eventservice_xml = "<?scpd xmlns=\"urn:Belkin:service-1-0\"?>"
            "<actionList>"
              "<action>"
                "<name>SetBinaryState</name>"
                "<argumentList>"
                  "<argument>"
                    "<retval/>"
                    "<name>BinaryState</name>"
                    "<relatedStateVariable>BinaryState</relatedStateVariable>"
                    "<direction>in</direction>"
                  "</argument>"
                "</argumentList>"
                 "<serviceStateTable>"
                  "<stateVariable sendEvents=\"yes\">"
                    "<name>BinaryState</name>"
                    "<dataType>Boolean</dataType>"
                    "<defaultValue>0</defaultValue>"
                  "</stateVariable>"
                  "<stateVariable sendEvents=\"yes\">"
                    "<name>level</name>"
                    "<dataType>string</dataType>"
                    "<defaultValue>0</defaultValue>"
                  "</stateVariable>"
                "</serviceStateTable>"
              "</action>"
            "</scpd>\r\n"
            "\r\n";
            
      HTTP.send(200, "text/plain", eventservice_xml.c_str());
    });
    
    HTTP.on("/setup.xml", HTTP_GET, [](){
      Serial.println(" ============ Responding to setup.xml ... ============\n");

      IPAddress localIP = WiFi.localIP();
      char s[16];
      sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
    
      String setup_xml = "<?xml version=\"1.0\"?>"
            "<root>"
             "<device>"
                "<deviceType>urn:Belkin:device:controllee:1</deviceType>"
                "<friendlyName>"+DeviceName+"</friendlyName>"
                "<manufacturer>Belkin International Inc.</manufacturer>"
                "<modelName>Emulated Socket</modelName>"
                "<modelNumber>3.1415</modelNumber>"
                "<UDN>uuid:"+ persistent_uuid +"</UDN>"
                "<serialNumber>221517K0101769</serialNumber>"
                "<binaryState>0</binaryState>"
                "<serviceList>"
                  "<service>"
                      "<serviceType>urn:Belkin:service:basicevent:0</serviceType>"
                      "<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>"
                      "<controlURL>/upnp/control/basicevent1</controlURL>"
                      "<eventSubURL>/upnp/event/basicevent1</eventSubURL>"
                      "<SCPDURL>/eventservice.xml</SCPDURL>"
                  "</service>"
              "</serviceList>" 
              "</device>"
            "</root>\r\n"
            "\r\n";
            
        HTTP.send(200, "text/xml", setup_xml.c_str());
        
        Serial.print("Sending :");
        Serial.println(setup_xml);
    });
    
    HTTP.begin();  
    Serial.println("HTTP Server started ..");
}


      
// connect to wifi – returns true if successful or false if not
//=================
boolean connectWifi()
//=================
{
  boolean state = true;
  boolean blink = false;
  int i = 0;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID.c_str(), Password.c_str());
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting ...");
  while (WiFi.status() != WL_CONNECTED) {
    if(blink)
        {
        blink = false; 
        digitalWrite(LED_GREEN, HIGH);
        }
    else
        {
        blink = true;
        digitalWrite(LED_GREEN, LOW);
        }      
    delay(500);
    Serial.print(".");
    if (i > 10){
      state = false;
      break;  
    }
    i++; 
  }
  
  if (state){
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("");
    Serial.println("Connection failed.");
  }
  
  return state;
}
//=================
boolean connectUDP()
//=================
{
  boolean state = false;
  
  Serial.println("");
  Serial.println("Connecting to UDP");
  
  if(UDP.beginMulticast(WiFi.localIP(), ipMulti, portMulti)) {
    Serial.println("Connection successful");
    state = true;
  }
  else{
    Serial.println("Connection failed");
  }
  
  return state;
}
