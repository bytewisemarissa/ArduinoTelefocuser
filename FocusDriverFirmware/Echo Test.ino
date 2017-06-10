//#define INPUT_SIZE 3
//
//byte inputBytes[INPUT_SIZE];
//
//void setup() {
//	Serial.begin(115200, SERIAL_8N1);
//	memset(inputBytes, 0, sizeof(inputBytes));
//}
//
//void loop() {  
//  if(Serial.available() > 0)
//  {
//      int inputSize = Serial.readBytes(inputBytes, INPUT_SIZE);//pull command from serial buffer
//
//	  for(int i = 0; i < inputSize; i++)
//	  {
//		  Serial.write(inputBytes[i]);
//	  }
//  }
//}
