import struct
import sys

START_SEED = 0x01234567

#python has no 32 bit limitation like cpp uint32_t
#so we have to work with those bitwise &
def xorshift32(x):
    x &= 0xFFFFFFFF
    x ^= (x << 13) & 0xFFFFFFFF
    x ^= x >> 17
    x ^= (x << 5) & 0xFFFFFFFF
    return x & 0xFFFFFFFF


filename = sys.argv[1]

with open(filename, "rb") as f:
    seednum = 0
    offset = 0

    while True:
        data = f.read(4)

        if not data:
            print("Verification successful!")
            break

        if len(data) != 4:
            print(f"ERROR: incomplete 4-byte value at offset {offset}")
            break

        expected = xorshift32(START_SEED + seednum)
        actual = struct.unpack("<I", data)[0]

        if actual != expected:
            print(
                f"ERROR at offset {offset}: "
                f"expected 0x{expected:08X}, "
                f"got 0x{actual:08X}"
            )
            break

        seednum += 1
        offset += 4