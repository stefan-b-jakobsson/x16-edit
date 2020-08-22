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
;Function name.......: screen_init
;Purpose.............: Initializes the screen
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_init
    ;Set screen mode text 80x60
    lda #$0f
    jsr KERNAL_PRINT

    ;Set ISO charset
    lda #1
    jsr KERNAL_SCREEN_SET_CHARSET

    ;Initial screen setup
    jsr screen_print_header
    jmp screen_print_footer

    rts
.endproc

;******************************************************************************
;Function name.......: screen_print_header
;Purpose.............: Prints program header at top of screen
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_print_header
    ;Prepare printing from topleft corner
    lda #0
    sta VERA_L
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ldx #0
:   lda msg,x
    beq bg_color
    sta VERA_D0
    inx
    jmp :-

bg_color:
    lda #1
    sta VERA_L
    lda #0
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ldx #80
    lda #64             ;Background color selected here
    
:   sta VERA_D0
    dex
    bne :-

    rts

msg:
  .byt 88, 49, 54, 32, 69, 100, 105, 116, 111, 114, 32, 48, 46, 48, 46, 49, 32, 45, 32, 91, 78, 79, 78, 65, 77, 69, 93,  0
.endproc

;******************************************************************************
;Function name.......: screen_print_footer
;Purpose.............: Prints program footer at bottom of screen
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_print_footer
    ;Prepare printing row 58
    lda #0
    sta VERA_L
    lda #58
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ;Printing row 58
    ldx #0
:   lda row1,x
    beq nextrow
    sta VERA_D0
    inx
    jmp :-

nextrow:
    ;Prepare printing row 59
    lda #0
    sta VERA_L
    lda #59
    sta VERA_M

    ;Printing row 59
    ldx #0
:   lda row2,x
    beq bg_color
    sta VERA_D0
    inx
    jmp :-

bg_color:
    ;Prepare setting bg color for row 58
    lda #1
    sta VERA_L
    lda #58
    sta VERA_M

    ;Setting bg color for row 58
    ldx #0
:   lda color,x
    beq bg_color_next
    sta VERA_D0
    inx
    jmp :-

bg_color_next:
    ;Prepare setting bg color for row 59
    lda #1
    sta VERA_L
    lda #59
    sta VERA_M
    
    ;Setting bg color for row 59
    ldx #0
:   lda color,x
    beq exit
    sta VERA_D0
    inx
    jmp :-

exit:
    rts

row1:
    .byt "^g get help ^s save file ^y prev page ^k cut   ^c cur pos", 0 
row2:
    .byt "^x exit     ^o open file ^v next page ^u uncut ^w where is", 0
color:
    .byt 64,64,97,97,97,97,97,97,97,97,97,97,64,64,97,97,97,97,97,97,97,97,97,97,97,64,64,97,97,97,97,97,97,97,97,97,97,97,64,64,97,97,97,97,97,97,97,64,64,97,97,97,97,97,97,97,97,97,97,0
.endproc

.proc screen_print_footer_helpscreen
    
.endproc

.proc screen_show_help
    jsr screen_clear

    lda #0
    sta VERA_L
    lda #2
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    lda #<msg
    sta TMP1_ADR
    lda #>msg
    sta TMP1_ADR+1

    ldy #0

loop:
    lda (TMP1_ADR),y
    beq exit

    cmp #LF
    bne :+

    stz VERA_L
    inc VERA_M
    jmp :++

:   sta VERA_D0

:   iny
    bne loop

    inc TMP1_ADR+1
    jmp loop

exit:
    rts

msg:
    .byt 88, 49, 54, 32, 69, 100, 105, 116, 32, 104, 101, 108, 112, 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 10, 88
    .byt 49, 54, 32, 69, 100, 105, 116, 32, 105, 115, 32, 97, 32, 115, 105, 109, 112, 108, 101, 32, 116, 101, 120, 116, 32, 101, 100, 105, 116, 111
    .byt 114, 32, 105, 110, 115, 112, 105, 114, 101, 100, 32, 98, 121, 32, 97, 110, 100, 32, 105, 109, 112, 108, 101, 109, 101, 110, 116, 105, 110, 103
    .byt 32, 116, 104, 101, 32, 108, 111, 111, 107, 32, 97, 110, 100, 10, 102, 101, 101, 108, 32, 111, 102, 32, 116, 104, 101, 32, 71, 78, 85, 32
    .byt 78, 97, 110, 111, 32, 97, 110, 100, 32, 85, 87, 32, 80, 105, 99, 111, 32, 116, 101, 120, 116, 32, 101, 100, 105, 116, 111, 114, 115, 46
    .byt 32, 72, 111, 119, 101, 118, 101, 114, 44, 32, 88, 49, 54, 32, 69, 100, 105, 116, 32, 100, 111, 101, 115, 32, 110, 111, 116, 10, 115, 117
    .byt 112, 112, 111, 114, 116, 32, 101, 118, 101, 114, 121, 32, 102, 117, 110, 99, 116, 105, 111, 110, 32, 105, 110, 32, 71, 78, 85, 32, 78, 97
    .byt 110, 111, 32, 97, 110, 100, 32, 85, 87, 32, 80, 105, 99, 111, 44, 32, 102, 111, 114, 32, 105, 110, 115, 116, 97, 110, 99, 101, 32, 115
    .byt 111, 102, 116, 32, 108, 105, 110, 101, 32, 98, 114, 101, 97, 107, 115, 46, 10, 10, 67, 111, 109, 109, 97, 110, 100, 115, 32, 97, 114, 101
    .byt 32, 109, 97, 100, 101, 32, 119, 105, 116, 104, 32, 107, 101, 121, 98, 111, 97, 114, 100, 32, 115, 104, 111, 114, 116, 99, 117, 116, 115, 44
    .byt 32, 101, 105, 116, 104, 101, 114, 32, 97, 32, 99, 111, 110, 116, 114, 111, 108, 45, 107, 101, 121, 32, 115, 101, 113, 117, 101, 110, 99, 101
    .byt 32, 111, 114, 10, 97, 110, 32, 97, 108, 116, 101, 114, 110, 97, 116, 105, 118, 101, 32, 102, 117, 110, 99, 116, 105, 111, 110, 32, 107, 101
    .byt 121, 44, 32, 97, 115, 32, 115, 112, 101, 99, 105, 102, 105, 101, 100, 32, 98, 101, 108, 111, 119, 46, 10, 10, 10, 94, 71, 32, 32, 40
    .byt 70, 49, 41, 32, 32, 68, 105, 115, 112, 108, 97, 121, 32, 116, 104, 105, 115, 32, 104, 101, 108, 112, 32, 116, 101, 120, 116, 10, 94, 88
    .byt 32, 32, 40, 70, 50, 41, 32, 32, 80, 114, 111, 103, 114, 97, 109, 32, 101, 120, 105, 116, 10,  0

.endproc

;******************************************************************************
;Function name.......: screen_refresh
;Purpose.............: Refreshes all visible lines of text. Print will start 
;                      from memory address pointed to by zero page vectors 
;                      SCR_BNK, SCR_ADR and SCR_IDX.
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_refresh
    ;Setup temporary vectors to traverse text to be printed
    lda SCR_BNK
    sta TMP1_BNK
    sta BNK_SEL

    lda SCR_ADR+1
    sta TMP1_ADR+1
    stz TMP1_ADR

    ;Number of columns per row to be printed
    lda #79
    sta columns

    ;Number of rows to be printed
    lda #54
    sta rows

    ;Prepare printing from column 0, row 2
    lda #0
    sta VERA_L
    lda #2
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ;Get mem page length, and store it
    ldy #4
    lda (TMP1_ADR),y
    sta len     

    ;Set offset +5 to skip mem page metadata
    lda #5
    sta TMP1_ADR

    ;Set char index to start printing from
    ldy SCR_IDX

loop:
    ;Check if we are at end of mem page, and if so go to next mem page
    cpy len
    bcs next_mem_page

    ;Get char to print
    lda (TMP1_ADR),y
    cmp #10             ;end of line, prepare for next line
    beq next_line

    ldx columns         ;If 0 we have printed all visible columns, skip_char
    beq skip_char
    
    sta VERA_D0         ;Otherwise let's print char to screen
    dec columns

skip_char:
    iny
    jmp loop

next_mem_page:
    stz TMP1_ADR        ;Set offset to 0 to read mem page metadata

    ldy #2
    lda (TMP1_ADR),y    ;Next bank
    pha
    ldy #3
    lda (TMP1_ADR),y    ;Next page
    beq clean_stack     ;Next page = 0 => We're at end of file, clean stack and exit
    
    sta TMP1_ADR+1

    pla
    sta BNK_SEL
    sta TMP1_BNK

    ldy #4
    lda (TMP1_ADR),y    ;Next page len
    sta len

    lda #5              ;Set offset to 5 to skip mem page metadata
    sta TMP1_ADR

    ldy #0              ;Start reading from first char in next mem page

    jmp loop

next_line:
    ;Clear end of line by printing spaces
    ldx columns
    beq :++
    lda #32
:   sta VERA_D0
    dec columns
    bne :-

:   dec rows
    beq exit

    inc VERA_M
    stz VERA_L
    
    lda #79
    sta columns

    iny

    jmp loop

clean_stack:
    pla         ;Clean stack

exit:
    ;Clear end of last line by printing spaces
    lda #32
:   sta VERA_D0
    dec columns
    bne :-

    rts

len:
    .byt 0

columns:
    .byt 0

rows:
    .byt 0

.endproc

;******************************************************************************
;Function name.......: screen_println
;Purpose.............: Prints the currently edited line. Print will start from
;                      memory address pointed to by zero page vectors
;                      LNV_BNK, LNV_ADR and LNV_IDX.
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_println
    ;Prepare printing from first column of current row
    lda #0
    sta VERA_L
    lda CRS_Y
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ;Setup temporary vectors where printing starts
    lda LNV_ADR+1
    sta TMP1_ADR+1
    
    lda LNV_BNK
    sta TMP1_BNK
    sta BNK_SEL

    ;Get length of mem_page where printing starts
    ldy #4
    lda (LNV_ADR),y
    sta page_len

    ;Offset 5 to skip mem page metadata
    lda #5
    sta TMP1_ADR

    ;Set chars to print
    lda #79
    sta counter

    ;Get start index
    ldy LNV_IDX

print_loop:
    ;Check if we are at end of mem page, if so get next mem page
    cpy page_len
    bcs next_page

    ;Get char to print
    lda (TMP1_ADR),y
    cmp #LF                 ;Abort if we get lf
    beq clean_line

    sta VERA_D0             ;Otherwise output char

    dec counter            
    beq clean_line          ;If counter = 0, we're done

    iny
    jmp print_loop

next_page:
    stz TMP1_ADR            ;Restore to read mem page metadata
    
    ldy #2
    lda (TMP1_ADR),y        ;Next bank
    pha
    ldy #3
    lda (TMP1_ADR),y        ;Next page
    beq clean_stack         ;If 0, we're at enf of file, clean stack and exit

    sta TMP1_ADR+1
    pla
    sta TMP1_BNK
    sta BNK_SEL

    ;Get next page len
    ldy #4         
    lda (TMP1_ADR),y
    sta page_len

    ;Set offset to skip mem page metadata
    lda #5
    sta TMP1_ADR

    ;Start reading from first index
    ldy #0

    jmp print_loop

clean_stack:
    pla

clean_line:
    ;Fills end of line with spaces
    lda #32

clean_line_loop:
    sta VERA_D0
    ldx counter
    beq exit
    dec counter
    jmp clean_line_loop
    
exit:
    rts

page_len:
    .byt 0

counter:
    .byt 0

.endproc

;******************************************************************************
;Function name.......: screen_clear
;Purpose.............: Clears the part of screen containing text
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_clear
    lda #0
    sta VERA_L
    lda #2
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    lda #32
    ldx #80
    ldy #54

loop:
    sta VERA_D0
    dex
    bne loop
    stz VERA_L
    inc VERA_M
    ldx #80
    dey
    bne loop

    rts
.endproc

;******************************************************************************
;Function name.......: screen_print_status
;Purpose.............: Prints a null terminated status message of max 80 chars
;Preparatory routines: None
;Input...............: X=pointer to message LSB, Y=pointer to message MSB
;Returns.............: Nothing
;Error returns.......: None
.proc screen_print_status
    stx TMP1_ADR
    sty TMP1_ADR+1
    stz len

    ;Get message len
    ldy #0
:   lda (TMP1_ADR),y
    cmp #0
    beq continue
    iny
    cpy #80
    beq continue    ;Don't allow messages over 80 chars long
    jmp :-

continue:
    sty len

    ;Prepare printing
    lda #80
    sec
    sbc len
    and #%11111110
    sta VERA_L
    lda #57
    sta VERA_M
    lda #(1<<4)
    sta VERA_H

    ldy #0
    ldx #160

loop:
    lda len
    beq exit

    lda (TMP1_ADR),y
    sta VERA_D0         ;Char
    stx VERA_D0         ;Color
    iny
    dec len
    bne loop

exit:
    rts

len:
    .byt 0

start_at:
    .byt 0

.endproc

.proc screen_clear_status
    stz VERA_L
    lda #57
    sta VERA_M
    lda #(1<<4)
    sta VERA_H

    ldx #80
    lda #32 ;Blank space
    ldy #97 ;Color

:   sta VERA_D0
    sty VERA_D0
    dex
    bne :-
    rts
.endproc