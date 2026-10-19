#ifndef DIALOGUE_SPEECH_H
#define DIALOGUE_SPEECH_H
#include "ui.h"
enum { SPEECH_PLAY=8,SPEECH_STOP=9 };
/* Policy around the audio owner's mailbox, not a second mailbox. */
typedef struct {Ui16 wanted,generation,applied,applied_generation,op,arg,issued_generation,inflight,fault;} Speech;
int speech_want(Speech *s,Ui16 clip,Ui16 generation);
Ui16 speech_next(Speech const *s);
int speech_sent(Speech *s,Ui16 op);
void speech_complete(Speech *s,Ui16 failed);
#endif
