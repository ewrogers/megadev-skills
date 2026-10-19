# Rendering and performance

## Verified integration boundary

Audited Megadev revision: `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`.
Use the actual definitions in
[`main/vdp.h`](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/vdp.h),
[`main/bios.h`](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.h)
and [`main/io.def.h`](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/io.def.h).
The Lab's bridge preserves C callee-saved registers for BIOS services, keeps
Sub INT2/CD service alive and polls joypads once through the existing VBlank
owner. The bounded publisher runs before the variable-duration joypad scan;
input edges latch after that scan. It never installs an effects-specific
interrupt or mailbox.

## Sprite limits are not particle limits

This profile is H40, 320×224: 80 hardware sprite entries per frame, with 20
entries and 320 sprite pixels per scanline. Transparent pixels still count
against the fetch width. Larger logical sprites can consume several entries.
The Mega CD does not add VDP palettes, sprite slots or scanline capacity.
See [VDP sprite behavior](https://plutiedev.com/sprites).

The Lab emits one 16×16 hero followed by 8×8 effects, in priority order:
impacts/smoke, trails, then weather. The submitted list caps at **56 entries
including the hero**. It conservatively charges each intersected eight-line
band; a band admits at most 16 width units. An 8px effect costs one and the
16px hero costs two. Thus both entry count and fetched pixels remain below
hardware limits, even for sprites crossing band boundaries. This only proves
the Lab's fixed sizes; adapt the accounting for other dimensions and any other
sprite owner. It is conservative and may hide effects that hardware could fit.

Decorative 8px cells use whole-cell left/top culling (a cell with a negative
position is omitted rather than partially clipped). The hero is constrained
inside the viewport. Offscreen particles still simulate and expire but do not enter the SAT. Render
rejection does not mutate the model or its random state. The list's final link
is zero; an empty list must clear the first SAT entry to prevent stale sprites.
A game may use a rotating admission cursor for fairer decoration, provided it
never demotes important gameplay and its scheduling stays bounded.

## Resident art and publication

The font occupies VRAM 0400–0FFF, the hero/effect atlas 1000–119F, and deduplicated
background patterns start at 2000. Static map/palette data is uploaded with the
display disabled. The generator checks the background stays below the B800 SAT.
Palette 0 is HUD text, 2 is the background, 3 is the hero/effects; palette 1 is
unused. Each hardware sprite selects one palette. There is no extra color bank
from using Sub or the ASIC.

Sparks, smoke and trails choose resident tiles by lifetime; snow/rain reuse
resident tiles. No per-particle tile uploads, palette rewrites, BIOS text calls,
heap work or sorting occur in VBlank. Foreground builds one of two snapshots;
a nonzero published index freezes that snapshot until consumed. Late callbacks
leave it pending instead of writing outside the guarded early VBlank interval.

The maximum data-port payload is 56×8 SAT bytes + 320 HUD bytes + 8 scroll bytes
= **776 bytes per published frame**, plus control-port commands and instruction
cost. This is a byte bound, not proof the callback fits. Measure the code and
worst combined workload. A different game's uploader should merge this work
into its own frame owner rather than install a parallel publisher.

## Shake and optimization

The Lab's deterministic 18-tick shake decays from two pixels to one, then zero.
The same snapshot offsets world sprites and background scroll. The window HUD
stays anchored. Collision/model coordinates remain unchanged; direction reversals
and pause cannot accumulate displacement. Fill the camera's border/overscan so
small offsets do not reveal uninitialized map cells. Up+Start disables shake.

The fixed-size builder emits the 16px hero directly and uses a specialized
8px path for decorations. It avoids a generic sprite function call, signed
division and variable-size band loops per particle. This optimization depends
on the verified fixed dimensions; it must not silently handle arbitrary sprites.

Measure simulation, list construction and publication separately. Use dense
weather **and** bursts, not isolated cases. Common improvements are resident art,
explicit effect allowances, cheap fixed-size emission, coarse conservative
scanline accounting and trimming lower-priority decoration. Avoid modulo,
wide division, generic object dispatch and repeated full-array scans in hot
loops unless measured affordable. Do not move heavy work to Sub merely to hide
Main costs: Word RAM ownership and result publication would need a real design.
