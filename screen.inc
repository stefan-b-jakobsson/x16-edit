.proc screen_println
    lda #0
    sta VERA_L
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ldy #4
    lda (DSP_ADR),y
    sta page_len

    lda #5
    sta TMP1_ADR
    lda DSP_ADR+1
    sta TMP1_ADR+1

    ldy DSP_IDX

    lda DSP_BNK
    sta TMP1_BNK
    sta BNK_SEL

    lda #79
    sta counter

print_loop:
    lda (TMP1_ADR),y
    sta VERA_D0

    dec counter
    beq clean_line

    iny
    cpy page_len
    bne print_loop

    stz TMP1_ADR
    ldy #2
    lda (TMP1_ADR),y
    pha
    ldy #3
    lda (TMP1_ADR),y
    beq clean_stack

    sta TMP1_ADR+1
    pla
    sta TMP1_BNK
    sta BNK_SEL

    ldy #4
    lda (TMP1_ADR),y
    sta page_len

    lda #5
    sta TMP1_ADR

    ldy #0

    jmp print_loop

clean_stack:
    pla

clean_line:
    lda #32

clean_line_loop:
    sta VERA_D0
    ldx counter
    beq exit
    dec counter
    jmp clean_line_loop
    
exit:
    rts

page_len:
    .byt 0

counter:
    .byt 0

.endproc