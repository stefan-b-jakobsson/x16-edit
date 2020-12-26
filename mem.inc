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

;**********************************************************
;Function name.......: mem_init
;Description.........: Initializes program's memory
;                      usage, and allocates the first
;                      page of banked RAM.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_init
    ;Get mem top
    bridge_setaddr KERNAL_MEMTOP
    sec
    bridge_call KERNAL_MEMTOP
    sec
    sbc #1
    cmp mem_top
    bcs :+
    sta mem_top

    ;Mark pages in banks 0 to mem_start as allocated (not free)
:   lda mem_start
    sta BNK_SEL

    lda #<mem_map
    sta TMP1_ADR
    lda #>mem_map
    sta TMP1_ADR+1
    
    ldx #0
    ldy #0
    lda #255
mem_start_init:
    sta (TMP1_ADR),y
    iny
    sta (TMP1_ADR),y
    iny
    sta (TMP1_ADR),y
    iny
    sta (TMP1_ADR),y
    iny
    bne :+
    inc TMP1_ADR+1
:   inx
    cpx mem_start
    bcc mem_start_init
    beq mem_start_init

    ;Mark pages in banks mem_start+1 to mem_top as free, except the head of buffer
    lda #%00000001
mem_usable_init:
    sta (TMP1_ADR),y
    lda #0
    iny
    sta (TMP1_ADR),y
    iny
    sta (TMP1_ADR),y
    iny
    sta (TMP1_ADR),y
    iny
    bne :+
    inc TMP1_ADR+1
:   cpx mem_top
    bcs :+
    inx
    jmp mem_usable_init

;Mark pages in banks mem_top+1 to 255 as allocated (not free)
:   cpx #255
    beq blocks_free_counter

    inx
    lda #255
mem_top_init:
    sta (TMP1_ADR),y
    iny
    sta (TMP1_ADR),y
    iny
    sta (TMP1_ADR),y
    iny
    sta (TMP1_ADR),y
    iny
    bne :+
    inc TMP1_ADR+1
:   inx
    bne mem_top_init

blocks_free_counter:
    ;Calculate free blocks
    sec
    lda mem_top
    sbc mem_start

    stz mem_blocks_free
    sta mem_blocks_free+1

    ldx #3
:   lsr mem_blocks_free+1
    ror mem_blocks_free
    dex
    bne :-

    ;Decrease mem blocks free counter by 1 to account for the head of buffer
    sec
    lda mem_blocks_free
    sbc #1
    sta mem_blocks_free
    lda mem_blocks_free+1
    sbc #0
    sta mem_blocks_free+1

    ;Setup zero page vectors
    ;Bank
    lda mem_start
    ina
    sta BNK_SEL
    sta CRS_BNK     ;Cursor
    sta LNV_BNK     ;Line first visible char
    sta SCR_BNK     ;Screen first visible char

    ;Mem page, i.e. most significant byte of address
    lda #$a0
    sta CRS_ADR+1   ;Cursor
    sta LNV_ADR+1   ;Line first visible char
    sta SCR_ADR+1   ;Screen first visible char

    ;Address, least significant byte
    stz CRS_ADR     ;Cursor
    stz LNV_ADR     ;Line first visible char
    stz SCR_ADR     ;Screen first visible char

    ;Memory offset values
    stz CRS_IDX     ;Cursor
    stz LNV_IDX     ;Line first visible char
    stz SCR_IDX     ;Screen first visible char

    ;Setup initial memory page metadata (bank mem_start+1, page $a0), bank selected above already
    lda #0
    
    ldy #0
    sta (CRS_ADR),y     ;Previous bank = 0, doesn't really matter
    
    ldy #1
    sta (CRS_ADR),y     ;Previous page = 0, i.e. null

    ldy #2
    sta (CRS_ADR),y     ;Next bank = 0, doesn't really matter

    ldy #3
    sta (CRS_ADR),y     ;Next page = 0, i.e. null

    ldy #4
    sta (CRS_ADR),y     ;Len = 0

    ;Set initial memory page for defrag runner
    lda mem_start
    ina
    sta mem_run_defrag_page
    lda #$a0
    sta mem_run_defrag_page+1

    ;Set current line=1 and column=1
    lda #1
    sta mem_cur_line
    sta mem_cur_col
    
    stz mem_cur_line+1
    stz mem_cur_line+2

    stz mem_cur_col+1
    stz mem_cur_col+2

    ;Set memory not modified
    stz mem_modified

    rts
.endproc

;**********************************************************
;Function name.......: mem_is_head
;Description.........: Is page head of buffer, i.e.
;                      bank mem_start+1, page $a0
;Input...............: Y    Bank
;                      X    Page
;Returns.............: C = 1    Is head
;                      C = 0    Is not head
;Error returns.......: None
.proc mem_is_head
    dey
    cpy mem_start
    bne :+
    cpx #$a0
    bne :+

    sec
    rts

:   clc
    rts

.endproc

;**********************************************************
;Function name.......: mem_at_bof
;Description.........: Is cursor at beginning of file, 
;                      i.e. first char, not necessarily
;                      the same as head of buffer
;Input...............: Nothing
;Returns.............: C = 1    Cursor at beginning of file
;                      C = 0    Not at beginning of file
;Error returns.......: Nothing
.proc mem_at_bof
    ;Get cursor index
    lda CRS_IDX
    beq :+

    ;If cursor index > 0 we are not at bof
    clc
    rts

    ;Search for bof, return if we find a char before that
:   lda CRS_BNK
    sta BNK_SEL

    lda CRS_ADR+1
    sta TMP1_ADR+1
    stz TMP1_ADR

:   ldy #1
    lda (TMP1_ADR),y        ;Prev page
    beq at_bof

    tax
    ldy #0
    lda (TMP1_ADR),y        ;Bank
    sta BNK_SEL
    stx TMP1_ADR+1
    
    ldy #4
    lda (TMP1_ADR),y        ;Page len
    beq :-                  ;Loop until we find non zero len page

not_bof:
    clc
    rts

at_bof:
    sec
    rts
.endproc

;**********************************************************
;Function name.......: mem_at_eof
;Description.........: Returns if cursor is at end of file.
;                      When inserting text at the end of the
;                      text buffer, the cursor will be
;                      one step right of the last char.
;                      This function returns true both
;                      when the cursor is at the last
;                      char and one step right of that.
;Input...............: Nothing
;Returns.............: C = 1    Cursor at end of file
;                      C = 0    Not at end of file
;Error returns.......: Nothing
.proc mem_at_eof
    ;Setup temp vector
    lda CRS_BNK
    sta BNK_SEL
    lda CRS_ADR+1
    sta TMP1_ADR+1
    stz TMP1_ADR

    ;Is cursor index + 1 < page len, we're not at end of file
    lda CRS_IDX
    clc
    adc #1
    ldy #4
    cmp (TMP1_ADR),y
    bcc not_eof

:   ldy #3
    lda (TMP1_ADR),y        ;Next page
    tax
    beq at_eof              ;Next page=0 => there is no next page, i.e. eof

    tax
    ldy #2
    lda (TMP1_ADR),y        ;Next bank
    sta BNK_SEL
    stx TMP1_ADR+1

    ldy #4
    lda (TMP1_ADR),y        ;Next page len
    beq :-                  ;Loop until we find a non zero len page

not_eof:
    clc
    rts

at_eof:
    sec
    rts

.endproc

;**********************************************************
;Funcion name........: mem_alloc
;Description.........: Allocates a page in banked RAM. The
;                      newly allocated page will be linked
;                      in after the page pointed to by the
;                      cursor.
;Input...............: Nothing
;Returns.............: Allocated memory page, Y= bank, and 
;                      X=page (address MSB)
;Error returns.......: X=0 if memory full
.proc mem_alloc
    ;Select RAM bank mem_start
    lda mem_start
    sta BNK_SEL

    ;Set vector to memory map
    lda #<mem_map
    sta TMP1_ADR
    lda #>mem_map
    sta TMP1_ADR+1

    ldy #0
loop:
    ;Search memory map for free pages: search for entries != $ff
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
    ;If we reach this point there was no free memory, return X=0
    ldx #0
    rts

free_mem_found:    
    ;Store the value of the byte in mem_map where we found free memory, will be needed later in the function
    sta value           

    ;Step 1. Divide the LSB of the number of search loops (= value of .Y) by 4 and we have the first part of the bank
    tya
    lsr
    lsr
    sta newbank

    ;Step 2. Multiply the MSB of the number of search loops (>TMP1_ADR minus >mem_map) by 64, and add that to the bank
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
    adc newbank
    sta newbank

    ;Step 3. Multiply the LSB of the number of search loops (= value of .Y) by 8 and we have the first part of the page
    tya
    asl
    asl
    asl

    ;Step 4. And that value by $1f to keep within page boundary (00-1f)
    and #$1f
    sta newpage

    ;Step 5. The second part of the page is found in the mem_map value stored earlier. Add the positional value of the first bit not set to the page
    ldx #0

:   lda value
    and bitmask,x
    beq mark_page_allocated
    inx
    jmp :-

mark_page_allocated:
    lda value
    ora bitmask,x
    sta (TMP1_ADR),y

    ;Step 6. Finally add start of banked ram ($a0) to the page. And we're done.
    txa
    clc
    adc newpage
    adc #$a0
    sta newpage

    ;Update mem page metadata; we need to change the current page, the new page, and the page following that
    lda CRS_BNK
    sta BNK_SEL
    
    ldy #2
    lda (CRS_ADR),y
    sta nextbank
    ldy #3
    lda (CRS_ADR),y
    sta nextpage

    lda newpage
    sta (CRS_ADR),y
    ldy #2
    lda newbank
    sta (CRS_ADR),y

    lda newbank
    sta BNK_SEL
    lda newpage
    sta TMP1_ADR+1
    stz TMP1_ADR

    ldy #0
    lda CRS_BNK
    sta (TMP1_ADR),y
    ldy #1
    lda CRS_ADR+1
    sta (TMP1_ADR),y
    ldy #2
    lda nextbank
    sta (TMP1_ADR),y
    ldy #3
    lda nextpage
    sta (TMP1_ADR),y
    ldy #4
    lda #0
    sta (TMP1_ADR),y

    lda nextpage
    beq exit

    sta TMP2_ADR+1
    stz TMP2_ADR
    lda nextbank
    sta BNK_SEL

    ldy #0
    lda newbank
    sta (TMP2_ADR),y
    ldy #1
    lda newpage
    sta (TMP2_ADR),y

    ;Return allocated bank and page
exit:
    ldy newbank
    ldx newpage

    ;Decrease mem blocks free counter
    dec mem_blocks_free
    lda mem_blocks_free
    cmp #$ff
    bne :+
    dec mem_blocks_free+1

:   rts

newbank = tempvars          ;1 byte
newpage = tempvars+1        ;1 byte
value = tempvars+2          ;1 byte
nextbank = tempvars+3       ;1 byte
nextpage = tempvars + 4     ;1 byte

bitmask:
    .byt 1, 2, 4, 8, 16, 32, 64, 128
.endproc

;**********************************************************
;Function name.......: mem_free
;Description.........: Frees a page in banked RAM
;Input...............: Y    Bank
;                      X    Page
;Returns.............: Nohting
;Error returns.......: C = 1    Could not free page
.proc mem_free
    ;Save input params
    sty bank
    stx page
    
    ;Check if bank is reserved, exit without deallocating if true
    cpy mem_start
    beq is_reserved
    bcc is_reserved
    
    cpy mem_top
    beq :+
    bcs is_reserved     ;Don't touch unpopulated memory

:   tya                 ;And don't touch head of buffer - bank mem_start+1, page $a0
    dea
    cmp mem_start
    bne :+
    lda page
    cmp #$a0
    bne :+

is_reserved:
    sec
    rts

    ;Continue freeing that mem page...
    ;Find out position in mem_map corresponding to specified bank/page
:   lda mem_start              ;Select bank mem_start to access mem_map
    sta BNK_SEL

    lda bank
    lsr
    lsr
    lsr
    lsr
    lsr
    lsr
    clc
    adc #>mem_map
    sta TMP1_ADR+1      ;High byte = bank/64 + mem_mapH
    
    lda bank
    asl
    asl
    adc #<mem_map
    sta TMP1_ADR        ;Low byte = bank * 4 + mem_mapL
    lda #0
    adc TMP1_ADR+1
    sta TMP1_ADR+1      ;Add possible overflow to high byte

    lda page
    sec
    sbc #$a0
    lsr
    lsr
    lsr
    tay                 ;Offset, Y = page/8

    lda page
    and #7
    tax                 ;X is now bitmask index corresponding to the page

    lda bitmask,x
    and (TMP1_ADR),y
    beq exit2           ;Page already free, exit
    
    lda bitmask,x
    eor #255
    and (TMP1_ADR),y
    sta (TMP1_ADR),y    ;Mark page as free

    ;Get prev and next page
    lda bank
    sta BNK_SEL
    lda page
    sta TMP1_ADR+1
    stz TMP1_ADR

    ldy #0
    lda (TMP1_ADR),y
    sta prev
    ldy #1
    lda (TMP1_ADR),y
    sta prev+1

    ldy #2
    lda (TMP1_ADR),y
    sta next
    ldy #3
    lda (TMP1_ADR),y
    sta next+1

    ;Set prev page pointer
    lda prev+1
    beq next_page_link      ;Prev page null, shouldn't happen as we don't allow freeing the head of the buffer
   
    sta TMP1_ADR+1
    lda prev
    sta BNK_SEL

    ldy #2
    lda next
    sta (TMP1_ADR),y
    ldy #3
    lda next+1
    sta (TMP1_ADR),y

    ;Set next page pointer
next_page_link:
    lda next+1
    beq exit             ;Next page null
    
    sta TMP1_ADR+1
    lda next
    sta BNK_SEL

    ldy #0
    lda prev
    sta (TMP1_ADR),y
    ldy #1
    lda prev+1
    sta (TMP1_ADR),y

exit:
    clc

    ;Increase mem block free counter
    inc mem_blocks_free
    bne exit2
    inc mem_blocks_free+1

exit2:
    rts

bank = tempvars     ;1 byte
page = tempvars+1   ;1 byte
prev = tempvars+2   ;2 bytes
next = tempvars+4   ;2 bytes

bitmask:
    .byt 1,2,4,8,16,32,64,128
.endproc

;**********************************************************
;Function name.......: mem_run_defrag
;Description.........: This function is meant to be invoked
;                      once every interrupt if the program
;                      has nothing else to do. It will run 
;                      the defrag routine on one mem page 
;                      per invokation.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_run_defrag
    ;Prepare zero page vector
    ;mem_run_defrag_page (2 bytes) refer to the next bank/page that we will try to defrag
    lda mem_run_defrag_page
    sta BNK_SEL
    lda mem_run_defrag_page+1
    sta TMP1_ADR+1
    stz TMP1_ADR

    ;Exit if next page contains the first visible char of the screen - otherwise we might lose the pointer to that char
    ldy #2
    lda (TMP1_ADR),y
    cmp SCR_BNK
    bne :+
    ldy #3
    lda (TMP1_ADR),y
    cmp SCR_ADR+1
    bne :+
    jmp exit

    ;Exit if next page contains the first visible char of the line - otherwise we might lose the pointer to that char
:   ldy #2
    lda (TMP1_ADR),y
    cmp LNV_BNK
    bne :+
    ldy #3
    lda (TMP1_ADR),y
    cmp LNV_ADR+1
    bne :+
    jmp exit

    ;Exit if the cursor is at the next page - otherwise we might lose the pointer to the char at the cursor
    ldy #2
    lda (TMP1_ADR),y
    cmp CRS_BNK
    bne :+
    ldy #3
    lda (TMP1_ADR),y
    cmp CRS_ADR+1
    bne :+
    jmp exit

    ;Do not defrag the page where the cursor is - that's not effective as we are typing here, and it could also cause display errors
:   lda mem_run_defrag_page
    cmp CRS_BNK
    bne :+
    lda mem_run_defrag_page+1
    cmp CRS_ADR+1
    bne :+
    jmp exit

    ;Do not defrag the page before the page where the cursor is - that's not effective as we are typing here, and it could also cause display errors
:   lda CRS_BNK
    sta BNK_SEL
    ldy #0
    lda (CRS_ADR),y
    cmp mem_run_defrag_page
    bne :+
    ldy #1
    lda (CRS_ADR),y
    cmp mem_run_defrag_page+1
    bne :+
    jmp exit

    ;Dot not defrag the page after the page where the cursor is - that's not effective as we are typing here, and it could also cause display errors
:   ldy #2
    lda (CRS_ADR),y
    cmp mem_run_defrag_page
    bne :+
    ldy #3
    lda (CRS_ADR),y
    cmp mem_run_defrag_page+1
    bne :+
    jmp exit

    ;We passed all checks, go on and run the defrag function
:   ldy mem_run_defrag_page
    ldx mem_run_defrag_page+1
    jsr mem_defrag

exit:
    ;Select next page to defrag when calling this function next time
    lda mem_run_defrag_page
    sta BNK_SEL
    
    lda mem_run_defrag_page+1
    sta TMP1_ADR+1
    stz TMP1_ADR
    ldy #3
    lda (TMP1_ADR),y
    beq eof
    sta mem_run_defrag_page+1
    ldy #2
    lda (TMP1_ADR),y
    sta mem_run_defrag_page
    rts

eof:
    ;We were at the last page, start over from the head of the buffer
    lda mem_start
    ina
    sta mem_run_defrag_page
    lda #$a0
    sta mem_run_defrag_page+1
    rts

.endproc

;**********************************************************
;Function name.......: mem_defrag
;Description.........: Defragments a memory page. This
;                      function will copy text from the
;                      next page filling up the selected
;                      page. If the next page len is 0
;                      before or after this it will be 
;                      marked as free.
;Input...............: Y = bank, X = page
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_defrag
    ;Temp vectors
    sty BNK_SEL
    sty TMP1_BNK
    stx TMP1_ADR+1
    stz TMP1_ADR

    ;Page len, exit if page full
    ldy #4
    lda (TMP1_ADR),y
    cmp #251
    bcc :+
    rts

:   sta page_len

    ;Get next page, exit if null
    ldy #3
    lda (TMP1_ADR),y
    bne :+              
    rts

:   sta TMP2_ADR+1
    stz TMP2_ADR
    
    ldy #2
    lda (TMP1_ADR),y
    sta TMP2_BNK 

    ;Next page len
    lda TMP2_BNK
    sta BNK_SEL
    ldy #4
    lda (TMP2_ADR),y
    bne :+
    jmp free
:   sta nextpage_len

    ;Prepare copy
    lda #5
    sta TMP1_ADR
    sta TMP2_ADR
    stz nextpage_index

copy:
    lda nextpage_index
    cmp nextpage_len
    bcs copy_done

    lda page_len
    cmp #251
    bcs copy_done

    ldy nextpage_index
    ldx TMP2_BNK
    stx BNK_SEL
    lda (TMP2_ADR),y

    ldy page_len
    ldx TMP1_BNK
    stx BNK_SEL
    sta (TMP1_ADR),y

    inc nextpage_index
    inc page_len

    jmp copy

copy_done:
    ;Set page len after copy
    lda TMP1_BNK
    sta BNK_SEL
    stz TMP1_ADR
    ldy #4
    lda page_len
    sta (TMP1_ADR),y

    ;Set next page len after copy
    lda TMP2_BNK
    sta BNK_SEL
    stz TMP2_ADR
    sec
    lda nextpage_len
    sbc nextpage_index
    ldy #4
    sta (TMP2_ADR),y
    cmp #0
    beq free

move_nextpage:
    lda TMP2_BNK
    sta BNK_SEL

    stz nextpage_index2
    lda #5
    sta TMP2_ADR

:   ldy nextpage_index
    cpy #251
    beq exit
    lda (TMP2_ADR),y
    ldy nextpage_index2
    sta (TMP2_ADR),y
    inc nextpage_index
    inc nextpage_index2
    jmp :-

exit:
    rts

free:
    ldy TMP2_BNK
    ldx TMP2_ADR+1
    jmp mem_free

page_len = tempvars             ;1 byte
nextpage_len = tempvars +1      ;1 byte
nextpage_index = tempvars + 2   ;1 byte
nextpage_index2 = tempvars +3   ;1 byte
.endproc

;**********************************************************
;Function name.......: mem_push
;Description.........: Pushes content of a mem page
;                      one step forward
;Input...............: Y=mem bank, X=mem page, A=index to start pushing from
;Returns.............: C=1 if overflow; overflow char returned in .A
;Error returns.......: Nothing
.proc mem_push
    ;Save push start index for later use
    sta start

    ;Set zero page pointers
    sty BNK_SEL
    stx TMP1_ADR+1
    stz TMP1_ADR
    stx TMP2_ADR+1

    ;Get len
    ldy #4
    lda (TMP1_ADR),y
    sta len

    ;If len = 0 there's nothing to push. Set len = 1 and exit
    cmp #0
    bne :+

    lda #1
    sta (TMP1_ADR),y
    clc
    rts
    
    ;Save last char in mem page, if there is overflow to return later
:   ldy #255
    lda (TMP1_ADR),y
    sta last_char

    ;Is start index >= 250, skip push routine
    lda start
    cmp #250
    bcc :+
    jmp setlen
    
    ;Set offset used when pushing data
:   lda #5
    sta TMP1_ADR
    lda #6
    sta TMP2_ADR
    
    ldy #249

loop:
    lda (TMP1_ADR),y
    sta (TMP2_ADR),y

    cpy start
    beq setlen
    dey
    jmp loop

setlen:
    ;Restore to default value
    stz TMP1_ADR

    ;Len < 251? Yes: Increase len before exit
    lda len
    cmp #251
    bcc :+

    ;Len >= 251. Don't increase len. Return overflow, indicated by carry=1
    sec
    lda last_char
    rts

:   inc len
    lda len
    ldy #4
    sta (TMP1_ADR),y
    
    clc
    rts

start = tempvars        ;1 byte
len = tempvars+1        ;1 byte
last_char = tempvars+2  ;1 byte
.endproc

;**********************************************************
;Procedure name......: mem_insert
;Description.........: Inserts a char at cursor location
;Input...............: A=char to insert
;Returns.............: Nothing
;Error returns.......: C=1 if mem is full
.proc mem_insert
    ;Save char on stack
    pha

    ;Check if we are at beginning of mem page, and if there is room to insert
    ;the char in previous mem page
    lda CRS_IDX
    bne pushforward

    lda CRS_BNK
    sta BNK_SEL

    ldy #1
    lda (CRS_ADR),y
    beq pushforward
    tax
    ldy #0
    lda (CRS_ADR),y
    sta BNK_SEL
    sta TMP1_BNK
    stx TMP1_ADR+1
    stz TMP1_ADR

    ldy #4
    lda (TMP1_ADR),y
    cmp #251
    bcs pushforward

    ;There was room in previous mem page, let's insert char there
    tax     ;Temp storage for prev page len
    clc
    adc #5  ;Add 5 to skip mem page metadata
    tay
    pla     ;Get char from stack
    sta (TMP1_ADR),y

    inx     ;Increase prev page len
    txa
    ldy #4
    sta (TMP1_ADR),y

    dex             ;Decrease to represent last index in prev page

    ;Check if LNV_XXX is at CRS_XXX. If so, step left to char we inserted
    lda LNV_BNK
    cmp CRS_BNK
    bne :+
    lda LNV_ADR+1
    cmp CRS_ADR+1
    bne :+
    lda LNV_IDX
    cmp CRS_IDX
    bne :+

    lda TMP1_BNK
    sta LNV_BNK
    lda TMP1_ADR+1
    sta LNV_ADR+1
    stx LNV_IDX    

    ;Check if SCR_XXX is at CRS_XXX. If so, step left to char we inserted
:   lda SCR_BNK
    cmp CRS_BNK
    bne :+
    lda SCR_ADR+1
    cmp CRS_ADR+1
    bne :+
    lda SCR_IDX
    cmp CRS_IDX
    bne :+

    lda TMP1_BNK
    sta SCR_BNK
    lda TMP1_ADR+1
    sta SCR_ADR+1
    stx SCR_IDX

    ;Exit
:   lda #1
    sta mem_modified
    rts

pushforward:
    ldy CRS_BNK
    ldx CRS_ADR+1
    lda CRS_IDX
    jsr mem_push
    bcc insert

;Store overflow
    pha
    lda CRS_BNK
    sta BNK_SEL
    
    ldy #3
    lda (CRS_ADR),y
    beq alloc
    
    tax
    
    ldy #2
    lda (CRS_ADR),y
    
    stx TMP1_ADR+1
    stz TMP1_ADR
    sta BNK_SEL
    sta TMP1_BNK

    ldy #4
    lda (TMP1_ADR),y
    cmp #251
    bcs alloc
    
    ldy TMP1_BNK
    ldx TMP1_ADR+1
    lda #0
    jsr mem_push

    pla
    ldy #5
    stz TMP1_ADR
    sta (TMP1_ADR),y
    jmp insert

alloc:
    jsr mem_alloc
    cpx #0          ;Mem full?
    beq mem_full
    sty BNK_SEL
    stx TMP1_ADR+1
    stz TMP1_ADR
    
    pla
    ldy #5
    sta (TMP1_ADR),y
    ldy #4
    lda #1
    sta (TMP1_ADR),y

insert:
    lda CRS_BNK
    sta BNK_SEL

    lda #5
    sta CRS_ADR

    pla
    ldy CRS_IDX
    sta (CRS_ADR),y

    stz CRS_ADR  

    lda #1
    sta mem_modified

    jsr mem_crs_step_right
    cpx #2
    bne :+
    sec
    rts
    
:   clc
    rts

mem_full:
    pla     ;Clean stack
    pla

    sec
    rts
.endproc

;**********************************************************
;Procedure name......: mem_delete
;Description.........: Deletes the char left of cursor
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: C=1 if at file start, else C=0
;                      X=1 if at line break, else X=0
.proc mem_delete
    stz at_lf

    ;Cursor at beginning of file?
    jsr mem_at_bof
    bcc :+

    ldx #0
    sec
    rts

    ;Cursor at beginning of page?
:   lda CRS_IDX
    bne default

    ;Yes, it was at beginning of page: decrease len of the previous page without moving the cursor pointer
    lda CRS_BNK
    sta BNK_SEL
    lda CRS_ADR+1
    sta TMP1_ADR+1
    stz TMP1_ADR

:   ldy #0
    lda (TMP1_ADR),y
    tax
    ldy #1
    lda (TMP1_ADR),y
    sta TMP1_ADR+1
    stx BNK_SEL
    ldy #4
    lda (TMP1_ADR),y
    beq :-

    sec
    sbc #1
    sta (TMP1_ADR),y

    tay
    lda #5
    sta TMP1_ADR
    lda (TMP1_ADR),y
    cmp #LF
    bne :+
    inc at_lf

:   ldx at_lf
    clc
    lda #1
    sta mem_modified

    rts

    ;We are not at beginning of the page: Move the cursor pointer one step left, decrease page len, and copy text from the cursor to end of page one step left
default:
    jsr mem_crs_step_left

    lda CRS_BNK             ;Check if we deleted a line feed
    sta BNK_SEL
    lda #5
    sta CRS_ADR
    ldy CRS_IDX
    lda (CRS_ADR),y
    cmp #LF
    bne :+
    inc at_lf

:   stz CRS_ADR             ;Decrease page len
    ldy #4
    lda (CRS_ADR),y
    sec
    sbc #1
    sta (CRS_ADR),y

    lda #5
    sta CRS_ADR
    ldy CRS_IDX  
    iny

:   lda (CRS_ADR),y         ;Copy text from cursor to page end one step left
    dey
    sta (CRS_ADR),y
    iny
    iny
    cpy #251
    bne :-

    stz CRS_ADR             ;Restore vector

    ldy #4
    lda (CRS_ADR),y
    bne :+                  ;Mem page len > 0, exit

    jsr mem_at_eof          ;Mem page len = 0, check if we are at EOF, if not need to take one step left, otherwise the cursor would be at an undefined position
    bcs :+
    jsr mem_crs_step_left

:   ldx at_lf
    clc
    lda #1
    sta mem_modified
    rts

.segment "VARS"
    at_lf: .res 1
.CODE
.endproc


;**********************************************************
;Procedure name......: mem_crs_step_right
;Description.........: Moves the pointer to memory location
;                      of the cursor one step right
;Input...............: Nothing
;Returns.............: .C=1 if eof else .C=0
;                      .X=1 if LF else .X=0
;Error returns.......: X=2 if memory full error
.proc mem_crs_step_right
    ;Is cursor at LF?
    stz at_lf

    lda CRS_BNK
    sta BNK_SEL
    clc
    lda CRS_IDX
    adc #5
    tay
    lda (CRS_ADR),y
    cmp #LF
    bne :+
    inc at_lf

    ;Is cursor at end of file?
:   jsr mem_at_eof
    bcc :+

    lda CRS_BNK
    sta BNK_SEL
    lda CRS_IDX
    clc
    adc #1
    ldy #4
    cmp (CRS_ADR),y
    beq goto_eof
    
    sec
    ldx #0
    rts

:   ;Is cursor at end of mem page?
    lda CRS_BNK
    sta BNK_SEL
    lda CRS_IDX
    clc
    adc #1
    cmp (CRS_ADR),y
    bcc default

    ;Go to next mem page
:   ldy #2
    lda (CRS_ADR),y
    tax
    ldy #3
    lda (CRS_ADR),y
    stx CRS_BNK
    stx BNK_SEL
    sta CRS_ADR+1
    stz CRS_IDX
    ldy #4
    lda (CRS_ADR),y
    beq :-              ;Len = 0, continue searching...

    clc
    ldx at_lf
    rts

goto_eof:
    lda CRS_IDX
    cmp #250
    bne default

    jsr mem_alloc
    cpx #0              ;Mem full?
    beq mem_full

    sty CRS_BNK
    stx CRS_ADR+1
    stz CRS_IDX
    clc
    ldx at_lf
    rts

mem_full:
    ldx #2
    sec
    rts

default:
    inc CRS_IDX
    clc
    ldx at_lf
    rts

.segment "VARS"
    at_lf: .res 1
.CODE
.endproc

;**********************************************************
;Procedure name......: mem_crs_step_left
;Description.........: Moves the pointer to memory location
;                      of the cursor one step left
;Input...............: Nothing
;Returns.............: .C=1 if at beginning of file,
;                      otherwise .C=0
;                      .X=1 if line break, else .X=0
;Error returns.......: Nothing
.proc mem_crs_step_left
    ;Is cursor at beginning of file?
    jsr mem_at_bof
    bcc :+

    sec
    ldx #0
    rts

:   ;Is cursor at start of mem page?
    lda CRS_IDX
    bne default

    lda CRS_BNK
    sta BNK_SEL

:   ldy #1
    lda (CRS_ADR),y
    tax
    ldy #0
    lda (CRS_ADR),y
    sta CRS_BNK
    sta BNK_SEL
    stx CRS_ADR+1

    ldy #4
    lda (CRS_ADR),y
    beq :-              ;Len = 0, continue searching...

    sec
    sbc #1
    sta CRS_IDX

    jmp exit

default:
    dec CRS_IDX

exit:
    ldx #0
    
    lda CRS_BNK
    sta BNK_SEL
    
    clc
    lda CRS_IDX
    adc #5
    tay
    lda (CRS_ADR),y
    cmp #LF
    bne :+
    ldx #1
 
 :  clc
    rts

.segment "VARS"
    at_lf: .res 1
.CODE
.endproc

;**********************************************************
;Procedure name......: mem_crs_move_to_line_start
;Description.........: Moves the pointer to memory location
;                      at start of line
;Input...............: Nothing
;Returns.............: Numbers of steps the cursor was
;                      moved, LSB in X and MSB in Y
;Error returns.......: Nothing
.proc mem_crs_move_to_line_start
    stz count
    stz count+1
    stz count+2
  
loop:
    stz CRS_ADR

    jsr mem_crs_step_left
    bcs exit

    inc count
    bne :+
    inc count+1
    bne :+
    inc count+2

:   lda CRS_BNK
    sta BNK_SEL

    ldy CRS_IDX
    lda #5
    sta CRS_ADR
    lda (CRS_ADR),y
    cmp #LF
    bne loop

    stz CRS_ADR

    jsr mem_crs_step_right
    
    dec count
    lda count
    cmp #$ff
    bne exit
    dec count+1
    lda count+1
    cmp #$ff
    bne exit
    dec count+2

exit:
    ldx count
    ldy count+1
    lda count+2
   
    rts

.segment "VARS"
    count: .res 3
.CODE
.endproc

;**********************************************************
;Procedure name......: mem_crs_move_to_prev_blankspace
;Description.........: Moves the pointer to memory location
;                      at previous blank space, it will
;                      however stop at LF or BOF if no blank
;                      space was found before
;Input...............: Nothing
;Returns.............: Number of steps the cursor was moved,
;                      LSB in X and MSB in Y
;Error returns.......: C=1 if LF found before a blank space
.proc mem_crs_move_to_prev_blankspace
    ;Reset counter
    stz counter
    stz counter+1

    ;First take one step left before we begin searching
    jsr mem_crs_step_left

    ;Get page len
    lda CRS_BNK
    sta BNK_SEL

    ldy #4
    lda (CRS_ADR),y
    sta pagelen

check_first_char:
    lda CRS_BNK
    sta BNK_SEL
    
    lda #5
    sta CRS_ADR
    ldy CRS_IDX
    lda (CRS_ADR),y
    cmp #32
    bne loop
    
    inc counter
    jmp found

loop:
    ldy CRS_IDX
    lda (CRS_ADR),y
    cmp #32
    beq found
    cmp #LF
    beq notfound

    inc counter
    bne :+
    inc counter+1

:   lda CRS_IDX
    beq prevpage
    dec CRS_IDX
    jmp loop

prevpage:
    stz CRS_ADR
    
    ;Get prev page
    ldy #0
    lda (CRS_ADR),y
    tax
    ldy #1
    lda (CRS_ADR),y
    beq notfound
    stx BNK_SEL
    sta CRS_ADR+1

    ;Get prev page len
    ldy #4
    lda (CRS_ADR),y
    sta pagelen
    beq prevpage
    
    tay
    dey
    sty CRS_IDX

    lda #5
    sta CRS_ADR
    jmp loop


notfound:
    sec
    stz CRS_ADR
    ldx counter
    ldy counter+1
    rts

found:
    stz CRS_ADR
    jsr mem_crs_step_right
    
    clc
    ldx counter
    ldy counter+1
    rts

.segment "VARS"
    pagelen: .res 1
    counter: .res 2
.CODE

.endproc

;**********************************************************
;Procedure name......: mem_crs_move_to_line_end
;Description.........: Moves the pointer to memory location
;                      at end of line
;Input...............: Nothing
;Returns.............: Number of steps the cursor was moved,
;                      LSB in X and MSB in Y
;Error returns.......: Nothing
.proc mem_crs_move_to_line_end
    stz count
    stz count+1

    lda CRS_BNK
    sta BNK_SEL
loop:
    lda #5
    sta CRS_ADR

    ldy CRS_IDX
    lda (CRS_ADR),y
    cmp #LF
    beq exit

    stz CRS_ADR
    jsr mem_crs_step_right
    bcs exit

    inc count
    bne loop
    inc count+1

    jmp loop

exit:
    stz CRS_ADR
    ldy count+1 ;MSB
    ldx count   ;LSB

    rts

.segment "VARS"
    count: .res 2
.CODE

.endproc

;**********************************************************
;Procedure name......: mem_lnv_step_left
;Description.........: Moves the pointer to first visible
;                      char of current line
;Input...............: Nothing
;Returns.............: .C=1 if at beginning of file,
;                      otherwise .C=0
;Error returns.......: Nothing
.proc mem_lnv_step_left
    ;Backup CRS pointers
    lda CRS_BNK
    sta backup
    lda CRS_ADR+1
    sta backup+1
    lda CRS_IDX
    sta backup+2

    ;Move LNV pointers to CRS
    lda LNV_BNK
    sta CRS_BNK
    lda LNV_ADR+1
    sta CRS_ADR+1
    lda LNV_IDX
    sta CRS_IDX

    ;Call CRS step function
    jsr mem_crs_step_left

    ;Copy result to LNV pointers
    lda CRS_BNK
    sta LNV_BNK
    lda CRS_ADR+1
    sta LNV_ADR+1
    lda CRS_IDX
    sta LNV_IDX

    ;Restore CRS pointers
    lda backup
    sta CRS_BNK
    lda backup+1
    sta CRS_ADR+1
    lda backup+2
    sta CRS_IDX
    
    rts

.segment "VARS"
    backup: .res 3
.CODE
.endproc

;**********************************************************
;Procedure name......: mem_lnv_step_right
;Description.........: Moves the pointer to first visble
;                      char of current line
;Input...............: Nothing
;Returns.............: .C = 1 if at end of file, 
;                      otherwise .C=0
;Error returns.......: Nothing
.proc mem_lnv_step_right
    ;Backup CRS pointers
    lda CRS_BNK
    sta backup
    lda CRS_ADR+1
    sta backup+1
    lda CRS_IDX
    sta backup+2

    ;Move LNV pointers to CRS
    lda LNV_BNK
    sta CRS_BNK
    lda LNV_ADR+1
    sta CRS_ADR+1
    lda LNV_IDX
    sta CRS_IDX

    ;Call CRS step function
    jsr mem_crs_step_right

    ;Copy result to LNV pointers
    lda CRS_BNK
    sta LNV_BNK
    lda CRS_ADR+1
    sta LNV_ADR+1
    lda CRS_IDX
    sta LNV_IDX

    ;Restore CRS pointers
    lda backup
    sta CRS_BNK
    lda backup+1
    sta CRS_ADR+1
    lda backup+2
    sta CRS_IDX
    
    rts

.segment "VARS"
    backup: .res 3
.CODE
.endproc

;**********************************************************
;Procedure name......: mem_scr_move_down
;Description.........: Moves the pointer to first visible
;                      char of the screen one line down
;Input...............: Nothing
;Returns.............: .C = 1 if at end of file, 
;                      otherwise .C=0
;Error returns.......: Nothing
.proc mem_scr_move_down
    lda CRS_BNK
    sta backup
    lda CRS_ADR+1
    sta backup+1
    lda CRS_IDX
    sta backup+2

    lda SCR_BNK
    sta CRS_BNK
    lda SCR_ADR+1
    sta CRS_ADR+1
    lda SCR_IDX
    sta CRS_IDX

    jsr mem_crs_move_to_line_end
    jsr mem_crs_step_right

    lda CRS_BNK
    sta SCR_BNK
    lda CRS_ADR+1
    sta SCR_ADR+1
    lda CRS_IDX
    sta SCR_IDX

    lda backup
    sta CRS_BNK
    lda backup+1
    sta CRS_ADR+1
    lda backup+2
    sta CRS_IDX
    
    rts

.segment "VARS"    
    backup: .res 3
.CODE
.endproc

;**********************************************************
;Procedure name......: mem_scr_move_up
;Description.........: Moves the pointer to first visible
;                      char of the screen one line up
;Input...............: Nothing
;Returns.............: .C = 1 if at start of file, 
;                      otherwise .C=0
;Error returns.......: Nothing
.proc mem_scr_move_up
    lda CRS_BNK
    sta backup
    lda CRS_ADR+1
    sta backup+1
    lda CRS_IDX
    sta backup+2

    lda SCR_BNK
    sta CRS_BNK
    lda SCR_ADR+1
    sta CRS_ADR+1
    lda SCR_IDX
    sta CRS_IDX

    jsr mem_crs_move_to_line_start
    jsr mem_crs_step_left
    jsr mem_crs_move_to_line_start

    lda CRS_BNK
    sta SCR_BNK
    lda CRS_ADR+1
    sta SCR_ADR+1
    lda CRS_IDX
    sta SCR_IDX

    lda backup
    sta CRS_BNK
    lda backup+1
    sta CRS_ADR+1
    lda backup+2
    sta CRS_IDX
    
    rts

.segment "VARS"
    backup: .res 3
.CODE

.endproc

;**********************************************************
;Procedure name......: mem_set_lnv_to_crs
;Description.........: Inserts a char at cursor location
;Input...............: .A = char to insert
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_set_lnv_to_crs
    lda CRS_BNK
    sta LNV_BNK
    
    lda CRS_ADR
    sta LNV_ADR
    
    lda CRS_ADR+1
    sta LNV_ADR+1
    
    lda CRS_IDX
    sta LNV_IDX

    rts
.endproc

;**********************************************************
;Procedure name......: mem_cur_col_inc
;Description.........: Increases column index counter
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_cur_col_inc
    inc mem_cur_col
    bne exit
    inc mem_cur_col+1
    bne exit
    inc mem_cur_col+2

exit:
    rts
.endproc

;**********************************************************
;Procedure name......: mem_cur_col_dec
;Description.........: Decreases column index counter
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_cur_col_dec
    dec mem_cur_col
    lda mem_cur_col
    cmp #$ff
    bne exit

    dec mem_cur_col+1
    lda mem_cur_col+1
    cmp #$ff
    bne exit

    dec mem_cur_col+2

exit:
    rts
.endproc

;**********************************************************
;Procedure name......: mem_cur_col_ret
;Description.........: Set column index to 1
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_cur_col_ret
    lda #1
    sta mem_cur_col
    stz mem_cur_col+1
    stz mem_cur_col+2
    rts
.endproc

;**********************************************************
;Procedure name......: mem_cur_line_inc
;Description.........: Increases line index counter
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_cur_line_inc
    inc mem_cur_line
    bne exit
    inc mem_cur_line+1
    bne exit
    inc mem_cur_line+2

exit:
    rts
.endproc

;**********************************************************
;Procedure name......: mem_cur_col_dec
;Description.........: Decreases line index counter
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_cur_line_dec
    dec mem_cur_line
    lda mem_cur_line
    cmp #$ff
    bne exit

    dec mem_cur_line+1
    lda mem_cur_line+1
    cmp #$ff
    bne exit
    
    dec mem_cur_line+2

exit:
    rts
.endproc

.segment "VARS"
    mem_run_defrag_page:    .res 2
    mem_modified:           .res 1              ;1 byte 
    mem_blocks_free:        .res 2              ;2 bytes
    mem_start:              .res 1              ;1 byte
    mem_top:                .res 1              ;1 byte
    mem_cur_line:           .res 3              ;3 bytes
    mem_cur_col:            .res 3              ;3 bytes
.CODE