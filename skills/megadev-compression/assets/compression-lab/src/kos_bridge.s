/* C ABI adapter: the audited SDK C inline wrapper is not used. Kos_Decomp
   expects A0/A1 and clobbers D0-D6; preserve the full C callee-saved set.
   Call only after cp_kos_valid, with exclusive, immutable input/output. */
.section .text
.global cp_kos_decode
cp_kos_decode:
  movem.l d2-d7/a2-a6,-(sp)
  move.l 48(sp),a0
  move.l 52(sp),a1
  jsr Kos_Decomp
  movem.l (sp)+,d2-d7/a2-a6
  rts

/* Aligned Word RAM baseline, with a byte tail. Avoid comparing the optimized
   SDK decoder against a deliberately slow C byte-at-a-time copy. <=32768 B. */
.global cp_raw_copy
cp_raw_copy:
  move.l 4(sp),a0
  move.l 8(sp),a1
  move.l 12(sp),d0
  move.l d0,d1
  andi.w #3,d1
  lsr.l #2,d0
  beq 2f
  subq.w #1,d0
1:move.l (a0)+,(a1)+
  dbra d0,1b
2:tst.w d1
  beq 4f
  subq.w #1,d1
3:move.b (a0)+,(a1)+
  dbra d1,3b
4:rts
