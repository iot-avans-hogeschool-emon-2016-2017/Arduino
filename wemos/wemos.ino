/*Pins*/
const byte vInput = A0;

/*LDR values and dif margin*/
int oldValue = -1;
int newValue = -1;
int dif = 0;
const int minDif = 15;
const int maxDif = 30;

void nextValue();
void printValues();
void calcDif();
boolean isPeak();

void setup() {
  Serial.begin(9600);
  Serial.println("Setup wemos");  
  calcDif();
}

void loop() {    
  if (isPeak() == true) {
    Serial.println("Is peak!");
    printValues();
  }
  //Serial.println("----------");
  //delay(1000);
}

boolean isPeak() {
  nextValue();
  calcDif();
  boolean higherThanMinDif = dif > minDif;
  boolean lowerThanMaxDif = dif < maxDif;
  boolean inMargin = higherThanMinDif && lowerThanMaxDif;
  //Serial.print(higherThanMinDif);
  //Serial.print(", ");
  //Serial.print(lowerThanMaxDif);
  //Serial.print(" : ");
  //Serial.println(inMargin);
  return inMargin;
}

void calcDif() {
  //Serial.print("Calculate difference in values: ");
  dif = newValue - oldValue;  
  //Serial.print(dif);
  if (dif < 0) {
     dif = dif * -1;
  }
  //Serial.print(" becomes: ");
  //Serial.println(dif);
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
  //Serial.print("Get next value from pin: ");
  //Serial.println(vInput);
  oldValue = newValue;
  newValue = analogRead(vInput);
}


