import csv
import matplotlib.pyplot as plt
import sys
import numpy as np

x = []
y = []
y2 = []
y3 = []
y4 = []

with open(sys.argv[1], newline="") as csvfile:
    reader = csv.DictReader(csvfile)

    for row in reader:
        x.append(int(row["cnt"]))
        y.append(int(row["littlefs1M"]))
        y2.append(int(row["littlefs8M"]))
        y3.append(int(row["fatfs1M"]))
        y4.append(int(row["fatfs8M"]))

plt.plot(x, y, label="littlefs, SPI-Takt 1 MHz")
plt.plot(x, y2, label="littlefs, SPI-Takt 8 MHz")
plt.plot(x, y3, label="FatFs, SPI-Takt 1 MHz")
plt.plot(x, y4, label="FatFs, SPI-Takt 8 MHz")
plt.xlim(float(sys.argv[2]), float(sys.argv[3]))
plt.ylim(float(sys.argv[4]), float(sys.argv[5]))
plt.xlabel("Dateigröße in KB")
plt.ylabel("Δt (ms)")
plt.grid(True)

y_schrittweite = 10.0   

plt.yticks(np.arange(float(sys.argv[4]), float(sys.argv[5]) + y_schrittweite, y_schrittweite))
plt.legend()

plt.show()