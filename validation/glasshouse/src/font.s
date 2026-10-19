.section .rodata
.balign 2
.global res_font,res_actor,res_ui,res_palette
res_font: .incbin "res/font.bin"
.balign 2
res_actor: .incbin "res/actor.mda"
.balign 2
res_ui: .incbin "res/ui.bin"
.balign 2
res_palette: .incbin "res/ui.pal"
