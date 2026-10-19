#include <macros.s>
#include <system.macros.s>
#include <init.macros.s>
.section .init
GLABEL init
  DISABLE_INTERRUPTS
  movea.l #0x20000, sp
  BASIC_INIT
  jmp main
