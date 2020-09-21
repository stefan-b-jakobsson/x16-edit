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

;******************************************************************************
;Function name.......: util_convert_to_binary
;Purpose.............: Converts a 24 bit BCD value to a 24 bit binary value
;Input...............: 24 bit BCD value low=X, mid=Y, high=A
;Returns.............: 24 bit binary value X=low, Y=mid, A=high
;Error returns.......: None
;Credits.............: This routine is based on an article by Marvin L. De Jong
;                      as published here: https://www.atarimagazines.com/compute/issue17/221_1_A_GENERAL_PURPOSE_BCD-TO-BINARY_ROUTINE.php
;                      Mr. De Jong was in his turn inspired by the book 
;                      Microcomputer-Based Design, by John B. Peatman, 
;                      McGraw Hill, New York, 1977, p 400.
.proc util_convert_to_binary
    stx bcd_val
    sty bcd_val+1
    sta bcd_val+2

    ldy #24         ;Bit count
    ldx #2          ;Byte index

    clc             ;Clear carry and push to stack, 
    php

loop:
    plp
    ror bcd_val,x
    php

    ;Adjust high nibble: If bit 7=1 subtract $30
    bit bcd_val,x
    bpl :+
    sec
    lda bcd_val,x
    sbc #$30
    sta bcd_val,x

    ;Adjust low nibble: If bit 3=1 subtract $3
:   lda bcd_val,x
    and #8
    beq next_byte
    sec
    lda bcd_val,x
    sbc #3
    sta bcd_val,x

next_byte:
    dex
    cpx #$ff
    bne loop

    ;Reset X for next loop
    ldx #2

    ;Get C from stack, and rotate to binary output
    plp
    ror bin_val+2
    ror bin_val+1
    ror bin_val

    dey
    
    ;Prepare stack for next loop
    clc
    php
    
    bne loop

    ;Clean stack
    plp

    ;Fetch result for our beloved users
    ldx bin_val
    ldy bin_val+1
    lda bin_val+2    

    rts

    bcd_val:
        .byt 0,0,0
    bin_val:
        .byt 0,0,0
.endproc

