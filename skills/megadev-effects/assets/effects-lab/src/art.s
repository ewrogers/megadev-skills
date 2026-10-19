.section .rodata
.global res_actors,res_ground,res_map,res_palettes,res_ground_end
res_actors: .incbin "res/actors.bin"
res_ground: .incbin "res/ground.bin"
res_ground_end:
res_map: .incbin "res/map.bin"
res_palettes: .incbin "res/palettes.bin"
