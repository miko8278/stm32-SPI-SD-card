import sys
from pathlib import Path


def check_files(folder):
    folder_path = Path(folder)

    if not folder_path.is_dir():
        print(f"Error: '{folder}' is not a valid folder.")
        sys.exit(1)

    files_with_errors = 0
    total_errors = 0
    files_checked = 0

    for file_path in folder_path.iterdir():
        if not file_path.is_file():
            continue

        files_checked += 1

        try:
            with file_path.open("rb") as f:
                lines = f.readlines()

            if not lines:
                continue

            # First line is the reference line.
            first_line = lines[0].rstrip(b"\r\n")

            errors = 0

            for line_number, line in enumerate(lines[1:], start=2):
                line = line.rstrip(b"\r\n")

                if line != first_line:
                    errors += 1

            if errors > 0:
                files_with_errors += 1
                total_errors += errors

                print(f"{file_path.name}: {errors} error(s)")

        except OSError as e:
            print(f"{file_path.name}: could not be read ({e})")

    print()
    print(f"Files checked:     {files_checked}")
    print(f"Files with errors: {files_with_errors}")
    print(f"Total errors:      {total_errors}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: python3 {Path(sys.argv[0]).name} <folder>")
        sys.exit(1)

    check_files(sys.argv[1])