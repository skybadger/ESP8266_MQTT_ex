/*
 It connects to an MQTT server then:
  - on 0 switches off relay
  - on 1 switches on relay
  - on 2 switches the state of the relay

  - sends 0 on off relay
  - sends 1 on on relay

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 The current state is stored in EEPROM and restored on bootup
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h> //https://pubsubclient.knolleary.net/api.html
#include <EEPROM.h>
#include <Wire.h> //https://playground.arduino.cc/Main/WireLibraryDetailedReference
#include <Time.h> //Look at https://github.com/PaulStoffregen/Time for a more useful internal timebase library
#include <WiFiUdp.h>
//#include <Bounce2.h>
//#include <OneWire.h>

IPAddress timeServer(193,238,191,249); // pool.ntp.org NTP server
unsigned long NTPseconds; //since 1970

//Strings
const char* ssid = "Skybadger Away";
const char* password = "This is Skybadger Away";
const char* pubsubUserID = "publicbadger";
const char* pubsubUserPwd = "alltooeasy";
const char* mqtt_server = "192.168.1.136";
const char* thisID = "BadgerIoT1";
const char* outTopic = "Skybadger/IoT/Devices/";
const char* inTopic = "Skybadger/IoT/Devices/#";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
unsigned int localPort = 2390;      // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

void setup_wifi()
{
  delay(10);
   
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.hostname( "BadgerSens01");
  WiFi.begin(ssid, password);
  Serial.print("Searching for WiFi..");
  while (WiFi.status() != WL_CONNECTED) 
  {
    for(int i = 0; i<500; i++)
    {
      delay(1);
      Serial.print(".");
    }
    Serial.println("");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //Listen for UDP Packet
  Udp.begin(localPort);
  
  //Open a connection to MQTT
  client.connect(thisID, pubsubUserID, pubsubUserPwd );
  
}


/* MQTT callback for subscription and topic.
 * Only respond to valid states ""
 */
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String temp;
  char output[64];
  
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  //post output to subscribing client using the same process.
  if( (char)payload[0] == '0') 
  {
    //digitalWrite(relay_pin, LOW);   // Turn the LED on (Note that LOW is the voltage level
    Serial.println("Received '0' ");
    //EEPROM.write(0, relayState);    // Write state to EEPROM
    //EEPROM.commit();
  }
  else if ((char)payload[0] == '1') 
  {
    Serial.println("Received '1'" );
    //EEPROM.write(0, relayState);    // Write state to EEPROM
    //EEPROM.commit();
    // Do something about querying the I2C devices based on a mqtt query 
    checkTime();
    //read devices
    //publish to our device topic
    //https://www.arduino.cc/reference/en/language/variables/data-types/stringobject/
    temp = "{\"sensor\": \"temperature\", \"time\": 1234567890, \"value\": 567890 }";
    temp.toCharArray(output, temp.length() );
    client.publish( outTopic, output );
  } 
  else if ((char)payload[0] == '2') 
  {
    Serial.print("Received '2' ");
    //EEPROM.write(0, relayState);    // Write state to EEPROM
    //EEPROM.commit();
  }
}

void reconnect() 
{
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(thisID, pubsubUserID, pubsubUserPwd )) 
    {
      /*
      char* topic = (char*) malloc(sizeof(char) * (sizeof(outTopic) + sizeof("/") + sizeof(thisID) + 1) );
      memcpy( topic, outTopic, sizeof(outTopic)-1);
      memcpy( &topic[sizeof(outTopic)], "/", 1);
      memcpy( &topic[sizeof(outTopic)+1], thisID, sizeof( thisID));
      */            
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish( outTopic, "BadgerIoT1 connected");
      // ... and resubscribe
      client.subscribe(inTopic);
    }
    else
    {
     // Serial.print("failed, rc=");
      Serial.print(client.state());
      /*
Returns the current state of the client. If a connection attempt fails, this can be used to get more information about the failure.
int - the client state, which can take the following values (constants defined in PubSubClient.h):
-4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
-3 : MQTT_CONNECTION_LOST - the network connection was broken
-2 : MQTT_CONNECT_FAILED - the network connection failed
-1 : MQTT_DISCONNECTED - the client is disconnected cleanly
0 : MQTT_CONNECTED - the client is connected
1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
       */
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      for(int i = 0; i<5000; i++)
      {
        delay(1);
        //delay(20);
        //yield();
      }
    }
  }
}

void setup() 
{
  EEPROM.begin(512);              // Begin eeprom to store on/off state
/* For  the time when these devices are used to control a relay.. 
 *  probably via a io expander and PNP drivers for 8 relays. 
  pinMode(relay_pin, OUTPUT);     // Initialize the relay pin as an output
  pinMode(button_pin, INPUT);     // Initialize the relay pin as an output
  pinMode(13, OUTPUT);
  relayState = EEPROM.read(0);
  digitalWrite(relay_pin,relayState);  
 */
  // Device is an ESP8266-01 which has only 4 output pins, two are used to do serial i/f and remaining two take part in programming 
  // so we are careful in choice of pins and direction/states
  
  Serial.begin( 115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println();
  
  //Pins mode and direction setup
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, INPUT_PULLUP);
  
  //I2C setup SDA pin 0, SCL pin 2
  Wire.begin(0, 2);
  
  // Connect to wifi 
  setup_wifi();                   
  client.setServer(mqtt_server, 1883);
  
  //Create a timer-based callback that queries the i2C bus devices for data to publish.
  client.setCallback(callback);
}

void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
}

void checkTime()
{
  // send an NTP packet to a time server
  sendNTPpacket(timeServer); 
  
  // wait to see if a reply is available
  if (Udp.parsePacket()) 
  {
    Serial.println("packet received");
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = ");
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if (((epoch % 3600) / 60) < 10) 
    {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ((epoch % 60) < 10) 
    {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address) 
{
  //Serial.println("1");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  //Serial.println("2");
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  Serial.println("3");

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Serial.println("4");
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Serial.println("5");
  Udp.endPacket();
  Serial.println("6");
}

