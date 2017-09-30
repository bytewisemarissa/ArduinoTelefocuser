#include "ReturnCode.h"

//Pins
#define StepPin 2 // ANY TOGGLE is step motor
#define DirectionPin 3 // LOW is forward, HIGH is reverse
#define ModeSet1Pin 4
#define ModeSet2Pin 5
#define EnablePin 6 // HIGH is Disabled, LOW is Enabled

/**********************************************
* Microstep Select Resolution Truth Table     *
*                                             *
*  MS1 	MS2 	Microstep Resolution          *
*  ---------------------------------          *
*   L	  L	Full Step (2 Phase)           *
*   H	  L	Half Step                     *
*   L	  H	Quarter Step                  *
*   H  	  H	Eigth Step                    *
*                                             *
**********************************************/

//Constants
#define MAX_BYTES 7
#define MAX_STEP_BURST 5
#define SIG_START_UP 1
#define VERSION 1

//Globals
byte inputBytes[MAX_BYTES]; //input command buffer
bool isDebugEnabled;
bool isEnabled;
bool isLimitSet;
bool isSeeking;
bool stepPhase;
int currentStep;
int enablePinStateCheck;
int limitStep;
int targetStep;


/************************************
*  Core Logic                       *
************************************/

void setup() {
  pinMode(StepPin, OUTPUT);
  pinMode(DirectionPin, OUTPUT);
  pinMode(ModeSet1Pin, OUTPUT);
  pinMode(ModeSet2Pin, OUTPUT);
  pinMode(EnablePin, OUTPUT);
  ResetMotorDriverPins(); 
  
  isLimitSet = false;
  isSeeking = false;
  isDebugEnabled = false;
  isEnabled = false;
  stepPhase = false;

  currentStep = 0;
  enablePinStateCheck = 0;
  targetStep = 0;
  limitStep = 0;

  memset(inputBytes, 0, sizeof(inputBytes));
  
  Serial.begin(115200, SERIAL_8N1);
  Serial.flush();

  HandleReturnCode(StartedUp);
}

void loop() {  
  if(Serial.available() > 0)
  {
    int bytesRead = Serial.readBytesUntil('\n', (char*)inputBytes, MAX_BYTES); //pull command from serial buffer

    if(inputBytes[0] != 240)
    {
      ProcessCommand();//process the command
      Serial.flush();
    }
  }
  
  SyncPinStates();
  
  if(isEnabled)
  {
    WorkStepperMotor();
  }
}

void ProcessCommand()
{
  if(inputBytes[0] == 'A')
  {
    HandleAbsolutePositionCommand();
  }
  else if(inputBytes[0] == 'd')
  {
    HandleDisableDebug(); 
  }
  else if(inputBytes[0] == 'D')
  {
    HandleEnableDebug(); 
  }
  else if(inputBytes[0] == 'e')
  {
    HandleLineDisable(); 
  }
  else if(inputBytes[0] == 'E')
  {
    HandleLineEnable();
  }
  else if(inputBytes[0] == 'H')
  {
    HandleHardStopCommand();
  }
  else if(inputBytes[0] == 'I')
  {
    HandleInfoCommand(); 
  }
  else if(inputBytes[0] == 'l')
  {
    HandleLimitUnSetCommand();
  }
  else if(inputBytes[0] == 'L')
  {
    HandleLimitSetCommand();
  }
  else if(inputBytes[0] == 's')
  {
    HandleStepReverseCommand();
  }
  else if(inputBytes[0] == 'S')
  {
    HandleStepForwardCommand();
  }
  else if(inputBytes[0] == 'T')
  {
    HandleATOICommand();
  }
  else if(inputBytes[0] == 'V')
  {
    HandleVersionCommand();
  }
  else if(inputBytes[0] == 'Z')
  {
    HandleSetZeroCommand(); 
  }
  else
  {
    HandleReturnCode(UnknownCommand);
  }
  
  memset(inputBytes, 0, sizeof(inputBytes));
}

void SyncPinStates()
{
  //sync pin states with logical flags
  enablePinStateCheck = digitalRead(EnablePin);
  if(isEnabled != enablePinStateCheck)
  {
    ToggleEnabledPin();  
  }
}

void WorkStepperMotor()
{
  // seek the motor if required
  if(isSeeking && currentStep != targetStep)
  {    
    int seekDifference = abs(currentStep - targetStep);
    
    int numberOfStepsToWork;
    if(seekDifference > MAX_STEP_BURST)
    {
      numberOfStepsToWork = MAX_STEP_BURST; 
    }
    else
    {
      numberOfStepsToWork = seekDifference; 
    }
    
    if(currentStep > targetStep)
    {
      RollStepperReverse(numberOfStepsToWork); 
    }
    else
    {
      RollStepperForward(numberOfStepsToWork); 
    }
    
    if(currentStep == targetStep)
    {
      isSeeking = false; 
    }
  }
}

/************************************
*  Public Handlers                  *
************************************/

void HandleAbsolutePositionCommand()
{
  int absoluteStep = ConvertBufferToInt();
    
  if(absoluteStep < 0)
  {
    HandleReturnCode(BeyondZeroBound);
    return;
  }
    
  if(isLimitSet && absoluteStep > limitStep)
  {
    HandleReturnCode(BeyondLimitBound);   
    return;
  }
    
  targetStep = absoluteStep;
  isSeeking = true;
  HandleReturnCode(OK);
}

void HandleDisableDebug()
{
  isDebugEnabled = false;
  HandleReturnCode(OK);
}

void HandleEnableDebug()
{
  isDebugEnabled = true;
  HandleReturnCode(OK);
}

void HandleLineDisable()
{
  isEnabled = false;
  HandleReturnCode(OK);
}

void HandleLineEnable()
{
  isEnabled = true;
  HandleReturnCode(OK);
}

void HandleHardStopCommand()
{
  if(isSeeking)
  {
    isSeeking = false;
    targetStep = currentStep;
    HandleReturnCode(OK);
  }
  else
  {
    HandleReturnCode(NotCurrentlySeeking);
  }
}

void HandleInfoCommand()
{
  switch(inputBytes[1])
  {
    case 'C':
      Serial.write("IC");
      Serial.print(currentStep);
      break;
    case 'D':
      Serial.write("ID");
      Serial.print(isDebugEnabled);
      break;
    case 'E':
      Serial.write("IE");
      Serial.print(isEnabled);
      break;
    case 'l':
      Serial.write("Il");
      Serial.print(isLimitSet);
      break;
    case 'L':
      Serial.write("IL");
      Serial.print(limitStep);
      break;
    case 'S':
      Serial.write("IS");
      Serial.print(isSeeking);
      break;
    case 'T':
      Serial.write("IT");
      Serial.print(targetStep);
      break;
    default:
      HandleReturnCode(UnknownCommand);
      return;
  }
  Serial.write("\n");
}

void HandleLimitUnSetCommand()
{
  if(isSeeking)
  {
    HandleReturnCode(CurrentlySeeking);
    return;
  }
    
  limitStep = 0;
  isLimitSet = false;
  HandleReturnCode(OK);
}

void HandleLimitSetCommand()
{
  if(isSeeking)
  {
    HandleReturnCode(CurrentlySeeking); 
    return;
  }
  
  limitStep = currentStep;
  isLimitSet = true;
  HandleReturnCode(OK);
}

void HandleStepReverseCommand()
{
  int relativeSteps = ConvertBufferToInt();    
  int targetedPosition = currentStep - relativeSteps;
   
  if(targetedPosition < 0)
  {
    HandleReturnCode(BeyondZeroBound);
    return;
  }
    
  if(isLimitSet && targetedPosition > limitStep)
  {
    HandleReturnCode(BeyondLimitBound);
    return;
  }
    
  targetStep = targetedPosition;
  isSeeking = true;
  HandleReturnCode(OK);
}

void HandleStepForwardCommand()
{
  int relativeSteps = ConvertBufferToInt();  
  int targetedPosition = currentStep + relativeSteps;
    
  if(targetedPosition < 0)
  {
    HandleReturnCode(BeyondZeroBound);
    return;
  }
    
  if(isLimitSet && targetedPosition > limitStep)
  {
    HandleReturnCode(BeyondLimitBound);
    return;
  }
    
  targetStep = targetedPosition;
  isSeeking = true;
  HandleReturnCode(OK);
}

void HandleATOICommand()
{
  if(!isDebugEnabled)
  {
     HandleReturnCode(DebugRequired);
     return;
  }
  
  Serial.write("Testing ATOI function.\n");
  Serial.write("Recived following bytes:\n");
  Serial.write(inputBytes[0]);
  Serial.write(",");
  Serial.write(inputBytes[1]);
  Serial.write(",");
  Serial.write(inputBytes[2]);
  Serial.write(",");
  Serial.write(inputBytes[3]);
  Serial.write(",");
  Serial.write(inputBytes[4]);
  Serial.write(",");
  Serial.write(inputBytes[5]);
  Serial.write("\n");
  Serial.print(inputBytes[0], DEC);
  Serial.write(",");
  Serial.print(inputBytes[1], DEC);
  Serial.write(",");
  Serial.print(inputBytes[2], DEC);
  Serial.write(",");
  Serial.print(inputBytes[3], DEC);
  Serial.write(",");
  Serial.print(inputBytes[4], DEC);
  Serial.write(",");
  Serial.print(inputBytes[5], DEC);
  Serial.write("\n");  
  int atoiResult = ConvertBufferToInt();
  Serial.write("ATOI Result:");
  Serial.print(atoiResult);
  Serial.write("\n"); 
}

void HandleVersionCommand()
{
  HandleReturnCode(Version);
}

void HandleSetZeroCommand()
{
  if(isSeeking)
  {
    HandleReturnCode(CurrentlySeeking);
    return;
  }
  
  isLimitSet = false;
  isSeeking = false;
  isDebugEnabled = false;

  currentStep = 0;
  targetStep = 0;
  limitStep = 0;
  
  HandleReturnCode(OK);
}

/************************************
*  Internal Handlers                *
************************************/

void HandleReturnCode(ReturnCode code)
{
  switch(code)
  {
    case StartedUp:
      if(SIG_START_UP)
      {
        Serial.write("R S\n");  
      }
      break;
    case OK:
      Serial.write("R OK\n");
      break;
    case Error:
      Serial.write("R!ERR\n");
      break;
    case DebugRequired:
      Serial.write("R DR\n");
      break;
    case UnknownCommand:
      if(isDebugEnabled)
      {
        HandleCommandNotFoundVerbose();  
      }
      else
      {
        Serial.write("R!UC\n");  
      }
      break;
    case Version:
      Serial.write("R V");
      Serial.print(VERSION);
      Serial.write("\n");
      break;
    case CurrentlySeeking:
      Serial.write("R!CS\n");
      break;
    case NotCurrentlySeeking:
      Serial.write("R!NCS\n");
      break;
    case BeyondLimitBound:
      Serial.write("R!BLB\n");
      break;
    case BeyondZeroBound:
      Serial.write("R!BZB\n");
      break;
    default:
      Serial.write("R!URC\n");
      return;
  }
}

void HandleCommandNotFoundVerbose()
{
  Serial.write("Command not found.\n");   
  Serial.write("Recived following bytes:\n(");
  Serial.write(inputBytes[0]);
  Serial.write(inputBytes[1]);
  Serial.write(inputBytes[2]);
  Serial.write(inputBytes[3]);
  Serial.write(inputBytes[4]);
  Serial.write(inputBytes[5]);
  Serial.write(")\n(");
  Serial.print(inputBytes[0], DEC);
  Serial.write(",");
  Serial.print(inputBytes[1], DEC);
  Serial.write(",");
  Serial.print(inputBytes[2], DEC);
  Serial.write(",");
  Serial.print(inputBytes[3], DEC);
  Serial.write(",");
  Serial.print(inputBytes[4], DEC);
  Serial.write(",");
  Serial.print(inputBytes[5], DEC);
  Serial.write(")\n");
}

/************************************
*  Utilty Methods                   *
************************************/

int ConvertBufferToInt()
{
  int digitCounter = 0;
  int index = 1;
  
  while(true)
  {
    if(index == 6 || inputBytes[index] == 0)
    {
      break; 
    }
    
    digitCounter++;
    index++;
  }
  
  if(digitCounter == 0)
  {
    return 0; 
  }
  
  char digits[digitCounter];
  
  for(int i = 0; i < digitCounter; i++)
  {
    digits[i] = inputBytes[i + 1]; 
  }
  
  return atoi(digits);
}

void ResetMotorDriverPins()
{
  digitalWrite(StepPin, LOW);
  digitalWrite(DirectionPin, LOW);
  digitalWrite(ModeSet1Pin, LOW);
  digitalWrite(ModeSet2Pin, LOW);
  digitalWrite(EnablePin, HIGH);
  
  stepPhase = false;
  isEnabled = false;
}

/************************************
*  Servo Helpe Methods              *
************************************/

void RollStepperReverse(int steps)
{
  for(int i = 0; i < steps; i++)
  {    
    currentStep -= 1;
    
    PrintCurrentStep();
  } 
}

void RollStepperForward(int steps)
{
  for(int i = 0; i < steps; i++)
  {    
    currentStep += 1;
    
    PrintCurrentStep();
  } 
}

void ToggleEnabledPin()
{
  if(isEnabled)
  {
    digitalWrite(EnablePin, HIGH);
  }
  else
  {
    digitalWrite(EnablePin, LOW);
  }  
}

/************************************
*  Debug Methods                    *
************************************/

void PrintCurrentStep() //This is run in debug mode to slow down the seeking behavior
{
  if(isDebugEnabled)
  {
    Serial.write("Current Step : ");
    Serial.print(currentStep);
    Serial.write("\n"); 
  }
}




