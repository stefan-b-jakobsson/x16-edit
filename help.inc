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
    ;Backup r0 and r1
    lda r0
    pha
    lda r0+1
    pha
    lda r1
    pha
    lda r1+1
    pha

    ;Select first RAM bank used by the program
    lda mem_start
    sta BNK_SEL

    ;Vector to compressed text (input)
    lda #<help_bin
    sta r0
    lda #>help_bin
    sta r0+1

    ;Calculate end of copy
    sec
    lda #<help_bin_end
    sbc #<help_bin
    sta endL
    lda #>help_bin_end
    sbc #0
    sta endH

    ;Copy compressed text to clipboard mem, clipboard mem used temporarily for this purpose during editor initialization
    lda #<clipboard_mem
    sta r1
    lda #>clipboard_mem
    sta r1+1

    ldy #0

copy:   
    lda r0+1
    cmp endH
    bne :+
    cpy endL
    beq decompress

:   lda (r0),y
    sta (r1),y
    iny
    bne copy
    inc r0+1
    inc r1+1
    bra copy

decompress:  
    ;Vector to compressed text (input)
    lda #<clipboard_mem
    sta r0
    lda #>clipboard_mem
    sta r0+1

    ;Vector to decompressed text (output)
    lda #<help_txt
    sta r1
    lda #>help_txt
    sta r1+1

    ;Call Kernal decompress function
    bridge_setaddr KERNAL_DECOMPRESS
    bridge_call KERNAL_DECOMPRESS
    
    ;Add trailing zero to mark end of text
    lda #0
    sta (r1)

    ;Restore r0 and r1
    pla
    sta r1+1
    pla
    sta r1
    pla
    sta r0+1
    pla
    sta r0

    rts

help_bin:
    .incbin "help.bin"
help_bin_end:

endL = tempvars
endH = tempvars+1

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
    lda #2+VERA_BUFADR_M
    sta VERA_M
    lda #(2<<4 | VERA_BUFADR_H)
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
    clc
    lda #VERA_BUFADR_M
    adc #SCREEN_HEIGHT
    sec
    sbc #5
    cmp VERA_M
    bcc exit

next:
    iny
    bne loop
    inc TMP1_ADR+1
    bra loop

exit:
    rts

.endproc