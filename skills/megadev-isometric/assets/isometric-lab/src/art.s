.section .rodata
.align 2
.global res_actors,res_actors_end,res_ground,res_ground_end,res_palettes
res_actors: .incbin "res/actors.rle"
res_actors_end:
res_ground: .incbin "res/ground.rle"
res_ground_end:
.align 2
res_palettes: .incbin "res/palettes.bin"
.global res_map
res_map: .incbin "res/map.rle"
.global res_map_end
res_map_end:
