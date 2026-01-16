/****************************************************
 * LIBRARIES
 ****************************************************/

#include <Wire.h>                 // I2C-Kommunikation (OLED)
#include <Adafruit_GFX.h>         // Grafik-Grundfunktionen
#include <Adafruit_SSD1306.h>     // OLED Display
#include <Adafruit_NeoPixel.h>    // NeoPixel LED Ring


/****************************************************
 * OLED SETUP
 ****************************************************/

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1              // kein Reset-Pin
#define OLED_ADDR 0x3C             // typische I2C-Adresse

// OLED Objekt erstellen
Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);


/****************************************************
 * NEOPIXEL & BUZZER
 ****************************************************/

#define LED_PIN 6                  // Datenpin für NeoPixel
#define NUM_LEDS 24                // Anzahl LEDs im Ring
#define BUZZER_PIN 5               // Buzzer-Pin

// NeoPixel Objekt
Adafruit_NeoPixel ring(
  NUM_LEDS,
  LED_PIN,
  NEO_GRB + NEO_KHZ800
);


/****************************************************
 * BUTTONS
 ****************************************************/

#define BTN_PLUS  11
#define BTN_MINUS 10
#define BTN_OK    9
#define BTN_COLOR 7


/****************************************************
 * SPIELVARIABLEN
 ****************************************************/

int balance = 500;                // Startguthaben
int bet = 100;                    // Einsatz
int colorSelect = 0;              // 0=ROT, 1=BLAU, 2=GRUEN


/****************************************************
 * SPIELZUSTÄNDE
 ****************************************************/

enum GameState {
  MENU,                           // Menü anzeigen
  SPIN                            // Roulette dreht
};

GameState state = MENU;


/****************************************************
 * ENTPRELLUNG (Buttons)
 ****************************************************/

const unsigned long debounce = 150;

unsigned long lastPlus  = 0;
unsigned long lastMinus = 0;
unsigned long lastOk    = 0;
unsigned long lastColor = 0;


/****************************************************
 * HILFSFUNKTIONEN
 ****************************************************/

// Button lesen (INPUT_PULLUP -> gedrückt = LOW)
bool btn(int pin) {
  return digitalRead(pin) == LOW;
}

// Farbname für Anzeige
const char* getColorName(int c) {
  if (c == 0) return "ROT";
  if (c == 1) return "BLAU";
  return "GRUEN";
}

// Welche Farbe hat eine LED-Position?
int getFieldColor(int pos) {
  return (pos * 7) % 3;
  // Ergebnis ist immer 0,1 oder 2
}

// NeoPixel-Farbe zurückgeben
uint32_t pixelColor(int c) {
  if (c == 0) return ring.Color(255, 0, 0);
  if (c == 1) return ring.Color(0, 0, 255);
  return ring.Color(0, 255, 0);
}


/****************************************************
 * SOUND-FUNKTIONEN
 ****************************************************/

// Gewinn-Sound
void playWinSound() {
  int melody[] = {1000, 1400, 1800, 2200};
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, melody[i], 150);
    delay(180);
  }
  noTone(BUZZER_PIN);
}

// Verlust-Sound
void playLoseSound() {
  int melody[] = {800, 600, 400};
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, melody[i], 200);
    delay(230);
  }
  noTone(BUZZER_PIN);
}

// Tick-Sound beim Drehen
void playSpinTick(int d) {
  tone(BUZZER_PIN, 1200, 20);
  delay(d);
}


/****************************************************
 * OLED MENÜ ZEICHNEN
 ****************************************************/

void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Balance:");
  display.setCursor(70, 0);
  display.print(balance);

  display.setCursor(0, 20);
  display.print("Bet:");
  display.setCursor(30, 20);
  display.print(bet);

  display.setCursor(0, 40);
  display.print("Farbe:");
  display.setCursor(50, 40);
  display.print(getColorName(colorSelect));

  display.display();
}


/****************************************************
 * ROULETTE DREHEN
 ****************************************************/

void spinRoulette() {

  ring.clear();
  ring.show();

  // Wie viele volle Runden?
  int rounds = random(4, 9);

  // Zielposition
  int finalPos = random(0, NUM_LEDS);

  // Gesamtschritte
  int totalSteps = rounds * NUM_LEDS + finalPos;

  int pos = 0;
  int speed = 10;

  // Dreh-Animation
  for (int i = 0; i <= totalSteps; i++) {
    ring.clear();
    ring.setPixelColor(pos, pixelColor(getFieldColor(pos)));
    ring.show();

    playSpinTick(speed);

    pos = (pos + 1) % NUM_LEDS;

    // Abbremsen
    if (i > totalSteps * 0.6) speed += 3;
    if (i > totalSteps * 0.8) speed += 6;
  }

  // Endfarbe bestimmen
  int finalColor = getFieldColor(finalPos);

  // Blink-Effekt
  for (int i = 0; i < 4; i++) {
    ring.clear(); ring.show(); delay(150);
    ring.setPixelColor(finalPos, pixelColor(finalColor));
    ring.show(); delay(150);
  }

  // Ergebnis anzeigen
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(getColorName(finalColor));

  display.setTextSize(1);
  display.setCursor(0, 40);

  if (finalColor == colorSelect) {
    balance += bet;
    display.print("WIN +");
    display.print(bet);
    playWinSound();
  } else {
    balance -= bet;
    display.print("LOSE -");
    display.print(bet);
    playLoseSound();
  }

  display.display();
  delay(3000);

  state = MENU;
}


/****************************************************
 * SETUP
 ****************************************************/

void setup() {

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(BTN_PLUS, INPUT_PULLUP);
  pinMode(BTN_MINUS, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_COLOR, INPUT_PULLUP);

  ring.begin();
  ring.show();

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  randomSeed(micros());

  drawMenu();
}


/****************************************************
 * LOOP
 ****************************************************/

void loop() {

  unsigned long now = millis();

  if (state == MENU) {

    drawMenu();

    if (btn(BTN_PLUS) && now - lastPlus > debounce) {
      bet += 100;
      lastPlus = now;
    }

    if (btn(BTN_MINUS) && now - lastMinus > debounce) {
      bet -= 100;
      if (bet < 100) bet = 100;
      lastMinus = now;
    }

    if (btn(BTN_COLOR) && now - lastColor > debounce) {
      colorSelect = (colorSelect + 1) % 3;
      lastColor = now;
    }

    if (btn(BTN_OK) && now - lastOk > debounce) {
      state = SPIN;
      lastOk = now;
    }
  }

  else if (state == SPIN) {
    spinRoulette();
  }
}
