///////////////////////// TODO //////////////////////////////////////////////////
//
//  перекодить для esp8266
//
//
//
//
//
//
//
//
//
/////////////////////////////////////////////////////////////////////////////////////

#include <Ticker.h>

////////////  step motor  //////////////////////////////////////////////////////////
// declare variables for the motor pins
int motorPin1 = 8;    // Blue   / Синий     - 28BYJ48 pin 1
int motorPin2 = 9;    // Pink   / Розовый   - 28BYJ48 pin 2
int motorPin3 = 10;   // Yellow / Желтый    - 28BYJ48 pin 3
int motorPin4 = 11;   // Orange / Оранжевый - 28BYJ48 pin 4

// установить скорость шагового двигателя (задержка в ms).
int motorSpeed = 800;  //800-16000

int lookup[8] = {
  B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001};

/////////////////////////////////////////////////////////////////////////////////////

uint8_t blink_loop = 0;
uint8_t blink_mode = 0;

int sensorValue = 0;
int sensetive = 50;
int level = 300;

const int upButtonPin = 2;
const int downButtonPin = 3;
int upButtonState = 0;
int downButtonState = 0;


const int modeButtonPin = 7;
int oldModeButton = 0;
int newModeButton = 0;
long time = 0;
long debounce = 200;


const int encoderAPin = 12;
const int encoderBPin = 13;
int oldEncoderPosition = HIGH;
int newEncoderPosition = HIGH;

bool manual = 0;
int motorDirection = 2;
int stepcount = 0;

Ticker timer1;
Ticker blinker;

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
  sensorValue = analogRead(A5);
  upButtonState = digitalRead(upButtonPin);
  downButtonState = digitalRead(downButtonPin);
}

void debug()
{
  Serial.print("photo = "); Serial.print(sensorValue); Serial.print("     manual = "); Serial.print(manual); Serial.print("\n");
  Serial.print("time = "); Serial.print(time); Serial.print("\n");
//  Serial.print("old = "); Serial.print(digitalRead(oldEncoderPosition)); Serial.print("     new = "); Serial.print(digitalRead(newEncoderPosition));Serial.print("\n");
}

void modeButtonAction()
{
  newModeButton = digitalRead(modeButtonPin);

  if (newModeButton == HIGH && oldModeButton == LOW && millis() - time > debounce)
  {
    manual = !manual;
    Serial.print("manual = "); Serial.print(manual); Serial.print("\n");
    time = millis();    
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
void blinkIsr()
{
  if(blink_mode & 1<<(blink_loop&0x07)) digitalWrite(6, HIGH); 
    else digitalWrite(6, LOW);
  blink_loop++;
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
  Serial.println("start");

  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  digitalWrite(5, LOW);

  timer1.setCallback(timerIsr);
  timer1.setInterval(50);
  timer1.start();

  blink_mode = 0B00000001;  //blink_mode = 0B00000001; //количество срабатываний в период 1 раз из восьми тактов
  blinker.setCallback(blinkIsr); //blinker.attach(0.125, timerIsr); //0.125 - время одного периода
  blinker.setInterval(150);
  blinker.start();

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
  
}

void loop()
{
    timer1.update();
    blinker.update();
    encoderAction();
    modeButtonAction();
    photoresistorAction(level, sensetive);
    motorAction();
}


