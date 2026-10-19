"""Read the small ISO9660 data discs used by our build regressions."""
import hashlib


def digest(data):
    return {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def inspect_iso(data):
    if len(data) % 2048 or data[:16] != b"SEGADISCSYSTEM  ":
        raise ValueError("not a sector-aligned Sega CD data disc")
    pvd = data[16 * 2048:17 * 2048]
    if pvd[:7] != b"\x01CD001\x01":
        raise ValueError("missing ISO9660 primary volume descriptor")
    files = {}

    def walk(record, prefix, visited):
        sector = int.from_bytes(record[2:6], "little")
        size = int.from_bytes(record[10:14], "little")
        start = sector * 2048
        if size == 0 or start + size > len(data):
            raise ValueError("ISO extent outside image")
        if record[25] & 2:
            if sector in visited:
                raise ValueError("cyclic ISO directory")
            block = data[start:start + size]
            directory = prefix + "/" if prefix else ""
            offset = 0
            while offset < len(block):
                length = block[offset]
                if not length:
                    offset = (offset // 2048 + 1) * 2048
                    continue
                if length < 34 or offset + length > len(block):
                    raise ValueError("invalid ISO directory record")
                child = block[offset:offset + length]
                name = child[33:33 + child[32]]
                if name not in (b"\0", b"\1"):
                    walk(child, directory + name.decode("ascii").split(";")[0], visited | {sector})
                offset += length
        else:
            if prefix in files:
                raise ValueError("duplicate ISO path")
            files[prefix] = digest(data[start:start + size])

    root = pvd[156:156 + pvd[156]]
    if len(root) < 34:
        raise ValueError("invalid ISO root directory")
    walk(root, "", set())
    return {"boot": digest(data[:32768]), "files": files}
