;******************************************************************************
;Copyright 2020-2021, Stefan Jakobsson.
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
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_init
    ;jsr charset_detect
    jsr screen_charset_detect
    sta screen_mode
    bcc :+

    ;Unknown charset, set ISO charset
    bridge_setaddr KERNAL_CHROUT
    lda #$0f
    bridge_call KERNAL_CHROUT
    stz screen_mode

    ;Set default color
:   lda #97
    sta screen_color

    ;Clear screen
    jsr screen_clearall

    ;Initial screen setup
    jsr screen_print_header
    jsr screen_print_default_footer
    rts
.endproc

;******************************************************************************
;Function name.......: screen_charset_detect
;Purpose.............: Detects current char set
;Input...............: None
;Returns.............: A=0: ISO
;                      A=1: PETSCII upper case
;                      A=2: PETSCII lower case
;Error returns.......: C=1: Unknown charset
.proc screen_charset_detect
    ;VERA layer 0 or 1 active?
    lda VERA_VIDEO
    and #%00010000
    bne layer0
    lda VERA_VIDEO
    and #%00100000
    bne layer1

    ;Both layers inactive, return error. Can this happen? Better safe than sorry.
    sec
    rts

layer0:
    ;Get VERA tile base for layer 0
    lda VERA_L0_TILEBASE
    jmp detect

layer1:
    ;Get VERA tile base for layer 1
    lda VERA_L1_TILEBASE

detect:
    ;Set VERA mid (TILEBASE*2, discarding the lowest two bits in TILEBASE)
    and #%11111100
    asl
    sta VERA_M

    ;Set VERA low and high
    lda #8      ;8 => second char, as first char is the same in petscii uc and lc
    sta VERA_L  
    stz VERA_H

    ;Get first byte of the char definition, and use that value to detect charset mode
    lda VERA_D0
    cmp #$18
    beq petscii_uc
    cmp #$00
    beq petscii_lc
    cmp #$ff
    beq iso

unknown:
    sec
    rts

petscii_uc:
    ;PETSCII upper case and graphics
    clc
    lda #1
    rts

petscii_lc:
    ;PETSCII lower and upper case
    clc
    lda #2
    rts

iso:
    ;ISO charset
    clc
    lda #0
    rts

.endproc

;******************************************************************************
;Function name.......: screen_print_header
;Purpose.............: Prints program header at top of screen
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
name_loop:
    lda program_name,x
    beq name_done
    jsr screen_put_uc_char
    inx
    jmp name_loop

name_done:
    ;Print current file name (centered)
    lda file_cur_filename_len
    beq new_buffer

    ;Ensure file name length is less than 60
    cmp #60
    bcc :+
    lda #60
    sta filename_len
    jmp :++

:   sta filename_len

    ;Calulate where to start printing file name
:   sec
    lda #80
    sbc filename_len
    and #%11111110

    sta VERA_L
    stz VERA_M
    lda #(2<<4)
    sta VERA_H

    ldy #0
fn_loop:
    lda file_cur_filename,y
    jsr screen_put_char
    iny
    cpy filename_len
    bne fn_loop
    rts

new_buffer:
    ;Buffer not yet saved. Prints "NEW BUFFER" instead of a file name
    lda #(80-10)
    sta VERA_L
    stz VERA_M
    lda #(2<<4)
    sta VERA_H

    ldy #0
nb_loop:
    lda new_buffer_msg,y
    jsr screen_put_uc_char
    iny
    cpy #10
    bne nb_loop
    rts

program_name:
    .byt "x16 edit 0.3.3", 0

new_buffer_msg:
    .byt "new buffer"

filename_len = tempvars     ;1 byte
.endproc

;******************************************************************************
;Function name.......: screen_print_mod_status
;Purpose.............: Refresh buffer modified status at top right corner of
;                      screen
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_print_mod_status
    ;Set VERA address to print at first row column 77
    lda #77*2
    sta VERA_L
    stz VERA_M
    lda #(2<<4)
    sta VERA_H

    ;Get mod status
    lda mem_modified
    beq clear

    ;Print
    lda #'m'
    jsr screen_put_uc_char
    lda #'o'
    jsr screen_put_uc_char
    lda #'d'
    jsr screen_put_uc_char
    rts

clear:
    lda #32
    sta VERA_D0
    sta VERA_D0
    sta VERA_D0

    rts
.endproc


;******************************************************************************
;Function name.......: screen_print_footer
;Purpose.............: Prints program footer at two last rows of screen
;Input...............: Pointer to two consequtive null terminated strings
;                      one for each footer row, X=AddressL, Y=AddressH
;Returns.............: Nothing
;Error returns.......: None
.proc screen_print_footer
    ;Set vector to strings
    stx TMP1_ADR
    sty TMP1_ADR+1

    ;Set VERA address to print from start of row 58
    lda #0
    sta VERA_L
    lda #58
    sta VERA_M
    lda #(1<<4)
    sta VERA_H

    ldy #255
    ldx #0
    lda (TMP1_ADR),y
printloop:
    iny
    lda (TMP1_ADR),y
    beq nextrow
    cmp #':'
    bne :+
    ldx #2
    bra printloop

:   jsr screen_put_uc_char

    cpx #0      ;If X>0, print with different background color
    bne :+
    inc VERA_L
    bra printloop

:   dex
    lda #64
    sta VERA_D0
    bra printloop

nextrow:
    stz VERA_L
    inc VERA_M
    lda VERA_M
    cmp #60             ;End if we are at row 60
    bcc printloop

    rts
.endproc

;******************************************************************************
;Function name.......: screen_clear_footer
;Purpose.............: Clears program footer at two last rows of screen
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_clear_footer
     ;Set VERA address to print from start of row 58
    lda #0
    sta VERA_L
    lda #58
    sta VERA_M
    lda #(1<<4)
    sta VERA_H

    ldy #0
    lda #32
    ldx screen_color
:   sta VERA_D0
    stx VERA_D0
    iny
    cpy #80
    bne :-

    inc VERA_M
    stz VERA_L
    ldy VERA_M
    cpy #60
    beq exit
    ldy #0
    bra :-

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: screen_print_default_footer
;Purpose.............: Prints program footer at two last rows of screen
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_print_default_footer
    jsr screen_clear_footer

    ldx #<row1
    ldy #>row1
    jmp screen_print_footer

row1:
    .byt ":^g get help :^o write out :^y prev page :^k cut   :^c cur pos", 0 
row2:
    .byt ":^x exit     :^r open file :^v next page :^u uncut :^w where is", 0
.endproc

;******************************************************************************
;Function name.......: screen_print_file_ctx_footer
;Purpose.............: Prints program footer at two last rows of screen used
;                      when in file context
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_print_file_ctx_footer
    jsr screen_clear_footer

    ldx #<row1
    ldy #>row1
    jmp screen_print_footer
row1:
    .byt ":^t to files", 0 
row2:
    .byt 0
.endproc

;******************************************************************************
;Function name.......: screen_print_dir_ctx_footer
;Purpose.............: Prints program footer at two last rows of screen
;                      when in directory context
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_print_dir_ctx_footer
    jsr screen_clear_footer

    ldx #<row1
    ldy #>row1
    jmp screen_print_footer
row1:
    .byt ":^y page up", 0
row2:
    .byt ":^v page down",0
.endproc


;******************************************************************************
;Function name.......: screen_show_help
;Purpose.............: Displays help screen
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_show_help
    ;Clear editable part of screen
    jsr screen_clear

    ;Set VERA address to start printing from start of row 2
    lda #0
    sta VERA_L
    lda #2
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ;Set zero page vector to help screen content
    lda #<msg
    sta TMP1_ADR
    lda #>msg
    sta TMP1_ADR+1

    ldy #0
loop:
    ;Get next char
    lda (TMP1_ADR),y
    beq exit

    cmp #LF
    bne check_mode

    ;Line feed
    stz VERA_L
    inc VERA_M
    jmp step

check_mode:
    ldx screen_mode
    bne petscii

print:
    sta VERA_D0

step:
    iny
    bne loop

    inc TMP1_ADR+1
    jmp loop

exit:
    rts

petscii:
    cpx #2
    beq mod_pet_lc
    .scope
        charset_ascii_to_petscii_ucase
    .endscope
    jmp print

mod_pet_lc:
    .scope
        charset_ascii_to_petscii_lcase
    .endscope
    jmp print

;Help screen content (ASCII/ISO encoded nullterminated string)
msg:
    .byt 72, 69, 76, 80, 10, 10, 10, 88, 49, 54, 32, 69, 100, 105, 116, 32, 105, 115, 32, 97, 32, 116, 101, 120, 116, 32, 101, 100, 105, 116
    .byt 111, 114, 32, 102, 111, 114, 32, 116, 104, 101, 32, 67, 111, 109, 109, 97, 110, 100, 101, 114, 32, 88, 49, 54, 32, 112, 108, 97, 116, 102
    .byt 111, 114, 109, 32, 119, 105, 116, 104, 32, 97, 32, 108, 111, 111, 107, 32, 97, 110, 100, 32, 102, 101, 101, 108, 10, 105, 110, 115, 112, 105
    .byt 114, 101, 100, 32, 98, 121, 32, 71, 78, 85, 32, 78, 97, 110, 111, 46, 10, 10, 84, 104, 101, 32, 101, 100, 105, 116, 111, 114, 32, 105
    .byt 115, 32, 109, 111, 100, 101, 108, 101, 115, 115, 46, 32, 67, 111, 109, 109, 97, 110, 100, 115, 32, 97, 114, 101, 32, 115, 101, 108, 101, 99
    .byt 116, 101, 100, 32, 98, 121, 32, 67, 116, 114, 108, 43, 107, 101, 121, 32, 115, 101, 113, 117, 101, 110, 99, 101, 115, 46, 32, 89, 111, 117
    .byt 32, 109, 97, 121, 10, 97, 108, 116, 101, 114, 110, 97, 116, 105, 118, 101, 108, 121, 32, 112, 114, 101, 115, 115, 32, 97, 110, 100, 32, 114
    .byt 101, 108, 101, 97, 115, 101, 32, 69, 83, 67, 44, 32, 97, 110, 100, 32, 116, 104, 101, 32, 110, 101, 120, 116, 32, 107, 101, 121, 32, 119
    .byt 105, 108, 108, 32, 98, 101, 32, 105, 110, 116, 101, 114, 112, 114, 101, 116, 101, 100, 32, 97, 115, 10, 97, 32, 99, 111, 109, 109, 97, 110
    .byt 100, 46, 32, 83, 111, 109, 101, 32, 102, 117, 110, 99, 116, 105, 111, 110, 115, 32, 97, 114, 101, 32, 97, 108, 115, 111, 32, 97, 118, 97
    .byt 105, 108, 97, 98, 108, 101, 32, 118, 105, 97, 32, 97, 32, 102, 117, 110, 99, 116, 105, 111, 110, 32, 107, 101, 121, 46, 10, 10, 94, 71
    .byt 32, 32, 40, 70, 49, 41, 32, 32, 68, 105, 115, 112, 108, 97, 121, 32, 98, 117, 105, 108, 116, 45, 105, 110, 32, 104, 101, 108, 112, 32
    .byt 115, 99, 114, 101, 101, 110, 10, 94, 88, 32, 32, 40, 70, 50, 41, 32, 32, 69, 120, 105, 116, 32, 112, 114, 111, 103, 114, 97, 109, 10
    .byt 94, 79, 32, 32, 40, 70, 51, 41, 32, 32, 87, 114, 105, 116, 101, 32, 116, 101, 120, 116, 32, 98, 117, 102, 102, 101, 114, 32, 116, 111
    .byt 32, 102, 105, 108, 101, 10, 94, 82, 32, 32, 40, 70, 53, 41, 32, 32, 79, 112, 101, 110, 32, 97, 110, 100, 32, 114, 101, 97, 100, 32
    .byt 102, 105, 108, 101, 32, 105, 110, 116, 111, 32, 116, 101, 120, 116, 32, 98, 117, 102, 102, 101, 114, 10, 94, 78, 32, 32, 32, 32, 32, 32
    .byt 32, 32, 67, 114, 101, 97, 116, 101, 32, 110, 101, 119, 32, 116, 101, 120, 116, 32, 98, 117, 102, 102, 101, 114, 10, 94, 89, 32, 32, 40
    .byt 70, 55, 41, 32, 32, 80, 97, 103, 101, 85, 112, 10, 94, 86, 32, 32, 40, 70, 56, 41, 32, 32, 80, 97, 103, 101, 68, 111, 119, 110
    .byt 10, 94, 75, 32, 32, 32, 32, 32, 32, 32, 32, 67, 117, 116, 32, 99, 117, 114, 114, 101, 110, 116, 32, 108, 105, 110, 101, 32, 116, 111
    .byt 32, 99, 108, 105, 112, 98, 111, 97, 114, 100, 10, 94, 80, 32, 32, 32, 32, 32, 32, 32, 32, 67, 111, 112, 121, 32, 99, 117, 114, 114
    .byt 101, 110, 116, 32, 108, 105, 110, 101, 32, 116, 111, 32, 99, 108, 105, 112, 98, 111, 97, 114, 100, 10, 94, 85, 32, 32, 32, 32, 32, 32
    .byt 32, 32, 85, 110, 99, 117, 116, 32, 40, 112, 97, 115, 116, 101, 41, 32, 97, 108, 108, 32, 99, 111, 110, 116, 101, 110, 116, 32, 102, 114
    .byt 111, 109, 32, 99, 108, 105, 112, 98, 111, 97, 114, 100, 32, 116, 111, 32, 116, 101, 120, 116, 32, 98, 117, 102, 102, 101, 114, 10, 94, 67
    .byt 32, 32, 32, 32, 32, 32, 32, 32, 83, 104, 111, 119, 32, 99, 117, 114, 114, 101, 110, 116, 32, 112, 111, 115, 105, 116, 105, 111, 110, 32
    .byt 40, 108, 105, 110, 101, 47, 99, 111, 108, 117, 109, 110, 41, 10, 94, 87, 32, 32, 40, 70, 54, 41, 32, 32, 83, 101, 97, 114, 99, 104
    .byt 32, 97, 110, 100, 32, 102, 105, 110, 100, 32, 116, 101, 120, 116, 32, 40, 99, 97, 115, 101, 32, 115, 101, 110, 115, 105, 116, 105, 118, 101
    .byt 41, 10, 94, 83, 32, 32, 32, 32, 32, 32, 32, 32, 82, 101, 112, 108, 97, 99, 101, 32, 111, 110, 101, 32, 111, 114, 32, 97, 108, 108
    .byt 32, 111, 99, 99, 117, 114, 101, 110, 99, 101, 115, 32, 111, 102, 32, 97, 32, 115, 116, 114, 105, 110, 103, 32, 40, 99, 97, 115, 101, 32
    .byt 115, 101, 110, 115, 105, 116, 105, 118, 101, 41, 10, 94, 76, 32, 32, 32, 32, 32, 32, 32, 32, 71, 111, 32, 116, 111, 32, 108, 105, 110
    .byt 101, 32, 110, 117, 109, 98, 101, 114, 10, 94, 65, 32, 32, 32, 32, 32, 32, 32, 32, 84, 111, 103, 103, 108, 101, 32, 97, 117, 116, 111
    .byt 32, 105, 110, 100, 101, 110, 116, 32, 111, 110, 47, 111, 102, 102, 10, 94, 90, 32, 32, 32, 32, 32, 32, 32, 32, 84, 111, 103, 103, 108
    .byt 101, 32, 119, 111, 114, 100, 32, 119, 114, 97, 112, 32, 111, 110, 47, 111, 102, 102, 10, 94, 69, 32, 32, 32, 32, 32, 32, 32, 32, 67
    .byt 104, 97, 110, 103, 101, 32, 99, 104, 97, 114, 115, 101, 116, 10, 94, 68, 32, 32, 32, 32, 32, 32, 32, 32, 83, 101, 116, 32, 102, 105
    .byt 108, 101, 32, 115, 116, 111, 114, 97, 103, 101, 32, 100, 101, 118, 105, 99, 101, 32, 110, 117, 109, 98, 101, 114, 10, 94, 84, 32, 32, 32
    .byt 32, 32, 32, 32, 32, 83, 101, 116, 32, 116, 101, 120, 116, 32, 99, 111, 108, 111, 114, 32, 40, 100, 101, 102, 97, 117, 108, 116, 32, 119
    .byt 104, 105, 116, 101, 41, 10, 94, 66, 32, 32, 32, 32, 32, 32, 32, 32, 83, 101, 116, 32, 98, 97, 99, 107, 103, 114, 111, 117, 110, 100
    .byt 32, 99, 111, 108, 111, 114, 32, 40, 100, 101, 102, 97, 117, 108, 116, 32, 98, 108, 117, 101, 41, 10, 94, 77, 32, 32, 32, 32, 32, 32
    .byt 32, 32, 83, 104, 111, 119, 32, 109, 101, 109, 111, 114, 121, 32, 117, 115, 97, 103, 101, 32, 40, 49, 32, 98, 97, 110, 107, 61, 50, 53
    .byt 49, 32, 98, 121, 116, 101, 115, 32, 111, 102, 32, 116, 101, 120, 116, 41, 10, 94, 83, 80, 65, 67, 69, 32, 32, 32, 32, 73, 110, 115
    .byt 101, 114, 116, 32, 110, 111, 110, 45, 98, 114, 101, 97, 107, 105, 110, 103, 32, 115, 112, 97, 99, 101, 10, 10, 84, 97, 98, 32, 119, 105
    .byt 100, 116, 104, 44, 32, 97, 115, 32, 110, 117, 109, 98, 101, 114, 32, 111, 102, 32, 99, 104, 97, 114, 97, 99, 116, 101, 114, 115, 44, 32
    .byt 105, 115, 32, 115, 101, 116, 32, 98, 121, 32, 102, 105, 114, 115, 116, 32, 112, 114, 101, 115, 115, 105, 110, 103, 32, 97, 110, 100, 32, 114
    .byt 101, 108, 101, 97, 115, 105, 110, 103, 32, 69, 83, 67, 10, 97, 110, 100, 32, 116, 104, 101, 110, 32, 97, 32, 100, 105, 103, 105, 116, 32
    .byt 40, 49, 46, 46, 57, 41, 32, 114, 101, 112, 114, 101, 115, 101, 110, 116, 105, 110, 103, 32, 116, 104, 101, 32, 119, 105, 100, 116, 104, 46
    .byt 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 88, 49, 54, 32, 69, 100, 105, 116, 44, 32, 67, 111, 112, 121, 114, 105, 103, 104, 116
    .byt 32, 50, 48, 50, 48, 45, 50, 48, 50, 49, 44, 32, 83, 116, 101, 102, 97, 110, 32, 74, 97, 107, 111, 98, 115, 115, 111, 110, 46, 10
    .byt 10, 84, 104, 105, 115, 32, 112, 114, 111, 103, 114, 97, 109, 32, 105, 115, 32, 102, 114, 101, 101, 32, 115, 111, 102, 116, 119, 97, 114, 101
    .byt 32, 97, 110, 100, 32, 99, 111, 109, 101, 115, 32, 119, 105, 116, 104, 32, 65, 66, 83, 79, 76, 85, 84, 69, 76, 89, 32, 78, 79, 32
    .byt 87, 65, 82, 82, 65, 78, 84, 89, 46, 32, 89, 111, 117, 32, 109, 97, 121, 10, 114, 101, 100, 105, 115, 116, 114, 105, 98, 117, 116, 101
    .byt 32, 97, 110, 100, 47, 111, 114, 32, 109, 111, 100, 105, 102, 121, 32, 105, 116, 32, 117, 110, 100, 101, 114, 32, 116, 104, 101, 32, 116, 101
    .byt 114, 109, 115, 32, 111, 102, 32, 116, 104, 101, 32, 71, 78, 85, 32, 71, 101, 110, 101, 114, 97, 108, 32, 80, 117, 98, 108, 105, 99, 32
    .byt 76, 105, 99, 101, 110, 115, 101, 10, 97, 115, 32, 112, 117, 98, 108, 105, 115, 104, 101, 100, 32, 98, 121, 32, 116, 104, 101, 32, 70, 114
    .byt 101, 101, 32, 83, 111, 102, 116, 119, 97, 114, 101, 32, 70, 111, 117, 110, 100, 97, 116, 105, 111, 110, 44, 32, 101, 105, 116, 104, 101, 114
    .byt 32, 118, 101, 114, 115, 105, 111, 110, 32, 51, 32, 111, 102, 32, 116, 104, 101, 32, 76, 105, 99, 101, 110, 115, 101, 44, 10, 111, 114, 44
    .byt 32, 97, 116, 32, 121, 111, 117, 114, 32, 111, 112, 116, 105, 111, 110, 44, 32, 97, 110, 121, 32, 108, 97, 116, 101, 114, 32, 118, 101, 114
    .byt 115, 105, 111, 110, 46, 32, 70, 111, 114, 32, 100, 101, 116, 97, 105, 108, 101, 100, 32, 116, 101, 114, 109, 115, 32, 115, 101, 101, 32, 115
    .byt 117, 112, 112, 108, 101, 109, 101, 110, 116, 101, 100, 32, 102, 105, 108, 101, 10, 34, 108, 105, 99, 101, 110, 115, 101, 34, 46, 32, 65, 108
    .byt 115, 111, 32, 97, 118, 97, 105, 108, 97, 98, 108, 101, 32, 102, 114, 111, 109, 32, 60, 104, 116, 116, 112, 115, 58, 47, 47, 119, 119, 119
    .byt 46, 103, 110, 117, 46, 111, 114, 103, 47, 108, 105, 99, 101, 110, 115, 101, 115, 62, 46,  0
.endproc

;******************************************************************************
;Function name.......: screen_refresh
;Purpose.............: Refreshes all visible lines of text. Print will start 
;                      from memory address pointed to by zero page vectors 
;                      SCR_XXX
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_refresh
    ;Select start bank
    lda SCR_BNK
    sta BNK_SEL

    ;Setup temporary vector to traverse text
    lda SCR_ADR+1
    sta TMP1_ADR+1
    stz TMP1_ADR

    ;Set number of columns per row to be printed
    lda #SCREEN_WIDTH
    sta columns

    ;Set number of rows to be printed
    lda #54
    sta rows

    ;Set VERA address to print from start of row 2
    lda #0
    sta VERA_L
    lda #2
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ;Get length of mem page where we start printing
    ldy #4
    lda (TMP1_ADR),y
    sta len     

    ;Set offset +5 to skip mem page metadata
    lda #5
    sta TMP1_ADR

    ;Set char index to start printing from
    ldy SCR_IDX

    ;Select printing loop for ISO or PETSCII mode
    lda screen_mode
    bne petscii_loop

iso_loop:
    ;Check if we are at end of mem page, and if so go to next mem page
    cpy len
    bcc :+
    jsr next_mem_page

    ;Get char to print
:   lda (TMP1_ADR),y
    cmp #LF             ;end of line, prepare for next line
    bne :+
    jsr next_line
    jmp iso_loop

:   ldx columns         ;If 0 we have printed all visible columns, skip_char
    beq iso_skip_char
    
    sta VERA_D0         ;Otherwise let's print char to screen
    dec columns

iso_skip_char:
    iny
    jmp iso_loop

petscii_loop:
    ;Check if we are at end of mem page, and if so go to next mem page
    cpy len
    bcc :+
    jsr next_mem_page

    ;Get char to print
:   lda (TMP1_ADR),y
    cmp #LF             ;end of line, prepare for next line
    bne :+
    jsr next_line
    jmp petscii_loop

:   ldx columns         ;If 0 we have printed all visible columns, skip_char
    beq petscii_skip_char
    
    charset_petscii_to_scrcode
    sta VERA_D0         ;Otherwise let's print char to screen
    dec columns

petscii_skip_char:
    iny
    jmp petscii_loop

next_mem_page:
    stz TMP1_ADR        ;Set offset to 0 to read mem page metadata

    ldy #2
    lda (TMP1_ADR),y    ;Next bank
    tax
    ldy #3
    lda (TMP1_ADR),y    ;Next page
    bne :+
    pla                 ;Pull return address from stack, so next rts won't go to the printing loop that called next_mem_page
    pla
    jmp exit     
    
:   sta TMP1_ADR+1

    txa
    sta BNK_SEL
    sta TMP1_BNK

    ldy #4
    lda (TMP1_ADR),y    ;Next page len
    sta len
    beq next_mem_page

    lda #5              ;Set offset to 5 to skip mem page metadata
    sta TMP1_ADR

    ldy #0              ;Start reading from first char in next mem page

    rts

next_line:
    ;Clear end of line by printing spaces
    ldx columns
    beq :++
    lda #32
:   sta VERA_D0
    dec columns
    bne :-

    ;Decrease row counter, if 0 we are done
:   dec rows
    bne :+
    pla             ;Pull return address from stack, so next rts won't go to the printing loop that called next_line
    pla
    jmp exit

    ;Set VERA address to start off next line
:   inc VERA_M
    stz VERA_L
    
    lda #SCREEN_WIDTH
    sta columns

    iny

    rts

exit:
    ;Clear end of last line by printing spaces
    ldx columns
    beq :++
    lda #32
:   sta VERA_D0
    dex
    bne :-

    ;Clear any remaining lines at bottom of screen
:   inc VERA_M
    ldy rows
    beq exit_2      ;Exit if 0
    
    stz VERA_L      ;Set VERA address to start of next line
    lda #32
    ldx #SCREEN_WIDTH

:   sta VERA_D0
    dex
    bne :-
    ldx #SCREEN_WIDTH
    inc VERA_M
    stz VERA_L
    dey
    bne :-

exit_2:
    rts

.segment "VARS"
    len: .res 1
    columns: .res 1
    rows: .res 1
.CODE

.endproc

;******************************************************************************
;Function name.......: screen_println
;Purpose.............: Prints the currently edited line. Print will start from
;                      memory address pointed to by zero page vectors
;                      LNV_XXX
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_println
    ;Set VERA address to print from start of the row where the cursor is
    lda #0
    sta VERA_L
    lda CRS_Y
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ;Setup temporary vector where printing starts
    lda LNV_ADR+1
    sta TMP1_ADR+1
    
    lda LNV_BNK
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
    bne :+ 
    jmp clean_line

:   jsr screen_put_char     ;Otherwise output char

    dec counter            
    beq clean_line          ;If counter = 0, we're done

    iny
    jmp print_loop

next_page:
    stz TMP1_ADR            ;Restore to read mem page metadata
    
    ldy #2
    lda (TMP1_ADR),y        ;Next bank
    tax
    ldy #3
    lda (TMP1_ADR),y        ;Next page
    beq clean_line          ;If 0, we're at end of file, clean end of line and exit

    sta TMP1_ADR+1
    txa
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

.segment "VARS"
    page_len: .res 1
    counter: .res 1
.CODE

.endproc

;******************************************************************************
;Function name.......: screen_clear
;Purpose.............: Clears the editable part of the screen
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_clear
    ;Set VERA address to start printing from start of row 2
    lda #0
    sta VERA_L
    lda #2
    sta VERA_M
    lda #(1<<4)
    sta VERA_H

    ;Pepare printing
    lda #32     ;Blank space
    ldx #80     ;Columns
    ldy #54     ;Rows
    sty tempvars
    ldy screen_color
loop:
    sta VERA_D0
    sty VERA_D0
    dex
    bne loop
    stz VERA_L
    inc VERA_M
    ldx #80
    dec tempvars
    bne loop

    rts
rows = tempvars     ;1 byte
.endproc

;******************************************************************************
;Function name.......: screen_print_status
;Purpose.............: Prints a null terminated status message 
;Input...............: X=pointer to message LSB, Y=pointer to message MSB
;Returns.............: Nothing
;Error returns.......: None
.proc screen_print_status
    ;Set temp zero page vector to message
    stx TMP1_ADR
    sty TMP1_ADR+1
    
    ;Get string len
    jsr util_strlen
    cpy #80
    bcc :+
    ldy #80   
:   sty len

    ;Clear status line
    jsr screen_clear_status

continue:
    ;Set VERA address to start printing at (this will center the output)
    lda #80
    sec
    sbc len
    and #%11111110
    sta VERA_L
    lda #57
    sta VERA_M
    lda #(1<<4)
    sta VERA_H

    ;Prepare printing, Y=char index, X=color
    ldy #0
    ldx #160

    ;Print loop
    lda len
    beq exit

print_loop:
    lda (TMP1_ADR),y
    jsr screen_put_uc_char
    stx VERA_D0         ;Color
    iny
    dec len
    bne print_loop

exit:
    rts

len = tempvars          ;1 byte
start_at = tempvars+1   ;1 byte

.endproc

;******************************************************************************
;Function name.......: screen_clear_status
;Purpose.............: Clears status message
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_clear_status
    ;Set VERA address to start of row 57
    stz VERA_L
    lda #57
    sta VERA_M
    lda #(1<<4)
    sta VERA_H

    ;Prepare printing
    ldx #80 ;Column count
    lda #32 ;Blank space
    ldy screen_color ;Color

:   sta VERA_D0
    sty VERA_D0
    dex
    bne :-
    rts
.endproc

;******************************************************************************
;Function name.......: screen_clearall
;Purpose.............: Clears all screen
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc screen_clearall
    stz VERA_L
    stz VERA_M
    lda #(1<<4)
    sta VERA_H

    lda #32
    ldy screen_color
:   ldx #0
:   sta VERA_D0
    sty VERA_D0
    inx
    cpx #80
    bne :-

    inc VERA_M
    stz VERA_L
    ldx VERA_M
    cpx #60
    bne :--

    rts
.endproc

;******************************************************************************
;Function name.......: screen_put_char
;Purpose.............: Inserts value of A register at the current 
;                      VERA_L-M-H position. If in PETSCII mode, the char is 
;                      first converted to screen code
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc screen_put_char
    ldx screen_mode
    beq put
    charset_petscii_to_scrcode
put:
    sta VERA_D0
    rts
.endproc

;******************************************************************************
;Function name.......: screen_put_uc_char
;Purpose.............: Inserts value of A register at the current VERA_L-M-H
;                      position. If in PETSCII mode, the char is first
;                      converted to upper case.
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc screen_put_uc_char
    phx

    ldx screen_mode
    cpx #1
    beq puc
    cpx #2
    beq plc

put:
    sta VERA_D0
    plx
    rts

puc:
    .scope
        charset_ascii_to_petscii_ucase
    .endscope
    bra put

plc:
    .scope
        charset_ascii_to_petscii_lcase
    .endscope
    bra put
    
.endproc

screen_scrcodes:
    .byt 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143
    .byt 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159
    .byt 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47
    .byt 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63
    .byt 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    .byt 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    .byt 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79
    .byt 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
    .byt 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207
    .byt 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223
    .byt 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111
    .byt 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
    .byt 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79
    .byt 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
    .byt 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111
    .byt 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 94

.segment "VARS"
    screen_mode:            .res 1              ;1 byte
    screen_color:           .res 1              ;1 byte
.CODE