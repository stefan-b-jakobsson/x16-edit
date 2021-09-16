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
;Function name.......: prompt_init
;Purpose.............: Initializes input prompt and displays message
;Input...............: Pointer to message (X=LSB, Y=MSB)
;                      A = input max length
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_init
    ;Set pointer to prompt message
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
    
    ;Print prompt message at third row from bottom of screen
    stz VERA_L
    lda #SCREEN_HEIGHT-3
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ldy #0
msg_loop:
    lda (TMP1_ADR),y
    beq set_bgcolor
    jsr screen_put_uc_char
    iny
    bra msg_loop
    
set_bgcolor:
    ;Before we set background color, set column where user input starts (prompt message len+1)
    iny
    sty prompt_start

    ;Prepapre writing color data
    lda #1
    sta VERA_L
    
    ldx #SCREEN_WIDTH
    lda #160    ;Color

:   sta VERA_D0
    dex
    bne :-

    ;Set start values
    stz prompt_curpos
    stz prompt_fvc
    stz prompt_len

    ;Move cursor to prompt line
    ldx prompt_start
    ldy #SCREEN_HEIGHT-3
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
    ;Set pointer to input string
    stx TMP1_ADR
    sty TMP1_ADR+1
    sta prompt_len

    ldy #0
    sty index
loop:
    ;Copy default input string to prompt input buffer
    ldy index
    cpy prompt_len
    beq exit            
    cpy prompt_maxlen
    beq exit

    lda (TMP1_ADR),y
    sta prompt_input,y
    jsr prompt_go_right
    
    inc index
    bra loop

exit:
    jmp prompt_refresh
    
.segment "VARS"
    index: .res 1
.CODE

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
    lda #SCREEN_HEIGHT-3
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ;Set indexes
    ldy prompt_fvc
    ldx prompt_start

loop:
    cpy prompt_len              ;Exit if we are at end of user input
    bcs clear_line
    lda prompt_input,y
    phx
    jsr screen_put_char
    plx
    iny
    inx
    cpx #SCREEN_WIDTH           ;Also exit if we are at right border, rest of user input not visible
    bne loop
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
    ;Restore cursor pos
    ldx prompt_cursor_backup
    ldy prompt_cursor_backup+1
    jsr cursor_move

    ;Clear status line
    jmp screen_clear_status
.endproc

;******************************************************************************
;Function name.......: prompt_keypress
;Purpose.............: Handle keypess in prompt
;Input...............: A=ascii value
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_keypress
    ;Backspace key
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

    ;End key
    cmp #KBD_EXT_END
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
    jsr prompt_go_home
    jmp prompt_refresh

end:
    jsr prompt_go_end
    jmp prompt_refresh

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
    bra :-

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
    ldy #SCREEN_HEIGHT-3
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

;******************************************************************************
;Function name.......: prompt_go_home
;Purpose.............: Moves current editing position to start of prompt
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_go_home
    stz prompt_curpos
    stz prompt_fvc
    ldx prompt_start
    ldy CRS_Y
    jmp cursor_move
.endproc

;******************************************************************************
;Function name.......: prompt_go_end
;Purpose.............: Moves current editing position to end of prompt
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_go_end
    lda prompt_len
    sta prompt_curpos

    sec
    lda #SCREEN_WIDTH-1
    sbc prompt_start
    sta tempvars

    sec
    lda prompt_len
    sbc tempvars
    bcc :+
    sta prompt_fvc
    bra :++
:   stz prompt_fvc

:   sec
    lda prompt_curpos
    sbc prompt_fvc
    clc
    adc prompt_start
    
    tax
    ldy CRS_Y
    jmp cursor_move
.endproc

.segment "VARS"
    prompt_input:           .res 256
    prompt_start:           .res 1              ;1 byte
    prompt_fvc:             .res 1              ;1 byte
    prompt_curpos:          .res 1              ;1 byte
    prompt_len:             .res 1              ;1 byte
    prompt_maxlen:          .res 1              ;1 byte
    prompt_cursor_backup:   .res 2              ;2 bytes
.CODE