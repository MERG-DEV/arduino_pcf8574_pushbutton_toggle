#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include <Wire.h>

enum { PINS_PER_PCF8574           = 8,
       EXPANDER_A_PINS_BASE       = 100,
       EXPANDER_B_PINS_BASE       = EXPANDER_A_PINS_BASE + PINS_PER_PCF8574,
       NUMBER_OF_BUTTONS_AND_LEDS = 8};

MultiIoAbstractionRef multi_io = multiIoExpander(EXPANDER_A_PINS_BASE);


void switch_pressed(uint8_t switch_pin, bool switch_held)
{
  if (!switch_held)
  {
    int const button = 1 + switch_pin - EXPANDER_A_PINS_BASE;

    if ((button < 1) || (button > 8))
    {
      Serial.print(F("Unrecognised activation on pin "));
      Serial.println(switch_pin);
    }
    else
    {
      Serial.print(F("Button "));
      Serial.print(button);
      Serial.println(F(" pressed"));

      uint8_t const led_pin = button - 1 + EXPANDER_B_PINS_BASE;
      Serial.print(F("Toggling LED "));
      Serial.print(button);
      Serial.print(F(" on pin "));
      Serial.println(led_pin);
      multi_io->writeValue(led_pin, !multi_io->readValue(led_pin));
      multi_io->runLoop();
    }
  }
}


void switch_released(uint8_t switch_pin, bool)
{
    int const button = 1 + switch_pin - EXPANDER_A_PINS_BASE;

    if ((button < 1) || (button > 8))
    {
      Serial.print(F("Unrecognised deactivation on pin "));
      Serial.println(switch_pin);
    }
    else
    {
      Serial.print(F("Button "));
      Serial.print(button);
      Serial.println(F(" released"));
    }
}


void setup()
{
  Wire.begin(); // Start I2C
  Serial.begin(9600);

  Serial.println(F("Pins 0 to 99, Arduino"));
  Serial.println(F("Pins 100 to 107, first PCF8574 at I2C address 0x20"));
  multi_io->addIoExpander(ioFrom8574(0x20), PINS_PER_PCF8574);
  Serial.println(F("Pins 108 to 115, second PCF8574 at I2C address 0x21"));
  multi_io->addIoExpander(ioFrom8574(0x21), PINS_PER_PCF8574);

  Serial.println(F("IO pins defined, adding switches"));
  switches.initialise(multi_io, true);

  Serial.println(F("Button inputs on first expander, LED outputs on second"));
  for (int i = 0; i < NUMBER_OF_BUTTONS_AND_LEDS; i++)
  {
    uint8_t const switch_pin = EXPANDER_A_PINS_BASE + i;
    multi_io->pinDirection(switch_pin, INPUT_PULLUP);
    multi_io->writeValue(switch_pin, HIGH);
    switches.addSwitch(switch_pin, switch_pressed);
    switches.onRelease(switch_pin, switch_released);

    uint8_t const led_pin = EXPANDER_B_PINS_BASE + i;
    multi_io->pinDirection(led_pin, OUTPUT);
    multi_io->writeValue(led_pin, LOW);
  }

  multi_io->runLoop();

  Serial.println(F("Setup complete"));
}


void loop()
{
  taskManager.runLoop();
}
