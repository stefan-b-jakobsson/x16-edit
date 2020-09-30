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
    jsr KERNAL_CHROUT

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
    ;Clear and set color
    lda #0
    sta VERA_L
    lda #0
    sta VERA_M
    lda #(1<<4)
    sta VERA_H

    ldx #80
    ldy #32             ;Blank space
    lda #64             ;Color
    
:   sty VERA_D0
    sta VERA_D0
    dex
    bne :-

    ;Print program name
    lda #0
    sta VERA_L
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ldx #0
:   lda program_name,x
    beq :+
    sta VERA_D0
    inx
    jmp :-

:   jsr screen_print_mod_status

    ;Print current file name (centered)
    lda file_cur_filename_len
    beq new_buffer

    cmp #60
    bcc :+
    lda #60
    sta filename_len
    jmp :++

:   sta filename_len

:   sec
    lda #78
    sbc filename_len
    and #%11111110

    sta VERA_L
    stz VERA_M
    lda #(2<<4)
    sta VERA_H

    ldy #0
:   lda file_cur_filename,y
    sta VERA_D0
    iny
    cpy filename_len
    bne :-

    rts

new_buffer:
    lda #(78-10)
    sta VERA_L
    stz VERA_M
    lda #(2<<4)
    sta VERA_H

    ldy #0
:   lda new_buffer_msg,y
    sta VERA_D0
    iny
    cpy #10
    bne :-

    rts

program_name:
    .byt "x16 edit 0.0.4", 0

new_buffer_msg:
    .byt "new buffer"

filename_len:
    .byt 0

.endproc

.proc screen_print_mod_status
    ;Print modified status
    lda #77*2
    sta VERA_L
    stz VERA_M
    lda #(2<<4)
    sta VERA_H

    lda mem_modified
    beq :+

    lda #'m'
    sta VERA_D0
    lda #'o'
    sta VERA_D0
    lda #'d'
    sta VERA_D0
    rts

:   lda #32
    sta VERA_D0
    sta VERA_D0
    sta VERA_D0

    rts
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
    .byt "^g get help ^w write out ^y prev page ^k cut   ^p cur pos", 0 
row2:
    .byt "^x exit     ^o open file ^v next page ^u uncut ^f where is", 0
color:
    .byt 64,64,97,97,97,97,97,97,97,97,97,97,64,64,97,97,97,97,97,97,97,97,97,97,97,64,64,97,97,97,97,97,97,97,97,97,97,97,64,64,97,97,97,97,97,97,97,64,64,97,97,97,97,97,97,97,97,97,97,0
.endproc

;******************************************************************************
;Function name.......: screen_show_help
;Purpose.............: Displays help screen
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
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
.byt 72, 69, 76, 80, 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45
.byt 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45
.byt 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 10, 88, 49, 54
.byt 32, 69, 100, 105, 116, 32, 105, 115, 32, 97, 32, 116, 101, 120, 116, 32, 101, 100, 105, 116, 111, 114, 32, 102, 111, 114, 32, 116, 104, 101
.byt 32, 67, 111, 109, 109, 97, 110, 100, 101, 114, 32, 88, 49, 54, 32, 112, 108, 97, 116, 102, 111, 114, 109, 32, 119, 105, 116, 104, 32, 97
.byt 32, 108, 111, 111, 107, 32, 97, 110, 100, 32, 102, 101, 101, 108, 10, 105, 110, 115, 112, 105, 114, 101, 100, 32, 98, 121, 32, 71, 78, 85
.byt 32, 78, 97, 110, 111, 46, 10, 10, 67, 111, 109, 109, 97, 110, 100, 115, 32, 97, 114, 101, 32, 115, 101, 108, 101, 99, 116, 101, 100, 32
.byt 98, 121, 32, 67, 116, 114, 108, 43, 107, 101, 121, 32, 99, 111, 109, 98, 105, 110, 97, 116, 105, 111, 110, 115, 32, 97, 115, 32, 115, 101
.byt 116, 32, 111, 117, 116, 32, 98, 101, 108, 111, 119, 46, 32, 73, 110, 115, 116, 101, 97, 100, 32, 111, 102, 32, 10, 67, 116, 114, 108, 44
.byt 32, 121, 111, 117, 32, 109, 97, 121, 32, 112, 114, 101, 115, 115, 32, 97, 110, 100, 32, 114, 101, 108, 101, 97, 115, 101, 32, 69, 83, 67
.byt 32, 116, 111, 32, 101, 110, 116, 101, 114, 32, 99, 111, 109, 109, 97, 110, 100, 32, 115, 101, 108, 101, 99, 116, 32, 109, 111, 100, 101, 46
.byt 32, 83, 111, 109, 101, 32, 10, 102, 117, 110, 99, 116, 105, 111, 110, 115, 32, 97, 114, 101, 32, 97, 108, 115, 111, 32, 97, 118, 97, 105
.byt 108, 97, 98, 108, 101, 32, 118, 105, 97, 32, 97, 110, 32, 97, 108, 116, 101, 114, 110, 97, 116, 105, 118, 101, 32, 102, 117, 110, 99, 116
.byt 105, 111, 110, 32, 107, 101, 121, 46, 10, 10, 67, 116, 114, 108, 43, 71, 32, 32, 40, 70, 49, 41, 32, 32, 68, 105, 115, 112, 108, 97
.byt 121, 32, 116, 104, 105, 115, 32, 104, 101, 108, 112, 32, 116, 101, 120, 116, 10, 67, 116, 114, 108, 43, 88, 32, 32, 40, 70, 50, 41, 32
.byt 32, 80, 114, 111, 103, 114, 97, 109, 32, 101, 120, 105, 116, 10, 67, 116, 114, 108, 43, 87, 32, 32, 40, 70, 51, 41, 32, 32, 87, 114
.byt 105, 116, 101, 32, 116, 101, 120, 116, 32, 116, 111, 32, 102, 105, 108, 101, 10, 67, 116, 114, 108, 43, 79, 32, 32, 40, 70, 52, 41, 32
.byt 32, 79, 112, 101, 110, 32, 102, 105, 108, 101, 10, 67, 116, 114, 108, 43, 78, 32, 32, 32, 32, 32, 32, 32, 32, 78, 101, 119, 32, 98
.byt 117, 102, 102, 101, 114, 10, 67, 116, 114, 108, 43, 89, 32, 32, 32, 32, 32, 32, 32, 32, 80, 97, 103, 101, 85, 112, 10, 67, 116, 114
.byt 108, 43, 86, 32, 32, 32, 32, 32, 32, 32, 32, 80, 97, 103, 101, 68, 111, 119, 110, 10, 67, 116, 114, 108, 43, 75, 32, 32, 40, 70
.byt 53, 41, 32, 32, 67, 117, 116, 32, 99, 117, 114, 114, 101, 110, 116, 32, 108, 105, 110, 101, 32, 116, 111, 32, 99, 108, 105, 112, 98, 111
.byt 97, 114, 100, 10, 67, 116, 114, 108, 43, 66, 32, 32, 32, 32, 32, 32, 32, 32, 67, 111, 112, 121, 32, 99, 117, 114, 114, 101, 110, 116
.byt 32, 108, 105, 110, 101, 32, 116, 111, 32, 99, 108, 105, 112, 98, 111, 97, 114, 100, 10, 67, 116, 114, 108, 43, 85, 32, 32, 40, 70, 54
.byt 41, 32, 32, 85, 110, 99, 117, 116, 32, 40, 112, 97, 115, 116, 101, 41, 32, 102, 114, 111, 109, 32, 99, 108, 105, 112, 98, 111, 97, 114
.byt 100, 10, 67, 116, 114, 108, 43, 80, 32, 32, 40, 70, 55, 41, 32, 32, 67, 117, 114, 114, 101, 110, 116, 32, 112, 111, 115, 105, 116, 105
.byt 111, 110, 32, 40, 108, 105, 110, 101, 47, 99, 111, 108, 117, 109, 110, 41, 10, 67, 116, 114, 108, 43, 70, 32, 32, 40, 70, 56, 41, 32
.byt 32, 83, 101, 97, 114, 99, 104, 32, 97, 110, 100, 32, 102, 105, 110, 100, 32, 116, 101, 120, 116, 10, 10, 84, 104, 101, 32, 102, 111, 108
.byt 108, 111, 119, 105, 110, 103, 32, 99, 111, 109, 109, 97, 110, 100, 115, 32, 97, 114, 101, 32, 111, 110, 108, 121, 32, 97, 118, 97, 105, 108
.byt 97, 98, 108, 101, 32, 98, 121, 32, 102, 105, 114, 115, 116, 32, 112, 114, 101, 115, 115, 105, 110, 103, 32, 97, 110, 100, 32, 114, 101, 108
.byt 101, 97, 115, 105, 110, 103, 32, 69, 83, 67, 46, 10, 10, 49, 46, 46, 57, 32, 32, 32, 32, 83, 101, 116, 32, 116, 97, 98, 32, 119
.byt 105, 100, 116, 104, 10, 65, 32, 32, 32, 32, 32, 32, 32, 65, 117, 116, 111, 105, 110, 100, 101, 110, 116, 10, 77, 32, 32, 32, 32, 32
.byt 32, 32, 83, 104, 111, 119, 32, 109, 101, 109, 111, 114, 121, 32, 117, 115, 97, 103, 101, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10
.byt 10, 10, 10, 10, 10, 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45
.byt 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45
.byt 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 10, 88, 49, 54
.byt 32, 69, 100, 105, 116, 44, 32, 67, 111, 112, 121, 114, 105, 103, 104, 116, 32, 50, 48, 50, 48, 44, 32, 83, 116, 101, 102, 97, 110, 32
.byt 74, 97, 107, 111, 98, 115, 115, 111, 110, 46, 10, 10, 84, 104, 105, 115, 32, 112, 114, 111, 103, 114, 97, 109, 32, 105, 115, 32, 102, 114
.byt 101, 101, 32, 115, 111, 102, 116, 119, 97, 114, 101, 32, 97, 110, 100, 32, 99, 111, 109, 101, 115, 32, 119, 105, 116, 104, 32, 65, 66, 83
.byt 79, 76, 85, 84, 69, 76, 89, 32, 78, 79, 32, 87, 65, 82, 82, 65, 78, 84, 89, 46, 32, 89, 111, 117, 32, 109, 97, 121, 10, 114
.byt 101, 100, 105, 115, 116, 114, 105, 98, 117, 116, 101, 32, 97, 110, 100, 47, 111, 114, 32, 109, 111, 100, 105, 102, 121, 32, 105, 116, 32, 117
.byt 110, 100, 101, 114, 32, 116, 104, 101, 32, 116, 101, 114, 109, 115, 32, 111, 102, 32, 116, 104, 101, 32, 71, 78, 85, 32, 71, 101, 110, 101
.byt 114, 97, 108, 32, 80, 117, 98, 108, 105, 99, 32, 76, 105, 99, 101, 110, 115, 101, 10, 97, 115, 32, 112, 117, 98, 108, 105, 115, 104, 101
.byt 100, 32, 98, 121, 32, 116, 104, 101, 32, 70, 114, 101, 101, 32, 83, 111, 102, 116, 119, 97, 114, 101, 32, 70, 111, 117, 110, 100, 97, 116
.byt 105, 111, 110, 44, 32, 101, 105, 116, 104, 101, 114, 32, 118, 101, 114, 115, 105, 111, 110, 32, 51, 32, 111, 102, 32, 116, 104, 101, 32, 76
.byt 105, 99, 101, 110, 115, 101, 44, 10, 111, 114, 44, 32, 97, 116, 32, 121, 111, 117, 114, 32, 111, 112, 116, 105, 111, 110, 44, 32, 97, 110
.byt 121, 32, 108, 97, 116, 101, 114, 32, 118, 101, 114, 115, 105, 111, 110, 46, 32, 70, 111, 114, 32, 100, 101, 116, 97, 105, 108, 101, 100, 32
.byt 116, 101, 114, 109, 115, 32, 115, 101, 101, 32, 115, 117, 112, 112, 108, 101, 109, 101, 110, 116, 101, 100, 32, 102, 105, 108, 101, 10, 34, 108
.byt 105, 99, 101, 110, 115, 101, 34, 46, 32, 65, 108, 115, 111, 32, 97, 118, 97, 105, 108, 97, 98, 108, 101, 32, 102, 114, 111, 109, 32, 60
.byt 104, 116, 116, 112, 115, 58, 47, 47, 119, 119, 119, 46, 103, 110, 117, 46, 111, 114, 103, 47, 108, 105, 99, 101, 110, 115, 101, 115, 62, 46
.byt 10, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45
.byt 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45
.byt 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,  0

.endproc

;******************************************************************************
;Function name.......: screen_refresh
;Purpose.............: Refreshes all visible lines of text. Print will start 
;                      from memory address pointed to by zero page vectors 
;                      SCR_XXX
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
    lda #SCREEN_WIDTH
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
    cmp #LF             ;end of line, prepare for next line
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
    beq next_mem_page

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
    
    lda #SCREEN_WIDTH
    sta columns

    iny

    jmp loop

clean_stack:
    pla         ;Clean stack

exit:
    ;Clear end of last line by printing spaces
    ldx columns
    beq :++
    lda #32
:   sta VERA_D0
    dex
    bne :-

    ;Clear any remaining lines
:   inc VERA_M
    ldy rows
    beq exit_2

clearbottom:
    stz VERA_L  
    lda #32
    ldx #SCREEN_WIDTH
:   sta VERA_D0
    dex
    bne :-
    ldx #SCREEN_WIDTH
    inc VERA_M
    stz VERA_L
    dey
    bne clearbottom

exit_2:
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
;                      LNV_XXX
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
    lda #SCREEN_WIDTH
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
    beq next_page

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
;Purpose.............: Clears the editable part of screen
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
;Purpose.............: Prints a null terminated status message 
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

;******************************************************************************
;Function name.......: screen_clear_status
;Purpose.............: Clears status message
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
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