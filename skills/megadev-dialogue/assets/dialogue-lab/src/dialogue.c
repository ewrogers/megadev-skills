#include "dialogue.h"
#define SPAN(s,p,f,d) {s,sizeof(s)-1,p,f,d}
typedef struct {DSpan const *spans;Ui16 count,speaker,next,voice,paired;} Node;
static DSpan const welcome[]={SPAN("Welcome, traveler.\nThe clockwork bloom is waking.",1,0,0)};
static DSpan const question[]={SPAN("Should I wind the clock, or let the petals rest?",1,0,0)};
static DSpan const wind[]={SPAN("A gentle turn... ",1,0,0),SPAN("Listen.",2,1,60),SPAN(" The petals have a tune.\fOne turn is enough. We must not hurry a living thing.",1,0,0)};
static DSpan const rest[]={SPAN("We can wait. The garden keeps its own time.",1,0,0)};
static DSpan const thanks[]={SPAN("I'll write that down. Thank you.",1,0,0)};
static DSpan const listen[]={SPAN("Listen to the leaves. Each one keeps a different beat.",1,0,0)};
static DSpan const reply[]={SPAN("From below, I hear the clock. From above, you hear rain.",1,0,0)};
static DSpan const end_notes[]={SPAN("One room. ",1,0,0),SPAN("Two ways of listening.",2,1,60)};
static Node const nodes[]={
    {welcome,1,0,1,1,0},{question,1,1,D_END,0,0},
    {wind,3,0,4,0,0},{rest,1,0,4,0,0},{thanks,1,1,D_END,0,0},
    {listen,1,1,6,2,1},{reply,1,0,7,0,1},{end_notes,2,1,D_END,0,1}
};
_Static_assert(sizeof(DLayout)==1066,"Layout uses bounded byte glyphs and word delays");
static void copy_layout(DLayout *to,DLayout const *from) {
    to->pages=from->pages;
    for(Ui16 i=0;i<D_PAGES;i++)to->end[i]=from->end[i];
    for(Ui16 i=0;i<D_CAP;i++){to->glyph[i]=from->glyph[i];to->attr[i]=from->attr[i];to->delay[i]=from->delay[i];}
}
int d_compile(DLayout *out,DSpan const *spans,Ui16 count) {
    if(!out || !spans || !count || count>8)return 0;
    DLayout next;Ui16 pos=0,pending=0,budget=0;Ui16 boundary=1;
    for(Ui16 i=0;i<D_CAP;i++){next.glyph[i]=0;next.attr[i]=0;next.delay[i]=0;}
    next.pages=1;for(Ui16 i=0;i<D_PAGES;i++)next.end[i]=0;
    for(Ui16 span=0;span<count;span++) {
        DSpan const *a=&spans[span];
        if(!a->text || a->length>512-budget || a->palette>3 || a->face>1 || a->pause>9000-pending)return 0;
        budget+=a->length;pending+=a->pause;
        for(Ui16 i=0;i<a->length;i++) {
            Ui8 ch=(Ui8)a->text[i];
            if(ch=='\n'){pos=(Ui16)((pos/D_WIDTH+1)*D_WIDTH);boundary=1;continue;}
            if(ch=='\f'){pos=(Ui16)((pos/D_PAGE+1)*D_PAGE);boundary=1;continue;}
            if(ch==' ' && pos%D_WIDTH==0){boundary=1;continue;}
            if(boundary && ch!=' ') {
                Ui16 length=0;
                while(i+length<a->length && length<=D_WIDTH) {
                    Ui8 v=(Ui8)a->text[i+length];if(v==' ' || v=='\n' || v=='\f')break;length++;
                }
                Ui16 col=pos%D_WIDTH;
                if(length<=D_WIDTH && length>D_WIDTH-col)pos+=D_WIDTH-col;
            }
            if(pos>=D_CAP)return 0;
            if(ch<32 || ch>=127)ch='?';
            next.glyph[pos]=ch;next.attr[pos]=(Ui8)(a->face|(a->palette<<1));
            Ui16 delay=ch==' '?0:9;
            if(ch=='.' || ch=='!' || ch=='?')delay+=60;
            if(ch==',' || ch==';')delay+=30;
            if(ch!=' '){delay+=pending;pending=0;}
            next.delay[pos]=delay;
            Ui16 page=pos/D_PAGE;next.end[page]=(Ui16)(pos%D_PAGE+1);next.pages=page+1;
            boundary=ch==' ';pos++;
        }
    }
    if(!next.end[0])return 0;
    for(Ui16 i=0;i<next.pages;i++)if(!next.end[i])return 0;
    copy_layout(out,&next);return 1;
}
static void generation(Dialogue *s){s->generation++;if(s->generation>=65535u)s->generation=1;s->ready=0;s->age=0;}
static int enter(Dialogue *s,Ui16 node) {
    if(node>=8)return 0;
    DLayout layout;if(!d_compile(&layout,nodes[node].spans,nodes[node].count))return 0;
    copy_layout(&s->text,&layout);s->node=node;s->page=0;s->revealed=0;s->choice=0;s->cut=0;s->paused=0;s->mode=D_TALK;
    generation(s);return 1;
}
static int valid(Dialogue const *s) {
    if(!s || s->mode>D_OPTIONS || s->story>1 || !s->generation || s->generation==65535u || s->voice_on>1 || s->paused>1 || s->focus>2 || s->ready>1)return 0;
    if(s->mode==D_TALK) {
        if(s->node>=8 || !s->text.pages || s->text.pages>D_PAGES || s->page>=s->text.pages || s->choice>1)return 0;
        for(Ui16 i=0;i<s->text.pages;i++)if(!s->text.end[i] || s->text.end[i]>D_PAGE)return 0;
        if(s->revealed>s->text.end[s->page])return 0;
    }return 1;
}
void d_init(Dialogue *s) {
    Ui8 *p=(Ui8 *)s;for(Ui16 i=0;i<sizeof(*s);i++)p[i]=0;
    s->generation=1;s->voice_on=1;s->previous=D_END;
}
int d_start(Dialogue *s,Ui16 story) {
    if(!valid(s) || story>1)return 0;
    if(!enter(s,story?5:0))return 0;
    s->story=story;s->flags=0;s->previous=D_END;return 1;
}
void d_present(Dialogue *s,Ui16 id){if(valid(s) && s->mode==D_TALK && id==s->generation)s->ready=1;}
static Ui16 repeat(Dialogue *s,Ui16 pressed,Ui16 held,Ui16 elapsed) {
    Ui16 p=pressed&(UI_UP|UI_DOWN),h=held&(UI_UP|UI_DOWN);if(p==3)p=0;if(h==3)h=0;
    if(p){s->repeat_key=p;s->repeat_age=90;return p;}
    if(h && h==s->repeat_key){if(elapsed>=s->repeat_age){Ui16 extra=elapsed-s->repeat_age;s->repeat_age=extra>=24?24:(Ui16)(24-extra);return h;}s->repeat_age-=elapsed;}
    else {s->repeat_key=h;s->repeat_age=90;}
    return 0;
}
void d_step(Dialogue *s,Ui16 pressed,Ui16 held,Ui16 elapsed) {
    if(!valid(s))return;
    if(elapsed>300)elapsed=300;
    Ui16 key=repeat(s,pressed,held,elapsed);
    if(pressed&UI_C){s->voice_on^=1;s->cut=1;}
    if(s->mode==D_HUB) {
        if(key==UI_UP)s->focus=s->focus?(Ui16)(s->focus-1):2;
        if(key==UI_DOWN){s->focus++;if(s->focus==3)s->focus=0;}
        if(pressed&(UI_A|UI_START)){if(s->focus<2)d_start(s,s->focus);else{s->mode=D_OPTIONS;generation(s);}}
        return;
    }
    if(pressed&UI_B){s->mode=D_HUB;s->cut=1;s->paused=0;generation(s);return;}
    if(s->mode==D_OPTIONS){if(pressed&UI_A)s->voice_on^=1;return;}
    if(s->mode==D_DONE){if(pressed&(UI_A|UI_START)){s->mode=D_HUB;generation(s);}return;}
    if(pressed&UI_START){s->paused^=1;s->cut=1;return;}
    if(s->paused || !s->ready)return;
    s->clock+=elapsed;while(s->clock>=3000)s->clock-=3000;
    Ui16 end=s->text.end[s->page];
    if(pressed&UI_A) {
        if(s->revealed<end){s->revealed=end;s->age=0;s->cut=1;return;}
        if(s->page+1<s->text.pages){s->page++;s->revealed=0;s->cut=1;generation(s);return;}
        Ui16 next=nodes[s->node].next;
        if(s->node==1){next=s->choice?3:2;s->flags=(Ui16)(1u<<s->choice);}
        if(next==D_END){s->mode=D_DONE;s->cut=1;generation(s);return;}
        s->previous=s->node;enter(s,next);return;
    }
    if(s->revealed==end && s->page+1==s->text.pages && s->node==1 && key)s->choice^=1;
    s->age+=elapsed;
    for(Ui16 n=0;n<D_PAGE && s->revealed<end;n++) {
        Ui16 at=(Ui16)(s->page*D_PAGE+s->revealed),delay=s->text.delay[at];
        if(delay>s->age)break;
        s->age-=delay;s->revealed++;
    }
    if(s->revealed==end)s->age=0;
}
Ui16 d_voice(Dialogue const *s) {
    return valid(s) && s->mode==D_TALK && s->ready && s->voice_on && !s->cut && !s->paused && !s->page?nodes[s->node].voice:0;
}
static void text(UiCanvas *c,Ui16 x,Ui16 y,Ui16 width,char const *s){ui_text(c,x,y,width,0,1,s);}
static void big(UiCanvas *c,Ui16 x,Ui16 y,char const *s) {
    for(Ui16 i=0;s[i] && x+1<UI_COLS;i++,x+=2) {
        Ui16 n=(Ui16)(s[i]-'A');
        for(Ui16 dy=0;dy<2;dy++)for(Ui16 dx=0;dx<2;dx++)c->cells[(y+dy)*40+x+dx]=ui_tile((Ui16)(UI_HEADING+n*4+dy*2+dx),1);
    }
}
static void image(UiCanvas *c,Ui16 x,Ui16 y,Ui16 tile,Ui16 width,Ui16 height) {
    for(Ui16 dy=0;dy<height;dy++)for(Ui16 dx=0;dx<width;dx++)c->cells[(y+dy)*40+x+dx]=ui_tile((Ui16)(tile+dy*width+dx),3);
}
static void line(UiCanvas *c,DLayout const *layout,Ui16 page,Ui16 count,Ui16 y) {
    for(Ui16 i=0;i<count;i++) {
        Ui16 at=(Ui16)(page*D_PAGE+i);Ui8 ch=layout->glyph[at],a=layout->attr[at];
        if(!ch)continue;
        if(ch<32 || ch>=127)ch='?';
        c->cells[(y+i/D_WIDTH)*40+12+i%D_WIDTH]=ui_tile((Ui16)((a&1?UI_ALT:UI_FONT)+ch-32),(a>>1)&3);
    }
}
static void garden(UiCanvas *c) {
    ui_panel(c,(UiRect){4,2,32,11},1);
    for(Ui16 i=0;i<3;i++) {
        Ui16 x=(Ui16)(7+i*10);
        ui_panel(c,(UiRect){x,3,6,7},2);
        image(c,(Ui16)(x+2),6,(Ui16)(UI_ICONS+6*4),2,2);
        image(c,(Ui16)(x+2),10,(Ui16)(UI_ICONS+3*4),2,2);
    }
}
int d_draw(UiCanvas *c,Dialogue const *s,Ui16 hz) {
    if(!c || !valid(s) || (hz!=50 && hz!=60))return 0;
    for(Ui16 i=0;i<UI_CELLS;i++)c->cells[i]=0;
    text(c,3,1,26,"THE GLASS CONSERVATORY");
    text(c,31,1,8,s->voice_on?"VOICE ON":"NO VOICE");
    if(s->mode==D_HUB) {
        ui_panel(c,(UiRect){2,2,36,23},1);big(c,12,4,"DIALOGUE");big(c,17,7,"LAB");
        static char const * const options[]={"THE CLOCKWORK BLOOM","NOTES ACROSS THE ROOM","VOICE SETTINGS"};
        for(Ui16 i=0;i<3;i++) {
            ui_text(c,7,(Ui16)(12+i*3),29,0,s->focus==i?2:1,options[i]);
            if(s->focus==i)c->cells[(12+i*3)*40+5]=ui_tile(UI_PANEL+9,2);
        }
        text(c,6,22,30,"A SMALL STUDY IN CONVERSATION");
    } else if(s->mode==D_OPTIONS) {
        garden(c);ui_panel(c,(UiRect){3,8,34,15},1);
        text(c,6,10,28,"SHORT PCM VOICE EXCERPTS");text(c,6,13,28,s->voice_on?"ENABLED":"DISABLED");
        text(c,6,16,28,"A TOGGLE   B RETURN");text(c,6,19,28,"SUBTITLES ALWAYS STAY ON");
    } else if(s->mode==D_DONE) {
        garden(c);ui_panel(c,(UiRect){3,14,34,11},1);
        text(c,6,16,28,"A PAGE FOR THE NOTEBOOK");
        text(c,6,19,28,s->story?"TWO VOICES, ONE GARDEN.":s->flags==1?"THE CLOCK WAS WOUND.":"THE BLOOM WAS LEFT TO REST.");
        text(c,6,22,28,"A OR B RETURNS TO STORIES");
    } else {
        Node const *node=&nodes[s->node];
        if(node->paired && s->previous<8) {
            DLayout previous;
            if(!d_compile(&previous,nodes[s->previous].spans,nodes[s->previous].count))return 0;
            ui_panel(c,(UiRect){2,2,36,11},1);
            image(c,4,4,(Ui16)(UI_PORTRAIT+nodes[s->previous].speaker*108),6,6);
            text(c,12,3,22,nodes[s->previous].speaker?"IVO / ARCHIVIST":"MIRA / BOTANIST");
            line(c,&previous,0,previous.end[0],5);
        } else garden(c);
        ui_panel(c,(UiRect){2,15,36,10},1);
        Ui16 pose=s->clock>=2700 && s->clock<2860?2:!s->paused && s->ready && s->revealed<s->text.end[s->page]?(Ui16)((s->clock/45u)&1u):0;
        image(c,4,17,(Ui16)(UI_PORTRAIT+node->speaker*108+pose*36),6,6);
        text(c,12,16,22,node->speaker?"IVO / ARCHIVIST":"MIRA / BOTANIST");
        line(c,&s->text,s->page,s->revealed,18);
        if(s->revealed==s->text.end[s->page]) {
            if(s->node==1 && s->page+1==s->text.pages) {
                ui_text(c,13,22,21,0,s->choice?1:2,"WIND THE CLOCK");
                ui_text(c,13,23,21,0,s->choice?2:1,"LET IT REST");
                c->cells[(22+s->choice)*40+12]=ui_tile(UI_PANEL+9,2);
            } else c->cells[23*40+34]=ui_tile((Ui16)(UI_PANEL+9+((s->clock/90u)&1u)),2);
        }
        if(s->paused)text(c,8,14,25,"PAUSED / START RESUMES");
    }
    text(c,3,26,30,s->mode==D_HUB?"D-PAD CHOOSE A OPEN C VOICE":"A REVEAL/NEXT B STORIES");
    text(c,35,26,4,hz==50?"PAL":"NTSC");return 1;
}
