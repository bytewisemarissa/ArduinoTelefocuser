#include <Servo.h>

#define INPUT_SIZE 4
#define SERVO_STOP 90

int servoOffset = 3;
int servoPin = 3;
int incomingValue;
int inputSize;
int tokenCount;
int speedOffset;
Servo focusServo;
byte inputBytes[INPUT_SIZE];

void setup() {
	Serial.begin(115200, SERIAL_8N1);
	Serial.flush();

	focusServo.attach(servoPin);
	focusServo.write(SERVO_STOP + servoOffset);//stop
	
	Serial.write("Focuser Started Up#");
}

void loop() {  
  if(Serial.available() > 0)
  {
	  inputSize = Serial.readBytesUntil('#', inputBytes, INPUT_SIZE);//pull command from serial buffer

	  if(inputBytes[0] != 240)
	  {
		ProcessCommand(inputBytes);//process the command
		Serial.flush();
	  }
  }
}

void ProcessCommand(byte tokenizedCommands[3])
{
	if(tokenizedCommands[0] == 'E')
	{
		Serial.write("Focuser v0.1#"); 
	}
	else if(tokenizedCommands[0] == 'S')
	{
		byte currentTarget = tokenizedCommands[1];
		if(currentTarget == 'S')
		{
			speedOffset = tokenizedCommands[2];
			Serial.write("Set Complete.#");
		}
	}
	else if(tokenizedCommands[0] == 'M')
	{
		if(speedOffset == NULL)
		{
			Serial.write("Set move speed first.#");
			return;
		}
        
		if(tokenizedCommands[1] == 'F')
		{
			focusServo.write((SERVO_STOP + servoOffset) + speedOffset);
			Serial.write("Moving FWD.#");
		}
		else if (tokenizedCommands[1] == 'B')
		{
			focusServo.write((SERVO_STOP + servoOffset) - speedOffset);
			Serial.write("Moving BWD.#");
		}
		else if (tokenizedCommands[1] == 'S')
		{
			focusServo.write(SERVO_STOP + servoOffset);
			Serial.write("MotorStopped.#");
		}
		else
		{
			Serial.write("Direction not defined.#");
		}
  }
  else
  {
		Serial.write("Command not found (");
		Serial.print(tokenizedCommands[0], DEC);
		Serial.write(",");
		Serial.print(tokenizedCommands[1], DEC);
		Serial.write(",");
		Serial.print(tokenizedCommands[2], DEC);
		Serial.write(").#");
  }
  
  memset(tokenizedCommands, 0, sizeof(tokenizedCommands));
  memset(inputBytes, 0, sizeof(inputBytes));
}