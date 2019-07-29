#define HOUR 3600UL

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
    Pump(unsigned long runTime, unsigned long intervalTime, unsigned int pumpPin) {
      timeToSwitchSec = 0;
      runPeriodSec = runTime;
      sleepPeriodSec = intervalTime - runTime;
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
unsigned long lastSec = 0;

void setup() {
  //clock_prescale_set(clock_div_16); // уменьшим частоту

  pumps[0] = new Pump(60, 12UL * HOUR, PB0);
  pumps[1] = new Pump(60, 12UL * HOUR, PB1);
  for (int i = 0; i < pumpsCount; i++) {
    pumps[i]->initPin();
  }
  pinMode(PB2, INPUT_PULLUP);
  pinMode(PB4, OUTPUT);
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
  if (digitalRead(PB2) == HIGH) {
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
  unsigned long nowSec = millis() / 1000;
  unsigned long diffTime = diff(nowSec, lastSec);
  if (diffTime > 0) {
    lastSec = nowSec;
    for (unsigned long i = 0; i < diffTime; ++i) {
      tickRoutine(nowSec);
    }
  }
}

void tickRoutine(unsigned long nowSec) {
  boolean anyEnabled = false;
  for (int i = 0; i < pumpsCount; i++) {
    Pump* p = pumps[i];
    if (p->tickTime()) {
      p->switchState();
    }
    anyEnabled |= p->isEnabled();
  }
  if (anyEnabled || (nowSec % 2 == 0)) {
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
