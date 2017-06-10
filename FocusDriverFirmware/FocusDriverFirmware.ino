#define MAX_BYTES 5

int incomingValue;
int inputSize;
int tokenCount;
int currentStep;
int targetStep;
bool isSeeking;
int limitStep;
bool limitIsSet;
byte inputBytes[MAX_BYTES];

void setup() {
  currentStep = 0;
  targetStep = 0;
  limitStep = 0;
	limitIsSet = false;
  isSeeking = false;

  memset(inputBytes, 0, sizeof(inputBytes));
  
	Serial.begin(115200, SERIAL_8N1);
	Serial.flush();

	Serial.write("Focuser Started Up\n");
}

void loop() {  
  if(Serial.available() > 0)
  { 
	  int bytesRead = Serial.readBytesUntil('\n', inputBytes, MAX_BYTES); //pull command from serial buffer

	  if(inputBytes[0] != 240)
	  {
		  ProcessCommand(inputBytes);//process the command
		  Serial.flush();
	  }
  }

  WorkStep();
}

void ProcessCommand(byte commandBytes[MAX_BYTES])
{
	if(commandBytes[0] == 'V')
	{
		Serial.write("Focuser v0.1\n"); 
	}
  else if(commandBytes[0] == 'L')
  {
    if(isSeeking)
    {
      Serial.write("Can not set limit while seeking.");
      return;
    }
    
    Serial.write("Setting limit to ");
    Serial.print(currentStep);
    Serial.write(".\n");

    limitStep = currentStep;
    limitIsSet = true;    
  }
  else
  {
		Serial.write("Command not found (");
		Serial.print(commandBytes[0], DEC);
		Serial.write(",");
		Serial.print(commandBytes[1], DEC);
		Serial.write(",");
		Serial.print(commandBytes[2], DEC);
    Serial.write(",");
    Serial.print(commandBytes[3], DEC);
    Serial.write(",");
    Serial.print(commandBytes[4], DEC);
		Serial.write(").\n");
  }
  
  //memset(commandBytes, 0, sizeof(commandBytes));
  memset(inputBytes, 0, sizeof(inputBytes));
}

void WorkStep()
{
  if(currentStep != targetStep)
  {

  }
}

