#include "loc.h"
Lc16 lc_language(Lc16 language){return language<LC_LANGUAGES?language:0;}
/* Explicit zero/one/many wording for these four catalogs, not universal plurals. */
Lc16 lc_variant(Lc16 quantity){return !quantity?LC_M_ZERO:quantity==1?LC_M_ONE:LC_M_MANY;}
static Lc16 digits(Lc16 n,Lc8 *out){Lc8 reverse[5];Lc16 size=0;do{reverse[size++]=(Lc8)(17+n%10u);n/=10u;}while(n);for(Lc16 i=0;i<size;i++)out[i]=reverse[size-i-1];return size;}
Lc16 lc_format(Lc16 language,Lc16 message,Lc8 const *name,Lc16 name_length,Lc16 quantity,Lc8 *out,Lc16 capacity){
 if(message>=LC_MESSAGES || !out || !capacity || name_length>LC_NAME_MAX || (name_length && !name))return 0;
 for(Lc16 i=0;i<name_length;i++)if(!name[i] || name[i]>LC_GLYPHS)return 0;
 Lc16 index=(Lc16)(lc_language(language)*LC_MESSAGES+message),length=lc_lengths[index],n=0;
 Lc8 const *source=lc_blob+lc_offsets[index];Lc8 number[5];Lc16 number_length=digits(quantity,number);
 /* Size the complete substitution first; failure cannot partially alter out. */
 for(Lc16 i=0;i<length;i++){Lc8 c=source[i];n+=(c==253?name_length:c==254?number_length:1);if(n>=capacity || n>=LC_FORMAT_CAP)return 0;}
 n=0;
 for(Lc16 i=0;i<length;i++){
  Lc8 c=source[i];
  if(c==253){for(Lc16 k=0;k<name_length;k++)out[n++]=name[k];}
  else if(c==254){for(Lc16 k=0;k<number_length;k++)out[n++]=number[k];}
  else out[n++]=c;
 }
 out[n]=0;return (Lc16)(n+1);
}
Lc16 lc_wrap(Lc8 const *text,Lc16 length,Lc16 width,Lc16 lines,LcLayout *out){
 if(!text || !out || !width || width>LC_WIDTH || !lines || lines>LC_LINES || length>=LC_FORMAT_CAP)return 0;
 for(Lc16 i=0;i<length;i++)if(!text[i] || (text[i]>LC_GLYPHS && text[i]!=LC_NEWLINE))return 0;
 LcLayout temp;for(Lc16 i=0;i<sizeof(temp.cells);i++)temp.cells[i]=1;
 temp.pages=1;temp.width=width;temp.lines=lines;
 Lc16 page=0,row=0,col=0,pos=0;
 while(pos<length){
  if(text[pos]==LC_NEWLINE){pos++;col=width;}
  else if(text[pos]==1){pos++;if(col && col<width)col++;continue;}
  else{
   Lc16 word=0;while(pos+word<length && text[pos+word]!=1 && text[pos+word]!=LC_NEWLINE)word++;
   if(col && word>width-col)col=width;
   for(Lc16 k=0;k<word;k++){
    if(col==width){col=0;row++;if(row==lines){row=0;page++;}}
    if(page>=LC_MAX_PAGES)return 0;
    temp.cells[(page*LC_LINES+row)*LC_WIDTH+col++]=text[pos++];
    if(page>=temp.pages)temp.pages=page+1;
   }
   continue;
  }
  /* Explicit line breaks consume a row even at column zero, but don't create
     an empty trailing page until another glyph is actually written. */
  if(col==width){col=0;row++;if(row==lines){row=0;page++;}}
 }
 for(Lc16 i=0;i<sizeof(temp.cells);i++)out->cells[i]=temp.cells[i];
 out->pages=temp.pages;out->width=width;out->lines=lines;return temp.pages;
}
