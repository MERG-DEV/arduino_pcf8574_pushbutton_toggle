#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include <Wire.h>

enum : uint8_t { PINS_PER_PCF8574  = 8,
                 NUMBER_OF_BUTTONS = 8,
                 NUMBER_OF_LEDS    = NUMBER_OF_BUTTONS,
                 BUTTON_PINS_BASE  = 100,
                 LED_PINS_BASE     = BUTTON_PINS_BASE + NUMBER_OF_BUTTONS,
                 BUTTON_ON         = LOW,
                 BUTTON_OFF        = HIGH,
                 LED_ON            = LOW,
                 LED_OFF           = HIGH};

enum class Button_States { OFF, DEBOUNCING_ON, ON, DEBOUNCING_OFF};

uint8_t led_state[NUMBER_OF_LEDS] = {LED_OFF};
Button_States button_state[NUMBER_OF_BUTTONS] = {Button_States::OFF};

MultiIoAbstraction multi_io{BUTTON_PINS_BASE};


void set_led_state(int const led_number, uint8_t const new_state)
{
  if ((led_number < 1) || (led_number > NUMBER_OF_LEDS))
  {
    Serial.print(F("  Ignoring set request for invalid LED "));
    Serial.println(led_number);
  }
  else
  {
    uint8_t const led_index = led_number - 1;
    uint8_t const led_pin   = LED_PINS_BASE + led_index;

    Serial.print(F("  Turning LED "));
    Serial.print(led_number);
    Serial.print(F(" on pin "));
    Serial.print(led_pin);

    if (LED_ON == new_state)
    {
      Serial.println(F(" on"));
      multi_io.writeValue(led_pin, LED_ON);
      led_state[led_index] = LED_ON;
    }
    else
    {
      Serial.println(F(" off"));
      multi_io.writeValue(led_pin, LED_OFF);
      led_state[led_index] = LED_OFF;
    }
  }
}


void toggle_led_state(int const led_number)
{
  if ((led_number < 1) || (led_number > NUMBER_OF_LEDS))
  {
    Serial.print(F("  Ignoring toggle request for invalid LED "));
    Serial.println(led_number);
  }
  else
  {
    Serial.print(F("  Toggling LED "));
    Serial.println(led_number);
    set_led_state(led_number,
                  (LED_ON == led_state[led_number - 1]) ? LED_OFF : LED_ON);
  }
}


void switch_pressed(uint8_t const switch_pin)
{
  int const button_number = 1 + switch_pin - BUTTON_PINS_BASE;

  if ((button_number < 1) || (button_number > NUMBER_OF_BUTTONS))
  {
    Serial.print(F(" Unrecognised activation on pin "));
    Serial.println(switch_pin);
  }
  else
  {
    Serial.print(F(" Button "));
    Serial.print(button_number);
    Serial.println(F(" pressed"));

    toggle_led_state(button_number);
  }
}


void switch_released(uint8_t const switch_pin)
{
  int const button_number = 1 + switch_pin - BUTTON_PINS_BASE;

  if ((button_number < 1) || (button_number > 8))
  {
    Serial.print(F(" Unrecognised deactivation on pin "));
    Serial.println(switch_pin);
  }
  else
  {
    Serial.print(F(" Button "));
    Serial.print(button_number);
    Serial.println(F(" released"));
  }
}


void report_button_input_state(int     const button_number,
                               uint8_t const button_pin,
                               uint8_t const input_state)
{
  Serial.print(F("Button "));
  Serial.print(button_number);
  Serial.print(F(" input on pin "));
  Serial.print(button_pin);
  if (BUTTON_ON == input_state)
  {
    Serial.println(F(" is active"));
  }
  else
  {
    Serial.println(F(" is inactive"));
  }
}


void scan_button_inputs()
{
  multi_io.runLoop();

  for (int i = 0; i < NUMBER_OF_BUTTONS; i++)
  {
    uint8_t const button_pin = BUTTON_PINS_BASE + i;
    // report_button_input_state(i + 1,
    //                           button_pin,
    //                           multi_io.readValue(button_pin));
    if (BUTTON_ON == multi_io.readValue(button_pin))
    {
      switch (button_state[i])
      {
      case Button_States::OFF :
        button_state[i] = Button_States::DEBOUNCING_ON;
        break;

      case Button_States::DEBOUNCING_ON :
        switch_pressed(button_pin);

      case Button_States::DEBOUNCING_OFF:
        button_state[i] = Button_States::ON;
        break;

      default:
        break;
      }
    }
    else
    {
      switch (button_state[i])
      {
      case Button_States::ON :
        button_state[i] = Button_States::DEBOUNCING_OFF;
        break;

      case Button_States::DEBOUNCING_OFF :
        switch_released(button_pin);

      case Button_States::DEBOUNCING_ON:
        button_state[i] = Button_States::OFF;
        break;

      default:
        break;
      }
    }
  }
}


void setup()
{
  Wire.begin(); // Start I2C
  Serial.begin(9600);

  Serial.println();
  Serial.println(F("Pins 0 to 99, Arduino"));
  Serial.println(F("Pins 100 to 107, first PCF8574 at I2C address 0x20"));
  multi_io.addIoExpander(ioFrom8574(0x20), PINS_PER_PCF8574);
  Serial.println(F("Pins 108 to 115, second PCF8574 at I2C address 0x21"));
  multi_io.addIoExpander(ioFrom8574(0x21), PINS_PER_PCF8574);

  Serial.println(F("Configuring button inputs"));
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++)
  {
    uint8_t const button_pin = BUTTON_PINS_BASE + i;
    Serial.print(F("Button "));
    Serial.print(i + 1);
    Serial.print(F(" input on pin "));
    Serial.println(button_pin);
    multi_io.pinDirection(button_pin, INPUT_PULLUP);
    button_state[i] == Button_States::OFF;
  }

  Serial.println(F("Configuring LED outputs"));
  for (int i = 0; i < NUMBER_OF_LEDS; i++)
  {
    uint8_t const led_pin = LED_PINS_BASE + i;
    Serial.print(F("LED "));
    Serial.print(i + 1);
    Serial.print(F(" output on pin "));
    Serial.println(led_pin);
    multi_io.pinDirection(led_pin, OUTPUT);
    set_led_state(i + 1, LED_OFF);
  }

  taskManager.scheduleFixedRate(20, scan_button_inputs);

  Serial.println(F("Setup complete"));
}


void loop()
{
  taskManager.runLoop();
}
