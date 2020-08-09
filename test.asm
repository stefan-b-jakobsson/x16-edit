.include "common.inc"

.proc test_mem_init
    ;Scramble first mem page content
    lda #1
    sta BNK_SEL

    lda #$a0
    sta CRS_ADR+1
    stz CRS_ADR

    ldy #0
:   lda #$ff
    sta (CRS_ADR),y
    iny
    cpy #5
    bne :-

    ;Scramble zero page vectors
    lda #$ff
    
    sta CRS_BNK
    sta CRS_ADR
    sta CRS_ADR+1
    sta CRS_IDX
    
    sta DSP_BNK
    sta DSP_ADR
    sta DSP_ADR+1
    sta DSP_IDX

    ;Init test output   
    ldy #0
    lda #$ff
:   sta $5000,y
    iny
    cpy #$0c
    bne :-

    ;Call procedure we're testing
    jsr mem_init

    ;Assert CRS_BNK = 1
    lda CRS_BNK
    cmp #1
    beq :+
    stz $5000

    ;Assert CRS_ADR = 0
:   lda CRS_ADR
    beq :+
    stz $5001

    ;Assert CRS_ADR+1 = $a0
:   lda CRS_ADR+1
    cmp #$a0
    beq :+
    stz $5002

    ;Assert CRS_IDX = 0
:   lda CRS_IDX
    beq :+
    stz $5003

    ;Assert DSP_BNK = 1
:   lda DSP_BNK
    cmp #1
    beq :+
    stz $5004

    ;Assert DSP_ADR = 0
:   lda DSP_ADR
    beq :+
    stz $5005

    ;Assert DSP_ADR+1 = $a0
:   lda DSP_ADR+1
    cmp #$a0
    beq :+
    stz $5006

    ;Asser DSP_IDX = 0
:   lda DSP_IDX
    beq :+
    stz $5007

    ;Assert mem_map to mem_map+3 = $ff
:   lda mem_map
    cmp #255
    beq :+
    stz $5008

:   lda mem_map+1
    cmp #255
    beq :+
    stz $5008

:   lda mem_map+2
    cmp #255
    beq :+
    stz $5008

:   lda mem_map+3
    cmp #255
    beq :+
    stz $5008

    ;Assert mem_map+4 = 1
:   lda mem_map+4
    cmp #1
    beq :+
    stz $5009

    ;Assert mem_map+5 to mem_map+1023 = 0
    ldy #0

    lda mem_map+5,y
    beq :+
    stz $500a
    
:   lda mem_map+256,y
    beq :+
    stz $500a
 
:   lda mem_map+512,y
    beq :+
    stz $500a

:   lda mem_map+768,y
    beq :+
    stz $500a

    ;Assert content of bank 1, page $a0, first 5 bytes should be 0
:   lda #1
    sta BNK_SEL

    ldy #0
    lda (CRS_ADR),y
    beq :+
    stz $500b

    ldy #1
    lda (CRS_ADR),y
    beq :+
    stz $500b

    ldy #2
    lda (CRS_ADR),y
    beq :+
    stz $500b

    ldy #3
    lda (CRS_ADR),y
    beq :+
    stz $500b

    ldy #4
    lda (CRS_ADR),y
    beq :+
    stz $500b

:   rts
.endproc

.proc test_mem_alloc

.endproc

.include "mem.inc"