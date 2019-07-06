#define StepPin 2 // ANY TOGGLE is step motor
#define DirectionPin 3 // LOW is forward, HIGH is reverse
#define ModeSet1Pin 4
#define ModeSet2Pin 5
#define EnablePin 6 // HIGH is Disabled, LOW is Enabled

void setup() {
  pinMode(StepPin, OUTPUT);
  pinMode(DirectionPin, OUTPUT);
  pinMode(ModeSet1Pin, OUTPUT);
  pinMode(ModeSet2Pin, OUTPUT);
  pinMode(EnablePin, OUTPUT);
  
  digitalWrite(DirectionPin, LOW);
  digitalWrite(EnablePin, LOW);
  digitalWrite(ModeSet1Pin, LOW);
  digitalWrite(ModeSet2Pin, LOW);
}

void loop() {
  digitalWrite(StepPin, HIGH);
  delay(1);
  digitalWrite(StepPin, LOW);
  delay(1);
}
