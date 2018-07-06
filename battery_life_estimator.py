BEEP_CURRENT_A = 1.5e-3
BEEP_LENGTH_S = 20e-3
print "Beep current (A):", BEEP_CURRENT_A
print "Beep Length (S):", BEEP_LENGTH_S
print 

IDLE_CURRENT_A = 4.4e-6
SLEEP_LENGTH_S = 8
print "Idle current (A):", IDLE_CURRENT_A
print "Sleep Length (S):", SLEEP_LENGTH_S
ACTIVE_NOBEEP_CURRENT_A = 655.0e-6
WAKEUP_TURNAROUND_S = 21e-6
print "Active current w/o beeping (A):", ACTIVE_NOBEEP_CURRENT_A
print "Wakeup turnaround Time (S):", WAKEUP_TURNAROUND_S
AVG_SLEEP_CURRENT_A = (((SLEEP_LENGTH_S * IDLE_CURRENT_A) + (WAKEUP_TURNAROUND_S * ACTIVE_NOBEEP_CURRENT_A)) /
                       (SLEEP_LENGTH_S + WAKEUP_TURNAROUND_S))
print "\tAvg sleep current (A):", AVG_SLEEP_CURRENT_A
print

MIN_INTERBEEP_SLEEPS = 20
MAX_INTERBEEP_SLEEPS = 60
print "Min sleeps between beeps (#):", MIN_INTERBEEP_SLEEPS
print "Max sleeps between beeps (#):", MAX_INTERBEEP_SLEEPS
AVG_INTERBEEP_SLEEPS = ((MIN_INTERBEEP_SLEEPS + MAX_INTERBEEP_SLEEPS) / 2.0)
print "\tAvg sleeps between beeps (#):", AVG_INTERBEEP_SLEEPS
print

AVG_CURRENT = (((AVG_INTERBEEP_SLEEPS * AVG_SLEEP_CURRENT_A * (SLEEP_LENGTH_S + WAKEUP_TURNAROUND_S)) + (BEEP_CURRENT_A * BEEP_LENGTH_S)) /
               (AVG_INTERBEEP_SLEEPS * (SLEEP_LENGTH_S + WAKEUP_TURNAROUND_S) + BEEP_LENGTH_S))

print "\tTotal Avg current comsumption (A):", AVG_CURRENT
print

BATTERY_CAPACITY_AH=0.040
print "Battery Capacity (Ah):", BATTERY_CAPACITY_AH
ESTIMATED_LIFESPAN_H = BATTERY_CAPACITY_AH / AVG_CURRENT
ESTIMATED_LIFESPAN_D = ESTIMATED_LIFESPAN_H / 24
ESTIMATED_LIFESPAN_Y = ESTIMATED_LIFESPAN_D / 365
print "\tEstimated lifespan (years) --", ESTIMATED_LIFESPAN_Y
