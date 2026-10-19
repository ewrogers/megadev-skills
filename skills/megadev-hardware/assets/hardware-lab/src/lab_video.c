/* Resident native VDP renderer. Static redraws blank the display; dynamic
 * text uses one <=40-word transfer after the existing BIOS VBlank wait.
 * No framebuffer, DMA queue, new interrupt owner, or extra frame clock. */
#include "lab.h"
#include <main/bios.h>
#include <main/vdp.h>
#include "../build/art.h"
static u16 drawing;
static u16 lock(void) {
    u16 sr;__asm__ volatile("move.w %%sr,%0\n move.w #0x2700,%%sr":"=d"(sr)::"memory");return sr;
}
static void unlock(u16 sr) {__asm__ volatile("move.w %0,%%sr"::"d"(sr):"memory");}
static void reg(u16 index,u16 value) {bios_vdp_regs[index]=0x8000|(index<<8)|value;vdp_ctrl_16=bios_vdp_regs[index];}
static void address(u16 base,u16 x,u16 y) {vdp_ctrl_32=to_vdp_addr(base+y*128+x*2)|VRAM_W;}
void lab_video_init(void) {
    lab_display_off();u16 sr=lock();
    reg(2,0xc000>>10);reg(4,0xe000>>13);reg(5,0xf800>>9);
    reg(7,0);reg(11,0);reg(13,0xfc00>>10);reg(15,2);reg(16,1);reg(17,0);reg(18,0);
    static const u16 colors[16]={0x0200,0x0eee,0x0ee4,0x0a86,0x06e6,0x06af,0x0422,0x0642};
    for(u16 p=0;p<4;p++)for(u16 i=0;i<16;i++)bios_palette[p*16+i]=colors[i];
    bios_palette[17]=0x0ee4;bios_palette[33]=0x06e6;bios_palette[49]=0x0a86;
    /* Direct initialization plus BIOS mirror keep later BIOS updates coherent. */
    vdp_ctrl_32=0xc0000000;for(u16 i=0;i<64;i++)vdp_data_16=bios_palette[i];
    vdp_ctrl_32=to_vdp_addr(0)|VRAM_W;
    for(u16 i=0;i<sizeof(lab_tiles)/2;i++)vdp_data_16=lab_tiles[i];
    address(0xf800,0,0);for(u16 i=0;i<320;i++)vdp_data_16=0;
    address(0xfc00,0,0);vdp_data_16=0;vdp_data_16=0;
    vdp_ctrl_32=0x40000010;vdp_data_16=0;vdp_data_16=0;
    unlock(sr);
}
void lab_text(char const *s,u16 x,u16 y,u16 palette) {
    if(x>=40 || y>=28)return;
    u16 tiles[40],n=0;
    while(s[n] && n<40-x) {u8 c=(u8)s[n];tiles[n]=(palette<<13)+((c>=32 && c<128)?c-31:32);n++;}
    if(!drawing)lab_wait();
    u16 sr=lock();address(0xc000,x,y);for(u16 i=0;i<n;i++)vdp_data_16=tiles[i];unlock(sr);
}
void lab_line(char const *s,u16 x,u16 y) {lab_text(s,x,y,0);}
void lab_hex(u16 value,u16 x,u16 y) {
    static char const h[]="0123456789ABCDEF";char text[5];
    for(u16 i=0;i<4;i++)text[i]=h[(value>>(12-i*4))&15];
    text[4]=0;lab_line(text,x,y);
}
void lab_screen(char const *title) {
    lab_display_off();drawing=1;u16 sr=lock();
    for(u16 y=0;y<32;y++) {
        address(0xc000,0,y);for(u16 x=0;x<64;x++)vdp_data_16=0;
        address(0xe000,0,y);
        for(u16 x=0;x<64;x++)vdp_data_16=(x>=2&&x<38&&y>=6&&y<25)?(y==6?98:(x==2?99:97)):0;
    }
    for(u16 y=0;y<6;y++){address(0xc000,30,y+8);for(u16 x=0;x<6;x++)vdp_data_16=100+y*6+x;}
    unlock(sr);
    lab_text("MEGADEV / CD",3,1,1);lab_text(LAB_BUILD_ID,30,1,3);
    sr=lock();
    for(u16 y=0;y<2;y++) {
        address(0xc000,3,3+y);
        for(u16 n=0;title[n] && n<17;n++) {
            u16 c=title[n]>='A' && title[n]<='Z'?title[n]-'A':26;
            vdp_data_16=136+c*4+y*2;vdp_data_16=137+c*4+y*2;
        }
    }
    unlock(sr);
}
void lab_present(void) {drawing=0;lab_display_on();}
void lab_heartbeat(u16 tick) {
    lab_text((tick&32)?"* RUNNING":"  RUNNING",28,26,2);
}
