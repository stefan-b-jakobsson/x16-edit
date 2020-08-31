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
;General description of the memory model
;---------------------------------------
;Text entered by the user is stored in banked RAM.
;All available banks are used, and the text may be almost
;2 MB of size.
;
;The memory isn't organized as one continuous string since
;this would make the operation of the program too slow.
;This is the especially true for large text if you like
;to insert new text at the top. The program would then
;need to copy all of the text one step forward to make
;room for the text to be inserted.
;
;To avoid that, the memory is divided into memory
;pages of 256 bytes each. For example, bank 1, address
;range $a000 to $a0ff is a memory page.
;
;Each memory page contains a separate string.
;
;The memory pages are doubly-linked to each other,
;holding a reference both to the previous and to the next
;memory page. Each memory page also holds the length
;of the text stored within it.
;
;The memory pages are dynamically linked throughout the
;operation of the program, which makes it possible to
;avoid copying large amounts of data when new text is
;inserted. The program just links in a new memory page
;if needed.
;
;Each memory page contains the following fields:
;
;Offset Content
;00     Previous page / Bank number
;01     Previous page / AddressH
;02     Next page / Bank number
;03     Next page / AddressH
;04     Length of text stored in the memory page
;05-ff  Text (max 251 bytes)
;
;The memory model is somewhat inspired by the
;1541 disk format.
;
;The program uses the following three zero page pointers
;to the memory in order to display the text:
;
;1. CRS_BNK + CRS_ADR + CRS_IDX (Cursor location in memory)
;2. LNV_BNK + LNV_ADR + LNV_IDX (First visible char of the currently edited line)
;3. SCR_BNK + SCR_ADR + SCR_IDX (First visible char of the screen)
;
;LNV_ is used to scroll the current line horizontally
;SCR_ us used to scroll the text vertically


;**********************************************************
;Function name.......: mem_init
;Description.........: Initializes program's memory
;                      usage, and allocates the first
;                      page of banked RAM.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_init
    ;Clear memory memory map, mark all mem pages as free
    ldy #0
    lda #0

:   sta mem_map,y
    sta mem_map+256,y
    sta mem_map+512,y
    sta mem_map+768,y
    iny
    bne :-

    ;Mark all mem pages in bank 0 as used, these may be used by the Kernal
    lda #255
    sta mem_map
    sta mem_map+1
    sta mem_map+2
    sta mem_map+3

    ;Mark the initial memory page (bank 1, page $a0) as used
    lda #1
    sta mem_map+4

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
    sta (CRS_ADR),y     ;Len = 0

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

    rts

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
;Function name.......: mem_push
;Description.........: Pushes content of a mem page
;                      one step forward
;Input...............: Y=mem bank, X=mem page, A=index to start pushing from
;Returns.............: C=1 if overflow; overflow char returned in .A
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

    jmp set_page_len

push:
    ;Set offset 5 to skip mem page metadata
    lda #5
    sta TMP1_ADR

    ldy #249

loop:
    lda (TMP1_ADR),y
    iny
    sta (TMP1_ADR),y
    dey
    cpy start_push
    beq set_page_len
    dey
    jmp loop

set_page_len:
    ;Restore offset to get mem page metadata
    stz TMP1_ADR

    lda page_len
    cmp #251
    bcc no_overflow

    ;Page len is >=251, return overflow without increasing mem page len
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
;Procedure name......: mem_insert
;Description.........: Inserts a char at cursor location
;Input...............: A=char to insert
;Returns.............: Nothing
;Error returns.......: Nothing
.proc mem_insert
    ;Save char on stack
    pha

    ;Push mem page content to make room for new char
    ldy CRS_BNK
    ldx CRS_ADR+1
    lda CRS_IDX
    jsr mem_push
    bcc store_char

    ;Handle overflow from mem page push
    pha

    lda CRS_BNK
    sta BNK_SEL

    ;Get next mem page, if next page null, alloc new mem page
    ldy #2
    lda (CRS_ADR),y     ;bank
    tax
    ldy #3
    lda (CRS_ADR),y     ;page
    beq alloc_new
    
    sta TMP1_ADR+1
    stz TMP1_ADR
    
    stx TMP1_BNK
    stx BNK_SEL

    ;Get next page len, if it's full, alloc new page
    ldy #4
    lda (TMP1_ADR),y
    cmp #251
    bcs alloc_new

    ;Push next page content
    ldy TMP1_BNK
    ldx TMP1_ADR+1
    lda #0
    jsr mem_push 

    ldy #5              ;Store overflow char at beginning of next page
    pla                 ;Pull back overflow char
    sta (TMP1_ADR),y

    jmp store_char

alloc_new:
    jsr mem_alloc       ;Next page was not allocated or was full, we're allocating a new page

    sty BNK_SEL
    stx TMP1_ADR+1
    stz TMP1_ADR

    pla                 ;Pull back overflow char (02)
    ldy #5
    sta (TMP1_ADR),y    ;Store overflow char at beginning of the new page

    ldy #4
    lda #1
    sta (TMP1_ADR),y    ;Set new page len = 1

store_char:    
    lda CRS_BNK         ;And now, finally, store that char we liked to insert
    sta BNK_SEL

    lda #5              ;Set offset=5 to skip mem page metadata
    sta CRS_ADR
    pla                 ;Pull back char to insert
    ldy CRS_IDX
    sta (CRS_ADR),y

    stz CRS_ADR         ;Restore offset

    ;Move cursor memory pointer
    jsr mem_crs_step_right

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
    ;First take one step left
    jsr mem_crs_step_left
    bcc continue              
    
    ;C=1; we're at start of file
    ldx #0
    sec

    rts

continue:
    ;Get page len
    lda CRS_BNK
    sta BNK_SEL
    
    ldy #4
    lda (CRS_ADR),y
    sta page_len
    
    ldy CRS_IDX
    iny
    cpy page_len
    bcs at_end_of_mempage

    ;Not at end of mem page, prepare copying text one step to left in mem page
    lda CRS_ADR+1
    sta TMP1_ADR+1

    lda #4
    sta CRS_ADR

    lda #5
    sta TMP1_ADR

loop:
    ;Loop to move text from the cursor to the end of the mem page one step left
    lda (TMP1_ADR),y
    sta (CRS_ADR),y

    cpy page_len
    beq at_end_of_mempage

    iny
    jmp loop

at_end_of_mempage:
    ;If we're at end of the text stored in this mem page we don't need to move any text, just decrease page len by 1
    stz CRS_ADR

    ldy #4
    dec page_len
    lda page_len
    sta (CRS_ADR),y

    clc
    rts

page_len:
    .byt 0
.endproc

;**********************************************************
;Procedure name......: mem_crs_step_right
;Description.........: Moves the pointer to memory location
;                      of the cursor one step right
;Input...............: Nothing
;Returns.............: .C=1 if eof else .C=0
;                      .X=1 if LF else .X=0
;Error returns.......: Nothing
.proc mem_crs_step_right
    ;Get mem page len
    lda CRS_BNK
    sta BNK_SEL

    ldy #3
    lda (CRS_ADR),y
    sta next_page
    
    ;Try to get next page if crs_idx >=250
    lda CRS_IDX
    cmp #250
    bcs goto_next_page

    lda next_page
    beq at_last_page

    ;Index < 250
    ldy #4
    lda CRS_IDX
    cmp (CRS_ADR),y
    bcs goto_next_page

    ldx #0         ;Set default return value, not at end of line

    lda #5
    sta CRS_ADR
    ldy CRS_IDX
    lda (CRS_ADR),y
    cmp #LF
    bne :+
    ldx #1          ;At end of line, return X=1
:   inc CRS_IDX
    stz CRS_ADR     ;Restore to default value
    clc
    rts

at_last_page:
    ;We're at last mem page, increase crs_idx, go one beyond last char position if we are at end of file
    ldy #4
    lda CRS_IDX
    cmp (CRS_ADR),y
    bcs :++

    ldx #0          ;Set default return value, not at end of line
    lda #5
    sta CRS_ADR
    ldy CRS_IDX
    lda (CRS_ADR),y
    cmp #LF
    bne :+
    ldx #1          ;At end of line, return X=1
:   stz CRS_ADR     ;Restore to default value
    inc CRS_IDX
    clc
    rts

:   sta CRS_IDX
    ldx #0
    sec
    rts

goto_next_page:
    ldx #0          ;Set default return value
    lda #5
    sta CRS_ADR
    ldy CRS_IDX
    lda (CRS_ADR),y
    cmp #LF
    bne :+
    ldx #1          ;At line break, return X=1

:   stz CRS_ADR     ;Restore default value

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

    ldy #4
    lda (CRS_ADR),y
    beq goto_next_page

    stz CRS_IDX

    clc
    rts

eof:
    ;Clean stack
    pla

    jsr mem_alloc
    sty CRS_BNK
    stx CRS_ADR+1
    stz CRS_IDX

    ldx #0
    sec
    rts

next_page:
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
    lda CRS_BNK
    sta BNK_SEL

    ;Check if we are at beginning of mem page, if so get previous page
    lda CRS_IDX
    cmp #0
    beq get_prev_page

    ;Otherwise just decrease index
    dec CRS_IDX

    ldx #0
    lda CRS_IDX
    clc
    adc #5
    tay
    lda (CRS_ADR),y
    cmp #LF
    bne :+
    ldx #1
:   clc
    rts

get_prev_page:
    ;Get previous bank
    ldy #0
    lda (CRS_ADR),y
    tax

    ;Get previous mem page, check if at beginning of file
    ldy #1
    lda (CRS_ADR),y
    beq bof

    ;Set cursor to previous bank and mem page
    sta CRS_ADR+1
    stx CRS_BNK
    stx BNK_SEL

    ;Get page len
    ldy #4
    lda (CRS_ADR),y

    ;Len = 0?
    beq prev_page_empty

    ;Set index to last char = page len - 1
    sec
    sbc #1
    sta CRS_IDX

    ldx #0
    lda CRS_IDX
    clc
    adc #5
    tay
    lda (CRS_ADR),y
    cmp #LF
    bne :+
    ldx #1
:   clc
    rts

prev_page_empty:
    ;Try again!
    jmp get_prev_page

bof:
    ;At beginning of file, set cursor index=0 and exit
    stz CRS_IDX
    ldx #0
    sec
    rts

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

loop:
    stz CRS_ADR

    jsr mem_crs_step_left
    bcs exit

    inc count
    bne :+
    inc count+1

    lda CRS_BNK
    sta BNK_SEL

:   ldy CRS_IDX
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

exit:
    ldx count
    ldy count+1
    
    rts

count:
    .byt 0,0
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
    beq at_enter

    stz CRS_ADR
    jsr mem_crs_step_right
    bcs exit

    inc count
    bne loop
    inc count+1

    jmp loop

at_enter:
    ;jsr mem_crs_step_left
    bcs exit

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
    sta TMP1_BNK
    lda CRS_ADR+1
    sta TMP1_ADR+1
    lda CRS_IDX
    sta TMP1_IDX

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
    lda TMP1_BNK
    sta CRS_BNK
    lda TMP1_ADR+1
    sta CRS_ADR+1
    lda TMP1_IDX
    sta CRS_IDX
    
    rts
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
    sta TMP1_BNK
    lda CRS_ADR+1
    sta TMP1_ADR+1
    lda CRS_IDX
    sta TMP1_IDX

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
    lda TMP1_BNK
    sta CRS_BNK
    lda TMP1_ADR+1
    sta CRS_ADR+1
    lda TMP1_IDX
    sta CRS_IDX
    
    rts
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
    sta crs_backup
    lda CRS_ADR+1
    sta crs_backup+1
    lda CRS_IDX
    sta crs_backup+2

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

    lda crs_backup
    sta CRS_BNK
    lda crs_backup+1
    sta CRS_ADR+1
    lda crs_backup+2
    sta CRS_IDX
    
    rts
    
crs_backup:
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
    sta crs_backup
    lda CRS_ADR+1
    sta crs_backup+1
    lda CRS_IDX
    sta crs_backup+2

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

    lda crs_backup
    sta CRS_BNK
    lda crs_backup+1
    sta CRS_ADR+1
    lda crs_backup+2
    sta CRS_IDX
    
    rts

crs_backup:
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

;GLOBAL VARIABLES

mem_map:
    .byt 0