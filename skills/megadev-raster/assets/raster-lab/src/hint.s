/* Direct Mega CD Level-4 handler, original 68000. No C/BIOS calls in HBlank. */
#include <main/memmap.def.h>
#include <main/gate_arr.def.h>
.section .bss
.balign 2
.global raster_epoch,raster_armed,raster_hits,raster_done,raster_late,raster_timeout,raster_reg_off,raster_plan
.global raster_actual,raster_target,raster_setup
raster_actual: .space 2
raster_target: .space 2
raster_setup: .space 2
raster_epoch: .space 2
raster_armed: .space 2
raster_hits: .space 2
raster_done: .space 2
raster_late: .space 2
raster_timeout: .space 2
raster_reg_off: .space 2
raster_plan: .space 20
.global raster_old_gate,raster_old_vector,raster_installed
raster_old_gate: .space 2
raster_old_vector: .space 4
raster_installed: .space 2
.section .text
.global raster_install
raster_install:
  tst.w raster_installed
  bne.s 1f
  move.w (GA_REG_HBLANKVECT), raster_old_gate
  move.l (EXVEC_HBLANK), raster_old_vector
  move.l #raster_hint, d0
  move.l d0, (EXVEC_HBLANK)
  move.w d0, (GA_REG_HBLANKVECT)
  move.w #1, raster_installed
1:rts
/* Caller disables interrupts and disables IE1 through its VDP owner first.
   Never restore vectors while an armed H interrupt can still dispatch. */
.global raster_detach
raster_detach:
  tst.w raster_installed
  beq.s 1f
  clr.w raster_armed
  move.w raster_old_gate, (GA_REG_HBLANKVECT)
  move.l raster_old_vector, (EXVEC_HBLANK)
  clr.w raster_installed
1:rts
.global raster_hint
raster_hint:
  movem.l d0-d7/a0-a2, -(sp)
  lea 0xC00000, a0
  lea 0xC00004, a1
  /* A pending H request in VBlank must not consume the newly armed frame. */
  move.w (a1), d0
  btst #3, d0
  bne raster_exit
  move.w #0x2700, sr
  move.w raster_armed, d6
  beq raster_exit
  cmp.w raster_epoch, d6
  bne raster_exit
  /* Re-enabling IE1 can deliver an old pending request at the blank edge.
     Neither that request nor an early active-line request consumes the plan. */
  move.w 0xC00008, d0
  lsr.w #8, d0
  cmpi.w #224, d0
  bhs raster_exit
  move.w raster_plan, d1
  subq.w #1, d1
  cmp.w d1, d0
  blo raster_exit
  clr.w raster_armed
  move.w raster_reg_off, (a1)
  lea raster_plan, a2
  move.w (a2), d5
  move.w d5, raster_target
  move.w 2(a2), d7
  move.l 4(a2), d4
  move.w 8(a2), d2
  move.l 10(a2), d3
  addq.w #1, raster_hits
  /* The bounded edge wait and immediate writes are atomic. Validate before
     waiting, never spend the HBlank doing a second C/status/line check. */
  bsr raster_window
  tst.w d0
  beq raster_bad_line
  bsr raster_edge
  tst.w d0
  beq raster_bad_edge
  move.w d2, (a1)
  move.l #0x40000010, (a1)
  move.l d4, (a0)
  move.w #0x2400, sr
  btst #1, d7
  beq raster_complete
  move.w #0x2700, sr
  cmp.w raster_epoch, d6
  bne raster_exit
  bsr raster_window
  tst.w d0
  beq raster_bad_line
  bsr raster_edge
  tst.w d0
  beq raster_bad_edge
  /* Two preloaded colors on a fresh HBlank, not a bank-sized C loop. */
  move.l #0xC0680000, (a1)
  move.l d3, (a0)
raster_complete:
  addq.w #1, raster_done
  bra raster_exit
raster_bad_line:
  cmpi.w #-1, raster_late
  beq raster_exit
  addq.w #1, raster_late
  bra raster_exit
raster_bad_edge:
  cmp.w raster_epoch, d6
  bne raster_exit
  cmpi.w #-1, raster_timeout
  beq raster_exit
  addq.w #1, raster_timeout
raster_exit:
  movem.l (sp)+, d0-d7/a0-a2
.global raster_hint_return
raster_hint_return:
  rte
/* D0 = validity. Status disambiguates PAL blank-period V-counter wrap. */
raster_window:
  move.w (a1), d0
  btst #3, d0
  bne.s 1f
  move.w 0xC00008, d0
  lsr.w #8, d0
  move.w d0, raster_actual
  move.w d5, d1
  subq.w #1, d1
  cmp.w d1, d0
  blo.s 1f
  addq.w #5, d1
  cmp.w d1, d0
  bhs.s 1f
  moveq #1, d0
  rts
1:moveq #0, d0
  rts
/* Wait for fall, then rise. Each loop is bounded even on a stuck status bit. */
raster_edge:
  move.w #63, d1
1:move.w (a1), d0
  btst #2, d0
  beq.s 2f
  dbra d1, 1b
  moveq #0, d0
  rts
2:move.w #63, d1
3:move.w (a1), d0
  btst #2, d0
  bne.s 4f
  dbra d1, 3b
  moveq #0, d0
  rts
4:moveq #1, d0
  rts
