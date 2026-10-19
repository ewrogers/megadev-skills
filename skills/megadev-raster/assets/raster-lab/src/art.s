.section .rodata
.global res_actors,res_ground,res_ground_end,res_map,res_reflection,res_palettes
res_actors: .incbin "res/actors.bin"
res_ground:
#ifdef RASTER_SPLIT
.incbin "res/split_ground.bin"
#else
.incbin "res/water_ground.bin"
#endif
res_ground_end:
res_map:
#ifdef RASTER_SPLIT
.incbin "res/split_map.bin"
#else
.incbin "res/water_map.bin"
#endif
res_reflection:
#ifndef RASTER_SPLIT
.incbin "res/reflection_map.bin"
#endif
res_palettes:
#ifdef RASTER_SPLIT
.incbin "res/split_palettes.bin"
#else
.incbin "res/water_palettes.bin"
#endif
