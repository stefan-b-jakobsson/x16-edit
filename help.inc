;******************************************************************************
;Function name.......: help_decompress
;Purpose.............: Decompresses the lzsa compressed help text.
;                      The compressed binary should be created with the lzsa
;                      utility that you may get here:
;                      https://github.com/emmanuel-marty/lzsa
;                      Compress with the following command:
;                      lzsa -r -f2 help.txt helptxt.bin
;                      The decompressed text is stored in banked RAM.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc help_decompress
    lda mem_start       ;Select first RAM bank used by the program
    sta BNK_SEL

    lda #<help_bin      ;Vector to compressed text (input)
    sta r0
    lda #>help_bin
    sta r0+1

    lda #<help_txt      ;Vector to decompressed text (output)
    sta r1
    lda #>help_txt
    sta r1+1

    jmp KERNAL_DECOMPRESS

help_bin: 
    .incbin "helptxt.bin"

.endproc

;******************************************************************************
;Function name.......: help_show
;Purpose.............: Displays help screen
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc help_show
    ;Clear editable part of screen
    jsr screen_clear_editor

    ;Set VERA address to start printing from start of row 2
    stz VERA_L
    lda #2
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ;Set zero page vector to help screen content
    lda mem_start
    sta BNK_SEL

    lda #<help_txt
    sta TMP1_ADR
    lda #>help_txt
    sta TMP1_ADR+1

    ldy #0
loop:
    ;Get next char
    lda (TMP1_ADR),y
    beq exit

    cmp #LF
    beq :+
    jsr screen_put_uc_char
    bra next

    ;Line feed
:   stz VERA_L
    inc VERA_M

next:
    iny
    bne loop
    inc TMP1_ADR+1
    bra loop

exit:
    rts

.endproc