// Pump 1
const int P1_DIR1 = D1;  // GPIO5
const int P1_DIR2 = D2;  // GPIO4
const int P1_PWM  = D3;  

// Pump 2
const int P2_DIR1 = D5;  // GPIO14
const int P2_DIR2 = D6;  // GPIO12
const int P2_PWM  = D7;  // GPIO13

// Pump 3
const int P3_DIR1 = D0;  
const int P3_DIR2 = D4;  
const int P3_PWM  = D8;  

void setup() {
  // Direction pins
  pinMode(P1_DIR1, OUTPUT); pinMode(P1_DIR2, OUTPUT);
  pinMode(P2_DIR1, OUTPUT); pinMode(P2_DIR2, OUTPUT);
  pinMode(P3_DIR1, OUTPUT); pinMode(P3_DIR2, OUTPUT);

  // PWM pins
  pinMode(P1_PWM, OUTPUT);
  pinMode(P2_PWM, OUTPUT);
  pinMode(P3_PWM, OUTPUT);

  // Set all pumps to forward direction
  digitalWrite(P1_DIR1, HIGH); digitalWrite(P1_DIR2, LOW);
  digitalWrite(P2_DIR1, HIGH); digitalWrite(P2_DIR2, LOW);
  digitalWrite(P3_DIR1, HIGH); digitalWrite(P3_DIR2, LOW);
}

void loop() {
  // Set speeds (ESP8266 PWM: 0–1023)
  analogWrite(P1_PWM, 1023);   // 50% speed
  analogWrite(P2_PWM, 1023);   // 80% speed
  analogWrite(P3_PWM, 1023);  // 100% speed

  delay(10000);  // Run for 10 sec

  // Stop all
  analogWrite(P1_PWM, 0);
  analogWrite(P2_PWM, 0);
  analogWrite(P3_PWM, 0);

  delay(5000);  // Pause
}
