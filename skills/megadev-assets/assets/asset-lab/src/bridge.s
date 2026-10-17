/* C ABI adapters for BIOS calls; preserve callee-saved registers explicitly. */
#include <main/bios.def.h>
.section .text
.macro BIOS_C name, address
.globl \name
\name:
  movem.l d2-d7/a2-a6, -(sp)
  jsr \address
  movem.l (sp)+, d2-d7/a2-a6
  rts
.endm
BIOS_C lab_wait, BIOS_VBLANK_WAIT_DEFAULT
BIOS_C lab_display_on, BIOS_VDP_DISP_ENABLE
BIOS_C lab_display_off, BIOS_VDP_DISP_DISABLE
BIOS_C lab_font, BIOS_LOAD_FONT_DEFAULTS
BIOS_C lab_clear, BIOS_CLEAR_TABLES
.globl lab_print
lab_print:
  movem.l d2-d7/a2-a6, -(sp)
  move.l 48(sp), a1
  move.l 52(sp), d0
  jsr BIOS_PRINT
  movem.l (sp)+, d2-d7/a2-a6
  rts

/* The BIOS Word RAM variant also handles the hardware's final-word DMA quirk. */
.global lab_upload_work
lab_upload_work:
  movem.l d2-d7/a2-a6, -(sp)
  move.l 48(sp), d1
  move.l 52(sp), d0
  move.l 56(sp), d2
  jsr BIOS_DMA_XFER
  movem.l (sp)+, d2-d7/a2-a6
  rts
.global lab_upload_word
lab_upload_word:
  movem.l d2-d7/a2-a6, -(sp)
  move.l 48(sp), d1
  move.l 52(sp), d0
  move.l 56(sp), d2
  jsr BIOS_DMA_XFER_WORD_RAM
  movem.l (sp)+, d2-d7/a2-a6
  rts
