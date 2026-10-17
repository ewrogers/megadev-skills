#ifndef INPUT_LAB_PLATFORM_H
#define INPUT_LAB_PLATFORM_H
#include "input.h"
void input_poll(InSample samples[8]);
void input_install(void);
InU16 input_lock(void);
void input_unlock(InU16 previous_sr);
extern InU16 input_bus_fault;
#endif
