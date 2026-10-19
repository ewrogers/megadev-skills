.section .rodata
.balign 2
.global res_font,res_palette,res_poster
res_font: .incbin "res/font.bin"
res_palette: .incbin "res/palette.bin"
res_poster: .incbin "res/poster.bin"
