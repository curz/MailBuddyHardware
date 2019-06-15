#include <stdlib.h>
#include <SoftwareSerial.h>// import the serial library

#define CALIBRATION_SAMPLE_SIZE 1000
#define BLUETOOTH_MESSAGE_DELAY 100

String INFO_MAIL_STATUS = "mail_status";
String INFO_HALL_SENSOR_READING = "hall_reading";
String INFO_GENERAL_OUTPUT = "general_output";
String INFO_CONNECTION_STATUS = "connection_status";

float CALIBRATION_STEP_DURATION = 1000.0; // in ms
float CALIBRATION_TOTAL_DURATION = 5000.0; // in ms

int hallSensorPin = A5;
int ledPin = 13;
int hallData = 0;
int hallDataStandard;
int sensorTolerance = 5;
unsigned long currentTimeMillis = 0;
int updateRate = 100;
bool opened = false;
bool mailPresent = false;

//Configure bluetooth
char REQUEST_CHECK_MAIL = '1';
char REQUEST_TOGGLE_HALL_READ = '2';
char REQUEST_RESET = '3';
char REQUEST_TOGGLE_LED = '4';
char REQUEST_START_MONITORING = '5';

SoftwareSerial btModule(2,3);

int btData;
bool sendingHallData = false;
unsigned long lastHallDataMillis = 0;

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
  btModule.begin(9600);
  calibrate();
}

void loop() {
  if((unsigned long)(millis() - currentTimeMillis)> updateRate){ // Sleep for fixed duration
    currentTimeMillis = millis();
    
    hallData = analogRead(hallSensorPin);
    //btModule.println(analogRead(hallSensorPin));

    checkBluetooth();
    checkLid();
    if(sendingHallData){
      sendHallData();
    }
  }

  //Debugging
//  if(Serial.available()){
//    btModule.println(Serial.read());
//  }
//  if(btModule.available()){
//    Serial.println(btModule.read());
//  }
}

void sendHallData(){
  if((unsigned long)(millis() - lastHallDataMillis) > 500){
    lastHallDataMillis = millis();
    btModule.println(INFO_HALL_SENSOR_READING + ":" + hallData);
    delay(100);
  }
}

void checkBluetooth(){
  if(btModule.available()){
    btData = btModule.read();
    Serial.println(btData);
    Serial.println((String)"Mail present " + mailPresent);
    if(btData==REQUEST_CHECK_MAIL){
      btModule.println(INFO_MAIL_STATUS + ":" + mailPresent);
      delay(100);
    }else if(btData==REQUEST_TOGGLE_HALL_READ){
      sendingHallData = !sendingHallData;
      if(!sendingHallData){
        lastHallDataMillis = 0;
      }
    }else if(btData==REQUEST_RESET){
        resetFunc();
    }
    else if(btData==REQUEST_TOGGLE_LED){
      digitalWrite(ledPin, !digitalRead(ledPin));
      btModule.println("LED toggled!");
      delay(100);
    }
  }
  btModule.flush();
}

void checkLid(){
    // Start with closed lid
  if(abs(hallDataStandard-hallData) > sensorTolerance && !opened){ // if lid was opened aka hall sensor detected change
    digitalWrite(ledPin, HIGH);
      opened = true;
      mailPresent = true;
  }
}
void calibrate() {
  int startState[CALIBRATION_SAMPLE_SIZE];
  int currentState;
  int smallestState;
  int biggestState;
  unsigned long sumStates= 0;
  int countdown = CALIBRATION_TOTAL_DURATION/1000;

  Serial.print("Calibrating: ");
  Serial.println(countdown);
  Serial.println("s left");

  btModule.println(INFO_GENERAL_OUTPUT + ":" + "Calibrating: " + countdown + "s left");
  delay(100);

  for(int i = 0, k=0 ; i < CALIBRATION_SAMPLE_SIZE; i++, k++){
    if(currentTimeMillis == 0){
      currentTimeMillis = millis();
    }
    
    currentState = analogRead(hallSensorPin);
    if(i == 0){
      smallestState = currentState;
      biggestState = currentState;
    }
    
    startState[i] = currentState;
    sumStates += currentState;

    // keep track of smallest and biggest occuring sensor values
    if(currentState < smallestState) {
      smallestState = currentState;
    } else if (currentState > biggestState) {
      biggestState = currentState;
    }

    if(k >= CALIBRATION_SAMPLE_SIZE*CALIBRATION_STEP_DURATION/CALIBRATION_TOTAL_DURATION){ // Check if you have enough samples for the defined step duration
      while(millis() < currentTimeMillis + CALIBRATION_STEP_DURATION){} // Wait until the current step ended if necessary
      countdown--;
      Serial.println(countdown);
      btModule.println(INFO_GENERAL_OUTPUT + ":" + "Calibrating: " + countdown + "s left");
      currentTimeMillis = 0; // reset the timestamp
      k = 0; // reset counter for one step
    }

    digitalWrite(ledPin, LOW); // start with turned off LED
  }
  int deviation = biggestState - smallestState;
  if(deviation > sensorTolerance){
      sensorTolerance = deviation;
  }

  hallDataStandard = sumStates/CALIBRATION_SAMPLE_SIZE;
  
  Serial.println((String)"Collected samples: " + CALIBRATION_SAMPLE_SIZE);
  Serial.println((String)"Collected over a duration of: " + CALIBRATION_TOTAL_DURATION);
  Serial.println((String)"Calibration value: " + hallDataStandard);
  Serial.println((String)"Detected deviation: " + deviation);  
  Serial.println((String)"Chosen deviation: " + sensorTolerance);  
  
  btModule.println(INFO_GENERAL_OUTPUT + ":" + "Collected samples: " + CALIBRATION_SAMPLE_SIZE);
  delay(1000);
  btModule.println(INFO_GENERAL_OUTPUT + ":" + "Collected over a duration of: " + CALIBRATION_TOTAL_DURATION);
  delay(1000);
  hallDataStandard = sumStates/CALIBRATION_SAMPLE_SIZE;
  btModule.println(INFO_GENERAL_OUTPUT + ":" + "Calibration value: " + hallDataStandard);
  delay(1000);
  btModule.println(INFO_GENERAL_OUTPUT + ":" + "Detected deviation: " + deviation);  
  delay(1000);
  btModule.println(INFO_GENERAL_OUTPUT + ":" + "Chosen deviation: " + sensorTolerance);  
  delay(1000);
  btModule.println(INFO_GENERAL_OUTPUT + ":" + "Chosen deviation: " + sensorTolerance);  
}

  
