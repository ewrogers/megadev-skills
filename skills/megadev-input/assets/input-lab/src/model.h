#ifndef PROJECT_MODEL_H
#define PROJECT_MODEL_H
/* Plain C model; flags intentionally match verified 3-button BIOS input bits. */
#define LAB_UP 1
#define LAB_DOWN 2
#define LAB_LEFT 4
#define LAB_RIGHT 8
#define LAB_START 128
#define LAB_IDLE 0
#define LAB_ACK 1
#define LAB_DONE 2
#define LAB_FAILED 3
typedef struct {unsigned short phase, command, elapsed, limit, result;} LabJob;
typedef struct {unsigned short x,y,held,paused;} LabCursor;
void lab_cursor_step(LabCursor *c,unsigned short held);
int lab_request(LabJob *j,unsigned short command,unsigned short limit);
int lab_poll(LabJob *j,unsigned short status,unsigned short result);
#endif
