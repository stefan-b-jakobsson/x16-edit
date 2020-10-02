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
    sec
    jsr KERNAL_MEMTOP
    sec
    sbc #1
    sta mem_top

    ;Mark pages in banks 0-63
    ;Mark bank 0 as used (by Kernal)
    ;Bank 1 page $a0 as used (head of text)
    ;All other pages marked as free
    
    ldx #0
:   lda #0
    sta mem_map,x
    inx
    bne :-

    lda #255
    sta mem_map
    sta mem_map+1
    sta mem_map+2
    sta mem_map+3

    lda #1
    sta mem_map+4

    ;If mem_top >=127, mark pages in banks 64-127 as free, else mark as used
    ldx #0

    lda mem_top
    cmp #127
    bcc :+
    lda #0
    jmp :++
:   lda #255

:   ldx #0
:   sta mem_map+256,x
    inx
    bne :-

    ;If mem_top >=191, mark pages in banks 128-191 as free, else mark as used
    ldx #0

    lda mem_top
    cmp #191
    bcc :+
    lda #0
    jmp :++
:   lda #255

:   ldx #0
:   sta mem_map+512,x
    inx
    bne :-

    ;If mem_top=255, mark pages in banks 192-255 as free, else mark as used
    ldx #0

    lda mem_top
    cmp #255
    bne :+
    lda #0
    jmp :++
:   lda #255

:   ldx #0
:   sta mem_map+768,x
    inx
    bne :-

    ;Set mem blocks free counter
    lda mem_top
    sta mem_blocks_free+1
    ldx #3
:   lsr mem_blocks_free+1
    ror mem_blocks_free
    dex
    bne :-

    sec
    lda mem_blocks_free
    sbc #1
    sta mem_blocks_free
    lda mem_blocks_free+1
    sbc #0
    sta mem_blocks_free+1

    ;Setup zero page vectors

    ;Bank
    lda #1
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

    ;Setup initial memory page metadata (bank 1, page $a0)
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
    lda #0
    sta (CRS_ADR),y     ;Len = 1

    ;Set memory not modified
    stz mem_modified

    ;Set current line=1 and column=1
    lda #1
    sta mem_cur_line
    sta mem_cur_col
    
    stz mem_cur_line+1
    stz mem_cur_line+2

    stz mem_cur_col+1
    stz mem_cur_col+2

    rts
.endproc

;**********************************************************
;Procedure name......: mem_is_head
;Description.........: Finds out if page is head of buffer
;Input...............: Y    Bank
;                      X    Page
;Returns.............: C = 1    Is head
;                      C = 0    Is not head
;Error returns.......: None
.proc mem_is_head
    cpy #1
    bne :+
    cpx #$a0
    bne :+

    sec
    rts

:   clc
    rts

.endproc

;**********************************************************
;Procedure name......: mem_at_bof
;Description.........: Returns if current position is
;                      beginning of file, i.e. first char
;                      of the file
;Input...............: Nothing
;Returns.............: C = 1    Cursor at beginning of file
;                      C = 0    Not at beginning of file
;Error returns.......: Nothing
.proc mem_at_bof
    lda CRS_IDX
    beq :+

    ;If CRS_IDX > 0 we are not at bof
    clc
    rts

:   lda CRS_BNK
    sta BNK_SEL

    lda CRS_ADR+1
    sta TMP1_ADR+1
    stz TMP1_ADR

:   ldy #1
    lda (TMP1_ADR),y
    beq at_bof

    tax
    ldy #0
    lda (TMP1_ADR),y
    sta BNK_SEL
    stx TMP1_ADR+1
    
    ldy #4
    lda (TMP1_ADR),y
    beq :-

not_bof:
    clc
    rts

at_bof:
    sec
    rts
.endproc

;**********************************************************
;Procedure name......: mem_at_eof
;Description.........: Returns if current position is
;                      end of file, at last char or 
;                      beyond
;Input...............: Nothing
;Returns.............: C = 1    Cursor at end of file
;                      C = 0    Not at end of file
;Error returns.......: Nothing
.proc mem_at_eof
    lda CRS_BNK
    sta BNK_SEL
    lda CRS_ADR+1
    sta TMP1_ADR+1
    stz TMP1_ADR

    ldy #4
    lda CRS_IDX
    clc
    adc #1
    cmp (TMP1_ADR),y
    bcc not_eof

:   ldy #3
    lda (TMP1_ADR),y
    tax
    beq at_eof

    tax
    ldy #2
    lda (TMP1_ADR),y
    sta BNK_SEL
    stx TMP1_ADR+1

    ldy #4
    lda (TMP1_ADR),y
    beq :-

not_eof:
    clc
    rts

at_eof:
    sec
    rts

.endproc

;**********************************************************
;Procedure name......: mem_alloc
;Description.........: Allocates a page in banked RAM. The
;                      newly allocated page will be linked
;                      in after the page pointed to by the
;                      cursor.
;Input...............: Nothing
;Returns.............: Allocated memory page, Y= bank, and 
;                      X=page (address MSB)
;Error returns.......: X=0 if memory full
.proc mem_alloc
    ;Get vector to memory map
    lda #<mem_map
    sta TMP1_ADR
    lda #>mem_map
    sta TMP1_ADR+1

    ldy #0
loop:
    ;Loop through memory map searching for free memory, search for entries != $ff
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

    ;Setup mem pages
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

newbank:
    .byt 0
newpage:
    .byt 0

value:
    .byt 0

bitmask:
    .byt 1, 2, 4, 8, 16, 32, 64, 128

nextbank:
    .byt 0
nextpage:
    .byt 0

.endproc

;**********************************************************
;Procedure name......: mem_free
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
    lda bank
    cmp #0
    beq is_reserved
    
    cmp mem_top
    bcs is_reserved

    cmp #1
    bne :+
    lda page
    cmp #$a0
    bne :+

is_reserved:
    sec
    rts

    ;Check if page is head of buffer, exit without deallocating if true
:   ldy bank
    ldx page
    jsr mem_is_head
    bcc :+
    rts         ;Is head of buffer, exit

    ;Page not reserved or head of buffer ...
    ;Find out position in mem_map corresponding to specified bank/page
:   lda bank
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
    tay                 ;Set offset, Y = page/8

    lda page
    and #7
    tax                 ;Set X to bitmask position

    lda bitmask,x
    and (TMP1_ADR),y
    beq exit            ;Page already free, exit
    
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
    beq next_page_link      ;Prev page null, shouldn't happen ...
   
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
    bne :+
    inc mem_blocks_free+1

:   rts

bank:
    .byt 0

page:
    .byt 0

prev:
    .byt 0,0

next:
    .byt 0,0

bitmask:
    .byt 1,2,4,8,16,32,64,128
.endproc

;**********************************************************
;Function name.......: mem_defrag
;Description.........: Defragments one mem page per invocation
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_defrag
    rts
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

start:
    .byt 0
len:
    .byt 0
last_char:
    .byt 0
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

default:
    jsr mem_crs_step_left

    lda CRS_BNK
    sta BNK_SEL
    lda #5
    sta CRS_ADR
    ldy CRS_IDX
    lda (CRS_ADR),y
    cmp #LF
    bne :+
    inc at_lf

:   stz CRS_ADR
    ldy #4
    lda (CRS_ADR),y
    sec
    sbc #1
    sta (CRS_ADR),y

    lda #5
    sta CRS_ADR
    ldy CRS_IDX  
    iny

:   lda (CRS_ADR),y
    dey
    sta (CRS_ADR),y
    iny
    iny
    cpy #251
    bne :-

    stz CRS_ADR

    ldy #4
    lda (CRS_ADR),y
    bne :+

    jsr mem_at_eof
    bcs :+
    jsr mem_crs_step_left

:   ldx at_lf
    clc
    lda #1
    sta mem_modified
    rts

at_lf:
    .byt 0

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

at_lf:
    .byt 0
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

at_lf:
    .byt 0
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

count:
    .byt 0,0,0
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

count:
    .byt 0,0

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

backup:
    .byt 0,0,0
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

backup:
    .byt 0,0,0
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
    
backup:
    .byt 0,0,0
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

backup:
    .byt 0,0,0
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

.proc mem_cur_col_inc
    inc mem_cur_col
    bne exit
    inc mem_cur_col+1
    bne exit
    inc mem_cur_col+2

exit:
    rts
.endproc

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

.proc mem_cur_col_ret
    lda #1
    sta mem_cur_col
    stz mem_cur_col+1
    stz mem_cur_col+2
    rts
.endproc

.proc mem_cur_line_inc
    inc mem_cur_line
    bne exit
    inc mem_cur_line+1
    bne exit
    inc mem_cur_line+2

exit:
    rts
.endproc

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

;GLOBAL VARIABLES

mem_modified:
    .byt 0

mem_blocks_free:
    .byt 0,0

mem_top:
    .byt 0

mem_cur_line:
    .byt 1,0,0

mem_cur_col:
    .byt 1,0,0

mem_map:
    .byt 0