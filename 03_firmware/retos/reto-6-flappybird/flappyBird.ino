#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Preferences.h>

// ---- Pin Definitions (ESP32-C6 Supermini) ----
#define SDA_PIN     6
#define SCL_PIN     7
#define BUTTON_PIN  2   // TTP223 touch sensor (HIGH when touched)
#define BUZZER_PIN  19  // Passive buzzer

// OLED display settings
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET = -1;
const int SCREEN_ADDRESS = 0x3C;
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const bool DEBUG_PRINT = false;

// Touch sensor, buzzer & physics
const int SCALE_FACTOR  = 10;
const int GRAVITY       = 5;
const int JUMP_STRENGTH = -25;
const int PIPE_WIDTH    = 15 * SCALE_FACTOR;
const int PIPE_GAP      = 30 * SCALE_FACTOR;
const int PIPE_SPEED    = 10;
const int PIPE_SPACING  = 65 * SCALE_FACTOR;

// Size of player sprite
const int BIRD_WIDTH_PX  = 10;
const int BIRD_HEIGHT_PX = 8;

// Game variables
int birdY;
float velocity = 0;
bool gameOver = false;
int score = 0;
unsigned long frameCounter = 0;
unsigned long gameOverTime = 0;

// Game state
enum GameState { STARTSCREEN, HIGHSCORE, PLAYING };
GameState gameState = STARTSCREEN;

struct Pipe { int x; int height; };
const int MAX_PIPES = 3;
Pipe pipes[MAX_PIPES];

long bestScore = 0;
bool isNewBest = false;
unsigned long highscoreTime = 0;
Preferences prefs;

int cloudX[3]     = {10, 55, 100};
int cloudY[3]     = {1, 4, 2};
int cloudSpeed[3] = {1, 2, 1};
int groundScrollOffset = 0;

const int FRAME_TIME = 40;
unsigned long lastFrameTime = 0;

// ---- Bitmaps ----
// 'bird1', 10x8px
const unsigned char epd_bitmap_bird1[] PROGMEM = {
  0x0e, 0x00, 0x15, 0x00, 0x60, 0x80, 0x81, 0x80, 0xfc, 0xc0, 0xfb, 0x80, 0x71, 0x00, 0x1e, 0x00
};
// 'bird2', 10x8px
const unsigned char epd_bitmap_bird2[] PROGMEM = {
  0x0e, 0x00, 0x15, 0x00, 0x70, 0x80, 0xf9, 0x80, 0xfc, 0xc0, 0x83, 0x80, 0x71, 0x00, 0x1e, 0x00
};
const unsigned char* epd_bitmap_allArray[2] = { epd_bitmap_bird1, epd_bitmap_bird2 };

// 'cloud_large', 16x6px
const unsigned char cloud_large[] PROGMEM = {
  0x06, 0x00, 0x0F, 0x60, 0x1F, 0xF0, 0x7F, 0xF8, 0xFF, 0xFC, 0x7F, 0xF8
};
// 'cloud_small', 8x5px
const unsigned char cloud_small[] PROGMEM = {
  0x3C, 0x7E, 0xFF, 0xFF, 0x7E
};
// 'bird_dead', 10x8px
const unsigned char epd_bitmap_bird_dead[] PROGMEM = {
  0x1e, 0x00, 0x71, 0x00, 0xfb, 0x80, 0xfc, 0xc0,
  0x81, 0x80, 0x60, 0x80, 0x15, 0x00, 0x0e, 0x00
};

// ---- BuzzerNote Sequence Engine ----
struct BuzzerNote { int freq; int dur; };

const BuzzerNote* _seq       = nullptr;
int               _seqLen    = 0;
int               _noteIdx   = 0;
unsigned long     _noteStart = 0;
bool              _buzPlaying = false;

void updateBuzzer() {
  if (!_buzPlaying) return;
  unsigned long now = millis();
  if (now - _noteStart < (unsigned long)_seq[_noteIdx].dur) return;
  _noteIdx++;
  if (_noteIdx >= _seqLen) {
    noTone(BUZZER_PIN);
    _buzPlaying = false;
    return;
  }
  _noteStart = now;
  (_seq[_noteIdx].freq == 0) ? noTone(BUZZER_PIN) : tone(BUZZER_PIN, _seq[_noteIdx].freq);
}

void buzzer_play(const BuzzerNote* seq, int len) {
  _seq       = seq;
  _seqLen    = len;
  _noteIdx   = 0;
  _noteStart = millis();
  _buzPlaying = true;
  (seq[0].freq == 0) ? noTone(BUZZER_PIN) : tone(BUZZER_PIN, seq[0].freq);
}

#define PLAY(s) buzzer_play(s, sizeof(s) / sizeof(s[0]))

const BuzzerNote SND_FB_JUMP[]     = { { 1800, 18 }, { 0, 10 }, { 2200, 25 } };
const BuzzerNote SND_FB_GAMEOVER[] = { { 880, 80 }, { 0, 20 }, { 660, 80 }, { 0, 20 }, { 494, 80 }, { 0, 20 }, { 330, 180 } };
const BuzzerNote SND_FB_CLICK[]    = { { 1800, 18 }, { 0, 10 }, { 2200, 25 } };

void playGameOverSound() { PLAY(SND_FB_GAMEOVER); }
void playFlapSound()     { PLAY(SND_FB_JUMP); }
void playClickSound()    { PLAY(SND_FB_CLICK); }

// TTP223 logic: HIGH means touched
bool anyKeyPressed() {
  return digitalRead(BUTTON_PIN) == HIGH;
}

// ---- Setup ----
void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  pinMode(BUTTON_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  randomSeed(esp_random());

  prefs.begin("flappy", false);
  bestScore = prefs.getLong("best", 0);

  if (!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println(F("SH1106 allocation failed"));
    while (1) delay(100);
  }

  gameState = STARTSCREEN;
}

// ---- Main loop ----
void loop() {
  updateBuzzer();

  unsigned long currentTime = millis();
  if (currentTime - lastFrameTime < FRAME_TIME) return;
  lastFrameTime = currentTime;
  frameCounter++;

  switch (gameState) {
    case STARTSCREEN:
      renderStartScreen();
      if (anyKeyPressed()) {
        playClickSound();
        resetGame();
        gameState = PLAYING;
      }
      break;

    case PLAYING:
      if (gameOver) {
        if (millis() - gameOverTime > 3000) {
          gameState = HIGHSCORE;
          highscoreTime = millis();
        }
        if ((millis() - gameOverTime > 1000) && anyKeyPressed()) {
          playClickSound();
          resetGame();
          gameState = PLAYING;
        }
      } else {
        static bool lastButtonState = LOW;
        bool currentButtonState = digitalRead(BUTTON_PIN);
        if (currentButtonState == HIGH && lastButtonState == LOW) playFlapSound();
        lastButtonState = currentButtonState;

        if (currentButtonState == HIGH) velocity = JUMP_STRENGTH;
        velocity += GRAVITY;
        velocity = constrain(velocity, -50, 50);

        birdY += velocity;
        if (birdY < 0) birdY = 0;

        if (DEBUG_PRINT) { Serial.print("Vel: "); Serial.println(velocity); }

        movePipes();
        checkCollision();
      }
      drawGame();
      break;

    case HIGHSCORE:
      renderHighscoreScreen();
      if (millis() - gameOverTime > 8000) gameState = STARTSCREEN;
      if (anyKeyPressed()) {
        playClickSound();
        resetGame();
        gameState = PLAYING;
      }
      break;
  }
}

// ---- Reset game ----
void resetGame() {
  birdY    = SCREEN_HEIGHT * SCALE_FACTOR / 5;
  velocity = 0;
  score    = 0;
  gameOver = false;

  pipes[0].x      = SCREEN_WIDTH * SCALE_FACTOR;
  pipes[0].height = generatePipeHeight();
  for (int i = 1; i < MAX_PIPES; i++) {
    pipes[i].x      = SCREEN_WIDTH * SCALE_FACTOR + i * PIPE_SPACING + random(-20, 20) * SCALE_FACTOR;
    pipes[i].height = generatePipeHeight();
  }
}

int generatePipeHeight() {
  return random(10 * SCALE_FACTOR, SCREEN_HEIGHT * SCALE_FACTOR - PIPE_GAP - 10 * SCALE_FACTOR);
}

void movePipes() {
  int currentSpeed = PIPE_SPEED + score;
  for (int i = 0; i < MAX_PIPES; i++) {
    pipes[i].x -= currentSpeed;
    if (pipes[i].x < -PIPE_WIDTH) {
      pipes[i].x      = findFurthestPipe() + PIPE_SPACING + random(-20, 20) * SCALE_FACTOR;
      pipes[i].height = random(10 * SCALE_FACTOR, SCREEN_HEIGHT * SCALE_FACTOR - PIPE_GAP - 10 * SCALE_FACTOR);
      score++;
    }
  }
}

int findFurthestPipe() {
  int maxX = 0;
  for (int i = 0; i < MAX_PIPES; i++) if (pipes[i].x > maxX) maxX = pipes[i].x;
  return maxX;
}

// ---- Collision detection ----
void checkCollision() {
  for (int i = 0; i < MAX_PIPES; i++) {
    if (pipes[i].x < (15 + 2) * SCALE_FACTOR && pipes[i].x + PIPE_WIDTH > (15 - 2) * SCALE_FACTOR) {
      if ((birdY - 2 * SCALE_FACTOR) < pipes[i].height || (birdY + 2 * SCALE_FACTOR) > pipes[i].height + PIPE_GAP) {
        if (!gameOver) { gameOver = true; gameOverTime = millis(); playGameOverSound(); saveHighscores(score); }
      }
    }
  }
  if (birdY >= SCREEN_HEIGHT * SCALE_FACTOR) {
    if (!gameOver) { gameOver = true; gameOverTime = millis(); playGameOverSound(); saveHighscores(score); }
  }
}

// ---- Draw game ----
void drawGame() {
  display.clearDisplay();
  drawBird();
  drawPipes();
  drawScore();
  if (gameOver) displayGameOver();
  display.display();
}

void drawBird() {
  int birdFrame = (frameCounter / 5) % 2;
  display.drawBitmap(15 - BIRD_WIDTH_PX / 2, birdY / SCALE_FACTOR - BIRD_HEIGHT_PX / 2,
                     epd_bitmap_allArray[birdFrame], BIRD_WIDTH_PX, BIRD_HEIGHT_PX, SH110X_WHITE);
}

void drawPipes() {
  for (int pipeIndex = 0; pipeIndex < MAX_PIPES; pipeIndex++) {
    int pipeX          = pipes[pipeIndex].x / SCALE_FACTOR;
    int upperPipeHeight = pipes[pipeIndex].height / SCALE_FACTOR;
    int gapHeight      = PIPE_GAP / SCALE_FACTOR;
    int lowerPipeY     = upperPipeHeight + gapHeight;
    int lowerPipeHeight = SCREEN_HEIGHT - lowerPipeY;
    int pipeWidth      = PIPE_WIDTH / SCALE_FACTOR;

    display.fillRect(pipeX, 0, pipeWidth, upperPipeHeight - 6, SH110X_WHITE);
    display.fillRect(pipeX - 2, upperPipeHeight - 6, pipeWidth + 4, 6, SH110X_WHITE);
    display.fillRect(pipeX - 2, lowerPipeY, pipeWidth + 4, 6, SH110X_WHITE);
    display.fillRect(pipeX, lowerPipeY + 6, pipeWidth, lowerPipeHeight - 6, SH110X_WHITE);
  }
}

void drawScore() {
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(110, 2);
  display.print(score);
}

// ---- Start screen ----
void renderStartScreen() {
  display.clearDisplay();

  for (int i = 0; i < 3; i++) {
    if (i == 1) display.drawBitmap(cloudX[i], cloudY[i], cloud_small, 8, 5, SH110X_WHITE);
    else        display.drawBitmap(cloudX[i], cloudY[i], cloud_large, 16, 6, SH110X_WHITE);
    cloudX[i] -= cloudSpeed[i];
    if (cloudX[i] < -16) { cloudX[i] = 128 + random(5, 25); cloudY[i] = random(0, 7); }
  }

  display.drawRect(19, 9, 90, 18, SH110X_WHITE);
  display.drawRect(21, 11, 86, 14, SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(28, 14);
  display.print("FLAPPY  BIRD");

  int lpX = 3;
  display.fillRect(lpX, 10, 10, 12, SH110X_WHITE);
  display.fillRect(lpX - 2, 22, 14, 4, SH110X_WHITE);
  display.fillRect(lpX - 2, 42, 14, 4, SH110X_WHITE);
  display.fillRect(lpX, 46, 10, 11, SH110X_WHITE);

  int rpX = 115;
  display.fillRect(rpX, 10, 10, 15, SH110X_WHITE);
  display.fillRect(rpX - 2, 25, 14, 4, SH110X_WHITE);
  display.fillRect(rpX - 2, 45, 14, 4, SH110X_WHITE);
  display.fillRect(rpX, 49, 10, 8, SH110X_WHITE);

  int bobOffset  = (int)(3.0 * sin(frameCounter * 0.15));
  int birdScreenY = 34 + bobOffset;
  int birdFrame  = (frameCounter / 5) % 2;
  display.drawBitmap(59, birdScreenY, epd_bitmap_allArray[birdFrame], BIRD_WIDTH_PX, BIRD_HEIGHT_PX, SH110X_WHITE);

  // Best score (centered, always visible)
  if (bestScore > 0) {
    char bestBuf[20];
    sprintf(bestBuf, "BEST: %ld", bestScore);
    int len = strlen(bestBuf);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(64 - (len * 6) / 2, 37);
    display.print(bestBuf);
  }

  if ((frameCounter / 12) % 2 == 0) {
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(22, 47);
    display.print("Press to start");
  }

  int groundY = 57;
  for (int x = -groundScrollOffset; x < 128; x += 6) {
    display.drawPixel(x, groundY - 1, SH110X_WHITE);
    display.drawPixel(x + 1, groundY - 2, SH110X_WHITE);
    display.drawPixel(x + 2, groundY - 1, SH110X_WHITE);
  }
  display.fillRect(0, groundY, 128, 7, SH110X_WHITE);
  for (int x = -groundScrollOffset; x < 128; x += 8) {
    display.drawFastHLine(x, groundY + 2, 3, SH110X_BLACK);
    display.drawFastHLine(x + 4, groundY + 5, 2, SH110X_BLACK);
  }
  groundScrollOffset = (groundScrollOffset + 1) % 8;

  display.display();
}

// ---- Game Over overlay ----
void displayGameOver() {
  display.fillRect(20, 8, 88, 48, SH110X_BLACK);
  display.drawRect(20, 8, 88, 48, SH110X_WHITE);
  display.drawRect(22, 10, 84, 44, SH110X_WHITE);

  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(37, 14);
  display.print("GAME OVER");

  display.drawFastHLine(30, 24, 68, SH110X_WHITE);
  display.setCursor(49, 28);
  display.print("SCORE");

  display.setTextSize(2);
  char scoreBuffer[10];
  sprintf(scoreBuffer, "%d", score);
  int16_t x1, y1; uint16_t tw, th;
  display.getTextBounds(scoreBuffer, 0, 0, &x1, &y1, &tw, &th);
  display.setCursor(64 - tw / 2, 36);
  display.print(scoreBuffer);
}

// ---- Highscore screen ----
void renderHighscoreScreen() {
  display.clearDisplay();

  display.drawRect(19, 1, 90, 16, SH110X_WHITE);
  display.drawRect(21, 3, 86, 12, SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(34, 5);
  display.print("GAME  OVER");

  display.setTextSize(2);
  char scoreBuffer[10];
  sprintf(scoreBuffer, "%d", score);
  int16_t x1, y1; uint16_t tw, th;
  display.getTextBounds(scoreBuffer, 0, 0, &x1, &y1, &tw, &th);
  display.setCursor(64 - tw / 2, 19);
  display.print(scoreBuffer);

  display.setTextSize(1);
  if (isNewBest) {
    if ((frameCounter / 10) % 2 == 0) { display.setCursor(25, 37); display.print("* NEW BEST! *"); }
  } else {
    char bestBuf[20];
    sprintf(bestBuf, "BEST: %ld", bestScore);
    int len = strlen(bestBuf);
    display.setCursor(64 - (len * 6) / 2, 37);
    display.print(bestBuf);
  }

  display.drawBitmap(8, 49, epd_bitmap_bird_dead, BIRD_WIDTH_PX, BIRD_HEIGHT_PX, SH110X_WHITE);

  if ((frameCounter / 12) % 2 == 0) { display.setCursor(28, 47); display.print("Tap to retry"); }

  int groundY = 57;
  for (int x = -groundScrollOffset; x < 128; x += 6) {
    display.drawPixel(x, groundY - 1, SH110X_WHITE);
    display.drawPixel(x + 1, groundY - 2, SH110X_WHITE);
    display.drawPixel(x + 2, groundY - 1, SH110X_WHITE);
  }
  display.fillRect(0, groundY, 128, 7, SH110X_WHITE);
  for (int x = -groundScrollOffset; x < 128; x += 8) {
    display.drawFastHLine(x, groundY + 2, 3, SH110X_BLACK);
    display.drawFastHLine(x + 4, groundY + 5, 2, SH110X_BLACK);
  }
  groundScrollOffset = (groundScrollOffset + 1) % 8;

  display.display();
}

// ---- Save best score ----
void saveHighscores(int newScore) {
  isNewBest = false;
  if (newScore > bestScore) {
    bestScore = newScore;
    isNewBest = true;
    prefs.putLong("best", bestScore);
  }
}
