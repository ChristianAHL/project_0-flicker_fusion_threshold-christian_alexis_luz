#include <Arduino.h>

//Pin assignments
#define LED_PIN 12
#define BUTTON_INPUT 15
#define ANALOG_FREQUENCY_INPUT_PIN 14

//Configuration parameters
uint16_t DEBOUNCE_DELAY_MS = 200;

//Global variable declarations
uint16_t frequency_input_raw_value = 0; //Set variable size to the next highest vale closest to 10-bits
static double blink_half_period_us = 0;
volatile bool frequency_requested_flag = false; //To prevent optimization by the compiler, best for variables manipulated in interrupts

void ISR_button_press(); //Function prototype for button press

void setup()
{
  Serial.begin(9600);                                       //Set baud rate
  pinMode(LED_PIN, OUTPUT);                                 //Set pin to output mode for LED
  pinMode(ANALOG_FREQUENCY_INPUT_PIN, INPUT);               //Set pin to input mode for A/D conversion
  pinMode(BUTTON_INPUT, INPUT_PULLUP);                      //Set pin for button input to input mode, with internal pull up
  attachInterrupt(BUTTON_INPUT, ISR_button_press, FALLING); //Attach the button input to an external interrupt function
}

void loop()
{
  //Local variable declarations
  static uint32_t previous_micros_us = 0;
  static double threshold_frequency_hz = 0;
  static bool led_state_flag = LOW; //Why do we use state flags?
  uint32_t current_micros_us = micros(); //Save current elapsed time to current_micros_us

  frequency_input_raw_value = analogRead(ANALOG_FREQUENCY_INPUT_PIN);       //Get raw digital value of analog input from potentiometer
  blink_half_period_us = map(frequency_input_raw_value, 0, 1023, 10, 40000); //map raw potentiometer value to a half period duration in us

  if (frequency_requested_flag == true)
  {
    threshold_frequency_hz =  1000000 / (2 * blink_half_period_us); //Frequency is the reciprocal of the period
    Serial.print("Your flicker fusion frequency threshold is: ");
    Serial.print(threshold_frequency_hz, 2);
    Serial.println(" Hz");
    frequency_requested_flag = false;
  }

  if ((current_micros_us - previous_micros_us) > blink_half_period_us)
  {
    led_state_flag = !led_state_flag; //toggle led state
    previous_micros_us = current_micros_us;
    digitalWrite(LED_PIN, led_state_flag);
  }
}

void ISR_button_press()
{

  noInterrupts(); //Prevent other interrupts
  static uint32_t previous_press_time_ms = 0;
  uint32_t current_press_time_ms = millis();

  if ((current_press_time_ms - previous_press_time_ms) > DEBOUNCE_DELAY_MS)
  {
    frequency_requested_flag = true;
    previous_press_time_ms = current_press_time_ms;
  }
  interrupts(); //Enable other interrupts
}