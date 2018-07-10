from scipy.fftpack import fft
from scipy.io import wavfile as wav
from scipy.signal import spectrogram
import numpy as np
import os
import pickle
import sounddevice as sd
import sys

BEEPER_FREQ = 4000  # 4Khz
FFT_WINDOW_SIZE_MS = 200
MAX_FREQ_DEVIATION = 150   # How many Hz off from the buzzer frequency is allowable
MIN_SIGNAL_STRENGTH = 1e-4 # Determined experimentally by recording some beeps and seeing
                           # what the "signal" levels were in the spectrogram
DEVICE = "Yeti"
SAMPLERATE = 44000
CHANNELS = 2

START_TIMESTAMP_KEY = "st"
START_FREQUENCY_KEY = "sf"
START_SIGNAL_STRENGTH_KEY = "sss"
END_TIMESTAMP_KEY = "et"
END_FREQUENCY_KEY = "ef"
END_SIGNAL_STRENGTH_KEY = "ess"

current_beep = None

if len(sys.argv) != 2:
  print "Usage: python beeper_test.py log_filename.pickle"
  sys.exit(1)

LOG_FILENAME = sys.argv[1]
if os.path.isfile(LOG_FILENAME):
  print "The file '%s' already exists, overwrite it? (y/N)" % LOG_FILENAME
  response = raw_input()
  if response.lower() in ['y', 'yes']:
    print "Okay, deleting old log now...",
    os.remove(LOG_FILENAME)
    print "DONE"
  else:
    print "Better safe than sorry.  Try a different filename instead?"
    sys.exit(1)

def callback(indata, frames, time, error):
  global current_beep

  if error:
    print "ERROR: %s" % error
  else:
    # Compute a spectogram over the recording
    freqs, times, strengths = spectrogram(indata[:, 0], fs=SAMPLERATE)

    # Go through each time window checking for the buzzer beeping
    for timei in range(len(times)):
      # Find the strongest frequency in this window of time
      main_freqi = 0
      for freqi in range(len(freqs)):
        if strengths[freqi][timei] > strengths[main_freqi][timei]:
          main_freqi = freqi

      # Check to see if that max frequency looks like our buzzer (close in frequency and
      # has a signal strength above a certain threshold)
      distance_from_target_freq = abs(freqs[main_freqi] - BEEPER_FREQ)
      if (distance_from_target_freq <= MAX_FREQ_DEVIATION and
          strengths[main_freqi][timei] > MIN_SIGNAL_STRENGTH):
        if current_beep is None:
          current_beep = {}
          current_beep[START_TIMESTAMP_KEY] = time.inputBufferAdcTime + times[timei]
          current_beep[START_FREQUENCY_KEY] = freqs[main_freqi]
          current_beep[START_SIGNAL_STRENGTH_KEY] = strengths[main_freqi][timei]
          print "START:\t%f\t%f\t%f" % (current_beep[START_TIMESTAMP_KEY],
                                       current_beep[START_FREQUENCY_KEY],
                                       current_beep[START_SIGNAL_STRENGTH_KEY])
      else:
        if current_beep is not None:
          current_beep[END_TIMESTAMP_KEY] = time.inputBufferAdcTime + times[timei]
          current_beep[END_FREQUENCY_KEY] = freqs[main_freqi]
          current_beep[END_SIGNAL_STRENGTH_KEY] = strengths[main_freqi][timei]

          print "END:\t%f\t%f\t%f" % (current_beep[END_TIMESTAMP_KEY],
                                      current_beep[END_FREQUENCY_KEY],
                                      current_beep[END_SIGNAL_STRENGTH_KEY])

          duration_ms = ((current_beep[END_TIMESTAMP_KEY] - current_beep[START_TIMESTAMP_KEY]) * 1000)
          print "DURATION: %fms" % duration_ms
          print

          with open(LOG_FILENAME, 'ab') as f:
            pickle.dump(current_beep, f, pickle.HIGHEST_PROTOCOL)

          current_beep = None

def start():
  with sd.InputStream(samplerate=SAMPLERATE, device=DEVICE,
                      blocksize=int(SAMPLERATE * FFT_WINDOW_SIZE_MS / 1000),
                      channels=CHANNELS, callback=callback):
    print "Starting to record"
    while True:
      sd.sleep(1000)
  print "Done recording"

if __name__ == "__main__":
  start()
