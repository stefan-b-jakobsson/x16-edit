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

    ;Now we need to initialize the mem page
    ;The new mem page will be inserted after the page pointed to by CRS_ADR+1
    lda bank
    sta TMP1_BNK
    sta BNK_SEL
    lda page
    sta TMP1_ADR+1
    stz TMP1_ADR

    ldy #0              ;new.prv_bnk = crs.bnk
    lda CRS_BNK
    sta (TMP1_ADR),y

    ldy #1
    lda CRS_ADR+1
    sta (TMP1_ADR),y    ;new.prv_pag = crs.pag

    ldy #2
    lda CRS_BNK
    sta BNK_SEL
    lda (CRS_ADR),y
    pha
    ldy #3
    lda (CRS_ADR),y

    ldx TMP1_BNK
    stx BNK_SEL
    sta (TMP1_ADR),y    ;new.nxt_pag = crs.nxt_pag
    pla
    ldy #2
    sta (TMP1_ADR),y    ;new.nxt_bnk = crs.nxt_bnk

    ldy #4
    lda #0
    sta (TMP1_ADR),y    ;new.len = 0

    lda CRS_BNK
    sta BNK_SEL
    ldy #2
    lda TMP1_BNK
    sta (CRS_ADR),y     ;crs.next_bnk = new.bnk

    ldy #3
    lda TMP1_ADR+1
    ldy #3
    sta (CRS_ADR),y     ;crs.nxt_pag = new.pag

    lda TMP1_BNK
    sta BNK_SEL
    
    ldy #3
    lda (TMP1_ADR),y
    sta TMP2_ADR+1
    stz TMP2_ADR
    
    ldy #2
    lda (TMP1_ADR),y
    sta TMP2_BNK
    sta BNK_SEL

    ldy #0
    lda TMP1_BNK
    sta (TMP2_ADR),y
    ldy #1
    lda TMP1_ADR+1
    sta (TMP2_ADR),y

    ;Return bank and page allocated
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

    ;If start push index >=250 (i.e. last index in mem page)
    lda start_push
    cmp #250
    bcc push

    jmp inc_len

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

;**********************************************************
;Procedure name......: mem_crs_step_right
;Description.........: Moves the pointer to memory location
;                      of the cursor one step right
;Preparatory routines: mem_init (once)
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_crs_step_right
    lda CRS_BNK
    sta BNK_SEL

    ldy #3
    lda (CRS_ADR),y
    sta next_page       ;Get next page and store for further processing
    
    lda CRS_IDX
    cmp #250
    bcs goto_next_page  ;Goto next page if crs_idx >= 250 (i.e. last valid position)

    lda next_page
    beq at_last_page

    ;We're not at last mem page, goto next page if crs_idx >= page len, otherwise increase crs_idx
    ldy #4
    lda CRS_IDX
    cmp (CRS_ADR),y
    bcs goto_next_page

    inc CRS_IDX

    rts

at_last_page:
    ;We're at last mem page, increase crs_idx, go one beyond last char position if we are at end of file
    ldy #4
    lda CRS_IDX
    cmp (CRS_ADR),y
    bcs :+

    inc CRS_IDX

    rts

:   lda (CRS_ADR),y
    sta CRS_IDX
    
    rts

goto_next_page:
    ldy #2
    lda (CRS_ADR),y
    pha
    ldy #3
    lda (CRS_ADR),y
    beq eof

    sta CRS_ADR+1
    pla
    sta CRS_BNK
    sta BNK_SEL
    
    stz CRS_IDX

    rts

eof:
    pla     ;Clean stack

    jsr mem_alloc
    jmp goto_next_page

    rts

next_page:
    .byt 0

.endproc

;**********************************************************
;Procedure name......: mem_dsp_step_right
;Description.........: Moves the pointer to memory location
;                      wherefrom currently edited line is
;                      visible
;Preparatory routines: mem_init (once)
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_dsp_step_right
    lda DSP_BNK
    sta BNK_SEL

    ldy #4
    lda DSP_IDX
    cmp (DSP_ADR),y     ;Page len
    bcs goto_next_page

    cmp #250
    bcs goto_next_page

    inc DSP_IDX

    rts

goto_next_page:
    ldy #2
    lda (DSP_ADR),y
    pha
    ldy #3
    lda (DSP_ADR),y
    beq eof

    sta DSP_ADR+1
    pla
    sta DSP_BNK
    sta BNK_SEL
    
    stz DSP_IDX

    rts

eof:
    ;We're at end of file, just return
    pla     ;Clean stack

    rts
.endproc

;**********************************************************
;Procedure name......: mem_insert
;Description.........: Inserts a char at cursor location
;Preparatory routines: mem_init (once)
;Input...............: .A = char to insert
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_insert
    pha

    ldy CRS_BNK
    ldx CRS_ADR+1
    lda CRS_IDX
    
    jsr mem_push
    bcc store_char

    ;Handle overflow
    pha

    lda CRS_BNK
    sta BNK_SEL

    ldy #2
    lda (CRS_ADR),y     ;Next bnk
    pha
    ldy #3
    lda (CRS_ADR),y     ;Next page
    beq alloc_next      ;Next page not allocated, allocate it now
    
    sta TMP1_ADR+1
    stz TMP1_ADR
    
    pla
    sta TMP1_BNK
    sta BNK_SEL

    ldy #4
    lda (TMP1_ADR),y    ;Get next page len
    cmp #251
    bcs alloc_next      ;It's full, allocate a new page

    ldy TMP1_BNK        ;It's not full, push it
    ldx TMP1_ADR+1
    lda #0
    jsr mem_push 

    ldy #5              ;Store overflow char at beginning of next page
    pla
    sta (TMP1_ADR),y

    jmp store_char

alloc_next:
    jsr mem_alloc       ;Next page was not allocated or was full, we're allocating a new page

    sty BNK_SEL
    stx TMP1_ADR+1
    stz TMP1_ADR

    pla
    ldy #5
    sta (TMP1_ADR),y    ;Store overflow char at beginning of the new page

    ldy #4
    lda #1
    sta (TMP1_ADR),y    ;Set new page len = 1

store_char:    
    lda CRS_BNK         ;And now, store new char
    sta BNK_SEL

    lda #5
    sta CRS_ADR
    pla
    ldy CRS_IDX
    sta (CRS_ADR),y
    stz CRS_ADR

    jsr mem_crs_step_right

    rts

.endproc

mem_map:
    .res 1024, 0