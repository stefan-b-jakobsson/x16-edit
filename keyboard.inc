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
;Function name.......: keyboard_read_and_dispatch
;Purpose.............: Reads one character from the keyboard buffer and
;                      dispatches that to proper key handler
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_read_and_dispatch
    ;Get char from keyboard queue. Don't read if APP_MOD=16
    lda APP_MOD
    cmp #16
    beq gotachar
    
    bridge_setaddr KERNAL_GETIN
    bridge_call KERNAL_GETIN
    bne gotachar

    ;No input, run memory defrag instead during this interrupt cycle
    jmp mem_run_defrag
    
gotachar:
    ;Store char in Y temporarily
    tay

    ;Select key handler according to program mode
    ldx APP_MOD
    lda jmptbl_L,x
    sta eventhandler
    lda jmptbl_H,x
    sta eventhandler+1
    
    ;Restore char in A
    tya

    ;Jump to event handler
    jmp (eventhandler)

.segment "VARS"
    eventhandler: .res 2
.CODE

jmptbl_L:
    .byt <keyboard_mode_default
    .byt <keyboard_mode_helpscreen
    .byt <keyboard_mode_statusmessage
    .byt <keyboard_mode_command
    .byt <keyboard_mode_file_save
    .byt <keyboard_mode_file_open
    .byt <keyboard_mode_file_save_overwrite
    .byt <keyboard_mode_file_open_save_before
    .byt <keyboard_mode_exit_save_before
    .byt <keyboard_mode_new_buffer_save_before
    .byt <keyboard_mode_find
    .byt <keyboard_mode_goto_line
    .byt <keyboard_mode_replace_0
    .byt <keyboard_mode_replace_1
    .byt <keyboard_mode_replace_2
    .byt <keyboard_mode_word_wrap_prompt
    .byt <cmd_refresh_after_encoding_changed
    .byt <keyboard_mode_set_device
    .byt <keyboard_mode_file_dir
    .byt <keyboard_mode_file_dir

jmptbl_H:
    .byt >keyboard_mode_default
    .byt >keyboard_mode_helpscreen
    .byt >keyboard_mode_statusmessage
    .byt >keyboard_mode_command
    .byt >keyboard_mode_file_save
    .byt >keyboard_mode_file_open
    .byt >keyboard_mode_file_save_overwrite
    .byt >keyboard_mode_file_open_save_before
    .byt >keyboard_mode_exit_save_before
    .byt >keyboard_mode_new_buffer_save_before
    .byt >keyboard_mode_find
    .byt >keyboard_mode_goto_line
    .byt >keyboard_mode_replace_0
    .byt >keyboard_mode_replace_1
    .byt >keyboard_mode_replace_2
    .byt >keyboard_mode_word_wrap_prompt
    .byt >cmd_refresh_after_encoding_changed
    .byt >keyboard_mode_set_device
    .byt >keyboard_mode_file_dir
    .byt >keyboard_mode_file_dir
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_default
;Purpose.............: Key dispatcher for the default mode, i.e. text entry
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
;Keyboard shordcuts..: ^G=$07 Get Help
;                      ^X=$18 Exit Program
;                      ^O=$0f Write Out
;                      ^R=$12 Read file
;                      ^N=$0e New buffer
;                      ^Y=$19 PageUp
;                      ^V=$16 PageDown
;                      ^K=$0b Cut
;                      ^P=$10 Copy
;                      ^U=$15 Uncut (paste)
;                      ^C=$03 Show cursor position
;                      ^W=$17 WhereIs
;                      ^S=$13 Replace
;                      ^L=$0c Go to line
;                      ^A=$01 Auto indent on/off
;                      ^Z=$1a Word wrap on/off
;                      ^E=$05 Change character encoding
;                      ^D=$04 Set device number
;                      ^T=$14 Change text color
;                      ^B=$02 Change background color
;                      ^M=$0d Show memory usage
;                      ^SPACE=$a0 Insert non-breaking space
.proc keyboard_mode_default
    ;Store key in Y
    tay

    ;Get key modifiers
    bridge_setaddr KERNAL_KBDBUF_GET_MODIFIERS
    bridge_call KERNAL_KBDBUF_GET_MODIFIERS
    and #255-KBD_MODIFIER_SHIFT         ;Ignore shift key
    tax
    tya

    cpx #KBD_MODIFIER_CTRL
    bne modifier_none
    jmp modifier_ctrl

modifier_none:
    ;ESC key
    cmp #$03
    bne :+
    jmp keyboard_esc_key

    ;Enter key
:   cmp #13
    bne :+
    jmp keyboard_enter_key
    
    ;Delete key
:   cmp #$14
    bne :+
    jmp keyboard_delete_key

    ;Tab key
:   cmp #$09
    bne :+
    jmp cmd_insert_tab

    ;Left arrow key
:   cmp #$9d
    bne :+
    jmp keyboard_leftarrow_key

    ;Right arrow key
:   cmp #$1d
    bne :+
    jmp keyboard_rightarrow_key

    ;Up arrow key
:   cmp #$91
    bne :+
    jmp keyboard_uparrow_key

    ;Down arrow key
:   cmp #$11
    bne :+
    jmp keyboard_downarrow_key

    ;Home key
:   cmp #$13
    bne :+
    jmp keyboard_home_key

    ;Shift+Home = End key
:   cmp #$93
    bne :+
    jmp keyboard_end_key

    ;F1
:   cmp #$85
    bne :+
    jmp cmd_show_help

    ;F2
:   cmp #$89
    bne :+
    jmp cmd_exit

    ;F3
:   cmp #$86
    bne :+
    jmp cmd_file_save_prompt

    ;F5
:   cmp #$8a
    bne :+
    jmp cmd_file_open_prompt

    ;F6
:   cmp #$8b
    bne :+
    jmp cmd_find

    ;F7
:   cmp #$88
    bne :+
    jmp keyboard_cmd_pageup

    ;F8
:   cmp #$8c
    bne :+
    jmp keyboard_cmd_pagedown

:   ;Convert non-breaking space (Shift+space, ASCII 160) to normal space char
    cmp #$a0
    bne default_handler
    lda #$20

    ;Default key handler
default_handler:
    jsr cmd_insert
    bcs :+
    jmp screen_println
:   jsr screen_refresh
    jmp screen_println

modifier_ctrl:
    ;Ctrl+G
    cmp #$07
    bne :+
    jmp cmd_show_help

    ;Ctrl+X
:   cmp #$18
    bne :+
    jmp cmd_exit

    ;Ctrl+O
:   cmp #$0f
    bne :+
    jmp cmd_file_save_prompt

    ;Ctrl+R
:   cmp #$12
    bne :+
    lda #0
    jmp cmd_file_open_prompt

    ;Ctrl+N
:   cmp #$0e
    bne :+
    jmp cmd_new_buffer

    ;Ctrl+V
:   cmp #$16
    bne :+
    jmp keyboard_cmd_pagedown

    ;Ctrl+Y
:   cmp #$19
    bne :+
    jmp keyboard_cmd_pageup

    ;Ctrl+K
:   cmp #$0b
    bne :+
    jmp cmd_cut

    ;Ctrl+P
:   cmp #$10
    bne :+
    jmp cmd_copy

    ;Ctrl+U
:   cmp #$15
    bne :+
    jmp cmd_paste

    ;Ctrl+C
:   cmp #3
    bne :+
    jmp cmd_show_curpos

    ;Ctrl+W
:   cmp #$17
    bne :+
    jmp cmd_find

    ;Ctrl+S
:   cmp #$13
    bne :+
    lda #0
    jmp cmd_replace

    ;Ctrl+L
:   cmp #$0c
    bne :+
    jmp cmd_goto_line

    ;Ctrl+A
:   cmp #$01
    bne :+
    jmp cmd_auto_indent

    ;Ctrl+Z
:   cmp #$1a
    bne :+
    jmp cmd_set_word_wrap

    ;Ctrl+E
:   cmp #$05
    bne :+
    jmp cmd_change_encoding

    ;Ctrl+D
:   cmp #$04
    bne :+
    jmp cmd_file_set_device

    ;Ctrl+T
:   cmp #$14
    bne :+
    jmp cmd_rotate_text_color

    ;Ctrl+B
:   cmp #$02
    bne :+
    jmp cmd_rotate_background_color

    ;Ctrl+M
:   cmp #$0d
    bne :+
    jmp cmd_mem_usage

    ;Ctrl+space, insert non-breaking space
:   cmp #$a0
    bne :+
    jmp default_handler

:   rts

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_command
;Purpose.............: Key dispatcher: The user has pressed and released ESC
;                      and the next key is interpreted as a command
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_command
    ;1..9
    cmp #$31
    bcc :+
    cmp #$3b
    bcs :+
    
    sec
    sbc #48
    jsr cmd_set_tab_width

    stz APP_MOD
    jsr screen_clear_status
    jmp cursor_activate

    ;Space, convert to non-breaking space
:   cmp #$20
    bne :++
    stz APP_MOD
    jsr cursor_activate
    jsr screen_clear_status
    lda #160
    jsr cmd_insert
    bcs :+
    jmp screen_println
:   jsr screen_refresh
    jmp screen_println
    
    ;Clear bit 5 to make upper and lower case the same value
:   and #%11011111

    ;ESC key
    cmp #$03
    bne :+
    stz APP_MOD
    jsr cursor_activate
    jmp screen_clear_status

    ;a
:   cmp #$41
    bne :+
    stz APP_MOD
    jsr cursor_activate
    jmp cmd_auto_indent
    
    ;g
:   cmp #$47
    bne :+
    stz APP_MOD
    jmp cmd_show_help

    ;x
:   cmp #$58
    bne :+
    jsr cursor_activate
    jmp cmd_exit

    ;w
:   cmp #$57
    bne :+
    jsr cursor_activate
    jmp cmd_find

    ;o
:   cmp #$4f
    bne :+
    jsr cursor_activate
    jmp cmd_file_save_prompt

    ;n
:   cmp #$4e
    bne :+
    jsr cursor_activate
    jsr screen_clear_status
    jmp cmd_new_buffer

    ;y
:   cmp #$59
    bne :+
    jsr cursor_activate
    stz APP_MOD
    jmp keyboard_cmd_pageup

    ;v
:   cmp #$56
    bne :+
    jsr cursor_activate
    stz APP_MOD
    jmp keyboard_cmd_pagedown

    ;k
:   cmp #$4b
    bne :+
    jsr cursor_activate
    stz APP_MOD
    jsr cmd_cut
    jmp screen_clear_status

    ;b
:   cmp #$42
    bne :+
    stz APP_MOD
    jsr screen_clear_status
    jsr cursor_activate
    jmp cmd_rotate_background_color

    ;m
:   cmp #$4d
    bne :+
    jsr cursor_activate
    jmp cmd_mem_usage

    ;u
:   cmp #$55
    bne :+
    stz APP_MOD
    jsr screen_clear_status
    jsr cursor_activate
    jmp cmd_paste

    ;p
:   cmp #$50
    bne :+
    stz APP_MOD
    jsr cursor_activate
    jsr screen_clear_status
    jmp cmd_copy

    ;r
:   cmp #$52
    bne :+
    jsr cursor_activate
    lda #0
    jmp cmd_file_open_prompt

    ;e
:   cmp #$45
    bne :+
    jsr cursor_activate
    jmp cmd_change_encoding
    
    ;z
:   cmp #$5a
    bne :+
    stz APP_MOD
    jsr cursor_activate
    jmp cmd_set_word_wrap

    ;l
:   cmp #$4c
    bne :+
    jsr cursor_activate
    jmp cmd_goto_line

    ;d
:   cmp #$44
    bne :+
    jsr cursor_activate
    jmp cmd_file_set_device

    ;c
:   cmp #$43
    bne :+
    jsr cursor_activate
    jmp cmd_show_curpos

    ;s
:   cmp #$53
    bne :+
    jsr cursor_activate
    lda #0
    jmp cmd_replace

    ;t
:   cmp #$54
    bne unknown
    jmp cmd_rotate_text_color

    ;Uknown command, ignore
unknown:
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
    jmp cursor_activate
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
    jmp cursor_activate
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_exit_save_before
;Purpose.............: Key dispatcher: Prompting the user to save
;                      current buffer before program exit
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_exit_save_before
    ;Clear bit 5 to make upper and lower case the same value
    and #%11011111
    
    ;Y key pressed
    cmp #$59
    beq save

    ;N key pressed
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

    ;Ctrl+T
    cmp #$14
    bne :+

    bridge_setaddr KERNAL_KBDBUF_GET_MODIFIERS
    bridge_call KERNAL_KBDBUF_GET_MODIFIERS
    and #255-KBD_MODIFIER_SHIFT         ;Ignore shift key
    tax
    lda #$14
    cpx #KBD_MODIFIER_CTRL
    beq tofiles
    
    ;Default, send key to prompt
:   jmp prompt_keypress

abort:
    jsr prompt_close
    stz APP_MOD
    jsr screen_clear_footer
    jmp screen_print_default_footer

write_file:
    jsr screen_clear_footer
    jsr screen_print_default_footer
    ldx #<prompt_input
    ldy #>prompt_input
    lda prompt_len
    jmp cmd_file_save

tofiles:
    lda #19
    sta APP_MOD
    jsr cursor_disable
    jsr screen_print_dir_ctx_footer
    ldx #1
    ldy #0
    jmp dir_show
.endproc

;******************************************************************************
;Function name.......: keyboard_mode_file_save_overwrite
;Purpose.............: Key dispatcher: Prompting the user to confirm overwriting
;                      an existing file
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_file_save_overwrite
    ;Clear bit 5 to make upper and lower case the same value
    and #%11011111

    ;Y key pressed
    cmp #$59
    beq write_file

    ;N key
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

    ;Ctrl+T
    cmp #$14
    bne :+

    bridge_setaddr KERNAL_KBDBUF_GET_MODIFIERS
    bridge_call KERNAL_KBDBUF_GET_MODIFIERS
    and #255-KBD_MODIFIER_SHIFT         ;Ignore shift key
    tax
    lda #$14
    cpx #KBD_MODIFIER_CTRL
    beq tofiles

    ;Default, send key to prompt
:   jmp prompt_keypress

abort:
    jsr prompt_close
    stz APP_MOD
    jsr screen_clear_footer
    jmp screen_print_default_footer

read_file:
    jsr screen_clear_footer
    jsr screen_print_default_footer
    ldx #<prompt_input
    ldy #>prompt_input
    lda prompt_len
    jmp cmd_file_open

tofiles:
    lda #18
    sta APP_MOD
    jsr cursor_disable
    jsr screen_print_dir_ctx_footer
    ldx #1
    ldy #0
    jmp dir_show

.endproc

;******************************************************************************
;Function name.......: keyboard_mode_file_dir
;Purpose.............: Key dispatcher: Showing directory listing
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_file_dir
    ;Enter key
    cmp #13
    bne :+
    jmp enterkey

    ;ESC key
:   cmp #$03
    bne :+
    jmp abort

    ;Up arrow key
:   cmp #$91
    bne :+
    jmp uparrow

    ;Down arrow key
:   cmp #$11
    bne :+
    jmp downarrow

    ;Ctrl+Y
:   cmp #$19
    bne :+
    jmp prevpage
    
    ;Ctrl+V
:   cmp #$16
    bne :+
    jmp nextpage

    ;Default, no action
:   rts

enterkey:
    clc
    lda dir_first
    adc dir_selected
    tax
    lda dir_first+1
    adc #0
    tay
    jsr dir_getitem

    lda dir_type
    cmp #'d'
    bne :+
    ldx #<dir_entry
    ldy #>dir_entry
    lda dir_filename_len
    jmp dir_change

:   jsr screen_clear
    jsr prompt_close

    ldx #<dir_entry
    ldy #>dir_entry
    lda dir_filename_len
    jsr prompt_default_input
    jsr prompt_refresh
    
    lda APP_MOD
    cmp #18
    beq open
save:
    jsr screen_clear_footer
    jsr screen_print_default_footer
    jsr cursor_activate
    ldx #<prompt_input
    ldy #>prompt_input
    lda prompt_len
    jmp cmd_file_save

open:
    stz APP_MOD
    jsr screen_clear_footer
    jsr screen_print_default_footer
    jsr cursor_activate
    ldx #<prompt_input
    ldy #>prompt_input
    lda prompt_len
    jmp cmd_file_open

abort:
    lda APP_MOD
    cmp #18
    beq abortopen
abortsave:
    jsr screen_clear_footer
    jsr screen_print_file_ctx_footer
    jsr cursor_activate
    lda #4
    sta APP_MOD
    jmp :+
abortopen:
    jsr screen_clear_footer
    jsr screen_print_file_ctx_footer
    jsr cursor_activate
    lda #5
    sta APP_MOD
:   jsr screen_clear
    jmp screen_refresh

uparrow:
    ldx dir_selected
    dex
    jmp dir_select

downarrow:
    ldx dir_selected
    inx
    jmp dir_select

nextpage:
    jmp dir_nextpage

prevpage:
    jmp dir_prevpage

.endproc


;******************************************************************************
;Function name.......: keyboard_mode_file_open_save_before
;Purpose.............: Key dispatcher: Prompting the user to save the current
;                      text buffer before opening and reading a file
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: None
.proc keyboard_mode_file_open_save_before
    ;Clear bit 5 to make upper and lower case the same value
    and #%11011111
    
    ;Y key pressed
    cmp #$59
    beq save

    ;N key pressed
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
    ;Clear bit 5 to make upper and lower case the same value
    and #%11011111
    
    ;Y key pressed
    cmp #$59
    beq save

    ;N key pressed
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
    jmp screen_clear_status
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
    ;Clear bit 5 to make upper and lower case the same value
    and #%11011111
    
    ;Y
    cmp #121-32
    beq yes

    ;A
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

    ;Enter
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
    sta APP_MOD

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
    jmp screen_println
:   jmp screen_refresh
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
    
    jmp screen_println
:   jmp screen_refresh
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

.segment "VARS"
    linecount: .res 1
.CODE

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

.segment "VARS"
    linecount: .res 1
.CODE

.endproc

.segment "VARS"
    keyboard_tabwidth:      .res 1              ;1 byte
.CODE