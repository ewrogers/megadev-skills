/* ABI adapters around Megadev's documented Sub _BURAM entry. */
.section .text
.global bram_init,bram_read,bram_write,bram_format
bram_init:
    movem.l d2-d7/a2-a6,-(sp)
    lea bram_work,a0
    lea bram_strings,a1
    moveq #0,d0
    jsr 0x5F16
    bcs 1f
    moveq #3,d0
    bra 2f
1:  moveq #0,d0
    move.w d1,d0
2:  movem.l (sp)+,d2-d7/a2-a6
    rts
bram_read:
    movem.l d2-d7/a2-a6,-(sp)
    movea.l 52(sp),a2
    lea bram_a,a0
    tst.w 50(sp)
    beq 1f
    lea bram_b,a0
1:  moveq #2,d0
    jsr 0x5F16
    bcs 4f
    cmpi.w #1,d0
    bne 5f
    tst.b d1
    bne 5f
    /* Search changes A0. Re-select our exact name before reading. */
    lea bram_a,a0
    tst.w 50(sp)
    beq 2f
    lea bram_b,a0
2:  movea.l a2,a1
    moveq #3,d0
    jsr 0x5F16
    bcs 6f
    cmpi.w #1,d0
    bne 5f
    tst.b d1
    bne 5f
    moveq #0,d0
    bra 7f
4:  moveq #4,d0
    bra 7f
5:  moveq #9,d0
    bra 7f
6:  moveq #6,d0
7:  movem.l (sp)+,d2-d7/a2-a6
    rts
bram_write:
    movem.l d2-d7/a2-a6,-(sp)
    lea bram_a,a0
    tst.w 50(sp)
    beq 1f
    lea bram_b,a0
1:  movea.l 52(sp),a1
    moveq #0,d1 /* Sega Tech Bulletin #1 requirement. */
    moveq #4,d0
    jsr 0x5F16
    bcs 2f
    moveq #0,d0
    bra 3f
2:  moveq #6,d0
3:  movem.l (sp)+,d2-d7/a2-a6
    rts
/* Explicit destructive operation; caller must initialize and confirm first. */
bram_format:
    movem.l d2-d7/a2-a6,-(sp)
    moveq #6,d0
    jsr 0x5F16
    bcs 1f
    moveq #0,d0
    bra 2f
1:  moveq #6,d0
2:  movem.l (sp)+,d2-d7/a2-a6
    rts
.section .rodata
.balign 2
bram_a: .ascii "MDBESTLAB_A"
.byte 0
.word 1
bram_b: .ascii "MDBESTLAB_B"
.byte 0
.word 1
.section .bss
.balign 4
bram_work: .space 0x640
bram_strings: .space 12
