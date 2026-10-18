/* C ABI shim around Megadev's assembly find_file (A0, carry set on missing).
   Directory record fields are packed at byte offsets 14/18, not a C struct. */
.section .text
.global stream_file_size
stream_file_size:
  movem.l d2-d7/a2-a6, -(sp)
  move.l 48(sp), a0
  jsr find_file
  bcs 1f
  move.l 18(a0), d0
  bra 2f
1:moveq #0, d0
2:movem.l (sp)+, d2-d7/a2-a6
  rts
