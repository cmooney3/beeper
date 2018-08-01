import numpy as np
import pickle
import sys
import os

START_TIMESTAMP_KEY = "st"
START_FREQUENCY_KEY = "sf"
START_SIGNAL_STRENGTH_KEY = "sss"
END_TIMESTAMP_KEY = "et"
END_FREQUENCY_KEY = "ef"
END_SIGNAL_STRENGTH_KEY = "ess"

if len(sys.argv) != 2:
  print "USAGE: pythong process_log.py logfilename.pickle"
  sys.exit(1)
filename = sys.argv[1]

if not os.path.isfile(filename):
  print "ERROR: file '%s' not found1!" % filename
  sys.exit(1)

durations = []
interbeep_delays = []

last_beep_end_time = None
with open(filename, 'rb') as f:
  while True:
    try:
      beep = pickle.load(f)
      durations.append(1000 * (beep[END_TIMESTAMP_KEY] - beep[START_TIMESTAMP_KEY]))
      if last_beep_end_time is not None:
        interbeep_delays.append(beep[START_TIMESTAMP_KEY] - last_beep_end_time)
      last_beep_end_time = beep[END_TIMESTAMP_KEY]
    except EOFError:
      break

print "BEEP DURATIONS"
print "count:\t%d" % len(durations)
print "min:\t%fms" % np.min(durations)
print "max:\t%fms" % np.max(durations)
print "mean:\t%fms" % np.mean(durations)
print "stddev:\t%fms" % np.std(durations)
print

print "INTERBEEP PAUSES"
print "count:\t%d" % len(interbeep_delays)
print "min:\t%fs" % np.min(interbeep_delays)
print "max:\t%fs" % np.max(interbeep_delays)
print "mean:\t%fs" % np.mean(interbeep_delays)
print "stddev:\t%fs" % np.std(interbeep_delays)
print

print "FULL LOG:"
for i, (duration, delay) in enumerate(zip(durations, interbeep_delays)):
  hours = delay // (60 * 60)
  minutes = (delay - hours * (60 * 60)) // 60
  seconds = delay - hours * (60 * 60) - minutes * 60
  milliseconds = (seconds - int(seconds)) * 1000
  print i + 1, "\t", "BEEP: %02dms" % duration
  print "\t", "DELAY: %02d:%02d:%02d.%03d" % (hours, minutes, seconds, milliseconds)
print len(durations), "\t", "BEEP: %02dms" % durations[-1]
