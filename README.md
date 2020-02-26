# arduino_sequencer
A simple synth control voltage sequencer for Arduino.

This design uses a low pass filter on the PWM pins of an Arduino Nano to create an output voltage between 0 - 5v. This is then amplified and offset with an op-amp to a -5 - 5v range with an 8 bit granularity. This is repeated for three parallel channels.

Input is via six buttons, two potentiometers, and a 3.5mm jack:
 - Tempo potentiometer
 - CV potentiometer
 - Record Button
 - Skip Button
 - Channel select button
 - Pause button per channel

Output is via two jacks per channel, and a Neopixel strip:
 - One jack per channel for a gate voltage to trigger whenever a note is played from the sequence
 - One jack per channel for the Control Voltage output
 - A Neopixel strip of 8 LEDs to display the upcoming notes in the sequence. Each channel is displayed as a different colour, and each LED represents a step
