#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

RF24 radio(9, 10);

const byte txAddr[6] = "NODE1";
const byte rxAddr[6] = "NODE2";

// DATA
struct TXData {
  int move;
  int turn;
  bool lights;
  bool horn;
};

TXData txData;
TXData lastTxData = {0, 0, false, false};

// JOYSTICK
const int joyX = A1;
const int joyY = A0;
const int joySW = 3;
const int switchPin = 2;

void setup() {
  Serial.begin(115200);
  Serial.println("Transmitter START");

  pinMode(switchPin, INPUT_PULLUP);
  pinMode(joySW, INPUT_PULLUP);

  // 🔴 IMPORTANT DELAY (power stabilization)
  delay(300);

  Wire.begin();
  Wire.setClock(100000);

  // 🔴 OLED INIT WITH CHECK
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED FAILED");
    while (true); // stop here if OLED fails
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("OLED OK");
  display.display();
  delay(500);

  // RF
  radio.begin();
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);

  radio.enableAckPayload();
  radio.enableDynamicPayloads();

  radio.openWritingPipe(txAddr);
  radio.openReadingPipe(1, rxAddr);

  radio.stopListening();
}

void loop() {
  int x = analogRead(joyX);
  int y = analogRead(joyY);

  txData.move = 0;
  txData.turn = 0;

  if (y > 600) txData.move = 1;
  else if (y < 400) txData.move = -1;

  if (x > 600) txData.turn = 1;
  else if (x < 400) txData.turn = -1;

  txData.lights = (digitalRead(switchPin) == LOW);
  txData.horn = (digitalRead(joySW) == LOW);

  // Send data
  radio.write(&txData, sizeof(txData));

  // OLED DISPLAY
  display.clearDisplay();

  display.setCursor(0, 0);
  display.print("Move: ");
  display.print(txData.move);

  display.setCursor(0, 20);
  display.print("Turn: ");
  display.print(txData.turn);

  display.setCursor(0, 40);
  display.print("Horn: ");
  display.print(txData.horn ? "ON" : "OFF");

  display.display();

  delay(50);
}