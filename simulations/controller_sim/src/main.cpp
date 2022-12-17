#include <Arduino.h>

const int clockPin = DD3;  // Clock input pin
const int dataPin = DD4;   // Data output pin
const int controlPin = DD5;  // Control output pin

// Array to store button state
bool buttonState[32];

// Array to store controller state
bool controllerState[4];

// Initialize counter variable
int counter = 0;

void setup() {
    // Initialize serial communication
    Serial.begin(9600);

    // Set dataPin and controlPin as outputs
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(controlPin, OUTPUT);
}

void loop() {
    // Wait for start byte
    while (Serial.read() != '#') {}

    // Read in button state
    for (bool & i : buttonState) {
        i = (bool)Serial.read();
    }

    // Read in controller state
    for (bool & i : controllerState) {
        i = (bool)Serial.read();
    }

    // Check for end byte
    if (Serial.read() != '*') {
        // Raise error
        Serial.println("Error: End byte not received");
        return;
    }

    // Wait for clock signal to go high
    while (digitalRead(clockPin) == LOW) {}
    // Wait for clock signal to go low
    while (digitalRead(clockPin) == HIGH) {}
    // Set dataPin and controlPin based on counter value
    if (counter == 0) {
        digitalWrite(dataPin, buttonState[0]);
        digitalWrite(controlPin, HIGH);
    } else if (counter <= 4) {
        digitalWrite(dataPin, buttonState[counter]);
        digitalWrite(controlPin, controllerState[counter-1]);
    } else {
        digitalWrite(dataPin, buttonState[counter]);
        digitalWrite(controlPin, LOW);
    }

    // Main loop
    // Increment counter
    counter++;
    if (counter > 31) {
        counter = 0;
    }
}
