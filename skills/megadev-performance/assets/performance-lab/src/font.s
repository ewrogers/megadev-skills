.section .rodata
.balign 2
.global res_font
res_font: .incbin "res/font.bin"

.balign 2
.global scale_palette
scale_palette:
.incbin "res/palette.bin"
