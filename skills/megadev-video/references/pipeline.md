# Frame preparation and practical budgets

## Choose an output contract before an encoder

Start with native working dimensions, viewpoint, duration, desired image rate,
colors, captions and audio. Preserve the game's chosen style. For drawn assets,
use crisp native pixels; for photographic footage, evaluate crop, palette
quantization, temporal stability and dithering at the actual displayed size.
Dithering can improve gradients but can also shimmer between frames and make
changed-tile encoding worse. It is a choice to compare, not a required filter.

CIN1 is a deliberately simple application format: a 2,048-byte header and four
7,680-byte frames in a 32,768-byte file. Each 160×96 frame contains 240 sequential
8×8 tiles, four bits per pixel, in VDP tile order. The sequence has 48 frames and
12 files, totaling 393,216 payload bytes. Palette slot one is resident; glyphs
use a separate slot. Header fields and sums are big endian; runtime integers are
explicitly bounded. A sum16 detects accidental changes but is not authentication.

Header: `CIN1`, u16 version/width/height/first-frame/count/frame-bytes/300-Hz
period/total-frames at offsets 4..19. Four u16 frame sums are at 32..39. Remaining
header padding is reserved. Current version requires exactly four frames per
file, 160×96 pixels, 7,680 frame bytes, a 30-unit period and 48 total frames.
The exporter is standard-library Python and generates every pixel directly;
there is no resampling step or hidden external art dependency.

These frames are already VDP-native, so the runtime copies/checks them rather
than decompressing them. Do not call this an FMV compression codec. To import
footage, add a deliberate offline conversion tool with palette/length validation
and license/source provenance, then compare its on-console result.

## Rate and space are different budgets

At 75 data sectors/sec × 2,048 bytes, nominal 1× payload capacity is 153,600
bytes/sec before file-access, seek and scheduling costs. One 160×96 4bpp image
is 7,680 bytes: 10/sec costs 76,800 bytes/sec before headers; 60/sec costs 460,800.
Changing a timer cannot overcome that difference. Emulated drive latency is not
a worst-case physical seek budget.

Possible responses: smaller images, lower image rate, indexed stills with moving
sprites, larger contiguous groups, frame compression, or changed-tile streams.
Changed tiles can save disc and upload bandwidth on stable backgrounds; noisy
footage and moving dithering may defeat them. Validate worst-case keyframes,
packet limits and random-access/restart points. Decode into an inactive buffer;
never let a malformed delta overwrite unrelated live patterns. Every codec adds
CPU, scratch-memory and recovery costs that require measurement.

## Account for residency

The Lab reserves one 7,680-byte Main frame, a maximum 4,096-byte copied upload
batch, two 32,768-byte Sub PRG staging groups (28000..37FFF), and two 7,680-byte
VRAM image slots. Main/Sub code, fonts, poster, audio constants, stacks, HUD and
handshake state are additional costs. The fixed Sub staging range must not
intersect code, BSS, directory state, PCM sources or stacks in the final link.

VRAM image patterns are 2000..3DFF and 4000..5DFF. Window uses 9000; Plane B uses
A000; SAT B800, HScroll BC00, Plane A pages C000/E000. Both pages reference their
own pattern slot; the foreground text plane must not alias the second page.
The initial prototype flickered because default Plane B also used E000. Tests
now execute table setup and packet ranges to protect this exact failure.
