int ledPin   = 13;
int relayPin = 7;
int dt = 1000;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(ledPin, OUTPUT);     
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
}

// the loop routine runs over and over again forever:
void loop() {
  digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(dt);               // wait for a second
  digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
  delay(dt);               // wait for a second
  dt = dt - 25;
  if (dt <= 25) digitalWrite(relayPin, LOW);
  }
