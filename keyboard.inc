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
;Function name.......: keyboard_read_and_dispatch
;Purpose.............: Reads one character from the keyboard buffer and
;                      dispatches that to proper key handler
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_read_and_dispatch
    ;Before reading keyboard, check if mode=16, i.e. charset changed previous interrupt cycle
    lda APP_MOD
    cmp #16
    beq charset_changed

    ;Get char from keyboard queue
    jsr KERNAL_GETIN
    bne gotachar
    
    ;No input, run memory defrag instead during this interrupt cycle
    jmp mem_run_defrag
    rts

charset_changed:
    ;The charset was changed during previous interrupt cycle
    ;The content of the screen may now be updated. 
    ;If done sooner, there will be errors and artifacts on the screen. A small mystery...
    jsr screen_clearall
    jsr screen_print_header
    jsr screen_print_footer
    jsr screen_refresh
    jsr screen_println
    
    lda #2
    sta APP_MOD
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

    cpx #15
    beq mode_word_wrap_prompt

    cpx #17
    beq mode_set_device

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

mode_word_wrap_prompt:
    jmp keyboard_mode_word_wrap_prompt

mode_set_device:
    jmp keyboard_mode_set_device

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_default
;Purpose.............: Key dispatcher for the default mode, i.e. text entry
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_default
    ;ESC key
    cmp #$03
    bne :+
    jmp esc

    ;Enter key
:   cmp #13
    bne :+
    jmp enter
    
    ;Delete key
:   cmp #$14
    bne :+
    jmp delete

    ;Tab key
:   cmp #$09
    bne :+
    jmp tab_key

    ;Left arrow key
:   cmp #$9d
    bne :+
    jmp left_arrow

    ;Right arrow key
:   cmp #$1d
    bne :+
    jmp right_arrow

    ;Up arrow key
:   cmp #$91
    bne :+
    jmp up_arrow

    ;Down arrow key
:   cmp #$11
    bne :+
    jmp down_arrow

    ;Home key
:   cmp #$13
    bne :+
    jmp home

    ;Shift+Home = End key
:   cmp #$93
    bne :+
    jmp end

    ;Ctrl+G
:   cmp #$07
    bne :+
    jmp ctrl_g

    ;Ctrl+x
:   cmp #$18
    bne :+
    jmp ctrl_x

    ;Ctrl+Y
:   cmp #$19
    bne :+
    jmp ctrl_y

    ;Ctrl+V
:   cmp #$16
    bne :+
    jmp ctrl_v

    ;Ctrl+W
:   cmp #$17
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

    ;Ctrl+e
    cmp #$05
    beq ctrl_e

    ;Ctrl+a
    cmp #$01
    beq ctrl_a

    ;Ctrl+z
    cmp #$1a
    beq ctrl_z

    ;Ctrl+d
    cmp #$04
    beq ctrl_d

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
    jmp cmd_insert_tab

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
    jmp cmd_file_save_prompt

ctrl_o:
    jmp cmd_file_open_prompt

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

ctrl_e:
    jmp cmd_change_encoding

ctrl_a:
    jmp cmd_auto_indent

ctrl_z:
    jmp cmd_set_word_wrap

ctrl_d:
    jmp cmd_file_set_device

f1:
    jmp cmd_show_help

f2:
    jmp cmd_exit

f3:
    jmp cmd_file_save_prompt

f4:
    jmp cmd_file_open_prompt

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
;Purpose.............: Key dispatcher: Prompting the user to save
;                      current buffer before program exit
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_exit_save_before
    ;Y key pressed
    cmp #$79
    beq save
    cmp #$59
    beq save

    ;N key pressed
    cmp #$6e
    beq discard
    cmp #$4e
    beq discard

    ;ESC key pressed
    cmp #$03
    beq abort

    ;Else ignore keypress
    rts

save:
    jsr screen_clear_status
    jmp cmd_file_save_prompt

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
;Purpose.............: Key dispatcher: Showing the built-in help screen
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
;Purpose.............: Key dispatcher: Showing a message in the status bar
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
;Purpose.............: Key dispatcher: The user has pressed and released ESC
;                      and the next key is interpreted as a command
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_command
    ;ESC key
    cmp #$03
    bne :+
    jmp abort_command_mode

    ;a
:   cmp #$61
    bne :+
    jmp auto_indent
:   cmp #$41
    bne :+
    jmp auto_indent

    ;g
:   cmp #$67
    bne :+
    jmp show_help
:   cmp #$47
    bne :+
    jmp show_help

    ;x
:   cmp #$78
    bne :+
    jmp exit
:   cmp #$58
    bne :+
    jmp exit

    ;w
:   cmp #$77
    bne :+
    jmp save
:   cmp #$57
    bne :+
    jmp save

    ;o
:   cmp #$6f
    bne :+
    jmp open
:   cmp #$4f
    bne :+
    jmp open

    ;n
:   cmp #$6e
    bne :+
    jmp newbuffer
:   cmp #$4e
    bne :+
    jmp newbuffer

    ;y
:   cmp #$79
    bne :+
    jmp pgup
:   cmp #$59
    bne :+
    jmp pgup

    ;v
:   cmp #$76
    bne :+
    jmp pgdown
:   cmp #$56
    bne :+
    jmp pgdown

    ;k
:   cmp #$6b
    bne :+
    jmp cut
:   cmp #$4b
    bne :+
    jmp cut

    ;b
:   cmp #$86
    bne :+
    jmp copy
:   cmp #$66
    bne :+
    jmp copy

    ;m
:   cmp #$6d
    bne :+
    jmp mem_usage
:   cmp #$4d
    bne :+
    jmp mem_usage

    ;u
:   cmp #$75
    bne :+
    jmp paste
:   cmp #$55
    bne :+
    jmp paste

    ;p
:   cmp #$70
    bne :+
    jmp curpos
:   cmp #$50
    bne :+
    jmp curpos

    ;f
:   cmp #$66
    bne :+
    jmp find
:   cmp #$46
    bne :+
    jmp find

    ;r
:   cmp #$72
    bne :+
    jmp replace
:   cmp #$52
    bne :+
    jmp replace

    ;e
:   cmp #$65
    bne :+
    jmp change_encoding
:   cmp #$45
    bne :+
    jmp change_encoding
    
    ;z
:   cmp #$5a
    bne :+
    jmp set_word_wrap
:   cmp #$7a
    bne :+
    jmp set_word_wrap

    ;l
:   cmp #$6c
    bne :+
    jmp gotoline
:   cmp #$4c
    bne :+
    jmp gotoline

    ;d
:   cmp #$64
    bne :+
    jmp setdevice
:   cmp #$44
    bne :+
    jmp setdevice

    ;1..9
:   cmp #$31
    bcs :+
    jmp unknown
:   cmp #$3a
    bcs unknown
    jmp tab_width

    ;Uknown command, ignore
unknown:
    rts

auto_indent:
    stz APP_MOD
    jsr cursor_activate
    jmp cmd_auto_indent

show_help:
    stz APP_MOD
    jmp cmd_show_help

exit:
    jsr cursor_activate
    jmp cmd_exit

save:
    jsr cursor_activate
    jmp cmd_file_save_prompt

open:
    jsr cursor_activate
    jmp cmd_file_open_prompt

newbuffer:
    jsr cursor_activate
    jsr screen_clear_status
    jmp cmd_new_buffer

pgup:
    jsr cursor_activate
    stz APP_MOD
    jmp keyboard_cmd_pageup

pgdown:
    jsr cursor_activate
    stz APP_MOD
    jmp keyboard_cmd_pagedown

cut:
    jsr cmd_cut
    jsr cursor_activate
    stz APP_MOD
    jmp screen_clear_status

copy:
    jsr cmd_copy
    jsr cursor_activate
    stz APP_MOD
    jmp screen_clear_status

mem_usage:
    jsr cursor_activate
    jmp cmd_mem_usage

paste:
    jsr cmd_paste
    jsr cursor_activate
    stz APP_MOD
    jmp screen_clear_status

curpos:
    jsr cursor_activate
    jmp cmd_show_curpos

find:
    jsr cursor_activate
    jmp cmd_find

replace:
    jsr cursor_activate
    lda #0
    jmp cmd_replace

change_encoding:
    jsr cursor_activate
    jmp cmd_change_encoding

set_word_wrap:
    stz APP_MOD
    jsr cursor_activate
    jmp cmd_set_word_wrap

setdevice:
    jsr cursor_activate
    jmp cmd_file_set_device

tab_width:
    pha
    stz APP_MOD
    jsr screen_clear_status
    jsr cursor_activate
    pla
    sec
    sbc #48
    jmp cmd_set_tab_width

gotoline:
    jsr cursor_activate
    jmp cmd_goto_line

abort_command_mode:
    stz APP_MOD
    jsr cursor_activate
    jmp screen_clear_status

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_file_save
;Purpose.............: Key dispatcher: Prompting the user to enter a file
;                      name, whereto the buffer is to be saved
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
    jmp cmd_file_save
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_file_save_overwrite
;Purpose.............: Key dispatcher: Prompting the user to confirm overwriting
;                      an existing file
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_file_save_overwrite
    ;Y key pressed
    cmp #$79
    beq write_file
    cmp #$59
    beq write_file

    ;N key
    cmp #$6e
    beq abort
    cmp #$4e
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
    jmp cmd_file_save_overwrite

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_file_open
;Purpose.............: Key dispatcher: Prompting the user to enter a file name
;                      to be opened and read into the text buffer
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

    ;Default, send key to prompt
    jmp prompt_keypress

abort:
    jsr prompt_close
    lda #0              ;mode_default
    sta APP_MOD
    rts

read_file:
    jmp cmd_file_open

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_file_open_save_before
;Purpose.............: Key dispatcher: Prompting the user to save the current
;                      text buffer before opening and reading a file
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_file_open_save_before
    ;Y key pressed
    cmp #$79
    beq save
    cmp #$59
    beq save

    ;N key pressed
    cmp #$6e
    beq discard
    cmp #$4e
    beq discard

    ;ESC key
    cmp #$03
    beq abort

    ;Else ignore keypress
    rts

save:
    jmp cmd_file_save_prompt

discard:
    stz mem_modified
    jsr cmd_file_open_prompt
    inc mem_modified
    rts

abort:
    stz APP_MOD
    jsr screen_clear_status
    rts

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_new_buffer_save_before
;Purpose.............: Key dispatcher: Prompting the user to save the current
;                      text buffer before creating a new buffer
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_new_buffer_save_before
    ;Y key pressed
    cmp #$79
    beq save
    cmp #$59
    beq save

    ;N key pressed
    cmp #$6e
    beq discard
    cmp #$4e
    beq discard

    ;ESC key
    cmp #$03
    beq abort

    ;Else ignore keypress
    rts

save:
    jmp cmd_file_save_prompt

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
;Function name.......: keyboard_mode_set_device
;Purpose.............: Key dispatcher: Prompting the user to enter device
;                      number
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_set_device
    ;ESC key
    cmp #$03
    beq abort

    ;Enter
    cmp #13
    beq enter

    ;Default
    jmp prompt_keypress

abort:
    stz APP_MOD
    jmp prompt_close

enter:
    jmp cmd_file_do_set_device
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_find
;Purpose.............: Key dispatcher: Prompting the user to enter a search
;                      param
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
;Purpose.............: Key dispatcher: Prompting the user to enter a search
;                      param
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
;Purpose.............: Key dispatcher: Prompting the user to enter a string
;                      that will replace the search param
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
;Purpose.............: Key dispatcher: Prompting the user to confirm replacing
;                      one or all occurences
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_replace_2
    ;Y
    cmp #121
    beq yes
    cmp #121-32
    beq yes

    ;A
    cmp #97
    beq all
    cmp #97-32
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
;Function name.......: keyboard_mode_word_wrap_prompt
;Purpose.............: Key dispatcher: Prompting the user to enter at what
;                      column to wrap lines
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_word_wrap_prompt
    ;ESC
    cmp #$03
    beq abort

    ;ENTER
    cmp #13
    beq enter

    ;Default
    jmp prompt_keypress

enter:
    jmp cmd_set_word_wrap

abort:
    stz APP_MOD
    jmp prompt_close

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_goto_line
;Purpose.............: Key dispatcher: Prompting the user to enter a line
;                      number
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
;Function name.......: keyboard_esc_key
;Purpose.............: Key handler: Enter command select mode
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
;Purpose.............: Key handler: Insert line break
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_enter_key
    ;Insert linefeed char
    lda cmd_auto_indent_status
    beq :+
    jsr cursor_disable

:   lda #LF
    jsr cmd_insert

    lda cmd_auto_indent_status
    beq :+
    jsr cursor_activate

:   jmp screen_refresh
.endproc

;******************************************************************************
;Function name.......: keyboard_delete_key
;Purpose.............: Key handler: Delete
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
;Function name.......: keyboard_rightarrow_key
;Purpose.............: Key handler: Move right
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
;Purpose.............: Key handler: Move left
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
;Purpose.............: Key handler: Move up
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
;Purpose.............: Key handler: Move down
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
;Purpose.............: Key handler: Move home
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_home_key
    jsr cmd_go_home
    jmp screen_println
.endproc

;******************************************************************************
;Function name.......: keyboard_end_key
;Purpose.............: Key handler: Move end
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_end_key
    jsr cmd_go_end
    jmp screen_println
.endproc

;******************************************************************************
;Function name.......: keyboard_cmd_pageup
;Purpose.............: Key handler: Move PageUp
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
;Purpose.............: Key handler: Move PageDown
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
;Function name.......: keyboard_default_keyhandler
;Purpose.............: Key handler: Default operation, inserts character into
;                      text buffer
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_default_keyhandler
    jsr cmd_insert
    bcs refresh_screen

    jmp screen_println

refresh_screen:
    jsr screen_refresh
    jmp screen_println

.endproc

;GLOBAL VARIABLES
keyboard_tabwidth:
    .byt 4