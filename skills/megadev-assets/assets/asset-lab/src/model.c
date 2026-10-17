#include "model.h"
void lab_cursor_step(LabCursor *c,unsigned short held) {
    unsigned short pressed=held & (unsigned short)~c->held;c->held=held;
    if(pressed & LAB_START)c->paused^=1;
    if(c->paused)return;
    if((held & LAB_LEFT) && c->x>3)c->x--;
    if((held & LAB_RIGHT) && c->x<36)c->x++;
    if((held & LAB_UP) && c->y>17)c->y--;
    if((held & LAB_DOWN) && c->y<20)c->y++;
}
int lab_request(LabJob *j,unsigned short command,unsigned short limit) {
    if(j->phase!=LAB_IDLE || !command || !limit)return 0;
    j->command=command;j->limit=limit;j->elapsed=0;j->result=0;j->phase=LAB_ACK;return 1;
}
/* Return 1 on ACK (caller clears command), 2 on completion, -1 on fault. */
int lab_poll(LabJob *j,unsigned short status,unsigned short result) {
    if(j->phase!=LAB_ACK && j->phase!=LAB_DONE)return 0;
    if(status==0xFF){j->phase=LAB_FAILED;return -1;}
    if(j->phase==LAB_ACK && status==j->command){j->phase=LAB_DONE;return 1;}
    if(j->phase==LAB_DONE && !status){j->result=result;j->phase=LAB_IDLE;return 2;}
    if(++j->elapsed>=j->limit){j->phase=LAB_FAILED;return -1;}
    return 0;
}
