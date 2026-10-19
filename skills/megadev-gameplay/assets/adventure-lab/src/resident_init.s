#include <system.macros.s>
#include <init.macros.s>
#include <main/mmd.macros.s>
.section .init
  DISABLE_INTERRUPTS
  INIT_MMD
  movea.l #0xFFE000, sp
  move.l a0, -(sp)
  BASIC_INIT
  movea.l (sp)+, a0
  jmp (a0)
