#include <Arduino.h>

const int clockPin = DD3;  // Clock input pin
const int controller_pins[4] = {DD4, DD5, DD6, DD7}

bool newdata = false;
static boolean recvInProgress = false;
static byte index = 0;
char startMarker = '#';
char endMarker = '*';
char rc;


// Array to store button state
bool buttonState[32];

// Array to store controller state
bool controllerState[4];
// Initialize counter variable
int counter = 0;

int last_time = 0;
int ndx = 0;
bool newData = false;

void setup() {
    // Initialize serial communication
    Serial.begin(9600);

    // Set dataPin and controlPin as outputs
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(controlPin, OUTPUT);
}
void checkSerial() {
    char value = Serial.read();

    if (recvInProgress) {
        if (value == '>') {
            recvInProgress = false;
            ndx = 0;
            newData = true;
//            Serial.println("stop reciving");
            return;
        }
        if (ndx < 32) {
            buttonState[ndx] = value;
//            Serial.print(ndx);
//            Serial.print(" ");
//            Serial.print((int)value);
//            Serial.println(" button");
        } else if (ndx < 36) {
            controllerState[ndx-32] = value;
//            Serial.print(ndx);
//            Serial.print(" ");
//            Serial.print((int)value);
//            Serial.println(" controller");
        } else {
            Serial.println("Error: End byte not received");
        }
        ndx++;
        if (ndx >= 36) {
            ndx = 36 - 1;
        }
    } else if (value == '<') {
        recvInProgress = true;
//        Serial.println("reciving");
    }
}

void update_output() {
    if (counter < 7) {
        for (int i = 0; i < 4; i++) {
            digitalWrite(controller_pins[i], 0);
        }
    } else if (counter == 7) {
        for (int i = 0; i < 4; i++) {
            digitalWrite(controller_pins[i], controllerState[i]);
        }
    }else if (counter > 7){
        for (int i = 0; i < 4; i++) {
            digitalWrite(controller_pins[i], buttonState[(i*4)+(counter-8)]);
        }
    }
    counter++;
    if (counter >= 16) {
        counter = 0;
    }
}

void loop() {
    while (Serial.available() > 0) {
        checkSerial();
    }
    // Wait for start byte
//    if (Serial.available() && Serial.read() == '#') {
//
//        // Read in button state
//        for (bool &i: buttonState) {
//            i = (bool) Serial.read();
//            Serial.print(i);
//        }
//
//        // Read in controller state
//        for (bool &i: controllerState) {
//            i = (bool) Serial.read();
//            Serial.print(i);
//        }
//
//        // Check for end byte
//        if (Serial.read() != '*') {
//            // Raise error
//            Serial.println("Error: End byte not received");
//            return;
//        }
//    }
//    delay(500);

//    for (bool &i: buttonState) {
//        Serial.print(i);
//    }
//    Serial.print("  ");
//    // Read in controller state
//    for (bool &i: controllerState) {
//        Serial.print(i);
//    }
//    Serial.print("\n");

        // Wait for clock signal to go high
    while (digitalRead(clockPin) == LOW) {}
//    // Wait for clock signal to go low
    while (digitalRead(clockPin) == HIGH) {}
        // Set dataPin and controlPin based on counter value
        update_output();
    }
}
