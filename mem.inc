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

mem_map:
    .res 1024, 0