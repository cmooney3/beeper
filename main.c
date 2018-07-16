#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

// PIN CONFIGURATION:
// BUZZER - B4  (OC1B)
//    NOTE: Buzzer needs to be connected to Vcc & B4 (NOT ground) since we are
//          using the internal pull-ups on GPIOS to reduce power.  If it's on
//          GND, we'd need a pull-down instead which is an extra component.
//          Since the buzzer runs on AC, I think it should be just fine.
// DELAY JUMPER - B0 --- SWITCH/JUMPER -- 10kOhm --- B3
//    NOTE: This jumper/switch needs to lie on a trace connecting these two
//          GPIOS with a current limited resistor in series.  This system, as
//          opposed to a traditional 1-GPIO switch circuit saves lots and lots
//          of power.

#define RANDOM_SEED 0x8F
#define MIN_INTERBEEP_DELAY_S (60 * 5)
#define MAX_INTERBEEP_DELAY_S (60 * 20)
#define NUM_INITIAL_BEEPS_NODELAY 4  // +1 additional beep for the first regular beep
#define NUM_INITIAL_BEEPS_DELAYED 50
#define INITIAL_TIME_DELAY_S (30 * 24 * 60 * 60) // Approximately 1 month
#define BEEP_DURATION_MS 10

#define SLEEP_DURATION_S 8 // This is set by the WDT prescalar in enableWDTInterrupt()
#define MIN_SLEEPS_BETWEEN_BEEPS (MIN_INTERBEEP_DELAY_S / SLEEP_DURATION_S)
#define MAX_SLEEPS_BETWEEN_BEEPS (MAX_INTERBEEP_DELAY_S / SLEEP_DURATION_S)
#define NUM_INITIAL_SLEEPS_FOR_DELAY (INITIAL_TIME_DELAY_S / SLEEP_DURATION_S)


uint32_t sleeps_until_next_beep = 0;
uint8_t is_delay_jumper_connected;

static void setAllGPIOAsInputs() {
  // Configure all 6 GPIO pins as input (as a default and/or to save power)
  DDRB &= ~(_BV(PB0) | _BV(PB1) | _BV(PB2) | _BV(PB3) | _BV(PB4) | _BV(PB5));
  // Turn on the internal pullups for each of them as well.
  PORTB |= _BV(PB0) | _BV(PB1) | _BV(PB2) | _BV(PB3) | _BV(PB4) | _BV(PB5);
}

static void setupOutputGPIOs() {
  // Configure PB4 as an output (it'll be the PWM OC1B)
  DDRB |= _BV(PB4);
}

static void initPWM() {
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
  setupOutputGPIOs(); // Turn the buzzer's GPIO into an output
  initPWM();          // Set up the PWM HW for the right period/etc
  SetPWMOutput(128);  // Make the duty cycle 50%
  _delay_ms(BEEP_DURATION_MS);
  disablePWM();        // Turn off the PWM HW
  setAllGPIOAsInputs(); // Turn all GPIOs back into inputs to save power
}

static void adcDisable() {
  // This function disables the ADC.  It uses a lot of power, so we turn it off
  // since it's unused in this project
  ADCSRA &= ~(1<<ADEN);
}

//Sets the watchdog timer to wake us up, but not reset
//0=16ms, 1=32ms, 2=64ms, 3=125ms, 4=250ms, 5=500ms
//6=1sec, 7=2sec, 8=4sec, 9=8sec
static void enableWDTInterrupt(uint8_t timerPrescaler) {
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
static void sleepUntilWDTWake() {
  // For maximum power savings the ADC definitely should be disabled as well.  However for this
  // project we never need the ADC so it's just disabled once at the beginning of execution and never
  // touched again.  If you're actually using the ADC you'll have to disable and re-endable it each
  // time you sleep -- it makes a huuuge difference in power consumption.
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Configure what kind of sleep to use (very low power mode)
  sleep_enable();                      // Make it possible to go to sleep
  wdt_reset();                         // Reset the WDT to 0 to get the full delay before the interrupt fires
  sleep_mode();                        // Actually send the system to sleep
  sleep_disable();                     // System continues execution (wakes) here when watchdog times out
}

// Watchdog Interrupt Service / is executed when watchdog timed out.
// It doesn't have to really do anything because just by firing this interrupt
// execution is restarted and the system is woken up.
ISR(WDT_vect) {}

void checkTimeDelayJumper() {
  // This function runs immediately on boot, so it assumes everything is unconfigured.  It works
  // because PB0 and PB3 are connected to one another with a 10k resistor between them.  Here we
  // configure one as an input and one as an output and quickly check if the wire is still connecting
  // them by changing the value on the output and checking it at the input.  If the input value
  // matches the output one, then we know the jumper has not been cut.

  // Configure PB0 as an output
  DDRB |= _BV(PB0);
  // Configure PB3 as an input w/ pull-up (to detect the state)
  DDRB &= ~_BV(PB3);
  PORTB |= _BV(PB3);

  // Turn the output pin to 0, if they're connected, PB3 should read a 0
  PORTB |= _BV(PB0);
  // Then read in the value on PB3, and see the state of the jumper.  If it reads 0 (like PB0 is set
  // to) then we know the jumper is still connected.  Otherwise someone cut the jumper
  is_delay_jumper_connected = !(PINB & _BV(PB3));
  // Turn PB0 back into an input to save power.
  DDRB &= ~_BV(PB0);
}

int main (void) {
  // Very first thing to do is check if the time-delay jumper is set.  This sets the global
  // flag "is_delay_jumper_connected" for use in the rest of the program, and is only run once
  // on boot.  The system must be reset to get a new reading.
  checkTimeDelayJumper();
  if (is_delay_jumper_connected) {
    // If the delay jumper is connected, we start off requiring a huge number of sleeps before
    // the first beep is ever played -- thus adding a time-delay feature.
    sleeps_until_next_beep = NUM_INITIAL_SLEEPS_FOR_DELAY;
  }

  // TODO: Get a better random seed?  It's silly, but it's a reasonably easy thing to improve
  srand(RANDOM_SEED);  // To get a seemingly random pause length we need to seed the RNG
  adcDisable();  // We never use the ADC, so it should be immediately disabled for power savings
  enableWDTInterrupt(9); // Set up the WDT to run as slowly as possible (approx 8s)

  // Play a short burst of beeps on startup, so the user knows everything is working
  uint8_t num_initial_beeps = is_delay_jumper_connected ? NUM_INITIAL_BEEPS_DELAYED : NUM_INITIAL_BEEPS_NODELAY;
  for (uint8_t i = 0; i < num_initial_beeps; i++) {
    beep();
    _delay_ms(BEEP_DURATION_MS);  // Put an equal-length pause between beeps
  }

  // Main loop of the program.  It waits for a randomly selected # of low power "sleeps" in
  // between playing short beeps.
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
