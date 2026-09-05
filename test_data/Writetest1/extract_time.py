from pathlib import Path
import sys
import csv

directory = Path(sys.argv[2])

files = sorted(
    file for file in directory.iterdir()
    if file.is_file()
)

number = 0
cnt = 0
output_file = Path(sys.argv[1])
with open(output_file, "w", newline="") as csvfile:
    writer = csv.writer(csvfile)

    # Header
    writer.writerow(["cnt", "deltaT"])

    for file in files:
        print(f"{file.stem}")
        number_old = number
        # Get the filenumber
        number = int(file.stem.split("_")[3])
        deltaT = number - number_old
        writer.writerow([cnt, deltaT])
        cnt += 1
