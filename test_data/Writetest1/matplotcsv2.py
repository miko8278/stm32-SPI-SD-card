import csv
import matplotlib.pyplot as plt
import sys

x = []
y = []
y2 = []
y3 = []
y4 = []

with open(sys.argv[1], newline="") as csvfile:
    reader = csv.DictReader(csvfile)

    for row in reader:
        x.append(int(row["cnt"]))
        y.append(int(row["FAT32SFN"]))
        y2.append(int(row["FAT32SFNcorr"]))
        y3.append(int(row["littlefs"]))
        y4.append(int(row["FAT32LFN"]))

plt.plot(x, y, label="FAT32 SFN")
plt.plot(x, y2, label="Korruptes FAT32 SFN")
plt.plot(x, y3, label="littlefs")
plt.plot(x, y4, label="FAT32 LFN")
plt.xlim(float(sys.argv[2]), float(sys.argv[3]))
plt.ylim(float(sys.argv[4]), float(sys.argv[5]))
plt.xlabel("Im Ordner befindliche Dateien")
plt.ylabel("Δt (ms)")
plt.grid(True)
plt.legend()

plt.show()