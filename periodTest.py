import numpy as np
import matplotlib.pyplot as plt

# Parameters for the sine wave
frequency = 5        # Frequency in Hertz (Hz)
amplitude = 1         # Amplitude of the wave
sampling_rate = 1000  # Number of samples per second (Hz)
duration = 2          # Duration in seconds
noise_amplitude = 0.3
filter_size = 29

#Gets the period of the y values using the x values as time
def get_period(x, y):

    foundFirstPeak = False
    lastTime = 0
    periodList = []

    peak_size = 11

    for i in range(peak_size,len(x)-peak_size):
       peak_range = get_peak_range(i, peak_size, y)

       if detect_peak(peak_range):
            if not foundFirstPeak:
               lastTime = x[i]
               foundFirstPeak = True
            else:
                periodList.append(x[i] - lastTime)
                lastTime = x[i]
    
    return periodList             

#Get the range of values around the midpoint from -size to size      
def get_peak_range(location,size,values):
    peak_range = []
    for x in values[location - size:location+size+1]:
        peak_range.append(x)
    
    return peak_range

#Detect if the mp of values is a peak
def detect_peak(values):
    mp = values[len(values) // 2]

    for x in values:
        if x > mp:
            return False
    
    return True

#Noise filter using averaging
def simple_filter(values, size):
    filtered = values.copy()
    for i in range(size,len(values)-size):
        peak_range = get_peak_range(i,size,values)
        filtered[i] = sum(peak_range)/len(peak_range)
    
    return filtered[size:-size]
    
t = np.linspace(0, duration, int(sampling_rate * duration), endpoint=False)

# Generate sine wave values
y = amplitude * np.sin(2 * np.pi * frequency * t)

#Add noise
noise = noise_amplitude * np.random.normal(size=t.shape)
noisy_y = y + noise

filter_y = simple_filter(noisy_y, filter_size)

# Plot the sine wave
plt.figure(figsize=(10, 4))
plt.subplot(2,1,1)
plt.plot(t, noisy_y,label="Noisy Stuff")
plt.title(f"Sine Wave: {frequency} Hz")
plt.xlabel("Time [s]")
plt.ylabel("Amplitude")
plt.grid(True)

# Plot the sine wave
plt.subplot(2,1,2)
plt.plot(t[filter_size:-filter_size], filter_y,label="Filtered Signal")
plt.xlabel("Time [s]")
plt.ylabel("Amplitude")
plt.grid(True)
plt.show()

noisy_period = get_period(t, noisy_y)
print("Noisy period: " + str(sum(noisy_period)/len(noisy_period)))

filtered_period = get_period(t[filter_size:-filter_size], filter_y)
print("Filtered period: " +  str(sum(filtered_period)/len(filtered_period)))


