/* Original application helper: exactly 7680 aligned bytes, Main-owned Word RAM
   to private Main RAM. Returns the native big-endian word sum modulo 65536.
   No SDK API, libgcc dependency, CPU variant or out-of-range tail read. */
.section .text
.global cinema_copy_checked
cinema_copy_checked:
  movem.l d2-d7/a2-a6, -(sp)
  move.l 48(sp), a0
  move.l 52(sp), a1
  moveq #0, d0
  move.w #479, d2
1:
.rept 4
  move.l (a0)+, d1
  move.l d1, (a1)+
  add.w d1, d0
  swap d1
  add.w d1, d0
.endr
  dbra d2, 1b
  movem.l (sp)+, d2-d7/a2-a6
  rts
