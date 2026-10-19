#include <macros.s>
GLOBAL MODULE_ROM_ORIGIN 0x10000
GLOBAL MODULE_ROM_LENGTH 0xC000
/* One 16 KiB CDC staging buffer plus small state. Separate from module ROM
   (10000..1BFFF, including resident sample constants) and the existing Sub
   stack (1E000..1FFFF). The reservation includes unused headroom. */
GLOBAL MODULE_RAM_ORIGIN 0x24000
GLOBAL MODULE_RAM_LENGTH 0x8000
/* Sub stack: 0x1E000..0x20000, outside the module's code/data reservations. */
