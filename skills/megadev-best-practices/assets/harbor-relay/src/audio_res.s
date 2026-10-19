.section .rodata
.balign 2
.global pcm_bed,pcm_bell,pcm_pluck,pcm_drum
pcm_bed: .incbin "res/bed.pcm"
pcm_bell: .incbin "res/bell.pcm"
pcm_pluck: .incbin "res/pluck.pcm"
pcm_drum: .incbin "res/drum.pcm"
