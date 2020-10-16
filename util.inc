;******************************************************************************
;Copyright 2020, Stefan Jakobsson.
;
;This file is part of X16 Edit.
;
;X16 Edit is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.
;
;X16 Edit is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.
;
;You should have received a copy of the GNU General Public License
;along with X16 Edit.  If not, see <https://www.gnu.org/licenses/>.
;******************************************************************************

;******************************************************************************
;Function name.......: util_bin_to_bcd
;Purpose.............: Converts a binary value to a BCD value
;Input...............: 24 bit binary value, X=low, Y=mid, A=high
;Returns.............: Pointer to 32 bit BCD value, X=addressL, Y=addressH
;Error returns.......: None
;Credits.............: This routine is based on Andrew Jacobs code as published
;                      on http://www.6502.org/source/integers/hex2dec-more.htm
.proc util_bin_to_bcd
    ;Store 24 bit input
    stx input
    sty input+1
    sta input+2

    ;Clear 32 bit output
    stz output
    stz output+1
    stz output+2
    stz output+3

    ;Number of input bits
    ldx #24

    ;Set decimal mode
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

    ;Return pointer to output
    ldx #<output
    ldy #>output
 
    rts
    
input:
    .byt 0,0,0
output:
    .byt 0,0,0,0

.endproc

;******************************************************************************
;Function name.......: util_bcd_to_bin
;Purpose.............: Converts a BCD value to a binary value
;Input...............: Pointer to 32 bit BCD value, X=AddressL, Y=AddressH
;Returns.............: 24 bit binary value, X=low, Y=mid, A=high
;Error returns.......: None
;Credits.............: This routine is based on an article by Marvin L. De Jong
;                      as published here: https://www.atarimagazines.com/compute/issue17/221_1_A_GENERAL_PURPOSE_BCD-TO-BINARY_ROUTINE.php
;                      Mr. De Jong was in his turn inspired by the book 
;                      Microcomputer-Based Design, by John B. Peatman, 
;                      McGraw Hill, New York, 1977, p 400.
.proc util_bcd_to_bin
    ;Copy input
    stx TMP1_ADR
    sty TMP1_ADR+1
    ldy #0

:   lda (TMP1_ADR),y
    sta bcd_val,y
    iny
    cpy #5
    bne :-

    lda bcd_val
    
    ;Prepare
    ldy #32         ;Bit count
    ldx #3          ;Byte index

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
    ldx #3

    ;Get C from stack, and rotate to binary output
    plp
    ror bin_val+3
    ror bin_val+2
    ror bin_val+1
    ror bin_val
    
    ;Prepare stack for next loop
    clc
    php
    
    dey
    bne loop

    ;Clean stack
    plp

    ;Fetch result and exit
    ldx bin_val
    ldy bin_val+1
    lda bin_val+2    

    rts

    bcd_val:
        .byt 0,0,0,0
    bin_val:
        .byt 0,0,0,0
.endproc

;******************************************************************************
;Function name.......: util_bcd_to_str
;Purpose.............: Converts a BCD value to a null terminated string
;Input...............: Pointer to 32 bit BCD value, X=AddressL, Y=AddressH
;Returns.............: Pointer to string, X=AddressL, Y=AddressH
;Error returns.......: None
.proc util_bcd_to_str
    ;Copy input
    stx TMP1_ADR
    sty TMP1_ADR+1
    ldy #0

:   lda (TMP1_ADR),y
    sta input,y
    iny
    cpy #5
    bne :-
    

    ldx #4
    ldy #0

build_str:
    lda input-1,x
    lsr
    lsr
    lsr
    lsr
    clc
    adc #48
    sta output,y
    iny

    lda input-1,x
    and #15
    clc
    adc #48
    sta output,y
    iny

    dex
    bne build_str

strip_leading_zeros:
    ldx #0
:   lda output,x
    cmp #48
    bne exit
    inx
    cpx #7
    bcc :-

exit:
    txa
    clc
    adc #<output
    tax
    lda #0
    adc #>output
    tay
 
    rts
    
input:
    .byt 0,0,0,0
output:
    .byt 0,0,0,0,0,0,0,0,0
.endproc

;******************************************************************************
;Function name.......: util_str_to_bcd
;Purpose.............: Converts string to bcd value
;Preparatory routines: Nothing
;Input...............: Pointer to null terminated string, X=AddressL, Y=AddressH
;                      Input beyond 8 digits will be ignored
;Returns.............: Pointer to 32 bit BCD value, X=AddressL, Y=AddressH
;Error returns.......: C=1 if invalid input, else C=0
.proc util_str_to_bcd
    stx TMP1_ADR
    sty TMP1_ADR+1

    stz bcd_val
    stz bcd_val+1
    stz bcd_val+2
    stz bcd_val+3

;Goto end of string
    ldy #0
:   lda (TMP1_ADR),y
    beq prepare
    iny
    jmp :-

prepare:
    dey
    ldx #0

loop:
    lda (TMP1_ADR),y
    sec
    sbc #48
    bcc invalid
    cmp #10
    bcs invalid
    sta bcd_val,x
    
    dey
    cpy #$ff
    beq end

    lda (TMP1_ADR),y
    sec
    sbc #48
    bcc invalid
    cmp #10
    bcs invalid
    asl
    asl
    asl
    asl
    ora bcd_val,x
    sta bcd_val,x
    
    inx
    dey
    cpy #$ff
    bne loop

end:
    clc
    ldx #<bcd_val
    ldy #>bcd_val
    rts

invalid:
    sec
    ldx #<bcd_val
    ldy #>bcd_val
    rts

len:
    .byt 0

bcd_val:
    .byt 0,0,0,0
    
.endproc