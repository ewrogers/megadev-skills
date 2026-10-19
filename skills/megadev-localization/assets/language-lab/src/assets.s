.section .rodata
.balign 2
.global res_fonts, res_panels, res_icons, res_portrait, res_backdrop, res_palette
res_fonts: .incbin "res/fonts.bin"
res_panels: .incbin "res/panels.bin"
res_icons: .incbin "res/icons.bin"
res_portrait: .incbin "res/portrait.bin"
res_backdrop: .incbin "res/backdrop.bin"
res_palette: .incbin "res/palette.bin"
