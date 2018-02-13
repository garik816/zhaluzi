#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WiFiUdp.h>    //ntp

int sec = 0;
int ts = 0;
int h = 0;
int m = 0;

////////////  step motor  //////////////////////////////////////////////////////////
// declare variables for the motor pins
int motorPin1 = 16;   // Blue   / Синий     - 28BYJ48 pin 1
int motorPin2 = 5;    // Pink   / Розовый   - 28BYJ48 pin 2
int motorPin3 = 4;    // Yellow / Желтый    - 28BYJ48 pin 3
int motorPin4 = 0;    // Orange / Оранжевый - 28BYJ48 pin 4

// установить скорость шагового двигателя.
//variable to set stepper speed.
int motorSpeed = 1200;  

int lookup[8] = {
  B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001};

/////////////////////////////////////////////////////////////////////////////////////

//  конектимся к роутеру
const char* ssid = "kyivstar wi-fi";
const char* password = "dreamteam";

unsigned int localPort = 2390;                  //ntp
IPAddress timeServerIP;                         //ntp
const char* ntpServerName = "time.nist.gov";    //ntp
const int NTP_PACKET_SIZE = 48;                 //ntp
byte packetBuffer[ NTP_PACKET_SIZE];            //ntp
WiFiUDP udp;                                    //ntp

uint8_t   blink_loop = 0;
uint8_t   blink_mode = 0;
int   sensorValue = 0;
int   sensetive = 25;
int   level = 500;
uint8_t i;

Ticker blinker;
Ticker getWebTime;
const int upButtonPin = 12;
const int downButtonPin = 14;
int upButtonState = 0;
int downButtonState = 0;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

int manual = 0;
int motorDirection = 2;
int val = 2;

void timerIsr()
{
  if(  blink_mode & 1<<(blink_loop&0x07) ) digitalWrite(2, HIGH); 
    else  digitalWrite(2, LOW);
  blink_loop++;

  sensorValue = analogRead(A0);
  upButtonState = digitalRead(upButtonPin);
  downButtonState = digitalRead(downButtonPin);
}

void unixTimeToTime(int ts)
{
  sec = ts%86400;
  ts /= 86400;
  h = (sec/3600)+2;
  m = sec/60%60;
  sec = sec%60;
}

void getTime()
{
unixTimeToTime(ts);
}

// функция поворачивает мотор против часовой стрелки.
void anticlockwise()
{
  for(int i = 0; i < 8; i++)
  {
    setOutput(i);
    delayMicroseconds(motorSpeed);
  }
}

// функция поворачивает мотор по часовой стрелке.
void clockwise()
{
  for(int i = 7; i >= 0; i--)
  {
    setOutput(i);
    delayMicroseconds(motorSpeed);
  }
}

void setOutput(int out)
{
  digitalWrite(motorPin1, bitRead(lookup[out], 0));
  digitalWrite(motorPin2, bitRead(lookup[out], 1));
  digitalWrite(motorPin3, bitRead(lookup[out], 2));
  digitalWrite(motorPin4, bitRead(lookup[out], 3));
}


void   motorAction()
  {
  if(motorDirection == 0 || val == 0)
    if(upButtonState == HIGH)
      {motorZero();}
    else
      {anticlockwise();}
  
  if (motorDirection == 1 || val == 1)
    if (downButtonState == HIGH)
      {motorZero();}
    else
      {clockwise();}
  else
  {motorZero();}
        
return;
}

void photoresistorAction(int level, int sensetive)
{
   if (sensorValue < level - sensetive)
    {motorDirection = 1;}
   else if (sensorValue > level + sensetive)
    {motorDirection = 0;}
   else
    {
      motorDirection = 2;
      return;
    }
}

void motorZero()
{
  digitalWrite(motorPin1, 0);
  digitalWrite(motorPin2, 0);
  digitalWrite(motorPin3, 0);
  digitalWrite(motorPin4, 0);
}

void setup()
{
  Serial.begin(115200);
//  delay(10);                        //check

  pinMode(2,OUTPUT);
  blink_mode = 0B11111110;  //blink_mode = 0B11111110; //количество срабатываний в период 1 раз из восьми тактов
  blinker.attach(0.250, timerIsr); //blinker.attach(0.125, timerIsr); //0.125 - время одного периода
  
  getWebTime.attach(15, getTime); //check
  
  pinMode(upButtonPin, INPUT);
  pinMode(downButtonPin, INPUT);

// двигло
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  // конектимся к роутеру
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);                           //check
    Serial.print(".");
  }
  
  Serial.println("WiFi connected");
  udp.begin(localPort);

  
  // Start the server
  server.begin();
}

unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


void loop()
{

  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
//  delay(1000);                                 //check
  
  int cb = udp.parsePacket();
//  if (!cb) {                                  //check
//    Serial.println("no packet yet");
//  }

  while(!cb){                                   //check
    delay(1);
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);
    ts=epoch;


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }


  
     

//  sensorValue = analogRead(A0);   //опрос АЦП (фоторезистор)
//  float voltage = sensorValue * (5.0 / 1023.0); //перевод данных в напряжение
 if (manual == 0)
 {
  photoresistorAction(level, sensetive);
 }
 motorAction();

  if (h >= 0 && h <= 5){level=100;}
  if (h >= 6 && h <= 7){level=200;}
  if (h >= 8 && h <= 9){level=400;}
  if (h >= 10 && h <= 12){level=600;}
  if (h >= 13 && h <= 14){level=700;}
  if (h==15){level=800;}
  if (h==16){level=500;}
  if (h==17){level=300;}
  if (h==18){level=200;}
  if (h >= 19 && h <= 23){level=100;}
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  client.flush();

  // Match the request (up)
  if (req.indexOf("/?send_m=0") != -1)
  {
    val = 0;
    manual = 1;
    motorDirection = 0;
  }
  // Match the request (down)
  else if (req.indexOf("/?send_m=1") != -1)
  {
    val = 1;
    manual = 1;
    motorDirection = 1;
  }
  // Match the request (stop)
  else if (req.indexOf("/?send_m=2") != -1)
  {
    val = 2;
    motorDirection = 2;
    motorZero();
    manual = 1;
  }
  // Match the request (auto)
    else if (req.indexOf("/?send_m=4") != -1){manual = 0;}
    
    else if (req.indexOf("/?send_l=0") != -1){level=0;}
    else if (req.indexOf("/?send_l=50") != -1){level=50;}
    else if (req.indexOf("/?send_l=100") != -1){level=100;}
    else if (req.indexOf("/?send_l=200") != -1){level=200;}
    else if (req.indexOf("/?send_l=300") != -1){level=300;}  
    else if (req.indexOf("/?send_l=400") != -1){level=400;}
    else if (req.indexOf("/?send_l=510") != -1){level=510;}
    else if (req.indexOf("/?send_l=600") != -1){level=600;}
    else if (req.indexOf("/?send_l=700") != -1){level=700;}
    else if (req.indexOf("/?send_l=800") != -1){level=800;}
    else if (req.indexOf("/?send_l=900") != -1){level=900;}
    else if (req.indexOf("/?send_l=999") != -1){level=999;}
    
    else if (req.indexOf("/?send_s=9") != -1){sensetive=9;}
    else if (req.indexOf("/?send_s=25") != -1){sensetive=25;}
    else if (req.indexOf("/?send_s=50") != -1){sensetive=50;}
    
    else if (req.indexOf("/?send_time=1") != -1){getTime();}

  else {
    String f = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html style=\"width:380px\">\r\n";
//    f += "<meta http-equiv=\"refresh\" content=\"1\">";
    f += "<fieldset> <legend>mode</legend>";
    f += "<form method=\"get\" action=\"\">";
    f += "<button style=\"width:100px;height:32px\" value=\"0\" type=\"submit\" name=\"send_m\">up</button><br><br>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"2\" type=\"submit\" name=\"send_m\">stop</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"4\" type=\"submit\" name=\"send_m\">auto</button>";
    f += "<button style=\"width:100px;height:32px\" value=\"1\" type=\"submit\" name=\"send_time\">time</button><br><br>";
    f += "<button style=\"width:100px;height:32px\" value=\"1\" type=\"submit\" name=\"send_m\">down</button>";
    f += "</form></fieldset>";
    f += "<fieldset><legend>level</legend>";
    f += "<form method=\"get\" action=\"\">";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"0\" type=\"submit\" name=\"send_l\">level 0</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"50\" type=\"submit\" name=\"send_l\">level 50</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"100\" type=\"submit\" name=\"send_l\">level 100</button><br><br>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"200\" type=\"submit\" name=\"send_l\">level 200</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"300\" type=\"submit\" name=\"send_l\">level 300</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"400\" type=\"submit\" name=\"send_l\">level 400</button><br><br>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"510\" type=\"submit\" name=\"send_l\">level 510</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"600\" type=\"submit\" name=\"send_l\">level 600</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"700\" type=\"submit\" name=\"send_l\">level 700</button><br><br>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"800\" type=\"submit\" name=\"send_l\">level 800</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"900\" type=\"submit\" name=\"send_l\">level 900</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"999\" type=\"submit\" name=\"send_l\">level 999</button>";
    f += "</form></fieldset>";
    f += "<fieldset><legend>sens</legend>";
    f += "<form method=\"get\" action=\"\">";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"9\" type=\"submit\" name=\"send_s\">sens 9</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"25\" type=\"submit\" name=\"send_s\">sens 25</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"50\" type=\"submit\" name=\"send_s\">sens 50</button>";
    f += "</form></fieldset>";
    f += "<fieldset><legend>info</legend>";
    f += "level+/-sens = ";
    f += level;
    f += "+/-";
    f += sensetive;
    f += "<br>sensor value = ";
    f += sensorValue;
    f += "<br>Time: ";
    f += h;
    f += ":";
    f += m;
    f += ":";
    f += sec;
    f += "<br>voltage value = ";
    float voltage = sensorValue * (3.3 / 1023.0);
    f += voltage;  
    f += "</html>\n";
    client.print(f);    
    client.stop();
    return;
  }
  
  client.flush();

  // Prepare the response
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html style=\"width:380px\">\r\n";
//    s += "<meta http-equiv=\"refresh\" content=\"1\">";
    s += "<style media=\"handheld\"> body {max-width: 320px;} </style>";
    s += "<fieldset> <legend>mode</legend>";
    s += "<form method=\"get\" action=\"\">";
    s += "<button style=\"width:100px;height:32px\" value=\"0\" type=\"submit\" name=\"send_m\">up</button><br><br>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"2\" type=\"submit\" name=\"send_m\">stop</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"4\" type=\"submit\" name=\"send_m\">auto</button>";
    s += "<button style=\"width:100px;height:32px\" value=\"1\" type=\"submit\" name=\"send_time\">time</button><br><br>";
    s += "<button style=\"width:100px;height:32px\" value=\"1\" type=\"submit\" name=\"send_m\">down</button>";
    s += "</form></fieldset>";
    s += "<fieldset><legend>level</legend>";
    s += "<form method=\"get\" action=\"\">";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"0\" type=\"submit\" name=\"send_l\">level 0</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"50\" type=\"submit\" name=\"send_l\">level 50</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"100\" type=\"submit\" name=\"send_l\">level 100</button><br><br>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"200\" type=\"submit\" name=\"send_l\">level 200</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"300\" type=\"submit\" name=\"send_l\">level 300</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"400\" type=\"submit\" name=\"send_l\">level 400</button><br><br>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"510\" type=\"submit\" name=\"send_l\">level 510</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"600\" type=\"submit\" name=\"send_l\">level 600</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"700\" type=\"submit\" name=\"send_l\">level 700</button><br><br>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"800\" type=\"submit\" name=\"send_l\">level 800</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"900\" type=\"submit\" name=\"send_l\">level 900</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"999\" type=\"submit\" name=\"send_l\">level 999</button>";
    s += "</form></fieldset>";
    s += "<fieldset><legend>sens</legend>";
    s += "<form method=\"get\" action=\"\">";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"9\" type=\"submit\" name=\"send_s\">sens 9</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"25\" type=\"submit\" name=\"send_s\">sens 25</button>";
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"50\" type=\"submit\" name=\"send_s\">sens 50</button>";
    s += "</form></fieldset>";
    s += "<fieldset><legend>info</legend>";
    s += "level+/-sens = ";
    s += level;
    s += "+/-";
    s += sensetive;
    s += "<br>sensor value = ";
    s += sensorValue;
    s += "<br>Time: ";
    s += h;
    s += ":";
    s += m;
    s += ":";
    s += sec;
    s += "<br>voltage value = ";
    float voltage = sensorValue * (3.3 / 1023.0);
    s += voltage;
    s += "</html>\n";
   
  // Send the response to the client
  client.print(s);
  delay(1);

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}


