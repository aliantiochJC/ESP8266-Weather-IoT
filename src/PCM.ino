const int ledPin = 2; // built-in LED, active-LOW
const int visualSampleRate = 50; // samples per second — slow enough to actually see

struct Note { float freq; int durationMs; }; // freq here means "cycles per second" of brightness, not audio pitch
Note melody[] = {
  {1.0, 1000}, // one full brightness cycle over 1 second
  {2.0, 1000}, // two cycles over 1 second (faster pulsing)
  {0.5, 1500}  // slow half-cycle, long fade
};
const int numNotes = 3;

int noteIndex = 0;
unsigned long sampleCounter = 0;
unsigned long lastSampleTime = 0;
bool playing = true;

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  if (!playing) return;

  unsigned long now = millis();
  if (now - lastSampleTime < (1000 / visualSampleRate)) return; // wait until next sample is due
  lastSampleTime = now;

  Note &n = melody[noteIndex];
  unsigned long samplesForNote = (n.durationMs * visualSampleRate) / 1000;

  float t = (float)sampleCounter / visualSampleRate;
  float value = sinf(2.0 * PI * n.freq * t); // -1 to 1

  int brightness = (int)(511 + value * 511); // 0–1023, centered
  analogWrite(ledPin, 1023 - brightness); // inverted because active-LOW

  sampleCounter++;
  if (sampleCounter >= samplesForNote) {
    sampleCounter = 0;
    noteIndex++;
    if (noteIndex >= numNotes) {
      noteIndex = 0; // loop the pattern so you can keep watching it
    }
  }
}