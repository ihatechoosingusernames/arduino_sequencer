//  The intent of this project is to create an Analog
//  three track sequencer that outputs a recorded and
//  quantised voltage output for each step and channel.

#include <Adafruit_NeoPixel.h>

// Pin defines
#define neopixel_pin 14
#define num_pixels 8

#define channel_1_out 3
#define channel_2_out 5
#define channel_3_out 6

#define channel_1_gate 11
#define channel_2_gate 12
#define channel_3_gate 13

#define channel_1_in 7
#define channel_2_in 8
#define channel_3_in 9

#define clock_out 18	// A4

#define vc_input 15	// A1
#define speed_input 17	// A3
#define channel_select_pin 2
#define record_pin 4
#define skip_pin 10

// Global defines
#define counter_period 10	// milliseconds
#define num_channels 3
#define max_steps 32

struct Button {
	int pin;
	bool is_held = false;
};

struct Channel {
	byte sequence[max_steps];
	byte step_counter = 0;
	byte steps = 0;
  bool play = false;

  Button button;

	int out_pin;
	int gate_pin;
};

// Global definitions
unsigned int ms_per_clock = 500;
unsigned int clock_counter = 0;

byte selected_channel = 0;
bool channel_selected = false;

Channel channel[3];
Button channel_select, record, skip;
Adafruit_NeoPixel pixels(num_pixels, neopixel_pin, NEO_GRB + NEO_KHZ800);

void setup() {

	// For testing
	Serial.begin(9600);

  pixels.begin();

	// Set clock dividers to 1 to achieve clock speed of 31250 Hz on the PWM pins
	TCCR1B = TCCR1B & 0b11111000 | 0x01;
  TCCR2B = TCCR2B & 0b11111000 | 0x01;

  // Set up channels
  pinMode(channel_1_out, OUTPUT);
  pinMode(channel_2_out, OUTPUT);
  pinMode(channel_3_out, OUTPUT);

  pinMode(channel_1_gate, OUTPUT);
  pinMode(channel_2_gate, OUTPUT);
  pinMode(channel_3_gate, OUTPUT);

  pinMode(channel_1_in, INPUT_PULLUP);
  pinMode(channel_2_in, INPUT_PULLUP);
  pinMode(channel_3_in, INPUT_PULLUP);

  channel[0].out_pin = channel_1_out;
  channel[0].button.pin = channel_1_in;
  channel[0].gate_pin = channel_1_gate;

  channel[1].out_pin = channel_2_out;
  channel[1].button.pin = channel_2_in;
  channel[1].gate_pin = channel_2_gate;

  channel[2].out_pin = channel_3_out;
  channel[2].button.pin = channel_3_in;
  channel[2].gate_pin = channel_3_gate;

  pinMode(vc_input, INPUT);
  pinMode(speed_input, INPUT);

  pinMode(channel_select_pin, INPUT_PULLUP);
  channel_select.pin = channel_select_pin;

  pinMode(record_pin, INPUT_PULLUP);
  record.pin = record_pin;

  pinMode(skip_pin, INPUT_PULLUP);
  skip.pin = skip_pin;

  pinMode(clock_out, OUTPUT);
}

void loop() {
  delay(counter_period);

	digitalWrite(clock_out, LOW);
  checkInputs();

  if ((++clock_counter * counter_period) >= ms_per_clock) {
  	clock_counter = 0;
	  digitalWrite(clock_out, HIGH);

	  updateChannel(&channel[0]);
	  updateChannel(&channel[1]);
	  updateChannel(&channel[2]);

	  showLEDs();
  }
}

void updateChannel(Channel* channel) {
	if (!channel->play) {
		return;
	}

  if (channel->step_counter == channel->steps) {
		channel->step_counter = 0;
	} else {
	  channel->step_counter++;
	}

	Serial.print("Channel at step ");
	Serial.println(channel->step_counter);

	analogWrite(channel->out_pin, channel->sequence[channel->step_counter]);
	digitalWrite(channel->gate_pin, channel->sequence[channel->step_counter] != 0);
}

void checkInputs() {
	if (digitalRead(channel_select.pin)) {
		if (!channel_select.is_held) {
			channel_selected = false;
			channel_select.is_held = true;
	    Serial.println("Channel select pressed");
		}
	} else if (channel_select.is_held) {	// On channel_select release...
		channel_select.is_held = false;
    Serial.println("Channel select released");

		if (!channel_selected) {
      Serial.println("Channel select pressed on its own");
			selected_channel = num_channels;	// Defaults to all channels if none selected

			if (channel[0].play || channel[1].play || channel[2].play) {	// Pauses all channels if any are playing
				channel[0].play = false;
				channel[1].play = false;
				channel[2].play = false;
			} else {			// Otherwise plays all channels
				channel[0].play = true;
				channel[1].play = true;
				channel[2].play = true;
			}
		}
	}

	for (byte index = 0; index < num_channels; index++) {
		if (digitalRead(channel[index].button.pin)) {
			if (!channel[index].button.is_held) {
				channel[index].button.is_held = true;

				if (channel_select.is_held) {
					if (channel_selected == true && selected_channel == index) {	// If channel double tapped while channel select is held, clear buffer
						Serial.print("Channel ");
						Serial.print(index);
						Serial.println(" cleared");
						channel[index].steps = 0;
						channel[index].step_counter = 0;
					} else {
						Serial.print("Channel ");
						Serial.print(index);
						Serial.println(" selected");
						selected_channel = index;
						channel_selected = true;
					}
				} else {	// Pause/Play if channel button individually pressed
					channel[index].play = !channel[index].play;
					Serial.print("Channel ");
					Serial.print(index);
					if (channel[index].play) {
						Serial.println(" playing");
					} else {
						Serial.println(" paused");
					}
				}
			}
		} else if (channel[index].button.is_held) {
			channel[index].button.is_held = false;	// Reset is_held if button no longer pressed
		}
	}

	unsigned int speed_in = analogRead(speed_input);
	if (speed_in != ms_per_clock) {
		ms_per_clock = speed_in;
	}

	if (!digitalRead(record.pin)) {
		if (!record.is_held && selected_channel < num_channels) {
			record.is_held = true;

			int val = (analogRead(vc_input) >> 3);
		
			channel[selected_channel].sequence[channel[selected_channel].step_counter] = val;

			Serial.print("Recorded value of ");
			Serial.print(val);
			Serial.print(" to step ");
			Serial.print(channel[selected_channel].step_counter);
			Serial.print(" of ");
			Serial.println(channel[selected_channel].steps);

			if (!channel[selected_channel].play) {	// If channel paused
				if (channel[selected_channel].step_counter == channel[selected_channel].steps && channel[selected_channel].steps <= max_steps) {	// If this is the last step
					++channel[selected_channel].steps;	// Add another step
				}
				++channel[selected_channel].step_counter;
			}
		}
	} else {
		record.is_held = false;
	}

	if (!digitalRead(skip.pin)) {
		if (!skip.is_held && selected_channel < num_channels) {
			skip.is_held = true;
		
			channel[selected_channel].sequence[channel[selected_channel].step_counter] = 0;

			Serial.print("Skipped value of ");
			Serial.print(analogRead(vc_input) >> 3);
			Serial.print(" at step ");
			Serial.print(channel[selected_channel].step_counter);
			Serial.print(" of ");
			Serial.println(channel[selected_channel].steps);

			if (!channel[selected_channel].play) {	// If channel paused
				if (channel[selected_channel].step_counter == channel[selected_channel].steps && channel[selected_channel].steps <= max_steps) {	// If this is the last step
					++channel[selected_channel].steps;	// Add another step
				}
				++channel[selected_channel].step_counter;
			}
		}
	} else {
		skip.is_held = false;
	}
}

void showLEDs() {
  pixels.clear();
  pixels.setBrightness(28);
  
	for (unsigned int step_index = 0; step_index < num_pixels; step_index++) {
		unsigned int colours[3];
		for (unsigned int channel_index = 0; channel_index < num_channels; channel_index++) {
			if (channel_index == selected_channel || selected_channel == num_channels) {
				unsigned int step = (channel[channel_index].step_counter + step_index) % channel[channel_index].steps;
				colours[channel_index] = channel[channel_index].sequence[step];
			} else {
				colours[channel_index] = 0;
			}
		}

		pixels.setPixelColor(step_index, pixels.Color(colours[0], colours[1], colours[2]));
	}

	pixels.show();
}

void printState() {
  for (unsigned int channel_index = 0; channel_index < num_channels; channel_index++) {
	  Serial.print("");
  }
}
