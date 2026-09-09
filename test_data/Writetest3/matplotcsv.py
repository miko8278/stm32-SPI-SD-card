import csv
import matplotlib.pyplot as plt
import sys

x = []
y = []

with open(sys.argv[1], newline="") as csvfile:
    reader = csv.DictReader(csvfile)

    for row in reader:
        x.append(int(row["cnt"]))
        y.append(int(row["deltaT"]))

plt.plot(x, y)
plt.xlim(float(sys.argv[2]), float(sys.argv[3]))
plt.ylim(float(sys.argv[4]), float(sys.argv[5]))
plt.xlabel("Im Ordner befindliche Dateien")
plt.ylabel("Δt (ms)")
plt.grid(True)

plt.show()