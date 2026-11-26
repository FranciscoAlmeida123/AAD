#!/usr/bin/env python3
"""make_inputs.py --- creates our own input files in inputs/ (PDF, PNG, PPM, HTML, TXT).

usage: python3 scripts/make_inputs.py [output_dir]   (default: inputs)
Only the standard library is used (zlib for the PNG).
"""
import struct
import sys
import zlib
from pathlib import Path

out = Path(sys.argv[1] if len(sys.argv) > 1 else "inputs")
out.mkdir(parents=True, exist_ok=True)

# --- image: 160x90 colour gradient with a few bands (shared by PPM and PNG) ---
W, H = 160, 90
rgb = bytearray()
for y in range(H):
    for x in range(W):
        rgb += bytes(((x * 255) // (W - 1), (y * 255) // (H - 1), ((x ^ y) * 4) & 0xFF))

(out / "aad_a1.ppm").write_bytes(b"P6\n%d %d\n255\n" % (W, H) + bytes(rgb))


def png_chunk(kind: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


raw = b"".join(b"\x00" + bytes(rgb[3 * W * y:3 * W * (y + 1)]) for y in range(H))
png = (b"\x89PNG\r\n\x1a\n"
       + png_chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
       + png_chunk(b"tEXt", b"Title\x00AAD A1 input image")
       + png_chunk(b"IDAT", zlib.compress(raw, 9))
       + png_chunk(b"IEND", b""))
(out / "aad_a1.png").write_bytes(png)

# --- PDF: one page with text; xref offsets computed exactly ---
text = b"BT /F1 24 Tf 72 720 Td (AAD 2026/2027 - assignment A1) Tj 0 -40 Td (SHA1 ending in 32 zero bits) Tj ET"
objs = [
    b"<< /Type /Catalog /Pages 2 0 R >>",
    b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
    b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>",
    b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
    b"<< /Length %d >>\nstream\n" % len(text) + text + b"\nendstream",
]
pdf = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
offsets = []
for i, body in enumerate(objs, 1):
    offsets.append(len(pdf))
    pdf += b"%d 0 obj\n" % i + body + b"\nendobj\n"
xref = len(pdf)
pdf += b"xref\n0 %d\n0000000000 65535 f \n" % (len(objs) + 1)
pdf += b"".join(b"%010d 00000 n \n" % o for o in offsets)
pdf += b"trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (len(objs) + 1, xref)
(out / "aad_a1.pdf").write_bytes(bytes(pdf))

# --- HTML and TXT ---
(out / "aad_a1.html").write_text(
    "<!DOCTYPE html>\n<html lang=\"en\">\n<head><meta charset=\"utf-8\"><title>AAD A1</title></head>\n"
    "<body>\n<h1>AAD 2026/2027 &mdash; assignment A1</h1>\n"
    "<p>The SHA1 of this file ends with 32 zero bits.</p>\n</body>\n</html>\n", encoding="utf-8")
(out / "aad_a1.txt").write_text(
    "AAD 2026/2027 - assignment A1\n"
    "The last line of this file was chosen so that its SHA1 ends with 32 zero bits.\n", encoding="utf-8")

print("inputs written to", out)
