// Pinos de controle do motor
int motorPin1 = 14; // Conecte ao IN1 da Ponte H
int motorPin2 = 26; // Conecte ao IN2 da Ponte H
int motorPin3 = 25; // Conecte ao IN1 da Ponte H
int motorPin4 = 27; // Conecte ao IN2 da Ponte H

void setup() {
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
}

void loop() {
  // Move para frente
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);
  delay(10000000); 
}
