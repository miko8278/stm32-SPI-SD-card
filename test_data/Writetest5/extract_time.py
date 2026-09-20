import sys
import csv

input_filename = sys.argv[1]
output_filename = sys.argv[2]
# its 32 for the 512 byte version or 64 for the 1024 byte version
lineskip = int(sys.argv[3])  

with open(input_filename, "r", encoding="utf-8") as infile, \
     open(output_filename, "w", newline="", encoding="utf-8") as outfile:

    writer = csv.writer(outfile)

    count = 0
    number_old = 0
    number = 0

    for line_number, line in enumerate(infile):
        # Only process every `lineskip` lines
        if line_number % lineskip == 0:
            value = line.strip()

            # Extract number after string
            number = int(value.split(":", 1)[1])

            if count == 0:
                number_old = number

            writer.writerow([count, number - number_old])

            number_old = number
            count += 1

print(f"Written {count} entries to {output_filename}")