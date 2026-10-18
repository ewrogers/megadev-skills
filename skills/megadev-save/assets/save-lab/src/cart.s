#include <main/bramcart.def.h>
/* ABI adapters around Megadev's documented Main BRAM_CART entry. */
.section .text
.global cart_init,cart_read,cart_write,cart_format
cart_init:
    movem.l d2-d7/a2-a6,-(sp)
    lea cart_work,a0
    lea cart_strings,a1
    moveq #0,d0
    jsr BRAM_CART
    bcs 1f
    moveq #3,d0
    bra 2f
1:  moveq #0,d0
    move.w d1,d0
2:  movem.l (sp)+,d2-d7/a2-a6
    rts
cart_read:
    movem.l d2-d7/a2-a6,-(sp)
    movea.l 52(sp),a2
    lea cart_a,a0
    tst.w 50(sp)
    beq 1f
    lea cart_b,a0
1:  moveq #2,d0
    jsr BRAM_CART
    bcs 4f
    cmpi.w #1,d0
    bne 5f
    tst.b d1
    bne 5f
    /* Search changes A0. Re-select our exact name before reading. */
    lea cart_a,a0
    tst.w 50(sp)
    beq 2f
    lea cart_b,a0
2:  movea.l a2,a1
    moveq #3,d0
    jsr BRAM_CART
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
cart_write:
    movem.l d2-d7/a2-a6,-(sp)
    lea cart_a,a0
    tst.w 50(sp)
    beq 1f
    lea cart_b,a0
1:  movea.l 52(sp),a1
    moveq #0,d1 /* Sega Tech Bulletin #1 requirement. */
    moveq #4,d0
    jsr BRAM_CART
    bcs 2f
    moveq #0,d0
    bra 3f
2:  moveq #6,d0
3:  movem.l (sp)+,d2-d7/a2-a6
    rts
/* Explicit destructive operation; caller must initialize and confirm first. */
cart_format:
    movem.l d2-d7/a2-a6,-(sp)
    moveq #6,d0
    jsr BRAM_CART
    bcs 1f
    moveq #0,d0
    bra 2f
1:  moveq #6,d0
2:  movem.l (sp)+,d2-d7/a2-a6
    rts
.section .rodata
.balign 2
cart_a: .ascii "MDSAVELAB_A"
.byte 0
.word 1
cart_b: .ascii "MDSAVELAB_B"
.byte 0
.word 1
.section .bss
.balign 4
cart_work: .space 0x640
cart_strings: .space 12
