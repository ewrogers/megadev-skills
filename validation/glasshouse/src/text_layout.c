#include "text_layout.h"
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
