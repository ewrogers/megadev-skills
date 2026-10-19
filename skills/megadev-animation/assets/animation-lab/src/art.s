.section .rodata
.align 2
.global res_actors,res_ground,res_palettes
res_actors: .incbin "res/actors.bin"
res_ground: .incbin "res/ground.bin"
res_palettes: .incbin "res/palettes.bin"
