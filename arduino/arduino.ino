//#include <Servo.h>
#include <TimerOne.h>

#define ARDUINO_ID 0

#define RELAY_CONTROL 4 // relay za led sijalicu
#define DC_MOTOR 5 // pin preko kojeg se upravlja ventilatorom (dc motor)
#define POT A0 // potenciometar za dc motor
#define T_ULTRASONIC_SENSOR 9  // TRIGGER PIN koji salje zvucni signal
#define R_ULTRASONIC_SENSOR 10 // ECHO PIN koji prima zvucni signal
#define SERVO_MOTOR 6 // pin koji je u vezi sa servo motorom i kontrolise vrata garaze
#define TEMPERATURE_SENSOR A1 // pin na koji je vezan temperaturni senzor
#define PHOTORESISTOR A2 // pin na koji se vezan fotootpornik (za merenje osvetljenja)

typedef enum { // moguca stanja masine stanja
    SEND,
    RECEIVE,
    CONTROL,
    WAIT
} States;

States state;

//unsigned long dayDurationCounter = 4320000;
volatile unsigned int timeCounter = 0; // pocetna vrednost vremenskog brojaca
int timeToTriggerInt = 2000; // vreme u mikrosekundama na koje se desava vremenski prekid
int timeToSend = 30000; // kada treba poslati poruku serveru 30000 * 2000 = 60000000 = 1min

float speedOfSound = 0.0343; // brzina zvuka u cm/us

int relayState = HIGH; // pocetno stanje releja

int oldPotValue = 0; // pamtimo prethodno procitanu vrednost potenciometra

volatile int currentDistance = 10; // bilo koja vrednost veca od 5 da bi garaza bila zatvorena
int oldDistValue = -1; // ovo mozda nije neophodno???
int garageStatus = LOW; // LOW = zatvoreno, HIGH = otvoreno
bool openGarageRemotely = false;

//Servo servoControl; // objekat za kontrolu servo motora

// podaci za izvestaj
int relayTimesSwitched = 0;
int garageTimesOpened = 0;

void openGarage() {
  	garageTimesOpened++;
    //Serial.println("OTVARAM");
	  //servoControl.write(90);
    //digitalWrite(SERVO_MOTOR, HIGH);
    analogWrite(SERVO_MOTOR, 250);
   
}

void closeGarage() {
    //Serial.println("ZATVARAM");
	//servoControl.write(0);
  analogWrite(SERVO_MOTOR, 0);
}

float getTemperature() {
	int temperatureReading = analogRead(TEMPERATURE_SENSOR);
  	float temp = temperatureReading/1023.0f; // 0.0 - 1.0
  	// 1 stepen Celzijusa == 10mV
  	// 0 (0mV) - 1023(500mV) 1
  	temp = (temp * 5) * 100 - 50;
    float lm = temperatureReading/1023.0f * 500;
    // LM35 -> T = 500 * analogValue / 1023.0f
  	return lm;
}

int getLightAmount() {
	int lightReading = analogRead(PHOTORESISTOR);
  	lightReading = map(lightReading, 0, 310, 0, 100);
  	return lightReading; // vracamo % osvetljenosti
}

// vremenski prekid (koristimo za ultrasonic sensor i slanje podataka)
void timeInterrupt() {
  timeCounter++;
  
  switch (state) {
    case SEND: {
        digitalWrite(T_ULTRASONIC_SENSOR, HIGH);
        if (timeCounter % 4 == 0) {
            state = RECEIVE;
        }
        break;
    }
    case RECEIVE: {
        digitalWrite(T_ULTRASONIC_SENSOR, LOW);
        unsigned long duration = pulseIn(R_ULTRASONIC_SENSOR, HIGH);
        currentDistance = (int)(speedOfSound * duration * 0.5);
        state = CONTROL;
        break;
    }
    case CONTROL: {
      if ( currentDistance != oldDistValue ) { // ovo mozda ne treba
    
        if ( (currentDistance <= 5) || openGarageRemotely ) {
          if ( garageStatus == LOW ) {
            // otvori
            //openGarage();
            analogWrite(SERVO_MOTOR, 250);
            garageStatus = HIGH;
          }
        }
        else if ( (currentDistance > 5) || !openGarageRemotely) {
           if ( garageStatus == HIGH ) {
            // zatvori
            //closeGarage();
            analogWrite(SERVO_MOTOR, 0);
            garageStatus = LOW;
          }
        }
      }


      state = WAIT;
    }
    case WAIT: {
        if ( timeCounter % 500 == 0 ) {
            state = SEND;
        }
        break;
    }
  }

  //NOTE: OVA 2 IF-A MOZDA IZMESTITI U CONTROL CASE ?????????????????????
  if (timeCounter % 3000 == 0) {
    //Serial.println( "TEMP: " + String(getTemperature()) );
    //Serial.println( "LIGHT: " + String(getLightAmount()) );
  }
  
  if ( timeCounter >= timeToSend ) {
  	// salje se na veb serveru svakog minuta
    sendMessage();
    timeCounter = 0;
  }
  
}

void sendMessage() {
  	// format poruke koja se salje serveru: ARDUINO_ID:SENSOR:VAL;
	  float temp = getTemperature();
  	int light = getLightAmount();

  	String msg = "#" + String(ARDUINO_ID) + ":temperature:" + String(temp) + ";" +
                 "#" + String(ARDUINO_ID) + ":photoresistor:" + String(light) + ";" +
                 "#" + String(ARDUINO_ID) + ":garage:" + String(garageTimesOpened) + ";" +
                 "#" + String(ARDUINO_ID) + ":relay:" + String(relayTimesSwitched) + ";" ;
  	Serial.println(msg);
}

void setup()
{
  Serial.begin(9600);
  pinMode(RELAY_CONTROL, OUTPUT);
  pinMode(DC_MOTOR, OUTPUT);
  
  pinMode(T_ULTRASONIC_SENSOR, OUTPUT);
  pinMode(R_ULTRASONIC_SENSOR, INPUT);
  
  state = SEND;
  
  digitalWrite(RELAY_CONTROL, relayState);
  
  //servoControl.attach(SERVO_MOTOR);
  pinMode(SERVO_MOTOR, OUTPUT);
  
  closeGarage();
  
  Timer1.initialize(timeToTriggerInt); //(u mikrosekundama)
  Timer1.attachInterrupt(timeInterrupt);
}

float getDistance() {
  digitalWrite(T_ULTRASONIC_SENSOR, LOW);
  delayMicroseconds(2);
  digitalWrite(T_ULTRASONIC_SENSOR, HIGH);
  delayMicroseconds(10);
  digitalWrite(T_ULTRASONIC_SENSOR, LOW);
  
  float echoValue = pulseIn(R_ULTRASONIC_SENSOR, HIGH);
  return echoValue * 0.0343 * 0.5;
}

void loop()
{
  // MESSAGE FORMAT -> ARDUINO_ID:R/W:PIN:VALUE[:DIR];
  if (Serial.available() > 0) {
    
  	String msg = Serial.readStringUntil(';');
    
    int firstSeparator = msg.indexOf(':');
    String arduinoId = msg.substring(0, firstSeparator);
    int aId = arduinoId.toInt();
    
    if ( firstSeparator > 0 && aId == ARDUINO_ID ) {
      
      int secondSeparator = msg.indexOf(':', firstSeparator+1);
      int thirdSeparator = msg.indexOf(':', secondSeparator+1);
      int fourthSeparator = msg.indexOf(':', thirdSeparator+1);
    
      String rw = String( msg.charAt((firstSeparator+1)) );
      rw.toUpperCase();
      String pinValue = msg.substring(secondSeparator+1, thirdSeparator);
      String commandValue = "";
      String dir = "";
      if (fourthSeparator == -1) {
      	commandValue = msg.substring(thirdSeparator+1);
      }
      else {
      	commandValue = msg.substring(thirdSeparator+1, fourthSeparator);
        dir = msg.substring(fourthSeparator+1);
      }
      commandValue.toUpperCase();

      int pin = pinValue.toInt();
      //int command = commandValue.toInt(); // ovo po potrebi u if-ovima
  	  
      // RELAY CONTROL (serial)  PIN 4
      if ( pin == RELAY_CONTROL && rw == "W" ) {
          relayTimesSwitched++;
          relayState = !relayState;
          digitalWrite(RELAY_CONTROL, relayState);
      }
      
      // DC MOTOR serial  PIN 5
      else if ( pin == DC_MOTOR && rw == "W" ) {
      	int command = commandValue.toInt();
        if ( command <= 100 && command >= 0) {
        	int dcValue = map(command, 0, 100, 0, 255);
        	analogWrite(DC_MOTOR, dcValue);
        }
      }
      
      // GARAGE (SERVO)  PIN 6
      else if ( pin == SERVO_MOTOR && rw == "W" ) {
        
        if ( commandValue == "OPEN" ) {
        	openGarageRemotely = true;
        }
        else if ( commandValue == "CLOSE" ) {
        	openGarageRemotely = false;
        }
        
      }
      
    }
    
    
  }
  // DC MOTOR
  int delta = 4;
  int potValue = analogRead(POT);
  potValue = map(potValue, 0, 1023, 0, 255);
  if (   !(  (potValue >= (oldPotValue-delta) ) && (potValue <= (oldPotValue+delta) )      )    ) {
  	analogWrite(DC_MOTOR, potValue);
  	oldPotValue = potValue;
  }
  
  // ULTRASONIC SENSOR
  //currentDistance = (int)getDistance(); // current distance preko vremenskog prekida
//  if ( currentDistance != oldDistValue ) {
//    
//    if ( (currentDistance <= 5) || openGarageRemotely ) {
//      if ( garageStatus == LOW ) {
//        // otvori
//        openGarage();
//        garageStatus = HIGH;
//      }
//    }
//    else if ( (currentDistance > 5) || !openGarageRemotely) { // mozda samo else?
//     	if ( garageStatus == HIGH ) {
//        // zatvori
//        closeGarage();
//        garageStatus = LOW;
//      }
//    }
//  }
 
  //Serial.println( "TEMP: " + String(getTemperature()) );
  //Serial.println( "LIGHT: " + String(getLightAmount()) );
   
}