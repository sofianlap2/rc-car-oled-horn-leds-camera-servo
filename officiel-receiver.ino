#include <SPI.h>
#include <RF24.h>
#include <Servo.h>

RF24 radio(9, 10);

const byte txAddr[6] = "NODE1";
const byte rxAddr[6] = "NODE2";

// ===== DATA =====
struct TXData {
  int move;
  int turn;
  bool lights;
  bool horn;
};

struct RXData {
  float battery;
  int signal;
};

TXData txData;
TXData lastRxData = {0, 0, false, false};
RXData rxData;

// ===== MOTOR PINS =====
const int IN1 = 2;
const int IN2 = 3;
const int IN3 = 4;
const int IN4 = 5;

// ===== LED + BUZZER =====
const int FRONT_LED = A5;
const int BACK_LED = A2;
const int BUZZER = A4;

// ===== SERVO =====
Servo myServo;
const int SERVO_PIN = 6;

int servoAngle = 0;
int servoDir = 1;
unsigned long lastServoUpdate = 0;
unsigned long lastServoHold = 0;

unsigned long lastReceiveTime = 0;

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial.println("Receiver START (Car + Servo)");

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(FRONT_LED, OUTPUT);
  pinMode(BACK_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // ===== SERVO INIT =====
  myServo.attach(SERVO_PIN);
  myServo.write(0);

  // ===== RADIO =====
  radio.begin();
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);

  radio.enableAckPayload();
  radio.enableDynamicPayloads();

  radio.openReadingPipe(1, txAddr);
  radio.openWritingPipe(rxAddr);

  radio.startListening();
}

// ================= LOOP =================
void loop() {

  // ===== RF RECEIVE =====
  if (radio.available()) {
    radio.read(&txData, sizeof(txData));
    lastReceiveTime = millis();

    bool changed = (
      txData.move != lastRxData.move ||
      txData.turn != lastRxData.turn ||
      txData.lights != lastRxData.lights ||
      txData.horn != lastRxData.horn
    );

    if (changed) {
      Serial.print("Move: ");
      Serial.print(txData.move);
      Serial.print(" | Turn: ");
      Serial.print(txData.turn);
      Serial.print(" | Lights: ");
      Serial.print(txData.lights);
      Serial.print(" | Horn: ");
      Serial.println(txData.horn);

      lastRxData = txData;
    }

    controlCar(txData.move, txData.turn);

    digitalWrite(FRONT_LED, txData.lights);
    digitalWrite(BACK_LED, txData.lights);

    if (txData.horn) tone(BUZZER, 2000);
    else noTone(BUZZER);
  }

  // ===== FAILSAFE =====
  if (millis() - lastReceiveTime > 500) {
    stopMotors();
    digitalWrite(FRONT_LED, LOW);
    digitalWrite(BACK_LED, LOW);
    noTone(BUZZER);
  }

  // ===== SERVO SWEEP (NON-BLOCKING) =====
  updateServo();
}

// ================= CAR CONTROL =================
void controlCar(int move, int turn) {

  if (move == 0 && turn == 0) {
    stopMotors();
    return;
  }

  if (move == 1) {
    if (turn == 1) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
    }
    else if (turn == -1) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    }
    else {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    }
  }

  else if (move == -1) {
    if (turn == 1) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
    }
    else if (turn == -1) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
    }
    else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
    }
  }

  else if (move == 0) {
    if (turn == 1) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
    }
    else if (turn == -1) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    }
  }
}

// ================= STOP =================
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ================= SERVO SWEEP =================
void updateServo() {

  unsigned long now = millis();

  // speed of movement
  if (now - lastServoUpdate > 15) {
    lastServoUpdate = now;

    servoAngle += servoDir;
    myServo.write(servoAngle);

    if (servoAngle >= 180) {
      servoAngle = 180;
      servoDir = -1;
      lastServoHold = now;   // start hold timer
    }

    if (servoAngle <= 0) {
      servoAngle = 0;
      servoDir = 1;
      lastServoHold = now;
    }
  }

  // hold at edges for 2 seconds
  if (now - lastServoHold < 2000) {
    return; // pause movement
  }
}