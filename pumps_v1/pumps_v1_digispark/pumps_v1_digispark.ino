#include <avr/power.h>

class Pump {
  private:
    int timeToSwitchSec;
    int runPeriodSec;
    int sleepPeriodSec;
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
const int timeScale = 16;
boolean wasStarted = false;
unsigned long buttonPressedTime = 0;
boolean started = false;
unsigned long lastStarted = 0;
unsigned long lastMSec = 0;

void setup() {
  clock_prescale_set(clock_div_16); // уменьшим частоту

  pumps[0] = new Pump(10, 10, PB0);
  pumps[1] = new Pump(20, 10, PB1);
  for (int i = 0; i < pumpsCount; i++) {
    pumps[i]->initPin();
  }
  pinMode(PB2, INPUT);
  pinMode(PB3, INPUT);
  pinMode(PB4, INPUT);
  pinMode(PB5, INPUT);
}

unsigned long diff(unsigned long now, unsigned long prev) {
  return (now - prev) * timeScale;
}

void switchGlobalState(unsigned long now) {
  if (diff(now, lastStarted) / 1000 > 2) {
    lastStarted = now;
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
    unsigned long now = millis();
    if (buttonPressedTime == 0) {
      buttonPressedTime = now;
    }
    if (diff(now, buttonPressedTime) > 500) {
      switchGlobalState(now);
    }
  }
}

void runRoutine() {
  unsigned long nowMSec = millis();
  if (diff(nowMSec, lastMSec) / 1000 > 0) {
    lastMSec = nowMSec;
    tickRoutine();
  }
}

void tickRoutine() {
  for (int i = 0; i < pumpsCount; i++) {
    Pump* p = pumps[i];
    if (p->tickTime()) {
      p->switchState();
    }
  }
}

void interruptAllPumps() {
  for (int i = 0; i < pumpsCount; i++) {
    Pump* p = pumps[i];
    p->interrupt();
  }
}
