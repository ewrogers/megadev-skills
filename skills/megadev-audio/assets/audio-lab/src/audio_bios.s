/* C ABI adapters around actual Megadev BIOS definitions. */
#include <sub/bios.def.h>
#include <sub/memmap.def.h>
.section .text
.global audio_cd_command
 audio_cd_command:
  movem.l d2-d7/a2-a6, -(sp)
  move.l 48(sp), d0
  move.l 52(sp), d1
  move.w d1, audio_track_arg
  lea audio_track_arg, a0
  jsr CDBIOS
  movem.l (sp)+, d2-d7/a2-a6
  rts
.global audio_cd_ready
 audio_cd_ready:
  movem.l d2-d7/a2-a6, -(sp)
  move.w #BIOS_CDB_CHK, d0
  jsr CDBIOS
  bcs 1f
  moveq #1, d0
  bra 2f
1:moveq #0, d0
2:movem.l (sp)+, d2-d7/a2-a6
  rts

.section .bss
.balign 2
audio_track_arg: .space 2
