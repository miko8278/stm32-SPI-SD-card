#This is only for fattest1 to fattest3 data
#I've changed the filename a little in fattest4 to be in line with littlefstest with timestamps
from pathlib import Path
import sys

directory = Path(sys.argv[1])

files = [file for file in directory.iterdir() if file.is_file()]

for file in files:
    # Check filesize
    size = file.stat().st_size
    if size != 1024:
        print(f"ERROR: {file.name}: wrong filesize ({size} bytes)")

    # Read all lines
    with file.open("rb") as f:
        lines = f.readlines()

    # Check number of lines
    if len(lines) != 64:
        print(f"ERROR: {file.name}: wrong number of lines ({len(lines)})")
        continue

    # Get the filenumber
    number = file.stem.split("_")[1]

    # Expected line
    expected = f"Write 00{number}!\n".encode()

    # Check every line
    for line_number, line in enumerate(lines, start=1):
        if line != expected:
            print(
                f"ERROR: {file.name}, line {line_number}: "
                f"expected {expected!r}, got {line!r}"
            )