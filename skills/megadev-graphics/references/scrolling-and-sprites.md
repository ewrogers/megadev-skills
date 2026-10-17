# Scrolling, sprites and animation

## Camera and planes

Store world coordinates separately from screen coordinates. Graphics Lab clamps its camera to a 1024-pixel illustrative world, then uses `screen_x = world_x - camera_x` for actors. It clips before adding the VDP's 128-pixel coordinate bias; casting first can wrap a very distant object back onto the screen.

The 64-cell plane width is 512 pixels. Prepared sky/quay maps are 64×24 cells, placed at row four beneath the fixed HUD. Their source rows use a 128-byte table stride, independent of the 320-pixel viewport. The remaining plane rows are not streamed content.

Full-plane horizontal scrolling writes two words at `BIOS_VDP_DEFAULT_HSCROLL`: **Plane A first, Plane B second**. The foreground uses negative camera offset and the sky uses one quarter of that offset. With parallax disabled both use the same offset. Wrap against the actual repeated image period; the Lab uses 512 pixels, not an arbitrary PNG width. The low ten bits encode the VDP scroll value, including negative values.

The Window covers the top four rows. Horizontal Window position zero leaves no left-hand region; vertical position four selects the top strip. It has no camera transform. Vertical scroll remains zero. Changing to per-tile/per-line scroll requires a larger table, a matching VDP mode and a new transfer budget. It is not accomplished by merely writing more full-plane words.

The art is repeated deliberately, not streamed from a larger world. Both native 256-pixel sources were extended with horizontally flipped copies; adjacent and wrapped edge columns match exactly, with no resampling. Repeated silhouettes are an illustrative trade-off. A production artist can supply a purpose-built panoramic repeat or a scrolling level pipeline.

## Depth and priority

The quay is Plane A with high-priority cells; the sky is low-priority Plane B. The controlled ship starts low priority and disappears behind opaque quay pixels. C changes that ship to high priority, putting it in front. Other ships remain low priority.

Hardware priority is a small set of plane/sprite relationships, not a general Z-buffer. Transparent pixels still expose underlying art. SAT order decides overlaps between sprites; nearer game objects can be submitted earlier if that is the intended rule. Do not infer depth sorting from a tile's screen Y alone for every game.

`gfx_sprite` writes explicit SAT words instead of relying on host/compiler bitfield packing. Width and height fields encode cell counts minus one. Ship frame patterns are contiguous in hardware column-major order, as prepared by the asset pipeline. Flip is a hardware bit, so turning left does not need duplicated art. Changing a pattern base must account for the entire selected frame fitting the allocated range.

## Clock and frame selection

The model advances at a 60-tick simulation rate: a 60 Hz display advances one tick per frame; a 50 Hz display accumulates 60/50, sometimes advancing two ticks. Movement, animation and effects use that clock. The two ship cels each hold six simulation ticks, so PAL does not simply play the animation 5/6 as fast. No object needs an independent Timer allocation.

This assumes the frame service meets the display cadence. It does not compensate for arbitrary missed VBlanks or measure emulator speed. Use the game's existing clock rather than introducing a second one. The sample pauses simulation while continuing to submit a stable frame, preserving input responsiveness and the HUD.

`gfx_frame` is simple looping frame selection. One-shot completion, gameplay events, hitboxes, crossfades and audio synchronization are separate animation systems, not implicit behavior of the VDP. The Lab's two-cel propeller glint is an example, not a prescribed animation style.
