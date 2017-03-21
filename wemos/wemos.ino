/*Pins*/
const byte vInput = A0;

/*LDR value*/
int oldValue = -1;
int newValue = -1;
int dif = 0;

void nextValue();
void printValues();
void calcDif();

void setup() {
  Serial.begin(9600);
  Serial.println("Setup wemos");  
  calcDif();
}

void loop() {
  Serial.println("----------");
  nextValue();  
  calcDif();
  printValues();
  Serial.println("----------");
  delay(1000);
}

void calcDif() {
  Serial.println("Calculate difference in values");
  dif = newValue - oldValue;
}

void printValues() {
  Serial.print("Values: [");
  Serial.print(oldValue);
  Serial.print("], [");
  Serial.print(newValue);
  Serial.print("] dif:");
  Serial.println(dif);
}

void nextValue() {
  Serial.print("Get next value from pin: ");
  Serial.println(vInput);
  oldValue = newValue;
  newValue = analogRead(vInput);
}

