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
;Function name.......: cmd_init
;Purpose.............: Init commands
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_init
    ;Set tab width
    lda #4
    sta keyboard_tabwidth

    ;Set auto indent = off
    stz cmd_auto_indent_status

    ;Set word wrap = off
    stz cmd_wordwrap_mode
    lda #80
    sta cmd_wordwrap_pos
    rts
.endproc

;******************************************************************************
;Function name.......: cmd_insert
;Purpose.............: Inserts one char at cursor position and moves the cursor
;                      one step right. Does not refresh screen; users of this 
;                      procedure are responsible to do screen refresh when done
;Input...............: A = Char to insert
;Returns.............: Nothing
;Error returns.......: C=1 if a line feed char was inserted, either directly
;                      or by the word wrap function
.proc cmd_insert
    ;Is linefeed char?
    cmp #LF
    bne :+ 
    jmp cmd_insert_lf
    
    ;Store char in mem
:   jsr mem_insert
    bcc :+
    jmp mem_full

    ;Increase current columm index
:   jsr mem_cur_col_inc

    ;Move cursor one step right
    jsr cursor_move_right
    bcs :+
    jmp wrap

    ;Carry set, cursor at rightmost position, need to scroll
:   jsr mem_lnv_step_right

wrap:
    ;Word wrap enabled?
    lda cmd_wordwrap_mode
    beq exit                ;Word wrap mode off, we're done
    jmp cmd_word_wrap

exit:
    clc
    rts

mem_full:
    jsr cmd_mem_full_msg
    clc
    rts
.endproc

;******************************************************************************
;Function name.......: cmd_word_wrap
;Purpose.............: If cursor is at set right margin, inserts a line break
;                      to wrap the current line at the previous blank space.
;                      If no blank space on the line, wraps at the right
;                      margin.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: C=1 if a line feed char was inserted, either directly
;                      or by the word wrap function
.proc cmd_word_wrap
    ;Check if we're at the column where to wrap the line
    lda mem_cur_col+1
    bne nowrap
    lda mem_cur_col+2
    bne nowrap
    lda mem_cur_col
    cmp cmd_wordwrap_pos
    beq wrap

nowrap:
    clc
    rts

wrap:
    ;Backup pointers
    lda CRS_BNK
    sta crs_backup
    lda CRS_ADR+1
    sta crs_backup+1
    lda CRS_IDX
    sta crs_backup+2

    ;Goto prev space
    jsr mem_crs_move_to_prev_blankspace
    bcs bs_not_found

bs_found:
    stx prevblank
    sty prevblank+1

    jsr cmd_insert_lf

wrap_step_back:
    lda prevblank
    bne :+
    lda prevblank+1
    bne :+
    jmp wrap_done

:   jsr cmd_go_right
    
    dec prevblank
    lda prevblank
    cmp #$ff
    bne wrap_step_back
    dec prevblank+1
    jmp wrap_step_back

wrap_done:
    sec
    rts

bs_not_found:
    lda crs_backup
    sta CRS_BNK
    lda crs_backup+1
    sta CRS_ADR+1
    lda crs_backup+2
    sta CRS_IDX

    lda #LF
    jsr cmd_insert_lf

    sec
    rts

crs_backup = cmd_word_wrap_crs_backup   ;3 bytes
prevblank = cmd_word_wrap_prevblank     ;2 bytes
.endproc

;******************************************************************************
;Function name.......: cmd_insert_lf
;Purpose.............: Inserts line break char at cursor position and moves the 
;                      cursor one step right. Does not refresh screen; users of 
;                      this procedure are responsible to do screen refresh when 
;                      done
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: C=1 if a line feed char was inserted, either directly
;                      or by the word wrap function
.proc cmd_insert_lf
    ;Is auto indent on?
    lda cmd_auto_indent_status
    beq linefeed_store          ;No, continue storing the line feed in buffer

    ;Backup mem pointer
    lda CRS_BNK
    sta crs_backup
    lda CRS_ADR+1
    sta crs_backup+1
    lda CRS_IDX
    sta crs_backup+2

    ;Goto line start
    jsr mem_crs_move_to_line_start
    stx counter1
    sty counter1+1

    stz counter2
    stz counter2+1

auto_indent_loop:
    ;Count leading blank spaces on the line
    lda counter1+1
    cmp counter2+1
    bne :+
    lda counter1
    cmp counter2
    beq auto_indent_stop

:   lda CRS_BNK
    sta BNK_SEL
    lda #5
    sta CRS_ADR
    ldy CRS_IDX
    lda (CRS_ADR),y
    stz CRS_ADR
    cmp #32
    bne auto_indent_stop

    jsr mem_crs_step_right

    inc counter2
    bne auto_indent_loop
    inc counter2+1
    jmp auto_indent_loop

auto_indent_stop:
    ;Restore mem pointer
    lda crs_backup
    sta CRS_BNK
    lda crs_backup+1
    sta CRS_ADR+1
    lda crs_backup+2
    sta CRS_IDX

linefeed_store:
    ;Store char in mem
    lda #LF
    jsr mem_insert
    bcs mem_full
    
    ;Set line and column index
    jsr mem_cur_col_ret
    jsr mem_cur_line_inc

    ;Move cursor to first column of next row
    jsr cursor_move_crlf
    bcc :+          
    
    ;C=1: We're at bottom of screen, need to scroll
    jsr mem_scr_move_down

:   jsr mem_set_lnv_to_crs        ;Set first visible char of line to cursor position, i.e. start of line

    ;If auto indent on, insert blank spaces
    lda cmd_auto_indent_status
    beq linefeed_exit

auto_indent_insert_blanks:
    lda counter2
    bne :+
    lda counter2+1
    beq linefeed_exit

:   lda #32
    jsr cmd_insert

    dec counter2
    lda counter2
    cmp #$ff
    bne auto_indent_insert_blanks
    dec counter2+1
    jmp auto_indent_insert_blanks

linefeed_exit:
    sec
    rts

mem_full:
    jsr cmd_mem_full_msg
    clc
    rts

crs_backup = cmd_insert_lf_crs_backup   ;3 bytes
counter1 = cmd_insert_lf_counter1       ;2 bytes
counter2 = cmd_insert_lf_counter2       ;2 bytes
.endproc

;******************************************************************************
;Function name.......: cmd_mem_full_msg
;Purpose.............: Shows memory full status message
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc cmd_mem_full_msg
    ldx #<msg
    ldy #>msg
    lda #2
    sta APP_MOD
    jsr screen_print_status
    rts

msg:
    .byt "memory full", 0
.endproc

;******************************************************************************
;Function name.......: cmd_insert_tab
;Purpose.............: Inserts blank spaces until we reach next tab stop
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc cmd_insert_tab
    ;Find next tab stop by adding the tab width until we pass the current column index
    lda #1
    sta counter
    stz counter+1
    stz counter+2
    
loop:
    lda counter+2
    cmp mem_cur_col+2
    bcc :+
    lda counter+1
    cmp mem_cur_col+1
    bcc :+
    lda counter
    cmp mem_cur_col
    beq :+
    bcs at_tab_stop

:   clc
    lda counter
    adc keyboard_tabwidth
    sta counter
    
    lda counter+1
    adc #0
    sta counter+1

    lda counter+2
    adc #0
    sta counter+2

    jmp loop
    

at_tab_stop:
    ;Calculate difference
    sec
    
    lda counter
    sbc mem_cur_col
    sta counter
    
    lda counter+1
    sbc #0
    sta counter+1
    
    lda counter+2
    sbc #0
    sta counter+2

    ;Insert blank spaces
:   lda #32
    jsr cmd_insert
    dec counter
    bne :-

    ;Exit and refresh line
    jmp screen_println

counter = cmd_insert_tab_counter    ;3 bytes

.endproc

;******************************************************************************
;Function name.......: cmd_delete
;Purpose.............: Moves the cursor one step left and deletes that char
;                      Does not refresh screen; users of this procedure are 
;                      responsible to do screen refresh when done
;Input...............: Nothing
;Returns.............: C=1 if at start of file, else C=0
;                      X=1 if a line feed was deleted, else X=0
;Error returns.......: None
.proc cmd_delete
    ;Delete one char from memory
    jsr mem_delete
    bcc char_deleted                ;C=1: We're at start of file, nothing was deleted
    
    ldx #0
    rts

    ;Check if we deleted a line break (indicated by X=1), a special case to handle
char_deleted:
    cpx #1
    beq linebreak_deleted

    ;We did not delete a line break, decrease column index, move cursor one column left and print line
    jsr mem_cur_col_dec
    jsr cursor_move_left
    bcc exit
    jsr mem_lnv_step_left
    
exit:
    clc
    ldx #0
    rts

linebreak_deleted:
    ;Move cursor the beginning of line above the current line
    stz CRS_X
    jsr cursor_move_up
    bcc :+
    
    ;Carry set, we're at top of screen and need to scroll vertically
    jsr mem_scr_move_up

:   ;Move to line start, and save the number of chars moved over
    jsr mem_crs_move_to_line_start
    stx counter
    sty counter+1

    ;Set first visible char of that line to start of line (cursor position)
    jsr mem_set_lnv_to_crs

    ;Set line and column index
    jsr mem_cur_col_ret
    jsr mem_cur_line_dec

;Loop to move the cursor back to the column we came from
loop:
    lda counter
    bne :+
    lda counter+1
    beq linebreak_deleted_exit

:   jsr mem_crs_step_right
    bcs linebreak_deleted_exit
    
    jsr mem_cur_col_inc
    jsr cursor_move_right
    bcc continue

    ;C=1: We need to scroll horizontally
    jsr mem_lnv_step_right

continue:
    ;Decrease column counter
    dec counter
    lda counter
    cmp #$ff
    bne loop
    dec counter+1
    jmp loop
    
linebreak_deleted_exit:
    clc
    ldx #1
    rts

counter = cmd_delete_counter    ;2 bytes
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

    ;Set line and column index
    jsr mem_cur_col_ret
    jsr mem_cur_line_dec

    ;Move cursor up one row to start of that row
    stz CRS_X
    jsr cursor_move_up
    bcc :+
    ;C=1, we're at top of screen, need to scroll
    jsr mem_scr_move_up

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

count = cmd_go_up_count     ;2 bytes
backup = cmd_go_up_backup   ;6 bytes
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

    jsr mem_cur_col_inc

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

count = cmd_go_down_count       ;2 bytes
backup = cmd_go_down_backup     ;6 bytes
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

counter = cmd_go_end_counter        ;2 bytes
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
;Function name.......: cmd_cut
;Purpose.............: Command: Cut line of text to clipboard
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_cut
    ;Backup pointers, should we need to restore on mem full
    lda CRS_BNK
    sta crs_backup
    lda CRS_ADR+1
    sta crs_backup+1
    lda CRS_IDX
    sta crs_backup+2

    lda SCR_BNK
    sta scr_backup
    lda SCR_ADR+1
    sta scr_backup+1
    lda SCR_IDX
    sta scr_backup+2

    lda LNV_BNK
    sta lnv_backup
    lda LNV_ADR+1
    sta lnv_backup+1
    lda LNV_IDX
    sta lnv_backup+2

    lda mem_cur_col
    sta col_backup
    lda mem_cur_col+1
    sta col_backup+1
    lda mem_cur_col+2
    sta col_backup+2

    lda mem_cur_line
    sta line_backup
    lda mem_cur_line+1
    sta line_backup+1
    lda mem_cur_line+2
    sta line_backup+2

    lda CRS_X
    sta cursor
    lda CRS_Y
    sta cursor+1

    ;Cut line
    jsr clipboard_cut
    bcs mem_full
    jmp screen_refresh

mem_full:
    ;Restore pointers
    lda crs_backup
    sta CRS_BNK
    lda crs_backup+1
    sta CRS_ADR+1
    lda crs_backup+2
    sta CRS_IDX
    
    lda scr_backup
    sta SCR_BNK
    lda scr_backup+1
    sta SCR_ADR+1
    lda scr_backup+2
    sta SCR_IDX

    lda lnv_backup
    sta LNV_BNK
    lda lnv_backup+1
    sta LNV_ADR+1
    lda lnv_backup+2
    sta LNV_IDX

    lda col_backup
    sta mem_cur_col
    lda col_backup+1
    sta mem_cur_col+1
    lda col_backup+2
    sta mem_cur_col+2

    lda line_backup
    sta mem_cur_line
    lda line_backup+1
    sta mem_cur_line+1
    lda line_backup+2
    sta mem_cur_line+2

    ldx cursor
    ldy cursor+1
    jsr cursor_move

    ;Display message
    lda #2
    sta APP_MOD
    
    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    rts

msg:
    .byt "clipboard full, could not copy line", 0

crs_backup = cmd_cut_crs_backup     ;3 bytes
scr_backup =  cmd_cut_scr_backup    ;3 bytes
lnv_backup = cmd_cut_lnv_backup     ;3 bytes
line_backup = cmd_cut_line_backup   ;3 bytes
col_backup = cmd_cut_col_backup     ;3 bytes
cursor = cmd_cut_cursor             ;2 bytes
.endproc

;******************************************************************************
;Function name.......: cmd_copy
;Purpose.............: Command: Copy line of text to clipboard
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_copy
    ;Backup pointers, should we need to restore on mem full
    lda CRS_BNK
    sta crs_backup
    lda CRS_ADR+1
    sta crs_backup+1
    lda CRS_IDX
    sta crs_backup+2

    lda SCR_BNK
    sta scr_backup
    lda SCR_ADR+1
    sta scr_backup+1
    lda SCR_IDX
    sta scr_backup+2

    lda LNV_BNK
    sta lnv_backup
    lda LNV_ADR+1
    sta lnv_backup+1
    lda LNV_IDX
    sta lnv_backup+2

    lda mem_cur_col
    sta col_backup
    lda mem_cur_col+1
    sta col_backup+1
    lda mem_cur_col+2
    sta col_backup+2

    lda mem_cur_line
    sta line_backup
    lda mem_cur_line+1
    sta line_backup+1
    lda mem_cur_line+2
    sta line_backup+2

    lda CRS_X
    sta cursor
    lda CRS_Y
    sta cursor+1

    ;Copy
    jsr clipboard_copy
    bcs mem_full
    jmp screen_refresh

mem_full:
    ;Restore pointers
    lda crs_backup
    sta CRS_BNK
    lda crs_backup+1
    sta CRS_ADR+1
    lda crs_backup+2
    sta CRS_IDX
    
    lda scr_backup
    sta SCR_BNK
    lda scr_backup+1
    sta SCR_ADR+1
    lda scr_backup+2
    sta SCR_IDX

    lda lnv_backup
    sta LNV_BNK
    lda lnv_backup+1
    sta LNV_ADR+1
    lda lnv_backup+2
    sta LNV_IDX

    lda col_backup
    sta mem_cur_col
    lda col_backup+1
    sta mem_cur_col+1
    lda col_backup+2
    sta mem_cur_col+2

    lda line_backup
    sta mem_cur_line
    lda line_backup+1
    sta mem_cur_line+1
    lda line_backup+2
    sta mem_cur_line+2

    ldx cursor
    ldy cursor+1
    jsr cursor_move

    ;Display message
    lda #2
    sta APP_MOD
    
    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    rts

msg:
    .byt "clipboard full, could not copy line", 0

crs_backup = cmd_copy_crs_backup     ;3 bytes
scr_backup = cmd_copy_scr_backup     ;3 bytes
lnv_backup = cmd_copy_lnv_backup     ;3 bytes
line_backup = cmd_copy_line_backup   ;3 bytes
col_backup = cmd_copy_col_backup     ;3 bytes
cursor = cmd_copy_cursor             ;2 bytes

.endproc

;******************************************************************************
;Function name.......: cmd_paste
;Purpose.............: Command: Paste lines from clipboard
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_paste
    ;Check if clipboard is empty
    lda clipboard_pointer+1
    bne not_empty
    lda clipboard_pointer
    cmp #>clipboard_mem
    bne not_empty

    lda #2
    sta APP_MOD
    ldx #<msg
    ldy #>msg
    jmp screen_print_status
    
not_empty:
    jsr cursor_disable
    jsr clipboard_paste
    jsr screen_refresh
    jmp cursor_activate

msg:
    .byt "clipboard empty", 0
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

    ;Prepare message start
    ldx #0
:   lda msg_start,x
    beq :+
    sta msg,x
    inx
    jmp :-

    ;Prepare index
:   lda #23
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

msg = cmd_display_msg

msg_start:
    .byt "current position: line ",0
msg_col:
    .byt ", column ", 0

index = cmd_show_curpos_index       ;1 byte
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
    .byt "search for:", 0
.endproc

;******************************************************************************
;Function name.......: cmd_do_find
;Purpose.............: Execute string search
;Preparatory routines: None
;Input...............: Pointer to search string, X=AddressL, Y=AddressH
;                      A=string len
;Returns.............: C=1 if not found, else C=0
;Error returns.......: None
.proc cmd_do_find
    sta string_len
    stx TMP2_ADR
    sty TMP2_ADR+1

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

    lda mem_cur_col
    sta col_backup
    lda mem_cur_col+1
    sta col_backup+1
    lda mem_cur_col+2
    sta col_backup+2

    lda mem_cur_line
    sta line_backup
    lda mem_cur_line+1
    sta line_backup+1
    lda mem_cur_line+2
    sta line_backup+2

    lda CRS_X
    sta cursor_backup
    lda CRS_Y
    sta cursor_backup+1

;Now search for that string

search_loop:
    ldy CRS_IDX
    lda #5
    sta CRS_ADR
    lda (CRS_ADR),y
    ldy match
    cmp (TMP2_ADR),y
    beq char_match

    ;No match, restore match counter
    stz match
    jmp next_char

char_match:
    inc match
    lda match
    cmp string_len
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

:   clc
    rts

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

    lda col_backup
    sta mem_cur_col
    lda col_backup+1
    sta mem_cur_col+1
    lda col_backup+2
    sta mem_cur_col+2

    lda line_backup
    sta mem_cur_line
    lda line_backup+1
    sta mem_cur_line+1
    lda line_backup+2
    sta mem_cur_line+2

    ldx cursor_backup
    ldy cursor_backup+1
    jsr cursor_move
    
    sec
    rts

string_len = cmd_do_find_string_len         ;1 byte
match = cmd_do_find_match                   ;1 byte
crs_backup = cmd_do_find_crs_backup         ;3 bytes
lnv_backup = cmd_do_find_lnv_backup         ;3 bytes
scr_backup = cmd_do_find_scr_backup         ;3 bytes
cursor_backup = cmd_do_find_cursor_backup   ;2 bytes
col_backup = cmd_do_find_col_backup         ;3 bytes
line_backup = cmd_do_find_line_backup       ;3 bytes

.endproc

;******************************************************************************
;Function name.......: cmd_replace
;Purpose.............: Replace string
;Input...............: A=0: Prompt for string to search for
;                      A=1: Save string to search for, and prompt for string
;                           to replace with; pointer to string to search for
;                           in X (AddressL) and Y (AddressH)
;                      A=2: Search for string and prompt user to confirm;
;                      pointer to string to replace with in X (AddressL) 
;                      and Y (AddressH)
;                      A=3: Execute replace command; X=0 replace this
;                      instance, X=1 replace all subsequent instances
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_replace
    ;A=0? Prompt for string to search for
    cmp #0
    bne prompt_replace
    
    lda #12
    sta APP_MOD

    ldx #<msg1
    ldy #>msg1
    lda #20
    jmp prompt_init

prompt_replace:
    cmp #1
    bne search

    ldy #0
    
:   cpy prompt_len
    bcs :+
    lda prompt_input,y
    sta searchfor,y
    iny
    jmp :-

:   lda #0
    sta searchfor,y
    lda prompt_len
    sta searchfor_len

    lda #13
    sta APP_MOD
    ldx #<msg2
    ldy #>msg2
    lda #20
    jmp prompt_init

search:
    cmp #2
    bne execute
    
    ;Search for string to replace
    ldx #<searchfor
    ldy #>searchfor
    lda searchfor_len
    jsr cmd_do_find
    bcs notfound
    jsr screen_println

    lda #14
    sta APP_MOD
    ldx #<msg3
    ldy #>msg3
    jmp screen_print_status

notfound:
    ldx #<msg_notfound
    ldy #>msg_notfound
    jsr screen_print_status
    lda #2
    sta APP_MOD
    rts

execute:
    cmp #3
    beq :+
    
    rts

:   stx replace_all
    stz APP_MOD
    jsr cursor_disable
    
    stz replaced_count
    stz replaced_count+1
    stz replaced_count+2

execute_loop:
    lda searchfor_len
    sta counter

:   lda counter
    beq :+
    dec counter
    jsr cmd_go_right
    jmp :-

:   lda searchfor_len
    sta counter

:   lda counter
    beq :+
    dec counter
    jsr cmd_delete
    jmp :-

:   ldx #0
    stx counter

:   ldx counter
    cpx prompt_len
    beq execute_done
    lda prompt_input,x
    jsr cmd_insert
    inc counter
    jmp :-

execute_done:
    inc replaced_count

    lda replace_all
    beq :+

    ldx #<searchfor
    ldy #>searchfor
    lda searchfor_len
    jsr cmd_do_find
    bcc execute_loop

:   jsr cursor_activate
    jsr screen_clear_status
    jsr screen_refresh
    jsr screen_println

    lda replace_all
    beq exit

    ;Prepare message
    ldx #0
:   lda msg4_s,x
    beq :+
    sta msg4,x
    inx
    jmp :-

:   ldx replaced_count
    ldy replaced_count+1
    lda replaced_count+2
    jsr util_bin_to_bcd
    jsr util_bcd_to_str

    stx TMP1_ADR
    sty TMP1_ADR+1
    ldy #0
:   lda (TMP1_ADR),y
    beq :+
    sta msg4+9,y
    iny
    jmp :-

:   ldx #0
:   lda msg4_e,x
    sta msg4+9,y
    cmp #0
    beq :+
    iny
    inx
    jmp :-

:   ldx #<msg4
    ldy #>msg4
    jsr screen_print_status
    lda #2
    sta APP_MOD

exit:    
    rts

msg1:
    .byt "search to replace:", 0

msg2:
    .byt "replace with:", 0

msg3:
    .byt "replace this instance? y=yes, a=all or press esc to abort",0

msg4 = cmd_display_msg

msg4_s:
.byt "replaced ",0

msg4_e:
    .byt " occurence(s)",0

msg_notfound:
    .byt "string not found",0

searchfor = cmd_display_msg
searchfor_len = cmd_replace_searchfor_len       ;1 byte
counter = cmd_replace_counter                   ;1 byte
replace_all = cmd_replace_replace_all           ;1 byte
replaced_count = cmd_replace_replaced_count     ;3 bytes

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
    
    stz APP_MOD

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
    ;Prepare message start
    ldx #0
:   lda msg_start,x
    beq :+
    sta cmd_display_msg,x
    inx
    jmp :-

    ;Get free mem
:   ldx mem_blocks_free
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

msg = cmd_display_msg

msg_start:
    .byt "memory usage: ",0
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

line = cmd_do_goto_line_line        ;3 bytes

msg:
    .byt "invalid line number",0
.endproc

;******************************************************************************
;Function name.......: cmd_auto_indent
;Purpose.............: Enable or disable auto indent feature
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_auto_indent
    lda cmd_auto_indent_status
    beq :+
    
    stz cmd_auto_indent_status
    
    jsr screen_clear_status
    ldx #<msg_off
    ldy #>msg_off
    lda #2
    sta APP_MOD
    jmp screen_print_status
    
:   lda #1
    sta cmd_auto_indent_status

    jsr screen_clear_status
    ldx #<msg_on
    ldy #>msg_on
    lda #2
    sta APP_MOD
    jmp screen_print_status 

msg_on:
    .byt "auto indent on", 0
msg_off:
    .byt "auto indent off",0
.endproc

;******************************************************************************
;Function name.......: cmd_change_encoding
;Purpose.............: Changes the current encoding (charset) used by the
;                      program, i.e. ISO, PETSCII upper case or PETSCII
;                      lower case
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_change_encoding
    inc screen_mode

    lda screen_mode
    beq iso
    cmp #1
    beq petscii_uc
    cmp #2
    beq petscii_lc

    stz screen_mode

iso:
    bridge_setaddr KERNAL_CHROUT
    lda #$0f
    bridge_call KERNAL_CHROUT
    jmp exit

petscii_uc:
    bridge_setaddr KERNAL_CHROUT
    lda #$8f
    bridge_call KERNAL_CHROUT
    lda #$8e
    bridge_call KERNAL_CHROUT
    jmp exit

petscii_lc:
    bridge_setaddr KERNAL_CHROUT
    lda #$0e
    bridge_call KERNAL_CHROUT

exit:
    lda #16
    sta APP_MOD
    rts
.endproc

;******************************************************************************
;Function name.......: cmd_refresh_after_encoding_changed
;Purpose.............: Refresh screen after encoding was changed
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_refresh_after_encoding_changed
    jsr screen_clearall
    jsr screen_print_header
    jsr screen_print_footer
    jsr screen_refresh
    jsr screen_println
    
    lda #2
    sta APP_MOD

    lda screen_mode
    beq iso
    cmp #1
    beq pet_uc
    cmp #2
    beq pet_lc

iso:
    ldx #<msg_iso
    ldy #>msg_iso
    jmp screen_print_status

pet_uc:
    ldx #<msg_puc
    ldy #>msg_puc
    jmp screen_print_status

pet_lc:
    ldx #<msg_plc
    ldy #>msg_plc
    jmp screen_print_status

msg_iso:
    .byt "iso charset selected",0
msg_puc:
    .byt "petscii upper case/graphics selected",0
msg_plc:
    .byt "petscii upper/lower case selected", 0
.endproc

;******************************************************************************
;Function name.......: cmd_rotate_text_color
;Purpose.............: Steps through the 16 available text colors, one for each
;                      invocation of this function
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_rotate_text_color
    stz APP_MOD

    lda screen_color
    and #240
    sta bgcolor

    lda screen_color
    ina
    and #15
    ora bgcolor
    sta screen_color

    jsr screen_clearall
    jsr screen_print_header
    jsr screen_print_footer
    jsr screen_refresh
    jmp screen_println

bgcolor = tempvars      ;1 bytes

.endproc

;******************************************************************************
;Function name.......: cmd_rotate_background_color
;Purpose.............: Steps through the 16 available text colors, one for each
;                      invocation of this function
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_rotate_background_color
    stz APP_MOD

    lda screen_color
    and #15
    sta fgcolor

    clc
    lda screen_color
    adc #16
    and #240
    ora fgcolor
    sta screen_color

    jsr screen_clearall
    jsr screen_print_header
    jsr screen_print_footer
    jsr screen_refresh
    jmp screen_println

fgcolor = tempvars
.endproc

;******************************************************************************
;Function name.......: cmd_set_word_wrap
;Purpose.............: Turns word wrap on or off
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_set_word_wrap
    lda cmd_wordwrap_mode
    bne off

on:
    lda APP_MOD
    beq showprompt

set:
    lda prompt_len
    beq illegal_value

    ldx #0
:   lda prompt_input,x
    sta col,x
    inx
    cpx prompt_len
    bne :-
    stz col,x

    ldx #<col
    ldy #>col
    jsr util_str_to_bcd
    bcs illegal_value
    jsr util_bcd_to_bin

    cpy #0
    bne illegal_value
    cpx #10
    bcc illegal_value
    cpx #251
    bcs illegal_value

    stx cmd_wordwrap_pos
    lda #1
    sta cmd_wordwrap_mode
    
    stz APP_MOD
    jmp prompt_close
    
illegal_value:
    jsr prompt_close
    lda #2
    sta APP_MOD
    ldx #<msg_illegal
    ldy #>msg_illegal
    jmp screen_print_status

showprompt:
    ldx #<msg_prompt
    ldy #>msg_prompt
    lda #3
    jsr prompt_init

    ldx cmd_wordwrap_pos
    ldy #0
    lda #0
    jsr util_bin_to_bcd
    jsr util_bcd_to_str
    stx TMP1_ADR
    sty TMP1_ADR+1
    
    ldy #0
:   lda (TMP1_ADR),y
    beq :+
    iny
    jmp :-

:   tya
    ldx TMP1_ADR
    ldy TMP1_ADR+1
    jsr prompt_default_input

    lda #15
    sta APP_MOD
    rts

off:
    stz cmd_wordwrap_mode
    lda #2
    sta APP_MOD
    ldx #<msg_off
    ldy #>msg_off
    jmp screen_print_status

col = cmd_set_word_wrap_col     ;4 bytes

msg_prompt:
    .byt "set word wrap at column:", 0

msg_off:
    .byt "word wrap off", 0

msg_illegal:
    .byt "invalid value, column must be in range 10..250",0

.endproc