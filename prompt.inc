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
;Function name.......: prompt_init
;Purpose.............: Initializes input prompt and displays message
;Input...............: Pointer to message (X=LSB, Y=MSB)
;                      A = input max length
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_init
    ;Set zero page vector
    stx TMP1_ADR
    sty TMP1_ADR+1
    
    ;Set prompt max input len
    sta prompt_maxlen

    ;Clear status line
    jsr screen_clear_status

    ;Backup cursor position. We need to go back where we came from when done
    lda CRS_X
    sta prompt_cursor_backup
    lda CRS_Y
    sta prompt_cursor_backup+1
    
    ;Print prompt message at row 57
    stz VERA_L
    lda #57
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ldy #0

    lda screen_mode
    beq loop_iso
    cmp #1
    beq loop_petscii_uc
    jmp loop_petscii_lc

loop_iso:
    lda (TMP1_ADR),y
    beq loop_iso_exit
    sta VERA_D0
    iny
    jmp loop_iso

loop_iso_exit:
    jmp set_bgcolor

loop_petscii_uc:
    lda (TMP1_ADR),y
    beq loop_petscii_uc_exit
    .scope
        charset_ascii_to_petscii_ucase
    .endscope
    sta VERA_D0
    iny
    jmp loop_petscii_uc

loop_petscii_uc_exit:
    jmp set_bgcolor

loop_petscii_lc:
    lda (TMP1_ADR),y
    beq set_bgcolor
    .scope
        charset_ascii_to_petscii_lcase
    .endscope
    sta VERA_D0
    iny
    jmp loop_petscii_lc
    
set_bgcolor:
    ;Before we set background color, set column where user input starts (prompt message len+1)
    iny
    sty prompt_start

    ;Set background color
    lda #1
    sta VERA_L
    
    ldx #80     ;Column count
    lda #160    ;Color

:   sta VERA_D0
    dex
    bne :-

    ;Set start values
    stz prompt_curpos
    stz prompt_fvc
    stz prompt_len

    ;Move cursor
    ldx prompt_start
    ldy #57
    jmp cursor_move

.endproc

;******************************************************************************
;Function name.......: prompt_default_input
;Purpose.............: Sets default input
;Input...............: Pointer to default input string, X=LSB and Y=MSB,
;                      A=string len
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_default_input
    ;Setup zero page vector to input string
    stx TMP1_ADR
    sty TMP1_ADR+1
    sta len

    ldy #0
    sty index

loop:
    ;This loop copies default input string to prompt input
    ldy index
    cpy len
    beq exit
    cpy prompt_maxlen
    beq exit

    lda (TMP1_ADR),y
    sta prompt_input,y
    inc prompt_len
    jsr prompt_go_right
    
    inc index
    jmp loop

exit:
    jmp prompt_refresh
    
len:
    .byt 0
index:
    .byt 0
.endproc

;******************************************************************************
;Function name.......: prompt_refresh
;Purpose.............: Refreshes user input
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_refresh
    ;Set VERA address to start of user input
    lda prompt_start
    asl
    sta VERA_L
    lda #57
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ;Set indexes
    ldy prompt_fvc
    ldx prompt_start

    ;Select printing routine for active charset
    lda screen_mode
    beq loop_iso
    cmp #1
    beq loop_petscii_uc
    jmp loop_petscii_lc

loop_iso:
    cpy prompt_len              ;Exit if we are at end of user input
    bcs loop_iso_exit
    lda prompt_input,y
    sta VERA_D0
    iny
    inx
    cpx #SCREEN_WIDTH           ;Also exit if we are at right border, rest of user input not visible
    bne loop_iso
    rts

loop_iso_exit:
    jmp clear_line

loop_petscii_uc:
    cpy prompt_len
    bcs loop_petscii_uc_exit
    lda prompt_input,y
    .scope
        charset_ascii_to_petscii_ucase
    .endscope
    sta VERA_D0
    iny
    inx
    cpx #SCREEN_WIDTH
    bne loop_petscii_uc
    rts

loop_petscii_uc_exit:
    jmp clear_line

loop_petscii_lc:
    cpy prompt_len
    bcs clear_line
    lda prompt_input,y
    .scope
        charset_petscii_to_scrcode
    .endscope
    sta VERA_D0
    iny
    inx
    cpx #SCREEN_WIDTH
    bne loop_petscii_lc
    rts

clear_line:
    ;Clears end of the prompt line
    lda #32
:   sta VERA_D0
    inx
    cpx #SCREEN_WIDTH
    bne :-

    rts
.endproc

;******************************************************************************
;Function name.......: prompt_close
;Purpose.............: Closes and cleans up prompt when done
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_close
    ;Clear status line
    jsr screen_clear_status

    ;Restore cursor pos
    ldx prompt_cursor_backup
    ldy prompt_cursor_backup+1
    jsr cursor_move

    rts
.endproc

;******************************************************************************
;Function name.......: prompt_keypress
;Purpose.............: Handle keypess in prompt
;Input...............: A=ascii value
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_keypress
    ;Delete key
    cmp #$14
    beq delete

    ;Left arrow key
    cmp #$9d
    beq left_arrow

    ;Right arrow key
    cmp #$1d
    beq right_arrow

    ;Home key
    cmp #$13
    beq home

    ;Shift+Home = End key
    cmp #$93
    beq end

    ;Ignore other control chars
    cmp #$20
    bcc exit

    ;Default insert
    jsr prompt_insert
    jsr prompt_go_right
    jmp prompt_refresh

delete:
    jsr prompt_delete
    jsr prompt_go_left
    jmp prompt_refresh

left_arrow:
    jsr prompt_go_left
    jmp prompt_refresh

right_arrow:
    jsr prompt_go_right
    jmp prompt_refresh

home:
    ;Not yet implemented
    rts

end:
    ;Not yet implemented
    rts

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: prompt_insert
;Purpose.............: Inserts one char in prompt input string
;Input...............: A=char to insert
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_insert
    ;Check if there is room for another char, exit if not
    ldx prompt_len
    cpx prompt_maxlen
    bcs exit

    ;Store char on stack
    pha

    ;Move all chars from cursor to end of input one step to the right, making room for the new char
    ldy #254
:   lda prompt_input,y
    sta prompt_input+1,y
    cpy prompt_curpos
    beq insert
    dey
    jmp :-

insert:
    pla
    sta prompt_input,y
    inc prompt_len

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: prompt_delete
;Purpose.............: Deletes one char in prompt input string
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_delete
    ;Exit if we are at start of user input, nothing to delete
    ldy prompt_curpos
    beq exit

    ;Move all chars from the cursor to end of user input one step left => deleting the char under cursor
:   lda prompt_input,y
    sta prompt_input-1,y
    iny
    bne :-

    dec prompt_len

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: prompt_go_right
;Purpose.............: Moves current editing position one step right
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_go_right
    ;Exit if cursor is at end of user input
    lda prompt_curpos
    cmp prompt_len
    bcs exit

    ;Move cursor
    inc prompt_curpos
    
    ;Check if we need to scroll user input horizontally
    lda CRS_X
    cmp #SCREEN_WIDTH-1
    beq scroll
    inc CRS_X
    ldy #57
    ldx CRS_X
    jmp cursor_move

scroll:
    inc prompt_fvc

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: prompt_go_left
;Purpose.............: Moves current editing position one step left
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_go_left
    ;Exit if cursor at start of user input
    lda prompt_curpos
    beq exit

    ;Move cursor
    dec prompt_curpos

    ;Check if we need to scroll horizontally
    lda prompt_fvc
    beq :+
    dec prompt_fvc      ;Scroll
    rts

:   ldx CRS_X
    dex
    ldy CRS_Y
    jsr cursor_move

exit:
    rts

.endproc

;GLOBAL VARIABLES
prompt_input:
    .res 256,1

prompt_start:
    .byt 0

prompt_fvc:
    .byt 0

prompt_curpos:
    .byt 0

prompt_len:
    .byt 0

prompt_maxlen:
    .byt 0

prompt_cursor_backup:
    .byt 0,0