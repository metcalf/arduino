#include <Wire.h>

#include <SparkFun_Alphanumeric_Display.h>
#include <SparkFun_SGP40_Arduino_Library.h>
#include <SparkFun_SCD4x_Arduino_Library.h>
#include <SparkFun_Qwiic_OpenLog_Arduino_Library.h>

HT16K33 display;
SGP40 sgpSensor;
SCD4x scdSensor;
OpenLog myLog;

int BUTTON_PIN = 12;
unsigned long CALIBRATE_HOLD_MILLIS = 5000UL;
unsigned long CALIBRATE_WAIT_MILLIS = 60UL * 4UL * 1000UL;
int CALIBRATE_PPM = 415;

long buttonPressStart = -1;

void setup() {
    Serial.begin(115200);
    Serial.println("Display init");
    Wire.begin(); // Join I2C

    //check if display will acknowledge
    if (display.begin() == false)
    {
        Serial.println("Display did not acknowledge! Freezing.");
        while(1);
    }

    display.print("INIT");

    //sgpSensor.enableDebugging();
    if (sgpSensor.begin() == false) {
        Serial.println(F("SGP40 not detected. Check connections. Freezing..."));
        display.print("ERRM");
        while (1);
    }

    //scdSensor.enableDebugging();
    if (scdSensor.begin(true, false, false) == false) {
        Serial.println(F("SCD40 not detected. Check connections. Freezing..."));
        display.print("ERRP");
        while (1);
    }

//    if (myLog.begin() == false) {
//      Serial.println(F("OpenLog init error. Freezing..."));
//      display.print("ERRL");
//      while(1);
//    }

    Serial.println(F("CO2\tT\tRH\tVOC\tmillis"));

    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  if (scdSensor.readMeasurement()) { // readMeasurement will return true when fresh data is available
    uint16_t co2 = scdSensor.getCO2();
    float temp = scdSensor.getTemperature();
    float rh = scdSensor.getHumidity();

    int32_t vocIndex = sgpSensor.getVOCindex(rh, temp);

    if (co2 > 9999) {
        display.print("HIGH");
    } else {
        display.print(String(co2));
    }

    String msg = String(co2) + "\t" + String(temp, 1) + "\t" + String(rh, 1) + "\t" + String(vocIndex) + "\t" + String(millis());

    Serial.println(msg);
//    myLog.println(msg);
  }

  checkCal();
  delay(500);
}

void checkCal() {
  if (digitalRead(BUTTON_PIN) == HIGH) {
    buttonPressStart = -1;
    return;
  }
  if (buttonPressStart == -1) {
    buttonPressStart = millis();
    return;
  }

  if((millis() - buttonPressStart) < CALIBRATE_HOLD_MILLIS) {
    return;
  }

  // Datasheet says wait > 3 minutes
  Serial.println("Waiting 4 minutes before calibrating");
  display.print("CAL");
  delay(CALIBRATE_WAIT_MILLIS);

  Serial.println("Stopping measurements");
  if(scdSensor.stopPeriodicMeasurement() == false) {
    display.print("ERRC");
    while(1);
  }

  Serial.println("Calibrating");
  float correction = scdSensor.performForcedRecalibration(CALIBRATE_PPM);
  if (correction == 0.0) {
    display.print("ERRC");
    while(1);
  }

  Serial.println("Calibrated, correction: " + String(correction, 2));

  if(scdSensor.startPeriodicMeasurement() == false) {
    display.print("ERRC");
    while(1);
  }
}
