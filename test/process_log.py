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
print "count:\t%fs" % len(durations)
print "min:\t%fms" % np.min(durations)
print "max:\t%fms" % np.max(durations)
print "mean:\t%fms" % np.mean(durations)
print "stddev:\t%fms" % np.std(durations)
print

print "INTERBEEP PAUSES"
print "count:\t%fs" % len(interbeep_delays)
print "min:\t%fs" % np.min(interbeep_delays)
print "max:\t%fs" % np.max(interbeep_delays)
print "mean:\t%fs" % np.mean(interbeep_delays)
print "stddev:\t%fs" % np.std(interbeep_delays)
