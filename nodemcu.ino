#include <ESP8266WiFi.h>
#include <Ticker.h>

////////////  step motor  //////////////////////////////////////////////////////////
// declare variables for the motor pins
int motorPin1 = 16;   // Blue   / Синий     - 28BYJ48 pin 1
int motorPin2 = 5;    // Pink   / Розовый   - 28BYJ48 pin 2
int motorPin3 = 4;    // Yellow / Желтый    - 28BYJ48 pin 3
int motorPin4 = 0;    // Orange / Оранжевый - 28BYJ48 pin 4

// установить скорость шагового двигателя.
int motorSpeed = 1200;

int lookup[8] = {
  B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001};

/////////////////////////////////////////////////////////////////////////////////////

//  конектимся к роутеру
const char* ssid = "kyivstar wi-fi";
const char* password = "dreamteam";

uint8_t blink_loop = 0;
uint8_t blink_mode = 0;
int sensorValue = 0;
int sensetive = 25;
int level = 500;
uint8_t i;

Ticker blinker;

const int upButtonPin = 12;
const int downButtonPin = 14;
int upButtonState = 0;
int downButtonState = 0;

const int modeButtonPin = 15;
int oldModeButton = 0;
int newModeButton = 0;
long buttonTime = 0;
long debounce = 200;

const int encoderAPin = 13;
const int encoderBPin = 3;
int oldEncoderPosition = HIGH;
int newEncoderPosition = HIGH;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

bool manual = 0;
int motorDirection = 2;
int val = 2;
int stepcount = 0;

//512 - оборот
void movetocw(int stepcount)
{
  manual = 1;
  for (int i = 0; i < stepcount; i++){
  clockwise();
  }
}

void movetoacw(int stepcount)
{
  manual = 1;
  for (int i = 0; i < stepcount; i++){
  anticlockwise();
  }
}

void timerIsr()
{
  if(blink_mode & 1<<(blink_loop&0x07)) digitalWrite(2, HIGH); 
    else  digitalWrite(2, LOW);
  blink_loop++;

  sensorValue = analogRead(A0);
  upButtonState = digitalRead(upButtonPin);
  downButtonState = digitalRead(downButtonPin);
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

void modeButtonAction()
{
  newModeButton = digitalRead(modeButtonPin);

  if (newModeButton == HIGH && oldModeButton == LOW && millis() - buttonTime > debounce)
  {
    manual = !manual;
    Serial.print("manual = "); Serial.print(manual); Serial.print("\n");
    buttonTime = millis();    
  }
  oldModeButton = newModeButton;
}

void encoderAction()
{
  
  newEncoderPosition = digitalRead(encoderAPin);
  if ((oldEncoderPosition == LOW) && (newEncoderPosition == HIGH))
  {
    if (digitalRead(encoderBPin) == LOW)
    {
       Serial.print ("left\n");
       movetoacw(23);
    }
    else
    {
      Serial.print ("right\n");
      movetocw(23);
    }
  }
  oldEncoderPosition = newEncoderPosition;
}

void motorAction()
  {
  if(manual == 0)
  {
    if(motorDirection == 0)
      if(upButtonState == HIGH)
        {motorZero();}
      else
        {anticlockwise();}
    
    if (motorDirection == 1)
      if (downButtonState == HIGH)
        {motorZero();}
      else
        {clockwise();}
    else
    {motorZero();}
  }        
return;
}

void photoresistorAction(int level, int sensetive)
{
   if (manual == 0)
   {
   if (sensorValue < level - sensetive)
    {motorDirection = 1;}
   else if (sensorValue > level + sensetive)
    {motorDirection = 0;}
   else
    {
      motorDirection = 2;
    }
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
  Serial.println("");
  Serial.println("");
  Serial.println("start");
//  delay(10);                        //check

  pinMode(2,OUTPUT);
  blink_mode = 0B11111110;  //blink_mode = 0B11111110; //количество срабатываний в период 1 раз из восьми тактов
  blinker.attach(0.250, timerIsr); //blinker.attach(0.125, timerIsr); //0.125 - время одного периода
  
  pinMode(upButtonPin, INPUT);
  pinMode(downButtonPin, INPUT);

  pinMode(encoderAPin, INPUT);
  pinMode(encoderBPin, INPUT);
  pinMode(modeButtonPin, INPUT);

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
    delay(500);                           //check
    Serial.print(".");
  }
  
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
}


void loop()
{

  encoderAction();
  modeButtonAction();
  photoresistorAction(level, sensetive);
  motorAction();

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

  else {
    String f = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html style=\"width:380px\">\r\n";
//    f += "<meta http-equiv=\"refresh\" content=\"1\">";
    f += "<fieldset> <legend>mode</legend>";
    f += "<form method=\"get\" action=\"\">";
    f += "<button style=\"width:100px;height:32px\" value=\"0\" type=\"submit\" name=\"send_m\">up</button><br><br>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"2\" type=\"submit\" name=\"send_m\">stop</button>";
    f += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"4\" type=\"submit\" name=\"send_m\">auto</button><br><br>";
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
    s += "<button style=\"width:100px;height:32px;margin-right:10px\" value=\"4\" type=\"submit\" name=\"send_m\">auto</button><br><br>";
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


