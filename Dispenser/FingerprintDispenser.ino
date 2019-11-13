#include <Servo.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

int addressMax = 0;

SoftwareSerial mySerial(2, 3);

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
uint8_t id = 1;

Servo servoTop;
Servo servoBot;

//Servo Pins
int servoPinT = 9;
int servoPinB = 10;
//ButtonPins
//int ledPin[] = {5,6};
//int buttonPin[] = {4,8};
int enrollLED = 6;
int enrollButton = 8;
int matchLED = 5;
int matchButton = 4;

//Button State
//int buttonVal = 0;
//int currButton = 0;
int matchState = 0;
int enrollState = 0;

void setup() {
  pinMode(enrollLED, OUTPUT);
  pinMode(matchLED, OUTPUT);
  pinMode(enrollButton, INPUT_PULLUP);
  pinMode(matchButton, INPUT_PULLUP);
      
  finger.begin(57600);
  Serial.begin(9600);
  Serial.print("setup");
  if (!finger.verifyPassword()) {
    while (1) { delay(1); }
  }
}

//Error Signal
void errorBlink(){
  for(int i=0; i<4; i++){
    digitalWrite(enrollLED, HIGH);
    digitalWrite(matchLED, HIGH);
    delay(250);
    digitalWrite(enrollLED, LOW);
    digitalWrite(matchLED, LOW);
  }
  loop();
}

//Denied Signal
void deniedBlink(){
  for(int i=0; i<4; i++){
    digitalWrite(matchLED, HIGH);
    delay(250);
    digitalWrite(matchLED, LOW);
  }
  loop();
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return p;
  // OK success!
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return p;  
  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  Serial.print("starter");
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;
  Serial.print("imaging");
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;
  Serial.print("double tap");
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  Serial.print("search");
  // found a match!
  return finger.fingerID; 
}

//Dispense Method
void dispense(){
  servoTop.write(70);
  delay(2000);
  servoBot.write(160);
  delay(4000);
  servoBot.write(70);
  delay(100);
  servoTop.write(160);

  loop();
}

//Find Fingerprint
void matchCall(){
  int fPrint = -1;
  int matchWait = millis();
 
  while(fPrint == -1){
    fPrint = getFingerprintIDez();
    delay(50);
    if (millis()-matchWait > 30000){
      loop();
    }
  }
  int i = 1;
  int address = 0;
  int idVal;
  int limVal;
  while(i<id && address < addressMax){
    idVal = EEPROM.read(address);
    if(idVal == fPrint){
      break;
    }
    address += 2;
    i++;
  }
  if(idVal < addressMax){
    limVal = EEPROM.read(address+1);
    if(limVal == 0){
      deniedBlink();
    }else{
      EEPROM.write(address+1, 0);
      servoTop.attach(servoPinT);
      dispense();
    }
  }
}

// Add Fingerprint
uint8_t getFingerprintEnroll() {
  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }
  p = finger.image2Tz(1);
  if(p != FINGERPRINT_OK)  return p;
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }
  p = finger.image2Tz(2);
  if(p != FINGERPRINT_OK)  return p;
  p = finger.createModel();
  if(p != FINGERPRINT_OK)  return p;
  p = finger.storeModel(id);
  if(p != FINGERPRINT_OK)  return p;

  EEPROM.write((id-1)*2, id);
  EEPROM.write(((id-1)*2) + 1, 1);
  id++;
  addressMax = (id-1)*2;
}

void loop() {
  servoTop.detach();
  int runtime = millis();
  if(id > 127 || runtime == 0){
    finger.emptyDatabase();
    id = 1;
    addressMax = 0;
  }

  //Time Until User can get more Candy
  if(runtime%60000==0 && runtime != 0){ //Set to 1 min for demo purposes
    int i;
    for(i=1; i<id; i++){
      EEPROM.write(((i-1)*2)+1, 1);
    }
  }
  Serial.print("loop1");
  matchState = digitalRead(matchButton);
  if(matchState == LOW){
    Serial.print("match cond");
    digitalWrite(matchLED, LOW);
    digitalWrite(enrollLED, LOW);
  
    matchCall();
  } else {
    digitalWrite(matchLED, HIGH);
  }
  
  enrollState = digitalRead(enrollButton);
  if(enrollState == LOW){
    Serial.print("enroll cond");
    digitalWrite(enrollLED, LOW);
    digitalWrite(matchLED, LOW);
  
    while(! getFingerprintEnroll());
  } else {
    digitalWrite(enrollLED, HIGH);
  }
  delay(250);
}


