#include <assert.h>
#include <string.h>
#include "model.h"
int main(void) {
    LabCursor c={20,18,0,0};
    lab_cursor_step(&c,LAB_START);assert(c.paused);
    lab_cursor_step(&c,LAB_START|LAB_RIGHT);assert(c.paused && c.x==20);
    lab_cursor_step(&c,0);lab_cursor_step(&c,LAB_START|LAB_RIGHT);assert(!c.paused && c.x==21);
    for(unsigned n=0;n<100;n++)lab_cursor_step(&c,LAB_RIGHT|LAB_DOWN);
    assert(c.x==36 && c.y==20);
    for(unsigned n=0;n<100;n++)lab_cursor_step(&c,LAB_LEFT|LAB_UP);
    assert(c.x==3 && c.y==17);
    LabJob j={0};assert(!lab_request(&j,0,4));assert(!lab_request(&j,1,0));
    assert(lab_request(&j,2,4));LabJob copy=j;
    assert(!lab_request(&j,1,4));assert(!memcmp(&j,&copy,sizeof(j)));
    assert(!lab_poll(&j,1,123));assert(j.phase==LAB_ACK); /* stale other command */
    assert(lab_poll(&j,2,123)==1);assert(j.phase==LAB_DONE);
    assert(!lab_poll(&j,2,123));assert(lab_poll(&j,0,0xBEEF)==2);
    assert(j.phase==LAB_IDLE && j.result==0xBEEF);assert(!lab_poll(&j,0,0));
    assert(lab_request(&j,1,2));assert(!lab_poll(&j,0,0));assert(lab_poll(&j,0,0)==-1);
    assert(j.phase==LAB_FAILED && !lab_request(&j,2,4));
    j=(LabJob){0};assert(lab_request(&j,1,2));assert(lab_poll(&j,0xFF,0)==-1);
    j=(LabJob){0};assert(lab_request(&j,1,2));assert(lab_poll(&j,1,0)==1);
    assert(!lab_poll(&j,1,0));assert(lab_poll(&j,1,0)==-1); /* stuck acknowledgement */
    return 0;
}
