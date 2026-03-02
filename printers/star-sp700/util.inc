;******************************************************************************
;Function name.......: util_str_to_bin
;Purpose.............: Converts integer string to binary value
;Input...............: Pointer to null terminated string at VARZP/VARZP+1
;Returns.............: 8 bit value in A
;Error returns.......: C=1 if invalid input, else C=0
.proc util_str_to_bin
    ; Search string end
    ldy #0
:   lda (VARZP),y
    beq :+
    iny
    bra :-

:   ; Empty string error
    cpy #0
    bne :+
    jmp err

    ; Prepare
:   stz result
    ldx #0
loop:
    ; Get char
    dey
    lda (VARZP),y
    
    ; Convert char to bin
    sec
    sbc #$30
    bcc err
    cmp #10
    bcs err
    sta val

    ; Multiply by 10^pos
    phx ; backup X

mult10:
    cpx #0
    beq next

    asl val
    bcs err2
    
    lda val
    asl
    bcs err2

    asl
    bcs err2
    
    adc val
    sta val
    bcs err2
    
    dex
    bra mult10
    
next:
    ; Update result
    clc
    lda result
    adc val
    sta result
    bcs err2
    
    ; Prepare for next char
    plx
    inx
    cpy #0
    bne loop

exit:
    clc
    lda result
    rts

err2:
    plx

err:
    sec
    rts

val: .res 1
result: .res 1
.endproc
