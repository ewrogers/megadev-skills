.section .rodata
.global res_actors,res_ground,res_ground_end,res_palettes,res_world,res_world_end,res_attrs
res_actors: .incbin "res/actors.bin"
res_ground: .incbin "res/ground.bin"
res_ground_end:
res_palettes: .incbin "res/palettes.bin"
res_attrs: .incbin "res/tileattrs.bin"
res_world: .incbin "res/world.bin"
res_world_end:
