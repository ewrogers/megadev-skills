#include "lab.h"
volatile u32 room_magic=0xA1B2C3D4;
volatile u32 room_zero[8];
void main(void){room_runs++;room_magic=0;room_zero[0]=1;}
