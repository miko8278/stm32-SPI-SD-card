import csv
import matplotlib.pyplot as plt
import sys
import numpy as np

x = []
y = []
y2 = []


with open(sys.argv[1], newline="") as csvfile:
    reader = csv.DictReader(csvfile)

    for row in reader:
        x.append(int(row["cnt"]))
        y.append(int(row["p512"]))
        y2.append(int(row["p1024"]))

plt.plot(x, y, label="NoFs 512 Byte Schreibvorgang, SPI-Takt 1 MHz")
plt.plot(x, y2, label="NoFs 1024 Byte Schreibvorgang, SPI-Takt 1 MHz")
plt.xlim(float(sys.argv[2]), float(sys.argv[3]))
plt.ylim(float(sys.argv[4]), float(sys.argv[5]))
plt.xlabel("Iteration")
plt.ylabel("Δt (ms)")
plt.grid(True)

y_schrittweite = 1.0   

plt.yticks(np.arange(float(sys.argv[4]), float(sys.argv[5]) + y_schrittweite, y_schrittweite))
plt.legend()

plt.show()