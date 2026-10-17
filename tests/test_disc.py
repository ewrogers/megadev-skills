import sys
from pathlib import Path
import unittest


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "skills/megadev-project/scripts"))
from disc import digest, inspect_iso


def record(name, sector, size, directory=False):
    result = bytearray(33 + len(name) + (len(name) % 2 == 0))
    result[0] = len(result)
    result[2:6] = sector.to_bytes(4, "little")
    result[6:10] = sector.to_bytes(4, "big")
    result[10:14] = size.to_bytes(4, "little")
    result[14:18] = size.to_bytes(4, "big")
    result[25] = 2 if directory else 0
    result[28:32] = b"\1\0\0\1"
    result[32] = len(name)
    result[33:33 + len(name)] = name
    return result


def image():
    data = bytearray(22 * 2048)
    data[:16] = b"SEGADISCSYSTEM  "
    data[16 * 2048:16 * 2048 + 7] = b"\x01CD001\x01"
    root = record(b"\0", 18, 2048, True)
    data[16 * 2048 + 156:16 * 2048 + 156 + len(root)] = root
    entries = root + record(b"\1", 18, 2048, True) + record(b"A.TXT;1", 20, 3) + record(b"SUB", 19, 2048, True)
    data[18 * 2048:18 * 2048 + len(entries)] = entries
    child = record(b"\0", 19, 2048, True) + record(b"\1", 18, 2048, True) + record(b"B.TXT;1", 21, 3)
    data[19 * 2048:19 * 2048 + len(child)] = child
    data[20 * 2048:20 * 2048 + 3] = b"one"
    data[21 * 2048:21 * 2048 + 3] = b"two"
    return data


class DiscAccounting(unittest.TestCase):
    def test_boot_only_disc_can_have_an_empty_filesystem(self):
        data = image()
        entries = record(b"\0", 18, 2048, True) + record(b"\1", 18, 2048, True)
        data[18 * 2048:19 * 2048] = entries.ljust(2048, b"\0")
        self.assertEqual(inspect_iso(data)["files"], {})

    def test_reads_nested_files_without_iso_metadata(self):
        data = image()
        result = inspect_iso(data)
        self.assertEqual(result["files"], {"A.TXT": digest(b"one"), "SUB/B.TXT": digest(b"two")})
        self.assertEqual(result["boot"], digest(data[:32768]))
        data[16 * 2048 + 813] = 1  # PVD creation date is metadata.
        self.assertEqual(inspect_iso(data), result)

    def test_corrupted_signature_extent_and_directory_are_rejected(self):
        for kind in ("signature", "pvd", "extent", "record", "cycle"):
            data = image()
            if kind == "signature":
                data[0] = 0
            elif kind == "pvd":
                data[16 * 2048 + 1] = 0
            elif kind == "extent":
                data[16 * 2048 + 158:16 * 2048 + 162] = (1000).to_bytes(4, "little")
            elif kind == "record":
                data[18 * 2048] = 1
            else:
                loop = record(b"LOOP", 18, 2048, True)
                data[18 * 2048:18 * 2048 + len(loop)] = loop
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                inspect_iso(data)


if __name__ == "__main__":
    unittest.main()
