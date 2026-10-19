/* C ABI adapters and one project-owned VBlank path. */
#include <main/bios.def.h>
#include <main/memmap.def.h>
.section .text
.macro BIOS_C name, address
.global \name
\name:
  movem.l d2-d7/a2-a6, -(sp)
  jsr \address
  movem.l (sp)+, d2-d7/a2-a6
  rts
.endm
BIOS_C lab_display_on, BIOS_VDP_DISP_ENABLE
BIOS_C lab_display_off, BIOS_VDP_DISP_DISABLE
BIOS_C lab_font, BIOS_LOAD_FONT_DEFAULTS
BIOS_C lab_clear, BIOS_CLEAR_TABLES
.global ui_install
ui_install:
  move.l #ui_irq, (EXVEC_VBLANK)
  rts
/* STOP atomically enables interrupts while waiting; no BIOS joypad poll. */
.global lab_wait
lab_wait:
  move.w #0x2700, sr
  stop #0x2000
  rts
.global ui_irq
ui_irq:
  movem.l d0-d7/a0-a6, -(sp)
  /* Keep the Sub INT2/CD access service alive without BIOS_COMM_SYNC polling. */
  jsr BIOS_TRIGGER_IFL2
  jsr BIOS_READ_JOYPAD
  jsr vblank_user
  movem.l (sp)+, d0-d7/a0-a6
.global ui_irq_return
ui_irq_return:
  rte
.global lab_print
lab_print:
  movem.l d2-d7/a2-a6, -(sp)
  move.l 48(sp), a1
  move.l 52(sp), d0
  jsr BIOS_PRINT
  movem.l (sp)+, d2-d7/a2-a6
  rts
.global lab_upload_work
lab_upload_work:
  movem.l d2-d7/a2-a6, -(sp)
  move.l 48(sp), d1
  move.l 52(sp), d0
  move.l 56(sp), d2
  jsr BIOS_DMA_XFER
  movem.l (sp)+, d2-d7/a2-a6
  rts
