.section .rodata
.balign 2
.global res_font,res_actor
res_font: .incbin "res/font.bin"
.balign 2
res_actor: .incbin "res/actor.mda"
.balign 2
.global res_beacon
res_beacon: .incbin "res/beacon.bin"
