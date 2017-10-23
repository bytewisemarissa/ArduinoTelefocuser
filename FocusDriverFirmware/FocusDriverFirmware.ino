#include "ReturnCode.h"
#include "StepMode.h"

//Pins
#define StepPin 2 // ANY TOGGLE is step motor
#define DirectionPin 3 // LOW is forward, HIGH is reverse
#define ModeSet1Pin 4
#define ModeSet2Pin 5
#define EnablePin 6 // HIGH is Disabled, LOW is Enabled
#define EnableButtonDriverPin 9
#define EnableButtonSensePin 8

/**********************************************
* Microstep Select Resolution Truth Table     *
*                                             *
*  MS1 	MS2 	Microstep Resolution          *
*  ---------------------------------          *
*   L	  L	    Full Step (2 Phase)       *
*   H	  L	    Half Step                 *
*   L	  H	    Quarter Step              *
*   H  	H	    Eigth Step                *
*                                             *
**********************************************/

//Constants
#define MAX_BYTES 7
#define MAX_STEP_BURST 16
#define SIG_START_UP 1
#define VERSION 1

//Globals
byte inputBytes[MAX_BYTES]; //input command buffer
bool isDebugEnabled;
bool isEnabled;
bool isLimitSet;
bool isSeeking; 
int limitStep;
int targetStep;
long currentStep;
StepMode currentStepMode;


/************************************
*  Core Logic                       *
************************************/

void setup() {
  pinMode(StepPin, OUTPUT);
  pinMode(DirectionPin, OUTPUT);
  pinMode(ModeSet1Pin, OUTPUT);
  pinMode(ModeSet2Pin, OUTPUT);
  pinMode(EnablePin, OUTPUT);
  pinMode(EnableButtonDriverPin, OUTPUT);
  pinMode(EnableButtonSensePin, INPUT);
  ResetMotorDriverPins(); 
  
  memset(inputBytes, 0, sizeof(inputBytes));
  
  isLimitSet = false;
  isSeeking = false;
  isDebugEnabled = false;
  isEnabled = false;

  currentStep = 0;
  targetStep = 0;
  limitStep = 0;
  
  currentStepMode = FullStep;

  Serial.begin(9600, SERIAL_8N1);
  Serial.flush();
  
  digitalWrite(EnableButtonDriverPin, HIGH);

  HandleReturnCode(StartedUp);
}

void loop() {  
  if(Serial.available() > 0)
  {
    int bytesRead = Serial.readBytesUntil('\n', (char*)inputBytes, MAX_BYTES); //pull command from serial buffer

    if(bytesRead > 0)
    {
      ProcessCommand();//process the command
      Serial.flush();
    }
  }
  
  SenseInputs();
  
  SyncPinStates();
  
  WorkStepperMotor();
}

void ProcessCommand()
{
  char commandChar = inputBytes[0];
  
  switch(commandChar)
  {
    case 'A':
      HandleAbsolutePositionCommand();
      break;
    case 'd':
      HandleDisableDebug();
      break;
    case 'D':
      HandleEnableDebug();
      break;
    case 'H':
      HandleHardStopCommand();
      break;
    case 'I':
      HandleInfoCommand(); 
      break;
    case 'l':
      HandleLimitUnSetCommand();
      break;
    case 'L':
      HandleLimitSetCommand();
      break;
    case 'M':
      HandleSetModeCommand();
      break;
    case 's':
      HandleStepReverseCommand();
      break;
    case 'S':
      HandleStepForwardCommand();
      break;
    case 'T':
      HandleATOICommand();
      break;
    case 'V':
      HandleVersionCommand();
      break;
    case 'Z':
      HandleSetZeroCommand(); 
      break;
    default:
      HandleReturnCode(UnknownCommand);
      break;
  }
  
  memset(inputBytes, 0, sizeof(inputBytes));
}

void SenseInputs()
{
  bool isEnableButtonFlipped = digitalRead(EnableButtonSensePin);
  isEnabled = isEnableButtonFlipped; 
}

void SyncPinStates()
{
  //sync pin states with logical flags
  bool enablePinStateCheck = digitalRead(EnablePin);
  if(isEnabled == enablePinStateCheck)
  {
    ToggleEnabledPin();  
  }
  
  bool modeSet1StateCheck = digitalRead(ModeSet1Pin);
  bool modeSet2StateCheck = digitalRead(ModeSet2Pin);
  StepMode pinStateStepMode = ConvertPinStateToStateMode(modeSet1StateCheck, modeSet2StateCheck);
  if(currentStepMode != pinStateStepMode)
  {
    SetStepMode(currentStepMode);  
  }
  
}

void WorkStepperMotor()
{
  // seek the motor if required
  if(isSeeking && currentStep != targetStep)
  {    
    int seekDifference = abs(currentStep - targetStep);
    int currentModeResolution = GetStepFactorForStepMode(currentStepMode);
    
    int numberOfStepsToWork;
    if(seekDifference > MAX_STEP_BURST)
    {
      numberOfStepsToWork = MAX_STEP_BURST; 
    }
    else
    {
      numberOfStepsToWork = seekDifference; 
    }
    
    int numberOfTriggers = numberOfStepsToWork / currentModeResolution;    
    
    if(isDebugEnabled)
    {
      Serial.print("Seek difference : ");
      Serial.print(seekDifference);
      Serial.print("\n");
      
      Serial.print("Current Mode Resolution : ");
      Serial.print(currentModeResolution);
      Serial.print("\n");
    
      Serial.print("Number of steps working this loop : ");
      Serial.print(numberOfStepsToWork);
      Serial.print("\n");
    
      Serial.print("Current Step : ");
      Serial.print(currentStep);
      Serial.print("\n");
      
      Serial.print("Target Step : ");
      Serial.print(targetStep);
      Serial.print("\n");
      
      Serial.print("Number of triggers : ");
      Serial.print(numberOfTriggers);
      Serial.print("\n");
    }
    
    if(currentStep > targetStep)
    {
      if(isDebugEnabled){ Serial.print("Going backward.\n"); }
      TriggerStepper(numberOfTriggers, false);
    }
    else
    {
      if(isDebugEnabled){ Serial.print("Going forward.\n"); }
      TriggerStepper(numberOfTriggers, true);
    }
    
    if(currentStep == targetStep)
    {
      if(isDebugEnabled){ Serial.print("Done Seeking.\n"); }
      isSeeking = false; 
    }
    
    if(isDebugEnabled){ Serial.print("\n"); }
  }
}

void SettleSubStepOverhang(int seekDifference, bool isMovingForward)
{
  Serial.print("Tripped Overhang Behavior/n");
  StepMode savedStepMode = currentStepMode;
  currentStepMode = EigthStep;
  SetStepMode(currentStepMode);
  TriggerStepper(seekDifference, isMovingForward);
  SetStepMode(savedStepMode);
  currentStepMode = savedStepMode;
}

/************************************
*  Public Handlers                  *
************************************/

void HandleAbsolutePositionCommand()
{
  if(!isEnabled)
  {
    HandleReturnCode(NotEnabled);  
    return;
  }
  
  int absoluteStep = ConvertBufferToInt();
  
  if(absoluteStep % GetStepFactorForStepMode(currentStepMode) != 0)
  {
    HandleReturnCode(NotModeMultiple);  
    return;
  }
    
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
      Serial.print(isEnabled, DEC);
      PrintEnabledLineState();
      break;
    case 'l':
      Serial.write("Il");
      Serial.print(isLimitSet);
      break;
    case 'L':
      Serial.write("IL");
      Serial.print(limitStep);
      break;
    case 'M':
      Serial.write("IM");
      Serial.print(limitStep);
      Serial.print(ConvertStepModeToId(currentStepMode), DEC);
      PrintModeSetLineState();
      break;
    case 'R':
      Serial.write("IR");
      PrintDirectionLineState();
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

void HandleSetModeCommand()
{
  int modeId = ConvertBufferToInt();
  StepMode requestedMode = ConvertIdToStepMode(modeId);
  
  if(requestedMode != InvalidStepMode)
  {
    currentStepMode = requestedMode; 
    HandleReturnCode(OK);  
  }
}

void HandleStepReverseCommand()
{
  if(!isEnabled)
  {
    HandleReturnCode(NotEnabled);  
    return;
  }
  
  int relativeSteps = ConvertBufferToInt();    
  int convertedSteps = GetStepFactorForStepMode(currentStepMode) * relativeSteps;
  int targetedPosition = currentStep - convertedSteps;
   
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
  if(!isEnabled)
  {
    HandleReturnCode(NotEnabled); 
    return; 
  }
  
  int relativeSteps = ConvertBufferToInt(); 
  int convertedSteps = GetStepFactorForStepMode(currentStepMode) * relativeSteps;
  int targetedPosition = currentStep + convertedSteps;
    
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
    case NotEnabled:
      Serial.write("R!NE\n");
      break;
    case NotModeMultiple:
      Serial.write("R!NMM\n");
      break;
    case InvalidMode:
      Serial.write("R!IM\n");
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
*  Info Command Handler Helpers     *
************************************/

void PrintEnabledLineState()
{
  bool enabledState = digitalRead(EnablePin);
  Serial.print(enabledState, DEC);
}

void PrintModeSetLineState()
{  
  bool modeSet1State = digitalRead(ModeSet1Pin);
  bool modeSet2State = digitalRead(ModeSet2Pin);
  Serial.print(modeSet1State, DEC);
  Serial.print(modeSet2State, DEC);  
}

void PrintDirectionLineState()
{
  bool directionState = digitalRead(DirectionPin);
  Serial.print(directionState, DEC);
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

StepMode ConvertIdToStepMode(int modeId)
{
  switch(modeId)
  {
    case 0:
     return FullStep;
    case 1:
     return HalfStep;
    case 2:
     return QuarterStep;
    case 3:
     return EigthStep;
    default:
     HandleReturnCode(InvalidMode);
     return InvalidStepMode;
  }
}

StepMode ConvertPinStateToStateMode(bool modeSet1, bool modeSet2)
{
  if(modeSet1 && modeSet2)
  {
    return EigthStep;
  }
  else if(!modeSet1 && modeSet2)
  {
    return QuarterStep;
  }
  else if(modeSet1 && !modeSet2)
  {
    return HalfStep;
  }
  else
  {
    return FullStep;
  }
}

int ConvertStepModeToId(StepMode mode)
{
  switch(mode)
  {
    case FullStep:
      return 0;
    case HalfStep:
      return 1;
    case QuarterStep:
      return 2;
    case EigthStep:
      return 3;
  }  
}

void ResetMotorDriverPins()
{
  digitalWrite(StepPin, LOW);
  digitalWrite(DirectionPin, LOW);
  digitalWrite(ModeSet1Pin, LOW);
  digitalWrite(ModeSet2Pin, LOW);
  digitalWrite(EnablePin, HIGH);
  
  isEnabled = false;
}

int GetStepFactorForStepMode(StepMode mode)
{
  switch(currentStepMode)
  {
    case FullStep:
      return 8;
      break;
    case HalfStep:
      return 4;
      break;
    case QuarterStep:
      return 2;
      break;
    case EigthStep:
      return 1;
      break;
  }
}

void UpdateCurrentStep(bool isMovingForward)
{
  
  int stepAmount = GetStepFactorForStepMode(currentStepMode); 
  
  if(isDebugEnabled)
  {
    Serial.print("Update Current Step By : ");
    Serial.print(stepAmount);
    Serial.print("\n");  
  }
  
  if(isMovingForward)
  {
    currentStep += stepAmount;  
  }
  else
  {
    currentStep -= stepAmount;
  }
}

/************************************
*  Stepper Helper Methods           *
************************************/

void TriggerStepper(int steps, bool isMovingForward)
{
  if(!isEnabled)
  {
    return;  
  }
  
  //set direction
  if(isMovingForward)
  {
    digitalWrite(DirectionPin, LOW);  
  }
  else
  {
    digitalWrite(DirectionPin, HIGH);
  }
  
  for(int i = 0; i < steps; i++)
  {
    
    //toggle step pin
    digitalWrite(StepPin, HIGH);
    delay(1);
    digitalWrite(StepPin, LOW);
    delay(1);
    
    UpdateCurrentStep(isMovingForward);
    PrintCurrentStep();
  }  
}

void SetStepMode(StepMode currentStepMode)
{
  switch(currentStepMode)
  {
    case FullStep:
      digitalWrite(ModeSet1Pin, LOW);
      digitalWrite(ModeSet2Pin, LOW);
      break;
    case HalfStep:
      digitalWrite(ModeSet1Pin, HIGH);
      digitalWrite(ModeSet2Pin, LOW);
      break;
    case QuarterStep:
      digitalWrite(ModeSet1Pin, LOW);
      digitalWrite(ModeSet2Pin, HIGH);
      break;
    case EigthStep:
      digitalWrite(ModeSet1Pin, HIGH);
      digitalWrite(ModeSet2Pin, HIGH);
      break;
  }  
}

void ToggleEnabledPin()
{
  if(isEnabled)
  {
    digitalWrite(EnablePin, LOW);
  }
  else
  {
    digitalWrite(EnablePin, HIGH);
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




