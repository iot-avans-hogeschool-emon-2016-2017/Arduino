int val = 0;

void countTicks();

void setup() {
  //Serial.begin(9600);
  //Serial.println("Setup wemos");
  pinMode(D5, INPUT);
  pinMode(D0, OUTPUT);
  
}

void loop() {
  val = digitalRead(D5);
  digitalWrite(D0, !val);
  //Serial.print("Digital: ");
  //Serial.println(val);  
  
}

