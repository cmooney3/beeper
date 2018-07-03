#include <avr/io.h>
#include <util/delay.h>

#define BEEP_DURATION_MS 20

void initPWM() {
  // Configure PB4 as an output (it'll be the PWM OC1B)
  DDRB = (0x01 << PORTB4);

  // Enable PWM output on timer1 b and configure how it'll work
  GTCCR = (0x01 << PWM1B) | (0x02 << COM1B0);

  // Set up the clock prescalar
  // The main clock is running at 1Mhz you can compute the new period (in ms) like so:
  //  1000 / ((1000000 / PRESCALAR) / OCR1C)
  OCR1C = 255;
  //TCCR1 = (0x07 << CS10); // Prescalar = 1:64, so the period is 16.4ms
  TCCR1 = (0x01 << CS10); // Prescalar = 1:1, so the freq is about 4000Hz
}

void disablePWM() {
  GTCCR = (0x00 << PWM1B) | (0x00 << COM1B0);
}

void SetPWMoutput(uint8_t duty) {
  OCR1B = duty;
}

void beep() {
  initPWM();          // Set up the PWM HW for the right period/etc
  SetPWMoutput(128);  // Make the duty cycle 50%
  _delay_ms(BEEP_DURATION_MS);
  disablePWM();        // Turn off the PWM HW
}

void adc_disable() {
  // This function disables the ADC.  It uses a lot of power, so we turn it off
  // since it's unused in this project
  ADCSRA &= ~(1<<ADEN);
}


int main (void)
{
  adc_disable();

  // Beep once
  beep();

  // Now do nothing for eternity
  while (1) {
    _delay_ms(60000);
  }

  return 1;
}
