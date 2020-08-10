.proc mem_init
    ;Clear memory memory map
    ldy #0
    lda #0

:   sta mem_map,y
    sta mem_map+256,y
    sta mem_map+512,y
    sta mem_map+768,y
    iny
    bne :-

    ;Pages in bank 0 used by Kernal, mark as allocated
    lda #255
    sta mem_map
    sta mem_map+1
    sta mem_map+2
    sta mem_map+3

    ;Allocate inital memory page
    lda #1
    sta mem_map+4

    ;Set zero page vectors
    lda #1
    sta CRS_BNK
    sta DSP_BNK

    lda #$a0
    sta CRS_ADR+1
    sta DSP_ADR+1

    stz CRS_ADR
    stz DSP_ADR

    stz CRS_IDX
    stz DSP_IDX

    ;Setup bank 1, page $a0
    ldy #0
    lda #0
    
    sta (CRS_ADR),y     ;Previous bank = 0
    
    ldy #1
    sta (CRS_ADR),y     ;Previous page = 0, i.e. null

    ldy #2
    sta (CRS_ADR),y     ;Next bank = 0

    ldy #3
    sta (CRS_ADR),y     ;Next page = 0, i.e. null

    ldy #4
    sta (CRS_ADR),y     ;Len = 0

    rts
.endproc

.proc mem_alloc
    lda #0
    sta bank
    sta page

    lda #<mem_map
    sta TMP1_ADR
    lda #>mem_map
    sta TMP1_ADR+1

    ldy #0

loop:
    lda (TMP1_ADR),y
    cmp #$ff
    bne free_mem_found
    
    iny
    bne loop

    inc TMP1_ADR+1
    lda TMP1_ADR+1
    sec
    sbc #>mem_map
    cmp #4
    bne loop

mem_full:
    stz $5010
    stz $5011
    rts

free_mem_found:
    sta value

    tya
    lsr
    lsr
    sta bank

    lda TMP1_ADR+1
    sec
    sbc #>mem_map
    asl
    asl
    asl
    asl
    asl
    asl
    clc
    adc bank
    sta bank

    tya
    asl
    asl
    asl
    and #$1f
    sta page

    ldx #0

:   lda value
    and bitmask,x
    beq finalize_bank_n_page
    inx
    jmp :-

finalize_bank_n_page:
    lda value
    ora bitmask,x
    sta (TMP1_ADR),y

    txa
    clc
    adc page
    adc #$a0
    sta page

    ldy bank
    ldx page

    rts

bank:
    .byt 0
page:
    .byt 0

value:
    .byt 0

bitmask:
    .byt 1, 2, 4, 8, 16, 32, 64, 128

.endproc

mem_map:
    .res 1024, 0