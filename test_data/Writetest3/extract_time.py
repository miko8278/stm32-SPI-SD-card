import sys
import csv

input_filename = sys.argv[1]
output_filename = sys.argv[2]

with open(input_filename, "r", encoding="utf-8") as infile, \
     open(output_filename, "w", newline="", encoding="utf-8") as outfile:

    writer = csv.writer(outfile)

    count = 0
    number = 0
    number_old = 0
    for line_number, line in enumerate(infile):
        if line_number % 128 == 0:
            number_old = number
            number = int(line.rstrip("\r\n"))
            writer.writerow([count, (number - number_old)])
            count += 1

print(f"Written {count} entries to {output_filename}")