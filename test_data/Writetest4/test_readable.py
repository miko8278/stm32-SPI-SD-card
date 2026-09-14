from pathlib import Path
import sys

directory = Path(sys.argv[1])

files = [file for file in directory.iterdir() if file.is_file()]

for file in files:

    # Read all lines
    with file.open("rb") as f:
        lines = f.readlines()

    # Get the filenumber
    number = file.stem[4:]

    # Expected line
    expected = f"Write 0000{number}!\n".encode()

    # Check every line
    for line_number, line in enumerate(lines, start=1):
        if line != expected:
            print(
                f"ERROR: {file.name}, line {line_number}: "
                f"expected {expected!r}, got {line!r}"
            )