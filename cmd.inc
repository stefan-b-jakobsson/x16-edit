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
;Function name.......: cmd_insert
;Purpose.............: Inserts one char at cursor position and moves the cursor
;                      one step right. Does not refresh screen; users of this 
;                      procedure are responsible to do screen refresh when done
;Input...............: A = Char to insert
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_insert
    ;Insert char into memory
    jsr mem_insert

    ;Increase current columm index
    jsr mem_cur_col_inc

    ;Move cursor
    jsr cursor_move_right
    bcc exit

    ;Carry set, cursor at rightmost position, need to scroll
    jsr mem_lnv_step_right

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: cmd_go_left
;Purpose.............: Moves all mem pointers and cursor one step left. Does 
;                      not refresh screen; users of this procedure are 
;                      responsible to do screen refresh when done
;Input...............: Nothing
;Returns.............: C=1 if line changed, else C=0
;Error returns.......: None
.proc cmd_go_left
    ;Move mem pointer one step left, if C set we are at start of file, if X=1 we moved to line above
    jsr mem_crs_step_left
    bcs at_filestart
    cpx #1
    beq at_linebreak
    
    ;Decrease current column index
    jsr mem_cur_col_dec

    ;Move cursor left, if C set we are at leftmost screen position, but not start of line (need to scroll)
    jsr cursor_move_left
    bcs at_leftmost

    clc
    rts

at_leftmost:   
    ;Cursor at leftmost screen position (but not start of line), scroll the line
    jsr mem_lnv_step_left

    clc
    rts

at_linebreak:
    ;Goto start of line
    jsr mem_crs_move_to_line_start

    ;Also set first visible char to start of line
    jsr mem_set_lnv_to_crs

    ;Move cursor up one row to start of that row
    stz CRS_X
    jsr cursor_move_up
    bcc :+
    ;C=1, we're at top of screen, need to scroll
    jsr mem_scr_move_up

    ;Set line and column index
    jsr mem_cur_col_ret
    jsr mem_cur_line_dec

    ;Use existing function to go to end of line
:   jsr keyboard_end_key
    sec
    rts

at_filestart:
    ;We are at start of file, nothing to do
    clc
    rts
.endproc

;******************************************************************************
;Function name.......: cmd_go_right
;Purpose.............: Moves all mem pointers and cursor one step right. Does 
;                      not refresh screen; users of this procedure are 
;                      responsible to do screen refresh when done
;Input...............: Nothing
;Returns.............: C=1 if line changed, else C=0
;                      X=1 if at EOF, else X=0
;Error returns.......: None
.proc cmd_go_right
    ;Move mem pointer one step right, carry set if at end of file, X=1 if at end of line
    jsr mem_crs_step_right
    bcs at_eof
    cpx #1
    beq at_eol

    ;Increase current column index
    jsr mem_cur_col_inc

    ;Move cursor one step right
    jsr cursor_move_right
    bcs at_rightmost
    
    clc
    ldx #0
    rts

at_rightmost:
    ;Increase current column index
    jsr mem_cur_col_inc

    ;Cursor at rightmost position but not end of line, need to scroll
    jsr mem_lnv_step_right

    clc
    ldx #0
    rts

at_eof:
    ;End of file, do nothing

    clc
    ldx #1
    rts

at_eol:
    ;Set current line and column to start of next row
    jsr mem_cur_col_ret
    jsr mem_cur_line_inc

    ;End of line, move cursor to start of next line, C=1 if at bottom of screen
    jsr cursor_move_crlf
    bcs at_bottom

    jsr mem_set_lnv_to_crs  ;Set first visible char to cursor, i.e. start of line

    sec
    ldx #0
    rts

at_bottom:
    ;Cursor at bottom of screen, but not end of file, need to scroll
    jsr mem_set_lnv_to_crs
    jsr mem_scr_move_down

    clc
    ldx #0
    rts
.endproc

;******************************************************************************
;Function name.......: cmd_go_up
;Purpose.............: Moves mem pointers and cursor one step up. Does not
;                      refresh screen; users of this procedure are responsible
;                      to do screen refresh
;Input...............: Nothing
;Returns.............: C=1 if at first line, else C=0
;Error returns.......: None
.proc cmd_go_up
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

    ;Set current line and column index to start of line above
    jsr mem_cur_col_ret
    jsr mem_cur_line_dec

    ;Move screen cursor to beginning of the line above
    stz CRS_X
    ldy CRS_Y
    jsr cursor_move_up
    bcc stepto_column                       ;If carry set, we're at top of screen, need to scroll first
    jsr mem_scr_move_up

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

    ;Increase current column index
    jsr mem_cur_col_inc

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

    clc

    rts

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

    sec

    rts

count:
    .byt 0,0

backup:
    .byt 0,0,0,0,0,0
.endproc

;******************************************************************************
;Function name.......: cmd_go_down
;Purpose.............: Moves mem pointers and cursor one step down. Does not
;                      refresh screen; users of this procedure are responsible
;                      to do screen refresh
;Input...............: Nothing
;Returns.............: C=1 if at last line, else C=0
;Error returns.......: None
.proc cmd_go_down
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

    ;Set current line and column index to start of next line
    jsr mem_cur_col_ret
    jsr mem_cur_line_inc

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

exit:
    stz CRS_ADR
    clc

    rts

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

    sec

    rts

count:
    .byt 0,0

backup:
    .byt 0,0,0,0,0,0
.endproc

;******************************************************************************
;Function name.......: cmd_go_home
;Purpose.............: Moves mem pointers and cursor to start of line. Does not
;                      refresh screen; users of this procedure are responsible
;                      to do screen refresh
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_go_home
    ;Move cursor to leftmost position
    ldx #0
    ldy CRS_Y
    jsr cursor_move

    ;Set current column index to start of line
    jsr mem_cur_col_ret
    
    ;Move cursor mem pointer to start of line, and set first visible char to same
    jsr mem_crs_move_to_line_start
    jsr mem_set_lnv_to_crs

    rts
.endproc

;******************************************************************************
;Function name.......: cmd_go_end
;Purpose.............: Moves mem pointers and cursor to end of line. Does not
;                      refresh screen; users of this procedure are responsible
;                      to do screen refresh
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_go_end
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

    ;Increase current column index
:   jsr mem_cur_col_inc

    ;Decrease counter
    dec counter
    lda counter
    cmp #$ff
    bne loop
    dec counter+1
    jmp loop

exit:
    rts

counter:
    .byt 0,0
.endproc

;******************************************************************************
;Function name.......: cmd_exit
;Purpose.............: Command: Initiate program exit
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_exit
    ;Check if document modified first
    lda mem_modified
    beq :+
    
    ldx #<msg
    ldy #>msg
    jsr screen_print_status

    lda #8
    sta APP_MOD
    rts

    ;Set quit signal
:   lda #1          ;APP_QUIT=1 signals to irq_handler to close down
    sta APP_QUIT
    rts

msg:
    .byt "buffer modified. save before exit? (y/n)",0
.endproc

;******************************************************************************
;Function name.......: cmd_show_help
;Purpose.............: Command: Show help screen
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_show_help
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
;Function name.......: cmd_file_save
;Purpose.............: Command: Write file to disk
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_file_save
    lda #4
    sta APP_MOD     ;mode_file_save

    ldx #<msg
    ldy #>msg
    lda #250        ;maxlen
    jsr prompt_init

    ldx #<file_cur_filename
    ldy #>file_cur_filename
    lda file_cur_filename_len
    jmp prompt_default_input

msg:
    .byt "write to file name:",0
.endproc

;******************************************************************************
;Function name.......: cmd_file_open
;Purpose.............: Command: Read file from disk
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_file_open
    lda mem_modified
    beq :+
    
    lda #7
    sta APP_MOD
    
    ldx #<msg2
    ldy #>msg2
    jsr screen_print_status

    rts

:   lda #5
    sta APP_MOD     ;mode_file_open

    ldx #<msg
    ldy #>msg
    lda #250        ;maxlen
    jmp prompt_init
msg:
    .byt "open file name:",0
msg2:
    .byt "buffer modified. save before open file? (y/n)",0
.endproc

;******************************************************************************
;Function name.......: cmd_cut
;Purpose.............: Command: Cut line of text to clipboard
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_cut
    lda #2
    sta APP_MOD         ;mode_statusmessage

    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    rts

msg:
    .byt "cut line not yet implemented", 0
.endproc

;******************************************************************************
;Function name.......: cmd_paste
;Purpose.............: Command: Paste lines from clipboard
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_paste
    lda #2 
    sta APP_MOD         ;mode_statusmessage

    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    rts

msg:
    .byt "uncut not yet implemented", 0
.endproc

;******************************************************************************
;Function name.......: cmd_show_curpos
;Purpose.............: Command: Display current cursor position
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_show_curpos
    lda #2
    sta APP_MOD         ;mode_statusmessage

    ;Prepare index
    lda #23
    sta index

    ;Print line
    ldx mem_cur_line
    ldy mem_cur_line+1
    lda mem_cur_line+2
    jsr util_bin_to_bcd
    jsr util_bcd_to_str

    stx TMP1_ADR
    sty TMP1_ADR+1
    ldy #0
    
:   lda (TMP1_ADR),y
    beq :+
    ldx index
    sta msg,x
    inc index
    iny
    jmp :-

    ;Print msg_col
:   ldy #0

:   lda msg_col,y
    beq :+
    ldx index
    sta msg,x
    iny
    inc index
    jmp :-

    ;Print column
:   ldx mem_cur_col
    ldy mem_cur_col+1
    lda mem_cur_col+2
    jsr util_bin_to_bcd
    jsr util_bcd_to_str
    
    stx TMP1_ADR
    sty TMP1_ADR+1

    ldy #0

:   lda (TMP1_ADR),y
    beq :+
    ldx index
    sta msg,x
    inc index
    iny
    jmp :-

    ;Terminate string
:   ldx index
    lda #0
    sta msg,x
    
    ;Display in status row
    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    rts

msg:
    .byt "current position: line "
    .res 9+16+1, 0
msg_col:
    .byt ", column ", 0

index:
    .byt 0
.endproc

;******************************************************************************
;Function name.......: cmd_find
;Purpose.............: Prompt user for string to search for
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_find
    lda #10
    sta APP_MOD         ;mode_statusmessage

    ldx #<msg
    ldy #>msg
    lda #20
    jmp prompt_init

msg:
    .byt "search:", 0
.endproc

;******************************************************************************
;Function name.......: cmd_do_find
;Purpose.............: Execute string search
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_do_find
    ;Init & backup pointers if they need to be restored
    stz match

    lda CRS_BNK
    sta BNK_SEL

    sta crs_backup
    lda CRS_ADR+1
    sta crs_backup+1
    lda CRS_IDX
    sta crs_backup+2

    lda LNV_BNK
    sta lnv_backup
    lda LNV_ADR+1
    sta lnv_backup+1
    lda LNV_IDX
    sta lnv_backup+2

    lda SCR_BNK
    sta scr_backup
    lda SCR_ADR+1
    sta scr_backup+1
    lda SCR_IDX
    sta scr_backup+2

    lda CRS_X
    sta cursor_backup
    lda CRS_Y
    sta cursor_backup+1

;Now search for that string
    jsr cursor_disable

search_loop:
    ldy CRS_IDX
    lda #5
    sta CRS_ADR
    lda (CRS_ADR),y
    ldx match
    cmp prompt_input,x
    beq char_match

    ;No match, restore match counter
    stz match
    jmp next_char

char_match:
    inc match
    lda match
    cmp prompt_len
    beq goto_match

next_char:
    stz CRS_ADR
    jsr cmd_go_right
    cpx #1
    beq eof
    jmp search_loop

goto_match:
    stz CRS_ADR

:   dec match
    beq :+
    jsr cmd_go_left
    jmp :-

:   jsr cursor_activate
    jsr screen_refresh
    jmp screen_println

eof:
    ;Restore pointers
    lda crs_backup
    sta CRS_BNK
    lda crs_backup+1
    sta CRS_ADR+1
    lda crs_backup+2
    sta CRS_IDX

    lda lnv_backup
    sta LNV_BNK
    lda lnv_backup+1
    sta LNV_ADR+1
    lda lnv_backup+2
    sta LNV_IDX

    lda scr_backup
    sta SCR_BNK
    lda scr_backup+1
    sta SCR_ADR+1
    lda scr_backup+2
    sta SCR_IDX

    ldx cursor_backup
    ldy cursor_backup+1
    jsr cursor_move
    jsr cursor_activate

    ;Display not found message
    lda #2
    sta APP_MOD
    ldx #<msg
    ldy #>msg
    jmp screen_print_status

match:
    .byt 0

crs_backup:
    .byt 0,0,0

lnv_backup:
    .byt 0,0,0

scr_backup:
    .byt 0,0,0

cursor_backup:
    .byt 0,0

msg:
    .byt "string not found", 0
.endproc

;******************************************************************************
;Function name.......: cmd_set_tab_width
;Purpose.............: Command: Set tab width (number of spaces)
;Preparatory routines: None
;Input...............: A = tab width (0..9)
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_set_tab_width
    sta keyboard_tabwidth
    rts
.endproc

;******************************************************************************
;Function name.......: cmd_new_buffer
;Purpose.............: Command: Create a new empty text buffer
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_new_buffer
    ;Check if document modified
    lda mem_modified
    beq :+

    ldx #<msg
    ldy #>msg
    jsr screen_print_status

    lda #9
    sta APP_MOD
    rts
    
:   jsr mem_init
    
    ldx #0
    ldy #2
    jsr cursor_move

    stz file_cur_filename_len

    jsr screen_refresh
    jsr screen_print_header
    rts

msg:
    .byt "buffer modified. save before create new buffer? (y/n)",0
.endproc

;******************************************************************************
;Function name.......: cmd_mem_usage
;Purpose.............: Displays memory usage
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_mem_usage
    ldx mem_blocks_free
    ldy mem_blocks_free+1
    lda #0
    jsr util_bin_to_bcd
    jsr util_bcd_to_str

    stx TMP1_ADR
    sty TMP1_ADR+1

    ldx #14
    ldy #0

:   lda (TMP1_ADR),y
    beq insert_msg_end
    sta msg,x
    iny
    inx
    jmp :-

insert_msg_end:
    ldy #0
:   lda msg_end,y
    beq exit
    sta msg,x
    iny
    inx
    jmp :-

exit:
    sta msg,x

    jsr screen_clear_status
    
    lda #2
    sta APP_MOD

    ldx #<msg
    ldy #>msg
    jsr screen_print_status

    rts  

msg:
    .byt "memory usage: 00000000             "

msg_end:
    .byt " blocks free", 0

.endproc

;******************************************************************************
;Function name.......: cmd_goto_line
;Purpose.............: Command: Goto line number, prompt for input
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_goto_line
    lda #11
    sta APP_MOD

    ldx #<msg
    ldy #>msg
    lda #7
    jmp prompt_init

msg:
    .byt "go to line number:",0
.endproc

;******************************************************************************
;Function name.......: cmd_do_goto_line
;Purpose.............: Command: Goto line number, execute
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_do_goto_line
    stz APP_MOD
    jsr prompt_close
    
    ;Parse prompt input
    ldx #<prompt_input
    ldy #>prompt_input

    ;Append null char
    stx TMP1_ADR
    sty TMP1_ADR+1
    ldy prompt_len
    lda #0
    sta (TMP1_ADR),y
    ldy TMP1_ADR+1

    ;Convert string to BCD value
    jsr util_str_to_bcd
    bcs invalid_input

    ;Convert BCD value to binary value, and store it
    jsr util_bcd_to_bin
    stx line
    sty line+1
    sta line+2

    ;Goto start of line
    jsr cmd_go_home

    ;Compare
    lda mem_cur_line+2
    cmp line+2
    bcc go_down
    beq :+
    jmp go_up

:   lda mem_cur_line+1
    cmp line+1
    bcc go_down
    beq :+
    jmp go_up

:   lda mem_cur_line
    cmp line
    bcc go_down
    beq :+
    jmp go_up

:   rts         ;Already at that line

go_up:
    jsr cmd_go_up
    bcs exit

    lda mem_cur_line
    cmp line
    bne go_up
    lda mem_cur_line+1
    cmp line+1
    bne go_up
    lda mem_cur_line+1
    cmp line+1
    bne go_up
    jsr screen_refresh
    rts

go_down:
    jsr cmd_go_down
    bcs exit

    lda mem_cur_line
    cmp line
    bne go_down
    lda mem_cur_line+1
    cmp line+1
    bne go_down
    lda mem_cur_line+1
    cmp line+1
    bne go_down

exit:
    jsr screen_refresh
    rts

invalid_input:
    lda #2
    sta APP_MOD
    ldx #<msg
    ldy #>msg
    jsr screen_print_status

    rts

line:
    .byt 0,0,0

msg:
    .byt "invalid line number",0
.endproc