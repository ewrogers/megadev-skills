.section .rodata
.balign 2
.global pcm_score,pcm_chime
pcm_score: .incbin "res/score.pcm"
pcm_chime: .incbin "res/chime.pcm"
