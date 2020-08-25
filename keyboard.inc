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
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_read
    ;Get char from keyboard queue. If A=0, the queue is empty.
    jsr KERNAL_GETIN
    bne gotachar
    rts

gotachar:
    ;Select key handler according to program mode
    ldx APP_MOD
    beq mode_default
    cpx #1
    beq mode_helpscreen
    cpx #2
    beq mode_statusmessage

mode_default:
    jmp keyboard_mode_default

mode_helpscreen:
    jmp keyboard_mode_helpscreen

mode_statusmessage:
    jmp keyboard_mode_statusmessage

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_default
;Purpose.............: Select key press actions for default mode
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_default
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
;Function name.......: keyboard_mode_helpscreen
;Purpose.............: Select key press actions for help screen
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_helpscreen
    ;Exit help screen if q was pressed, ignore all other input
    cmp #03         ;ESC key
    beq exit_help
    rts

exit_help:
    stz APP_MOD
    jsr screen_clear
    jsr screen_refresh
    jsr screen_clear_status

    jsr cursor_activate

    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_statusmessage
;Purpose.............: Select key press actions for when a status message has
;                      been displayed
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_statusmessage
    ;Save char on stack
    pha

    ;Set program to default mode (0) and clear status message
    lda #0
    stx APP_MOD
    jsr screen_clear_status
    
    ;Send char to handler for default mode (.i.e. to print it)
    pla
    jmp keyboard_mode_default
.endproc

;******************************************************************************
;Function name.......: keyboard_enter_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_enter_key
    ;Insert linefeed char
    lda #LF
    jsr mem_insert

    ;Move cursor to first column of next row
    jsr cursor_move_crlf
    bcc :+          
    
    ;C=1: We're at bottom of screen, need to scroll
    jsr mem_scr_move_down

:   jsr mem_set_lnv_to_crs        ;Set first visible char of line to cursor position, i.e. start of line
    jsr screen_refresh

    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_delete_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_delete_key
    ;Delete one char from memory
    jsr mem_delete
    bcs exit                ;C=1: We're at start of file, nothing was deleted

    ;Check if we deleted a line break (indicated by X=1), a special case to handle
    cpx #1
    beq linebreak_deleted

    ;We did not delete a line break, move cursor one column left and print line
    jsr cursor_move_left
    bcc :+
    jsr mem_lnv_step_left
:   jsr screen_println
    
exit:
    rts

linebreak_deleted:
    ;Move cursor the beginning of line above the current line
    stz CRS_X
    jsr cursor_move_up
    bcc :+
    
    ;Carry set, we're at top of screen and need to scroll
    jsr mem_scr_move_up

:   ;Move to line start, and save the number of chars moved over
    jsr mem_crs_move_to_line_start
    stx counter
    sty counter+1

    ;Set first visible char of that line to start of line (cursor position)
    jsr mem_set_lnv_to_crs

    jsr screen_refresh

;Loop to move the cursor back to the column we came from
loop:
    lda counter
    bne :+
    lda counter+1
    beq exit

:   jsr mem_crs_step_right
    jsr cursor_move_right
    bcc continue

    ;C=1: We need to scroll
    jsr mem_lnv_step_right

continue:
    ;Decrease column counter and exist loop if it's 0
    dec counter
    lda counter
    cmp #$ff
    bne loop
    lda counter+1
    beq linebreak_deleted_exit
    dec counter+1
    jmp loop
    
linebreak_deleted_exit:
    jsr screen_println
    rts

counter:
    .byt 0,0
.endproc

;******************************************************************************
;Function name.......: keyboard_rightarrow_key
;Purpose.............: Handles keypress
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
    ;Backup pointers so we can restore them if necessary
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

    ;Set pointer to first visible char on line to cursor position
    jsr mem_set_lnv_to_crs

    ;Move screen cursor to beginning of the line above
    stz CRS_X
    ldy CRS_Y
    jsr cursor_move_up
    bcc:+                       ;If carry set, we're at top of screen, need to scroll
    jsr mem_scr_move_up
:   jmp stepto_column

at_first_line:
    ;Restore pointers and exit
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
    ;Move back to the column we came from, but stop if we encounter a line separator before that
    
    ;Check column counter, exit if 0
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

    ;Move mem pointer one step right, exit C=1 (end of file)
    jsr mem_crs_step_right
    bcs exit

    ;Move cursor, if at rightmost position (but not end of line), need to scroll line
    jsr cursor_move_right
    bcs :+
    jmp dec_count
:   ;C=1, need to scroll
    jsr mem_lnv_step_right

dec_count:
    ;Decrease counter
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
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_downarrow_key
    ;Backup pointers so we can restore them later if necessary
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

    ;Move cursor mem pointer to start of line, and save number of chars stepped over
    jsr mem_crs_move_to_line_start
    stx count
    sty count+1

    ;Move cursor mem pointer to end of line, and move one more step to get to next line (carry set = end of file)
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
    ;Loop to move back to column we came from, however stop if we encounter a line separator

    ;Check column counter, exit if 0
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
    beq exit            ;Line separator found

    stz CRS_ADR
    jsr mem_crs_step_right
    bcs exit            ;At end of file, exit

    jsr cursor_move_right
    bcc loop_3
    ;At rightmost position of screen, need to scroll
    jsr mem_lnv_step_right

loop_3:
    ;Decrease counter
    dec count
    lda count
    cmp #$ff
    bne loop
    dec count+1
    jmp loop

at_last_line:
    ;Restore pointers, and exit
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
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_home_key
    ;Move cursor to leftmost position
    ldx #0
    ldy CRS_Y
    jsr cursor_move
    
    ;Move cursor mem pointer to start of line, and set first visible char to same
    jsr mem_crs_move_to_line_start
    jsr mem_set_lnv_to_crs
    jsr screen_println
    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_end_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_end_key
    ;Move cursor mem pointer to end of line, save number of chars stepped over
    jsr mem_crs_move_to_line_end
    sty counter+1
    stx counter

;Loop to move to end of line
loop:
    ;Check column counter, exit if 0
    lda counter
    bne :+
    lda counter+1
    beq exit

:   jsr cursor_move_right
    bcc :+                      
    ;Carry set = cursor at rightmost position of screen, need to scroll
    jsr mem_lnv_step_right

    ;Decrease counter
:   dec counter
    lda counter
    cmp #$ff
    bne loop
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
;Purpose.............: Command: Initiate program exit
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_ctrl_x
    lda #1          ;APP_QUIT=1 signals to irq_handler to close down
    sta APP_QUIT
    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_ctrl_g
;Purpose.............: Command: Show help
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_ctrl_g
    jsr cursor_disable

    lda #1              ;mode_helpscreen
    sta APP_MOD

    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    
    jmp screen_show_help

msg:
    .byt "press esc to exit help screen",0
.endproc

;******************************************************************************
;Function name.......: keyboard_ctrl_y
;Purpose.............: Command: PageUp
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
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_ctrl_w
    lda #2
    sta APP_MOD     ;mode_statusmessage

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
    sta APP_MOD     ;mode_statusmessage

    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    rts

msg:
    .byt "open file not yet implemented",0
.endproc

;******************************************************************************
;Function name.......: keyboard_default_keyhandler
;Purpose.............: Default keyhandler for user input
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_default_keyhandler
    ;Insert char into memory
    jsr mem_insert

    ;Move cursor
    jsr cursor_move_right
    bcc :+
    ;Carry set, cursor at rightmost position, need to scroll
    jsr mem_lnv_step_right

:   jsr screen_println

    rts
.endproc