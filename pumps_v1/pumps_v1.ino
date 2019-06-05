class Pump {
  private:
    int timeToSwitchSec;
    int runPeriodSec;
    int sleepPeriodSec;
    boolean runState;
    int pin;

  void enable() {
    Serial.println("Enabling pump");
    runState = true;
    timeToSwitchSec = runPeriodSec;
    digitalWrite(pin, HIGH);  
  }
  
  void disable() {
    Serial.println("Disabling pump");
    runState = false;
    timeToSwitchSec = sleepPeriodSec;
    digitalWrite(pin, LOW);  
  }
  
  public:

  Pump(int runTime, int sleepTime, int pumpPin) {
    timeToSwitchSec = 0;   
    runPeriodSec = runTime; 
    sleepPeriodSec = sleepTime;
    runState = false;
    pin = pumpPin;
  }
  
  void tickTime() {
    if (timeToSwitchSec > 0) {
      --timeToSwitchSec;
    }
  }

  void switchStateIfNeeded() {
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
};

Pump* pumps[2];
int pumpsCount;

void setup() {
  pumps[0] = new Pump(2, 60, PD3);
  pumps[1] = new Pump(3, 60, PD4);
  pumpsCount = 2;
  for (int i = 0; i < pumpsCount; i++) {
    pumps[i]->initPin();
  }
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(PD2), startISR, FALLING);
}

volatile boolean started = false;
volatile unsigned long lastStarted = 0;

void startISR() {
  unsigned long now = millis();
  if (now - lastStarted > 2000) {
    lastStarted = now;
    started = !started;
  }
}

boolean wasStarted = false;

void loop() {
  if (started) {
    digitalWrite(13, HIGH);
    wasStarted = true;
    runRoutine();       
  } else {
    if (wasStarted){
      interruptAllPumps();
      wasStarted = false;
    }
    digitalWrite(13, LOW);
  }
}

unsigned long lastSec = 0;
void runRoutine() {
  unsigned long nowSec = millis() / 1000;
  if (nowSec - lastSec > 0) {
    lastSec = nowSec;
    tickRoutine();
  }
}

void tickRoutine() {
  Serial.println("tick");
  for (int i = 0; i < pumpsCount; i++) {
    Pump* p = pumps[i];
    p->tickTime();
    p->switchStateIfNeeded();
  }
}

void interruptAllPumps() {
  for (int i = 0; i < pumpsCount; i++) {
    Pump* p = pumps[i];
    p->interrupt();
  }
}
