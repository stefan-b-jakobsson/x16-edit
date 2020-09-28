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

    cpx #1
    beq mode_helpscreen
    
    cpx #2
    beq mode_statusmessage
    
    cpx #3
    beq mode_command
    
    cpx #4
    beq mode_file_save
    
    cpx #5
    beq mode_file_open
    
    cpx #6
    beq mode_file_save_overwrite
    
    cpx #7
    beq mode_file_open_save_before
    
    cpx #8
    beq mode_exit_save_before
    
    cpx #9
    beq mode_new_buffer_save_before
    
    cpx #10
    beq mode_find
    
    cpx #11
    beq mode_goto_line
    
    cpx #12
    beq mode_replace_0
    
    cpx #13
    beq mode_replace_1
    
    cpx #14
    beq mode_replace_2

mode_default:
    jmp keyboard_mode_default

mode_helpscreen:
    jmp keyboard_mode_helpscreen

mode_statusmessage:
    jmp keyboard_mode_statusmessage

mode_command:
    jmp keyboard_mode_command

mode_file_save:
    jmp keyboard_mode_file_save

mode_file_open:
    jmp keyboard_mode_file_open

mode_file_save_overwrite:
    jmp keyboard_mode_file_save_overwrite

mode_file_open_save_before:
    jmp keyboard_mode_file_open_save_before

mode_exit_save_before:
    jmp keyboard_mode_exit_save_before

mode_new_buffer_save_before:
    jmp keyboard_mode_new_buffer_save_before

mode_find:
    jmp keyboard_mode_find

mode_goto_line:
    jmp keyboard_mode_goto_line

mode_replace_0:
    jmp keyboard_mode_replace_0
mode_replace_1:
    jmp keyboard_mode_replace_1
mode_replace_2:
    jmp keyboard_mode_replace_2

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_default
;Purpose.............: Select key press actions for default mode
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_default
    ;ESC key
    cmp #$03
    beq esc

    ;Enter key
    cmp #13
    beq enter
    
    ;Delete key
    cmp #$14
    beq delete

    ;Tab key
    cmp #$09
    beq tab_key

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

    ;Ctrl+k
    cmp #$0b
    beq ctrl_k

    ;Ctrl+b
    cmp #$02
    beq ctrl_b

    ;Ctrl+u
    cmp #$15
    beq ctrl_u

    ;Ctrl+p
    cmp #$10
    beq ctrl_p

    ;Ctrl+l
    cmp #$0c
    beq ctrl_l

    ;Ctrl+f
    cmp #$06
    beq ctrl_f

    ;Ctrl+r
    cmp #$12
    beq ctrl_r

    ;Ctrl+n
    cmp #$0e
    beq ctrl_n

    ;Function keys
    cmp #$85
    beq f1

    cmp #$89
    beq f2

    cmp #$86
    beq f3

    cmp #$8a
    beq f4

    cmp #$87
    beq f5

    cmp #$8b
    beq f6

    cmp #$88
    beq f7

    cmp #$8c
    beq f8

    ;If no other match, goto default handler
    jmp keyboard_default_keyhandler

esc:
    jmp keyboard_esc_key

enter:
    jmp keyboard_enter_key

delete:
    jmp keyboard_delete_key

tab_key:
    jmp keyboard_tab_key

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
    jmp cmd_show_help

ctrl_x:
    jmp cmd_exit

ctrl_w:
    jmp cmd_file_save

ctrl_o:
    jmp cmd_file_open

ctrl_y:
    jmp keyboard_cmd_pageup

ctrl_v:
    jmp keyboard_cmd_pagedown

ctrl_k:
    jmp cmd_cut

ctrl_b:
    jmp cmd_copy

ctrl_u:
    jmp cmd_paste

ctrl_p:
    jmp cmd_show_curpos

ctrl_l:
    jmp cmd_goto_line

ctrl_f:
    jmp cmd_find

ctrl_r:
    lda #0
    jmp cmd_replace

ctrl_n:
    jmp cmd_new_buffer

f1:
    jmp cmd_show_help

f2:
    jmp cmd_exit

f3:
    jmp cmd_file_save

f4:
    jmp cmd_file_open

f5:
    jmp cmd_cut

f6:
    jmp cmd_paste

f7:
    jmp cmd_show_curpos

f8:
    jmp cmd_find

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_exit_save_before
;Purpose.............: Key handler for prompt the user to save before exit 
;                      program
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_exit_save_before
    ;Y key pressed
    cmp #$79
    beq save

    ;N key pressed
    cmp #$6e
    beq discard

    ;ESC key pressed
    cmp #$03
    beq abort

    ;Else ignore keypress
    rts

save:
    jsr screen_clear_status
    jmp cmd_file_save

discard:
    ;Set app quit signal
    lda #1
    sta APP_QUIT
    rts

abort:
    stz APP_MOD
    jsr screen_clear_status
    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_helpscreen
;Purpose.............: Select key press actions for help screen
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_helpscreen
    ;Exit help screen if ESC was pressed, ignore all other input
    cmp #03
    beq exit_help
    rts

exit_help:
    stz APP_MOD
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
    ;Any key press will close the status message. 
    ;Set program to default mode (0) and clear status message
    stz APP_MOD
    jsr screen_clear_status
    jsr cursor_activate

    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_command
;Purpose.............: Select command after ESC has been pressed
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_command
    ;ESC key
    cmp #$03
    beq abort_command_mode

    ;g
    cmp #$67
    beq show_help

    ;x
    cmp #$78
    beq exit

    ;w
    cmp #$77
    beq save

    ;o
    cmp #$6f
    beq open

    ;y
    cmp #$79
    beq pgup

    ;v
    cmp #$76
    beq pgdown

    ;k
    cmp #$6b
    beq cut

    ;m
    cmp #$6d
    beq mem_usage

    ;u
    cmp #$75
    beq paste

    ;p
    cmp #$70
    beq curpos

    ;f
    cmp #$66
    beq find

    ;1..9
    cmp #$31
    bcc exit
    cmp #$3a
    bcc tab_width

    ;Uknown command, ignore
    rts

show_help:
    stz APP_MOD
    jsr screen_clear_status
    jmp cmd_show_help

exit:
    jmp cmd_exit

save:
    jmp cmd_file_save

open:
    jmp cmd_file_open

pgup:
    jmp keyboard_cmd_pageup

pgdown:
    jmp keyboard_cmd_pagedown

cut:
    jmp cmd_cut

mem_usage:
    jmp cmd_mem_usage

paste:
    jmp cmd_paste

curpos:
    jmp cmd_show_curpos

find:
    jmp cmd_find

tab_width:
    pha
    stz APP_MOD
    jsr screen_clear_status
    jsr cursor_activate
    pla
    sec
    sbc #48
    jmp cmd_set_tab_width

abort_command_mode:
    stz APP_MOD
    jsr cursor_activate
    jmp screen_clear_status

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_file_save
;Purpose.............: Handle keypress when prompting for file name to save to
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_file_save
    ;Enter key
    cmp #13
    beq write_file
    
    ;ESC key
    cmp #$03
    beq abort
    
    jmp prompt_keypress

abort:
    jsr prompt_close
    lda #0
    sta APP_MOD         ;mode_default
    rts

write_file:
    jsr cursor_disable

    lda prompt_len
    ldx #<prompt_input
    ldy #>prompt_input
    jsr file_write          ;Write file

    jsr prompt_close
    
    lda file_io_err         ;Check for Kernal I/O errors
    bne show_io_err
    
    jsr file_read_disk_status
    beq :+                  ;Do not set current file name if we had a disk error
    cmp #$63                ;File exists error
    beq confirm_overwrite
    jmp :++
    
:   stz mem_modified
    
    ldx #<prompt_input
    ldy #>prompt_input
    lda prompt_len
    jsr file_set_filename
    jsr screen_print_header

:   ldx #<file_disk_status
    ldy #>file_disk_status
    jsr screen_print_status ;Print disk status message

    jmp exit

show_io_err:
    ldx file_io_err
    dex
    lda file_ioerr_H,x
    tay
    lda file_ioerr_L,x
    tax
    jsr screen_print_status

exit:
    jsr cursor_activate
    jsr screen_refresh
    lda #2                  ;mode_statusmessage
    sta APP_MOD
    rts

confirm_overwrite:
    lda #6
    sta APP_MOD

    ldx #<overwrite_msg
    ldy #>overwrite_msg
    jmp screen_print_status

overwrite_msg:
    .byt "file exists. overwrite (y/n)?", 0

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_file_save_overwrite
;Purpose.............: Handle keypress, confirm overwriting file
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_file_save_overwrite
    ;Y key pressed
    cmp #$79
    beq write_file

    ;N key
    cmp #$6e
    beq abort

    ;ESC key
    cmp #$03
    beq abort

    ;Else ignore key
    rts

abort:
    stz APP_MOD         ;mode_default
    jsr cursor_activate
    jmp screen_clear_status

write_file:
    ;Prepend input with @0: to overwrite existing file
    stz prompt_curpos
    lda #64
    jsr prompt_insert
    inc prompt_curpos
    lda #'0'
    jsr prompt_insert
    inc prompt_curpos
    lda #':'
    jsr prompt_insert
    inc prompt_curpos
    
    ;Write file
    lda prompt_len
    ldx #<prompt_input
    ldy #>prompt_input
    jsr file_write

    jsr screen_clear_status
    
    lda file_io_err         ;Check for Kernal I/O errors
    bne show_io_err
    
    jsr file_read_disk_status
    bne :+                  ;Skip setting current file name if we had a disk error
    
    stz mem_modified

    ldx #<(prompt_input+3)
    ldy #>(prompt_input+3)
    sec
    lda prompt_len
    sbc #3
    jsr file_set_filename
    jsr screen_print_header

:   ldx #<file_disk_status
    ldy #>file_disk_status
    jsr screen_print_status ;Print disk status message

    jmp exit

show_io_err:
    ldx file_io_err
    dex
    lda file_ioerr_H,x
    tay
    lda file_ioerr_L,x
    tax
    jsr screen_print_status

exit:
    jsr cursor_activate
    jsr screen_refresh
    lda #2                  ;mode_statusmessage
    sta APP_MOD
    rts

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_file_open
;Purpose.............: Handle keypress when prompting for file name to rad from
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_file_open
    ;Enter key
    cmp #13
    beq read_file

    ;ESC key
    cmp #$03
    beq abort

    jmp prompt_keypress

abort:
    jsr prompt_close
    lda #0              ;mode_default
    sta APP_MOD
    rts

read_file:
    jsr cursor_disable

    lda prompt_len
    ldx #<prompt_input
    ldy #>prompt_input
    jsr file_read               ;Read file from disk

    jsr prompt_close

    lda file_io_err
    bne io_error

    jsr file_read_disk_status
    pha                         ;Save disk error code on stack

    jsr cursor_activate

    ldx #<file_disk_status      ;Display disk status message
    ldy #>file_disk_status
    jsr screen_print_status

    pla
    bne exit                    ;Exit if error code != $00

    ;Read successful
    ldx #0
    ldy #2
    jsr cursor_move

    stz mem_modified

    ldx #<prompt_input
    ldy #>prompt_input
    lda prompt_len
    jsr file_set_filename
    jsr screen_print_header
    
    jmp exit
    
io_error:
    ;Display error message
    ldx file_io_err
    dex
    lda file_ioerr_H,x
    tay
    lda file_ioerr_L,x
    tax
    jsr screen_print_status

exit:
    jsr cursor_activate
    jsr screen_refresh

    lda #2              ;mode_statusmessage
    sta APP_MOD

    rts

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_file_open_save_before
;Purpose.............: Handle keypress when asking to save file before open
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_file_open_save_before
    ;Y key pressed
    cmp #$79
    beq save

    ;N key pressed
    cmp #$6e
    beq discard

    ;ESC key
    cmp #$03
    beq abort

    ;Else ignore keypress
    rts

save:
    jsr screen_clear_status
    jmp cmd_file_save

discard:
    stz mem_modified
    jsr screen_clear_status
    jsr cmd_file_open
    inc mem_modified
    rts

abort:
    stz APP_MOD
    jsr screen_clear_status
    rts

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_new_buffer_save_before
;Purpose.............: Handle keypress when asking to save file before create
;                      new buffer
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_new_buffer_save_before
    ;Y key pressed
    cmp #$79
    beq save

    ;N key pressed
    cmp #$6e
    beq discard

    ;ESC key
    cmp #$03
    beq abort

    ;Else ignore keypress
    rts

save:
    jsr screen_clear_status
    jmp cmd_file_save

discard:
    jsr screen_clear_status
    stz APP_MOD
    stz mem_modified
    jmp cmd_new_buffer

abort:
    stz APP_MOD
    jsr screen_clear_status
    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_find
;Purpose.............: Handle keypress when entering search param
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_find
    ;Enter key
    cmp #13
    beq search

    ;ESC key
    cmp #$03
    beq abort

    jmp prompt_keypress

search:
    jsr prompt_close
    stz APP_MOD

    jsr cursor_disable

    ldx #<prompt_input
    ldy #>prompt_input
    lda prompt_len
    jsr cmd_do_find
    bcs notfound
    
    jsr screen_refresh
    jsr screen_println
    jmp cursor_activate

notfound:
    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    lda #2
    sta APP_MOD
    jmp cursor_activate

abort:
    jsr prompt_close
    stz APP_MOD
    rts

msg:
    .byt "string not found",0
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_replace_0
;Purpose.............: Handle keypress
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_replace_0
    ;Enter key
    cmp #13
    beq search

    ;ESC key
    cmp #$03
    beq abort

    jmp prompt_keypress

search:
    jsr prompt_close
    stz APP_MOD

    lda #1
    jmp cmd_replace

abort:
    jsr prompt_close
    stz APP_MOD
    rts

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_replace_1
;Purpose.............: Handle keypress
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_replace_1
    ;Enter key
    cmp #13
    beq search

    ;ESC key
    cmp #$03
    beq abort

    jmp prompt_keypress

search:
    jsr prompt_close
    stz APP_MOD

    lda #2
    jmp cmd_replace

abort:
    jsr prompt_close
    stz APP_MOD
    rts

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_replace_2
;Purpose.............: Handle keypress
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_replace_2
    ;Y
    cmp #121
    beq yes

    ;A
    cmp #97
    beq all

    ;ESC
    cmp #$03
    beq abort

    ;Ignore all other input
    rts

yes:
    ldx #0
    lda #3
    jmp cmd_replace

all:
    ldx #1
    lda #3
    jmp cmd_replace

abort:
    stz APP_MOD
    jmp screen_clear_status
.endproc
    
;******************************************************************************
;Function name.......: keyboard_esc_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_esc_key
    lda #3
    sta APP_MOD         ;mode_command

    jsr cursor_disable

    ldx #<msg
    ldy #>msg
    jmp screen_print_status

msg:
    .byt "enter command or press esc to abort",0
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
    jsr cmd_insert

    jmp screen_refresh
.endproc

;******************************************************************************
;Function name.......: keyboard_delete_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_delete_key
    jsr cmd_delete

    bcs exit            ;C=1, at start of file
    
    cpx #0
    beq update_line     ;X=0, we didn't delete a LF, no need to refresh all of screen

    jsr screen_refresh

update_line:
    jsr screen_println

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_tab_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_tab_key
    stz counter
    stz counter+1
    stz counter+2

    clc
    lda keyboard_tabwidth
    adc counter
    sta counter
    lda counter+1
    adc #0
    sta counter+1
    lda counter+2
    adc #0
    sta counter+2

:   lda #32
    jsr cmd_insert
    dec counter
    bne :-

    rts

counter:
    .byt 0,0,0
.endproc

;******************************************************************************
;Function name.......: keyboard_rightarrow_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_rightarrow_key
    jsr cmd_go_right
    bcs :+

    jsr screen_println
    rts

:   jsr screen_refresh
    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_leftarrow_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_leftarrow_key
    jsr cmd_go_left
    bcs :+
    
    jsr screen_println
    rts

:   jsr screen_refresh
    rts
.endproc

;******************************************************************************
;Function name.......: keyboard_uparrow_key
;Purpose.............: Handles keypress
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_uparrow_key
    jsr cmd_go_up
    jsr screen_refresh
    jmp screen_println
.endproc

;******************************************************************************
;Function name.......: keyboard_downarrow_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_downarrow_key
    jsr cmd_go_down
    jsr screen_refresh
    jmp screen_println
.endproc

;******************************************************************************
;Function name.......: keyboard_home_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_home_key
    jsr cmd_go_home
    jmp screen_println
.endproc

;******************************************************************************
;Function name.......: keyboard_end_key
;Purpose.............: Handles keypress
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_end_key
    jsr cmd_go_end
    jmp screen_println
.endproc

;******************************************************************************
;Function name.......: keyboard_cmd_pageup
;Purpose.............: Command: PageUp
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_cmd_pageup
    lda #27
    sta linecount

:   jsr cmd_go_up
    dec linecount
    bne :-

    jmp screen_refresh

linecount:
    .byt 0

.endproc

;******************************************************************************
;Function name.......: keyboard_cmd_pagedown
;Purpose.............: Command: PageDown
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_cmd_pagedown
    lda #27
    sta linecount

:   jsr cmd_go_down
    dec linecount
    bne :-

    jmp screen_refresh

linecount:
    .byt 0
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_goto_line
;Purpose.............: Handler for goto line function
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_goto_line
    ;Enter key
    cmp #13
    beq goto_line

    ;ESC key
    cmp #$03
    beq abort

    ;Default action
    jmp prompt_keypress

goto_line:
    jmp cmd_do_goto_line

abort:
    stz APP_MOD
    jmp prompt_close

.endproc


;******************************************************************************
;Function name.......: keyboard_default_keyhandler
;Purpose.............: Default keyhandler for user input
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_default_keyhandler
    jsr cmd_insert
    jmp screen_println
.endproc

;GLOBAL VARIABLES
keyboard_tabwidth:
    .byt 4