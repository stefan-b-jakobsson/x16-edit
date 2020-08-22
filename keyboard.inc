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
;Function name.......: keyboard_read
;Purpose.............: Reads one character from the keyboard buffer
;Preparatory routines: mem_init, screen_init, cursor_init
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_read
    ;Kernal call to check keyboard queue. If A=0 then queue is empty.
    jsr KERNAL_GETIN
    bne gotachar
    rts

gotachar:
    ;Select key handler according to program mode
    ldx APP_MOD
    beq mode0
    cpx #1
    beq mode1
    cpx #2
    beq mode2

mode0:
    jmp keyboard_mode0

mode1:
    jmp keyboard_mode1

mode2:
    pha
    lda #0
    stx APP_MOD
    jsr screen_clear_status
    pla
    jmp keyboard_mode0

.endproc

;******************************************************************************
;Function name.......: keyboard_mode0
;Purpose.............: Select key press actions for mode 0 (default mode)
;Preparatory routines: mem_init, screen_init, cursor_init
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode0
    ;Enter key
    cmp #13
    beq enter
    
    ;Delete key
    cmp #$14
    beq delete

    ;Left arrow key
    cmp #$9d
    beq left_arrow

    ;Right arrow key
    cmp #$1d
    beq right_arrow

    ;Up arrow key
    cmp #$91
    beq up_arrow

    ;Down arrow key
    cmp #$11
    beq down_arrow

    ;Home key
    cmp #$13
    beq home

    ;Shift+Home = End key
    cmp #$93
    beq end

    ;Ctrl+G
    cmp #$07
    beq ctrl_g

    ;Ctrl+x
    cmp #$18
    beq ctrl_x

    ;Ctrl+Y
    cmp #$19
    beq ctrl_y

    ;Ctrl+V
    cmp #$16
    beq ctrl_v

    ;Ctrl+W
    cmp #$17
    beq ctrl_w

    ;Ctrl+O
    cmp #$0f
    beq ctrl_o

    ;If no other match, goto default handler
    jmp keyboard_default_keyhandler

enter:
    jmp keyboard_enter_key

delete:
    jmp keyboard_delete_key

left_arrow:
    jmp keyboard_leftarrow_key

right_arrow:
    jmp keyboard_rightarrow_key

up_arrow:
    jmp keyboard_uparrow_key

down_arrow:
    jmp keyboard_downarrow_key

home:
    jmp keyboard_home_key

end:
    jmp keyboard_end_key

ctrl_g:
    jmp keyboard_ctrl_g

ctrl_x:
    jmp keyboard_ctrl_x

ctrl_y:
    jmp keyboard_ctrl_y

ctrl_v:
    jmp keyboard_ctrl_v

ctrl_w:
    jmp keyboard_ctrl_w

ctrl_o:
    jmp keyboard_ctrl_o

.endproc

;******************************************************************************
;Function name.......: keyboard_mode1
;Purpose.............: Select key press actions for mode 1 (help screen)
;Preparatory routines: mem_init, screen_init, cursor_init
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode1
    ;Exit help screen if q was pressed, ignore all other input
    cmp #'q'
    beq exit_help
    rts

exit_help:
    stz APP_MOD
    jsr screen_clear
    jsr screen_refresh

    jsr cursor_activate

    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_enter_key
;Purpose.............: Handles keypress
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_enter_key
    ;Insert linefeed
    lda #LF
    jsr mem_insert

    ;Move cursor to first column of next row
    jsr cursor_move_crlf
    bcc :+          ;Carry set = at bottom of screen, need to scroll
    
    jsr mem_scr_move_down

:   jsr mem_set_lnv_to_crs        ;Set first visible char of line to cursor position, i.e. start of line
    jsr screen_refresh

    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_delete_key
;Purpose.............: Handles keypress
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_delete_key
    ;Delete one char from memory
    jsr mem_delete
    bcs exit

    cpx #1
    beq at_linebreak

    jsr cursor_move_left
    jsr screen_println
    
exit:
    rts

at_linebreak:
    ;Move cursor the beginning of line above
    stz CRS_X
    jsr cursor_move_up
    bcc :+
    jsr mem_scr_move_up     ;Carry set, we're at top of screen and need to scroll

:   ;Move to line start, and set first visible char to start of line. Then move to line end.
    ;This is needed to properly show a line that is longer than the screen
    jsr mem_crs_move_to_line_start
    stx counter
    sty counter+1
    jsr mem_set_lnv_to_crs

    jsr screen_refresh

loop:
    lda counter
    bne :+
    lda counter+1
    beq exit

:   jsr mem_crs_step_right
    jsr cursor_move_right
    bcc noscroll
    jsr mem_lnv_step_right

noscroll:
    dec counter
    lda counter
    cmp #$ff
    bne loop
    lda counter+1
    beq at_linebreak_exit
    dec counter+1
    jmp loop
    
at_linebreak_exit:
    jsr screen_println
    rts

counter:
    .byt 0,0
.endproc

;******************************************************************************
;Function name.......: keyboard_rightarrow_key
;Purpose.............: Handles keypress
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_rightarrow_key
    ;Move mem pointer one step right, carry set if at end of file, X=1 if at end of line
    jsr mem_crs_step_right
    bcs at_eof
    cpx #1
    beq at_eol

    ;Move cursor one step right
    jsr cursor_move_right
    bcs at_rightmost

    jsr screen_println
    rts

at_rightmost:
    ;Cursor at rightmost position but not end of line, need to scroll
    jsr mem_lnv_step_right
    jsr screen_println
    rts

at_eof:
    ;End of file, do nothing
    jsr screen_println
    rts

at_eol:
    ;End of line, move cursor to start of next line, C=1 if at bottom of screen
    jsr cursor_move_crlf
    bcs at_bottom

    jsr mem_set_lnv_to_crs  ;Set first visible char to cursor, i.e. start of line
    jsr screen_refresh
    rts

at_bottom:
    ;Cursor at bottom of screen, but not end of file, need to scroll
    jsr mem_set_lnv_to_crs
    jsr mem_scr_move_down
    jsr screen_refresh
    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_leftarrow_key
;Purpose.............: Handles keypress
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_leftarrow_key
    ;Move mem pointer one step left, if C set we are at start of file, if X=1 we moved to line above
    jsr mem_crs_step_left
    bcs at_filestart
    cpx #1
    beq at_linebreak

    ;Move cursor left, if C set we are at leftmost screen position, but not start of line (need to scroll)
    jsr cursor_move_left
    bcs at_leftmost

    ;No special case to handle, refresh line
    jsr screen_println
    rts

at_filestart:
    ;We are at start of file, nothing to do
    rts

at_leftmost:
    ;Cursor at leftmost screen position (but not start of line), scroll the line
    jsr mem_lnv_step_left
    jsr screen_println
    rts

at_linebreak:
    ;Goto start of line
    jsr mem_crs_move_to_line_start

    ;Also set first visible char to start of line
    jsr mem_set_lnv_to_crs

    ;Move cursor up to start of line above
    stz CRS_X
    jsr cursor_move_up
    bcc :+
    ;C=1, we're at top of screen, need to scroll
    jsr mem_scr_move_up

    ;Use existing function to go to end of line
:   jsr screen_refresh
    jmp keyboard_end_key
.endproc

;******************************************************************************
;Function name.......: keyboard_uparrow_key
;Purpose.............: Handles keypress
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_uparrow_key
    ;Backup mem pointers
    lda CRS_BNK
    sta backup
    lda CRS_ADR+1
    sta backup+1
    lda CRS_IDX
    sta backup+2
    
    lda LNV_BNK
    sta backup+3
    lda LNV_ADR+1
    sta backup+4
    lda LNV_IDX
    sta backup+5

    ;Move to start of current line, and save number of chars stepped over
    jsr mem_crs_move_to_line_start
    stx count
    sty count+1

    ;Move one step left to get to end of line above
    jsr mem_crs_step_left
    bcs at_first_line           ;If carry set, we're at start of file

    ;Move to start of that line
    jsr mem_crs_move_to_line_start

    ;Set pointer to first visible char on line to cursor
    jsr mem_set_lnv_to_crs

    ;Move screen cursor to beginning of previous line
    stz CRS_X
    ldy CRS_Y
    jsr cursor_move_up
    bcc:+                       ;If carry set, we're at top of screen, need to scroll
    jsr mem_scr_move_up
:   jmp stepto_column

at_first_line:
    ;Restore crs mem pointer and exit
    lda backup
    sta CRS_BNK
    lda backup+1
    sta CRS_ADR+1
    lda backup+2
    sta CRS_IDX

    lda backup+3
    sta LNV_BNK
    lda backup+4
    sta LNV_ADR+1
    lda backup+5
    sta LNV_IDX
    rts

stepto_column:
    ;Move back to the column we came from, but stopping if we encounter LF before that
    
    ;Check column counter
    lda count
    bne :+
    lda count+1
    beq exit

:   ;If char = LF then exit
    lda #5
    sta CRS_ADR
    ldy CRS_IDX
    lda (CRS_ADR),y
    cmp #LF
    beq exit
    stz CRS_ADR

    ;Move mem pointer, exit C=1 (end of file)
    jsr mem_crs_step_right
    bcs exit

    ;Move cursor, if at rightmost position (but not end of line), need to scroll line
    jsr cursor_move_right
    bcs :+
    jmp dec_count
:   ;C=1, need to scroll
    jsr mem_lnv_step_right

dec_count:
    ;Decrease 16 bit counter, exit if 0
    dec count
    lda count
    cmp #$ff
    bne stepto_column

    lda count+1
    beq exit
    dec count+1

    jmp stepto_column

exit:
    stz CRS_ADR         ;Restore to default value, so that we don't mess up other functions use of this pointer
    
    jsr screen_refresh
    jsr screen_println

    rts

count:
    .byt 0,0

backup:
    .byt 0,0,0,0,0,0

.endproc

;******************************************************************************
;Function name.......: keyboard_downarrow_key
;Purpose.............: Handles keypress
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_downarrow_key
    ;Backup cursor mem pointer
    lda CRS_BNK
    sta backup
    lda CRS_ADR+1
    sta backup+1
    lda CRS_IDX
    sta backup+2
    
    lda LNV_BNK
    sta backup+3
    lda LNV_ADR+1
    sta backup+4
    lda LNV_IDX
    sta backup+5

    ;Move mem pointer to start of line, and save number of chars moved (our column)
    jsr mem_crs_move_to_line_start
    stx count
    sty count+1

    ;Move mem pointer to end of line, and move one more step to get to next line (carry set = end of file)
    jsr mem_crs_move_to_line_end
    jsr mem_crs_step_right
    bcs at_last_line

    ;Set pointer to first visible char on the line to cursor
    jsr mem_set_lnv_to_crs

    ;Move cursor to start of line below
    stz CRS_X
    ldy CRS_X
    jsr cursor_move_down
    bcc loop
    
    ;C=1, cursor at bottom of screen and we need to scroll
    jsr mem_scr_move_down

loop:
    ;Loop to move back to column we came from, however stopping if we encounter LF

    ;Check column counter
    lda count
    bne loop_2
    lda count+1
    beq exit

loop_2:
    lda #5
    sta CRS_ADR
    ldy CRS_IDX
    lda (CRS_ADR),y
    cmp #LF
    beq exit

    stz CRS_ADR
    jsr mem_crs_step_right
    bcs exit

    jsr cursor_move_right
    bcc loop_3
    jsr mem_lnv_step_right

loop_3:
    dec count
    lda count
    cmp #$ff
    bne loop
    dec count+1
    jmp loop

at_last_line:
    ;Restore cursor mem pointer, and exit
    lda backup
    sta CRS_BNK
    lda backup+1
    sta CRS_ADR+1
    lda backup+2
    sta CRS_IDX

    lda backup+3
    sta LNV_BNK
    lda backup+4
    sta LNV_ADR+1
    lda backup+5
    sta LNV_IDX

    rts

exit:
    stz CRS_ADR
    jsr screen_refresh
    jsr screen_println
    rts

count:
    .byt 0,0

backup:
    .byt 0,0,0,0,0,0
.endproc

;******************************************************************************
;Function name.......: keyboard_home_key
;Purpose.............: Handles keypress
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_home_key
    ldx #0
    ldy CRS_Y
    jsr cursor_move
    
    jsr mem_crs_move_to_line_start
    jsr mem_set_lnv_to_crs
    jsr screen_println
    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_end_key
;Purpose.............: Handles keypress
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_end_key
    ;Move cursor mem pointer to end of line, returns number of chars stepped over
    jsr mem_crs_move_to_line_end
    sty counter+1
    stx counter

    ;Check column counter, exit if 0 (i.e. we were already at end of line)
    cpy #0
    bne loop
    cpx #0
    bne loop

    rts

loop:
    jsr cursor_move_right
    bcc :+                      
    ;Carry set = cursor at end of line, need to scroll
    jsr mem_lnv_step_right

:   dec counter
    bne loop
    lda counter+1
    beq exit
    dec counter+1
    jmp loop

exit:
    jsr screen_println          ;To refresh current line
    rts

counter:
    .byt 0,0

.endproc

;******************************************************************************
;Function name.......: keyboard_ctrl_x
;Purpose.............: Command: Exit program
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_ctrl_x
    lda #1
    sta APP_QUIT
    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_ctrl_g
;Purpose.............: Command: Show help
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_ctrl_g
    jsr cursor_disable

    lda #1
    sta APP_MOD
    jmp screen_show_help
.endproc

;******************************************************************************
;Function name.......: keyboard_ctrl_y
;Purpose.............: Command: PageUp
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_ctrl_y
    lda #27
    sta linecount

:   jsr keyboard_uparrow_key
    dec linecount
    bne :-

    rts

linecount:
    .byt 0

msg:
    .byt "not yet implemented",0
.endproc

;******************************************************************************
;Function name.......: keyboard_ctrl_v
;Purpose.............: Command: PageDown
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_ctrl_v
    lda #27
    sta linecount

:   jsr keyboard_downarrow_key
    dec linecount
    bne :-

    rts

linecount:
    .byt 0
.endproc

;******************************************************************************
;Function name.......: keyboard_ctrl_w
;Purpose.............: Command: Write file
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_ctrl_w
    lda #2
    sta APP_MOD

    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    rts

msg:
    .byt "write text to file not yet implemented",0
.endproc

;******************************************************************************
;Function name.......: keyboard_ctrl_o
;Purpose.............: Command: Open file
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_ctrl_o
    lda #2
    sta APP_MOD

    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    rts

msg:
    .byt "open file not yet implemented",0
.endproc

;******************************************************************************
;Function name.......: keyboard_default_keyhandler
;Purpose.............: Default keyhandler for app mode 0 (default mode)
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_default_keyhandler
    jsr mem_insert
    jsr cursor_move_right
    bcc :+
    ;Carry set, cursor at rightmost position, need to scroll
    jsr mem_lnv_step_right

:   jsr screen_println

    rts
.endproc