
int pinReset = 13;
int sensor = 2;
int i = 0;
int stateSensor;
int oldState;
// the setup routine runs once when you press reset:

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(sensor, INPUT);
  pinMode(pinReset, OUTPUT);
  digitalWrite(pinReset, HIGH);
}


void loop() {
  oldState = stateSensor;
  stateSensor = digitalRead(sensor);
  //If no change
  if (oldState == stateSensor){
    i++;
  }
  //else if change detected from the distant arduino
  else{
    Serial.println("Timer reset, arduino still active");
    i= 0;
  }
  Serial.println(i);
  //if no change since 500 iterations
  if (i >= 500)
  {
    Serial.println("CRASH !! reset..");
    //turn off and on the alimentation pin
    digitalWrite(pinReset, LOW);
    delay(1000);
    digitalWrite(pinReset, HIGH);
    //waiting 5s
    i = 5;
    while (i > 0)
    {
      Serial.print("Waiting for reboot ");
      Serial.print(i);
      Serial.println("s remaining");
      i--;
      delay(1000);        // delay in between reads for stability
    }
    Serial.println("Reebot done.");
  }
}


