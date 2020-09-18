;******************************************************************************
;Function name.......: util_convert_to_decimal
;Purpose.............: Converts a 24 bit value to decimal
;Preparatory routines: Set VERA_L, VERA_M and VERA_H
;Input...............: 24 bit value low=X, mid=Y, high=A
;Returns.............: Pointer to output, X=addressL, Y=addressH
;Error returns.......: None
;Credits.............: This routine is based on Andrew Jacobs code as published
;                      on http://www.6502.org/source/integers/hex2dec-more.htm
.proc util_convert_to_decimal
    ;Store input
    stx input
    sty input+1
    sta input+2

    ;Clear output
    stz output
    stz output+1
    stz output+2
    stz output+3

    ;Number of bits
    ldx #24

    ;Decimal mode
    sed

loop:
    asl input
    rol input+1
    rol input+2
    
    lda output
    adc output
    sta output
    
    lda output+1
    adc output+1
    sta output+1

    lda output+2
    adc output+2
    sta output+2

    lda output+3
    adc output+3
    sta output+3

    dex
    bne loop

    ;Finished, set binary mode
    cld

    ;Build null terminated string, strip leading zeros
    ldx #4
    ldy #0

build_str:
    lda output-1,x
    lsr
    lsr
    lsr
    lsr
    clc
    adc #48
    sta output_str,y
    iny

    lda output-1,x
    and #15
    clc
    adc #48
    sta output_str,y
    iny

    dex
    bne build_str

strip_leading_zeros:
    ldx #0
:   lda output_str,x
    cmp #48
    bne exit
    inx
    cpx #8
    bcc :-

exit:
    txa
    clc
    adc #<output_str
    tax
    lda #0
    adc #>output_str
    tay
 
    rts
    
input:
    .byt 0,0,0
output:
    .byt 0,0,0,0
output_str:
    .byt 0,0,0,0,0,0,0,0,0

.endproc