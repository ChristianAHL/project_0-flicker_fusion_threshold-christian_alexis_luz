#include <Arduino.h> //Include the standard Arduino framwork library

//Pin assignments
#define LED_PIN 12                    //Pin to which output LED will be connected
#define BUTTON_INPUT 15               //Pin to which print frequency request push button will be connected
#define ANALOG_FREQUENCY_INPUT_PIN 14 //Pin to which potentiometer output will be connected to to set threshold frequency

//Configuration parameters
uint16_t DEBOUNCE_DELAY_MS = 200;    //Debounce time allocation window, in milliseconds. A/D conversion is 10 bit, size close to it was chosen
uint32_t MAX_HALF_PERIOD_US = 40000; //Maximum half period value of LED flicker signal, from which frequency will be calculated
uint32_t MIN_HALF_PERIOD_US = 10;    //Minimum half period value of LED flicker signal, from which frequency will be calculated

//Global variable declarations
uint16_t frequency_input_raw_value = 0;         //Raw converted analog signal on pin 14, to be mapped to a half period value
static double blink_half_period_us = 0;         //Half of period, from which frequency will be calculated for display
volatile bool frequency_requested_flag = false; //Volatile prevents optimization by the compiler, flag for button press event
void ISR_button_press();                        //Function prototype for button press, to facilitate

void setup() //Setup of pin modes, serial, and interrupt
{
  Serial.begin(9600);                                       //Set baud rate
  pinMode(LED_PIN, OUTPUT);                                 //Set pin to output mode for LED
  pinMode(ANALOG_FREQUENCY_INPUT_PIN, INPUT);               //Set pin to input mode for A/D conversion
  pinMode(BUTTON_INPUT, INPUT_PULLUP);                      //Set pin for button input to input mode, with internal pull up to avoid floating pin
  attachInterrupt(BUTTON_INPUT, ISR_button_press, FALLING); //Attach the button input to an external interrupt function
}

void loop() //Main program body
{
  //Local variable declarations
  static uint32_t previous_micros_us = 0;   //Previous elapsed time, for flicker function
  static double threshold_frequency_hz = 0; //Frequency to be printed via serial terminal, calculated from blink_half_period_us variable
  static bool led_state_flag = LOW;         //Why do we use state flags?
  uint32_t current_micros_us = micros();    //Save current elapsed time to current_micros_us

  frequency_input_raw_value = analogRead(ANALOG_FREQUENCY_INPUT_PIN);                                     //Get raw digital value of analog input from potentiometer, 0 to 1023
  blink_half_period_us = map(frequency_input_raw_value, 0, 1023, MIN_HALF_PERIOD_US, MAX_HALF_PERIOD_US); //Map raw potentiometer value to a half period duration in us

  if (frequency_requested_flag == true) //Print current frequency on serial terminal when button is pressed
  {
    threshold_frequency_hz = 1000000 / (2 * blink_half_period_us); //Frequency is the reciprocal of twice the half period. Note 1 s = 1000000 us
    Serial.print("Your flicker fusion frequency threshold is: ");
    Serial.print(threshold_frequency_hz, 2); //Print calculated threshold frequency on serial terminal for user
    Serial.println(" Hz");

    frequency_requested_flag = false; //Resets frequency print request flag, until the next button press. Prevent blocking of the program
  }

  if ((current_micros_us - previous_micros_us) > blink_half_period_us) //Code segment that handles flickering action of LED
  {
    led_state_flag = !led_state_flag;       //Toggle led state
    previous_micros_us = current_micros_us; //Updates previous time to facilitate correct flicker timing
    digitalWrite(LED_PIN, led_state_flag);  //Sets the LED pin to ON or OFF, depending on led_state_flag
  }
}

void ISR_button_press() //Interrupt service routine that handles print frequency request one the push button is pressed
{

  noInterrupts(); //Prevent other interrupts

  //Local variable declarations
  static uint32_t previous_press_time_ms = 0; //Previous time noted for debouncing
  uint32_t current_press_time_ms = millis();  //Current time noted for debouncing

  if ((current_press_time_ms - previous_press_time_ms) > DEBOUNCE_DELAY_MS) //Code segment that handles noting of frequency print request, with button debounce
  {
    frequency_requested_flag = true;                //Set flag to true to indicate a frequency print request at the main loop
    previous_press_time_ms = current_press_time_ms; //Updates previous time to facilitate correct debounce timing
  }
  interrupts(); //Re-enable other interrupts
}