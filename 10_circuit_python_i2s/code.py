import time
import array
import math
import audiocore
import board
import audiobusio

# Configuration
SAMPLE_RATE = 44100  # 44.1 kHz sample rate
FREQUENCY = 100       # 50 Hz sine wave
BUFFER_SIZE = SAMPLE_RATE // FREQUENCY  # Samples for one full cycle

# Initialize I2S with right-justified (LSBJ) format
audio = audiobusio.I2SOut(board.GP0, board.GP1, board.GP2, left_justified=False)

# Create sine wave buffer
tone_volume = 0.1  # Volume scaling factor
sine_wave = array.array("h", [0] * BUFFER_SIZE)  # 16-bit signed integer array
for i in range(BUFFER_SIZE):
    sine_wave[i] = int((math.sin(math.pi * 2 * i / BUFFER_SIZE)) * 30000)

# Create RawSample with explicit 44.1 kHz sample rate
sine_wave_sample = audiocore.RawSample(sine_wave, sample_rate=SAMPLE_RATE)

# Play continuously
audio.play(sine_wave_sample, loop=True)

while True:
    time.sleep(1)