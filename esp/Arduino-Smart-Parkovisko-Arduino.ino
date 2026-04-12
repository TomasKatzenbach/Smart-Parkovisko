#define NUM_SENSORS 8

const byte trigPins[NUM_SENSORS] = {2, 4, 6, 8, 10, 12, A0, A2};
const byte echoPins[NUM_SENSORS] = {3, 5, 7, 9, 11, 13, A1, A3};

const char* spotNames[NUM_SENSORS] = {
  "spot1", "spot2", "spot3", "spot4",
  "spot5", "spot6", "spot7", "spot8"
};

const int OCCUPIED_THRESHOLD = 8;
const int FREE_THRESHOLD = 10;

const unsigned long OCCUPIED_CONFIRM_TIME = 5000;
const unsigned long FREE_CONFIRM_TIME = 3000;
const unsigned long ERROR_CONFIRM_TIME = 2000;

const unsigned long ECHO_TIMEOUT = 30000UL;
const int MAX_ERRORS = 5;
const unsigned long SENSOR_DELAY_MS = 60;

String lastState[NUM_SENSORS];
String pendingState[NUM_SENSORS];

unsigned long stateChangeStart[NUM_SENSORS];
int errorCount[NUM_SENSORS];

long measureDistance(byte trigPin, byte echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH, ECHO_TIMEOUT);

  if (duration == 0) return -1;

  long distance = duration * 0.034 / 2;

  if (distance < 2 || distance > 400) return -1;

  return distance;
}

String getStateFromDistance(long distance, const String& previousState) {
  if (previousState == "Unknown" || previousState == "Error") {
    return (distance <= OCCUPIED_THRESHOLD) ? "Occupied" : "Free";
  }

  if (previousState == "Free" && distance <= OCCUPIED_THRESHOLD) {
    return "Occupied";
  }

  if (previousState == "Occupied" && distance >= FREE_THRESHOLD) {
    return "Free";
  }

  return previousState;
}

unsigned long getConfirmTimeForState(const String& state) {
  if (state == "Occupied") return OCCUPIED_CONFIRM_TIME;
  if (state == "Free") return FREE_CONFIRM_TIME;
  if (state == "Error") return ERROR_CONFIRM_TIME;
  return FREE_CONFIRM_TIME;
}

void sendState(int index, const String& state, long distance) {
  Serial.print(spotNames[index]);
  Serial.print(":");
  Serial.print(state);
  Serial.print(":");
  Serial.println(distance);
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    digitalWrite(trigPins[i], LOW);

    lastState[i] = "Unknown";
    pendingState[i] = "Unknown";
    stateChangeStart[i] = 0;
    errorCount[i] = 0;
  }
}

void loop() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    long distance = measureDistance(trigPins[i], echoPins[i]);
    String newState;

    if (distance == -1) {
      errorCount[i]++;
      if (errorCount[i] >= MAX_ERRORS) {
        newState = "Error";
      } else {
        delay(SENSOR_DELAY_MS);
        continue;
      }
    } else {
      errorCount[i] = 0;
      newState = getStateFromDistance(distance, lastState[i]);
    }

    if (newState != pendingState[i]) {
      pendingState[i] = newState;
      stateChangeStart[i] = millis();
    }

    if (pendingState[i] != lastState[i]) {
      unsigned long requiredTime = getConfirmTimeForState(pendingState[i]);

      if (millis() - stateChangeStart[i] >= requiredTime) {
        lastState[i] = pendingState[i];
        sendState(i, lastState[i], distance);
      }
    }

    delay(SENSOR_DELAY_MS);
  }
}