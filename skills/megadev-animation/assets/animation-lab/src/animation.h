#ifndef MEGADEV_LAB_ANIMATION_H
#define MEGADEV_LAB_ANIMATION_H
/* Application timeline, not Megadev SDK APIs. One call per simulation step. */
typedef unsigned short AnU16;
typedef signed short AnS16;
#define AN_NONE 65535u
#define AN_FOOT 1u
#define AN_SWISH 2u
#define AN_STARTED 0x8000u
#define AN_COMPLETE 0x4000u
#define AN_ERROR 0x2000u
enum {AN_IDLE,AN_WALK,AN_ATTACK,AN_HURT,AN_DEATH,AN_RISE,AN_CLIPS};
typedef struct {AnS16 x,y;AnU16 w,h;} AnBox;
typedef struct {AnU16 pose,hold,events,weapon;AnBox hit;} AnFrame;
typedef struct {char const *name;AnFrame const *frames;AnU16 count,loop;} AnClip;
typedef struct {AnU16 clip,frame,age,generation,finished;} AnPlayer;
extern AnClip const an_clips[AN_CLIPS];
int an_validate(AnClip const *clip);
AnU16 an_play(AnPlayer *p,AnU16 clip,AnU16 restart);
AnU16 an_step(AnPlayer *p);
AnFrame const *an_pose(AnPlayer const *p);
int an_hitbox(AnPlayer const *p,AnU16 left,AnS16 x,AnS16 y,AnBox *out);
int an_overlap(AnBox const *a,AnBox const *b);
#endif
