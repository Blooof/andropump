#include <avr/power.h>

class Pump {
  private:
    unsigned long timeToSwitchSec;
    unsigned long runPeriodSec;
    unsigned long sleepPeriodSec;
    boolean runState;
    unsigned int pin;

    void enable() {
      runState = true;
      timeToSwitchSec = runPeriodSec;
      digitalWrite(pin, HIGH);
    }

    void disable() {
      runState = false;
      timeToSwitchSec = sleepPeriodSec;
      digitalWrite(pin, LOW);
    }

  public:
    Pump(int runTime, int sleepTime, unsigned int pumpPin) {
      timeToSwitchSec = 0;
      runPeriodSec = runTime;
      sleepPeriodSec = sleepTime;
      runState = false;
      pin = pumpPin;
    }

    boolean tickTime() {
      if (timeToSwitchSec > 0) {
        --timeToSwitchSec;
      }
      return timeToSwitchSec == 0;
    }

    void switchState() {
      if (timeToSwitchSec > 0) {
        return;
      }
      if (runState) {
        disable();
      } else {
        enable();
      }
    }

    void initPin() {
      pinMode(pin, OUTPUT);
    }

    void interrupt() {
      disable();
      timeToSwitchSec = 0;
    }

    boolean isEnabled() {
      return runState;
    }
};

const int pumpsCount = 2;
Pump* pumps[pumpsCount];
const unsigned long timeScale = 1;
boolean wasStarted = false;
unsigned long buttonPressedTime = 0;
boolean started = false;
unsigned long lastStarted = 0;
unsigned long lastMSec = 0;

void setup() {
  //clock_prescale_set(clock_div_16); // уменьшим частоту

  pumps[0] = new Pump(120, 12 * 3600 - 120, PB0);
  pumps[1] = new Pump(300, 12 * 3600 - 300, PB1);
  for (int i = 0; i < pumpsCount; i++) {
    pumps[i]->initPin();
  }
  pinMode(PB2, INPUT);
  pinMode(PB3, INPUT);
  pinMode(PB4, OUTPUT);
  pinMode(PB5, INPUT);
}

unsigned long diff(unsigned long nowTime, unsigned long prevTime) {
  return (nowTime - prevTime) * timeScale;
}

void switchGlobalState(unsigned long nowTime) {
  if (diff(nowTime, lastStarted) / 1000 > 2) {
    lastStarted = nowTime;
    started = !started;
  }
}

void loop() {
  if (started) {
    wasStarted = true;
    runRoutine();
  } else {
    if (wasStarted) {
      interruptAllPumps();
      wasStarted = false;
    }
  }
  if (digitalRead(PB2) == LOW) {
    buttonPressedTime = 0;
  } else {
    unsigned long nowTime = millis();
    if (buttonPressedTime == 0) {
      buttonPressedTime = nowTime;
    }
    if (diff(nowTime, buttonPressedTime) > 500) {
      switchGlobalState(nowTime);
    }
  }
}

void runRoutine() {
  unsigned long nowMSec = millis();
  if (diff(nowMSec, lastMSec) / 1000 > 0) {
    lastMSec = nowMSec;
    tickRoutine(nowMSec);
  }
}

void tickRoutine(unsigned long nowMSec) {
  boolean anyEnabled = false;
  for (int i = 0; i < pumpsCount; i++) {
    Pump* p = pumps[i];
    if (p->tickTime()) {
      p->switchState();
    }
    anyEnabled |= p->isEnabled();
  }
  if (anyEnabled || ((nowMSec / 1000) % 2 == 0)) {
    digitalWrite(PB4, HIGH);
  } else {
    digitalWrite(PB4, LOW);
  }
}

void interruptAllPumps() {
  for (int i = 0; i < pumpsCount; i++) {
    Pump* p = pumps[i];
    p->interrupt();
  }
  digitalWrite(PB4, LOW);
}
