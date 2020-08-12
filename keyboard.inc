.proc keyboard_read
    jsr KERNAL_GETIN
    bne found
    rts

found:
    jsr mem_insert
    
    inc CRS_X
    lda CRS_X
    cmp #80
    bcs eol
    rts

eol:
    dec CRS_X
    jsr mem_dsp_step_right

    rts

.endproc