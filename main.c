#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdlib.h>

// PIN CONFIGURATION:
// BUZZER - B4  (OC1B)

#define RANDOM_SEED 0x8F
#define MIN_SLEEPS_BETWEEN_BEEPS 20
#define MAX_SLEEPS_BETWEEN_BEEPS 60
#define NUM_INITIAL_BEEPS 4
#define BEEP_DURATION_MS 20

uint8_t volatile sleeps_until_next_beep = 0;

static void initPWM() {
  // Configure PB4 as an output (it'll be the PWM OC1B)
  DDRB = (0x01 << PORTB4);

  // Enable PWM output on timer1 b and configure how it'll work
  GTCCR = (0x01 << PWM1B) | (0x02 << COM1B0);

  // Set up the clock prescalar and reset point (configure period)
  // The main clock is running at 1Mhz you can compute the new period (in ms) like so:
  //  1000 / ((1000000 / PRESCALAR) / OCR1C)
  OCR1C = 255;
  //TCCR1 = (0x07 << CS10); // Prescalar = 1:64, so the period is 16.4ms
  TCCR1 = (0x01 << CS10); // Prescalar = 1:1, so the freq is about 4000Hz
}

static void inline disablePWM() {
  GTCCR = (0x00 << PWM1B) | (0x00 << COM1B0);
}

static void inline SetPWMOutput(uint8_t duty) {
  OCR1B = duty;
}

static void beep() {
  initPWM();          // Set up the PWM HW for the right period/etc
  SetPWMOutput(128);  // Make the duty cycle 50%
  _delay_ms(BEEP_DURATION_MS);
  disablePWM();        // Turn off the PWM HW
}

static void adcDisable() {
  // This function disables the ADC.  It uses a lot of power, so we turn it off
  // since it's unused in this project
  ADCSRA &= ~(1<<ADEN);
}

//Sets the watchdog timer to wake us up, but not reset
//0=16ms, 1=32ms, 2=64ms, 3=125ms, 4=250ms, 5=500ms
//6=1sec, 7=2sec, 8=4sec, 9=8sec
static void enableWDTInterrupt(uint8_t timerPrescaler)
{
  uint8_t WDTCSR_ = (timerPrescaler & 0x07);
  WDTCSR_ |= (timerPrescaler > 7) ? _BV(WDP3) : 0x00; // Set WDP3 if prescalar > 7 (ie. 4.0s, 8.0s)
  WDTCSR_ |= _BV(WDIE); // Enable watchdog interrupt

  //This order of commands is important and cannot be combined (beyond what they are below)
  MCUSR &= ~_BV(WDRF); // Clear the watch dog reset

  // timed sequence
  ATOMIC_BLOCK(ATOMIC_FORCEON)
  {
    WDTCR |= _BV(WDCE) | _BV(WDE); // Set WD_change enable, set WD enable
    WDTCR = WDTCSR_; // Set new watchdog timeout value & enable
  }
}

// Send the whole system into deep sleep until the next WDT interrupt
// For maximum power savings the ADC definitely should be disabled as well.  However for this
// project we never need the ADC so it's just disabled once at the beginning of execution and never
// touched again.  If you're actually using the ADC you'll have to disable and re-endable it each
// time you sleep -- it makes a huuuge difference in power consumption.
static void sleepUntilWDTWake() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Configure what kind of sleep to use (very low power mode)
  sleep_enable();                      // Make it possible to go to sleep
  sleep_mode();                        // Actually send the system to sleep
  sleep_disable();                     // System continues execution (wakes) here when watchdog times out
}

// Watchdog Interrupt Service / is executed when watchdog timed out.
// It doesn't have to really do anything because just by firing this interrupt
// execution is restarted and the system is woken up.
ISR(WDT_vect) {}

int main (void) {
  srand(RANDOM_SEED);
  adcDisable();
  enableWDTInterrupt(9); // Set up the WDT to run as slowly as possible

  for (uint8_t i = 0; i < NUM_INITIAL_BEEPS; i++) {
    beep();
    _delay_ms(BEEP_DURATION_MS);  // Put an equal-length pause between beeps
  }

  while (1) {
    if (!sleeps_until_next_beep--) {
      // Determine how long the next delay will be
      sleeps_until_next_beep = rand() % (MAX_SLEEPS_BETWEEN_BEEPS - MIN_SLEEPS_BETWEEN_BEEPS) +
                               MIN_SLEEPS_BETWEEN_BEEPS;

      // Do the actual beeping!
      beep();
    }

    // Go into the lowest power mode for a while (saving battery while we wait)
    sleepUntilWDTWake();
  }

  return 1;
}

