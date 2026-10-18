# Game flow and transitions

## Choose the smallest useful state model

Title, play, pause, results and menus describe behavior. They do not require a
different binary or allocator each. A pause overlay can retain the current game
state; a new level may replace local actors while retaining shared art. Decide
which settings, progress and checkpoints survive before writing enter/leave code.

Give one Main-loop owner responsibility for requests. Useful policies include
refusing a request while busy or keeping one latest destination. Transition Lab
uses the latter. A queued request waits for the current transaction to finish;
it cannot overwrite a live CD request. Invalid or same-scene idle requests do
nothing. A generation counter refuses further changes before wrapping.

The Lab's bounded sequence is:

| Phase | What permits progress |
| --- | --- |
| Fade out | Palette consumer confirms the old area is actually black |
| Drain | Old reader/mailbox retired, Main owns 2M Word RAM, local leases released |
| Load | Target image passes validation and every bounded pattern upload completes |
| Commit | Last replacement map payload is consumed by VBlank |
| Fade in | Elapsed-time fade reaches its final shade |

Failures before commit retain the previous scene and its local progress. Failed
loads fade back to that view; they do not run its cleanup or invalidate its
handles. A stuck ownership transfer stays visibly waiting. The Lab's 30-second
stall notice is a diagnostic policy, not a guaranteed physical-drive timeout.

## Keep services alive

Use elapsed VBlank ticks, not UI redraw count. The Lab uses 300 Hz time units:
5 per NTSC tick, 6 per PAL tick. Its eight-step fade lasts 0.4 seconds. Pause
stops movement and actor animation; the loader, Sub INT2 continuation, audio
requests, input edges and VBlank consumer keep running. A pending scene change
disables ordinary play input; it does not disable the interrupt service.

Do not copy the Lab's input mapping or menu structure into an unrelated brief.
Persist the user's selected menu item when appropriate and discard old input
edges at transition boundaries so a held confirm cannot choose the next menu.

## Scenes and Megadev modules are different

Transition Lab keeps its scene behavior resident in IPX. Its small ROOM.MMD
remains a startup/ABI fixture and is not the scene representation. Data images
are streamed separately. This makes the lifetime contract visible without also
inventing dynamic code unloading.

For a larger project, Megadev modules can relocate code/data into chosen memory
regions. Before replacing one, retire every callback, ISR, return address and
import pointing into it. Preserve resident services and their shared ABI.
Loading another module does not automatically cancel old work or clean up C
globals. Inspect the selected version's initialization/BSS/import contracts.

Audited against Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`:
[module design](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/modules.md),
[Main BIOS declarations](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.h).
Use the selected container's actual source if its revision differs.
