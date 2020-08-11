;**********************************************************
;Procedure name......: mem_init
;Description.........: Initializes banked RAM
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
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

    ;All pages in bank 0 used by Kernal, mark as allocated
    lda #255
    sta mem_map
    sta mem_map+1
    sta mem_map+2
    sta mem_map+3

    ;Allocate the app's initial memory page (bank 1, page $a0)
    lda #1
    sta mem_map+4

    ;Set zero page vectors
    lda #1
    sta CRS_BNK     ;Memory pointer corresponding to the cursor
    sta DSP_BNK

    lda #$a0
    sta CRS_ADR+1   ;Memory pointer to first visible characted in currently edited line
    sta DSP_ADR+1

    stz CRS_ADR     ;LSB = 0
    stz DSP_ADR

    stz CRS_IDX     ;Memory offset values, initially 0
    stz DSP_IDX

    ;Setup initial memory page (bank 1, page $a0)
    lda #0
    
    ldy #0
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

;**********************************************************
;Procedure name......: mem_alloc
;Description.........: Allocates one page in banked RAM
;Preparatory routines: mem_init (once)
;Input...............: Nothing
;Returns.............: .Y = allocated bank
;                      .X allocated 
;Error returns.......: .Y = 0 and .X = 0 if memory full
.proc mem_alloc
    ;Get vector to memory map
    lda #<mem_map
    sta TMP1_ADR
    lda #>mem_map
    sta TMP1_ADR+1

    ldy #0
loop:
    ;Loop through memory map searching for free memory
    lda (TMP1_ADR),y
    cmp #$ff                    ;If not $ff there is at least one page of free mem
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
    ;If we reach this point there is no free memory, return bank = 0 and page = 0
    ldy #0
    ldx #0
    rts

free_mem_found:
    ;OK, we found free memory, but the result needs to be calculated

    sta value           ;Store the value of the byte in mem_map where we found free memory

    tya                 ;Step 1. Divide the LSB of the number of search loops (= value of .Y) by 4 and we have the first part of the bank
    lsr
    lsr
    sta bank

    lda TMP1_ADR+1      ;Step 2. Multiply the MSB of the number of search loops (>TMP1_ADR minus >mem_map) by 64, and add that to the bank
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

    tya                 ;Step 3. Multiply the LSB of the number of search loops (= value of .Y) by 8 and we have the first part of the page
    asl
    asl
    asl
    and #$1f            ;Step 4. And that value by $1f to keep within page boundary (00-1f)
    sta page

    ldx #0

:   lda value           ;Step 5. The second part of the page is found in the mem_map value stored earlier. Add the positional value of the first bit not set to the page
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
    adc #$a0            ;Step 6. Finally add start of banked ram ($a0) to the page. And we're done.
    sta page

    ldy bank            ;Return .Y=bank and .X=page
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

;**********************************************************
;Procedure name......: mem_push
;Description.........: Pushes content of a mem page
;                      one step forward
;Preparatory routines: mem_init (once)
;Input...............: .Y = mem bank, .X = mem page, .A = index to start pushing from
;Returns.............: .C = 1 if overflow, otherwise .C = 0. Overflow character returned in .A
;Error returns.......: Nothing
.proc mem_push
    sta start_push

    ;Get page len
    sty BNK_SEL
    sty TMP1_BNK

    stx TMP1_ADR+1
    stz TMP1_ADR

    ldy #4
    lda (TMP1_ADR),y
    sta page_len

    ;Save last char, should we need to return an overflow
    ldy #255
    lda (TMP1_ADR),y
    sta last_char

    ;If start push index >=250 (i.e. last index in mem page), we just return the last char as overflow
    lda start_push
    cmp #250
    bcc push

    lda last_char
    sec

    rts

    ;Start push index < 250, let's push it
push:
    lda #5
    sta TMP1_ADR

    ldy #249

loop:
    lda (TMP1_ADR),y
    iny
    sta (TMP1_ADR),y
    dey
    cpy start_push
    beq inc_len
    dey
    jmp loop

inc_len:
    stz TMP1_ADR

    lda page_len
    cmp #251
    bcc no_overflow

    ;Page len is max, return overflow
    lda last_char
    sec
    
    rts

no_overflow:
    inc page_len

    lda page_len
    ldy #4
    sta (TMP1_ADR),y

    clc

    rts

start_push:
    .byt 0

page_len:
    .byt 0

last_char:
    .byt 0

.endproc

mem_map:
    .res 1024, 0