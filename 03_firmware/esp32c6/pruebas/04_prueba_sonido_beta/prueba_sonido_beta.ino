#define SPEAKER_PIN 19
#define TOUCH_PIN 2

void setup() {

  pinMode(TOUCH_PIN, INPUT);

  ledcAttach(SPEAKER_PIN, 2000, 8);
}

void loop() {

  if (digitalRead(TOUCH_PIN) == HIGH) {

    ledcWriteTone(SPEAKER_PIN, 262);
    delay(150);

    ledcWriteTone(SPEAKER_PIN, 330);
    delay(150);

    ledcWriteTone(SPEAKER_PIN, 392);
    delay(150);

    ledcWriteTone(SPEAKER_PIN, 523);
    delay(250);

    ledcWriteTone(SPEAKER_PIN, 0);

    delay(500);
  }
}
