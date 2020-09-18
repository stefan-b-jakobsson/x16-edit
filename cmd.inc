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

    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    rts

msg:
    .byt "current position not yet implemented", 0
.endproc

;******************************************************************************
;Function name.......: cmd_find
;Purpose.............: Command: Find text in current document
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_find
    lda #2
    sta APP_MOD         ;mode_statusmessage

    ldx #<msg
    ldy #>msg
    jsr screen_print_status
    rts

msg:
    .byt "find not yet implemented", 0
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
    jsr util_convert_to_decimal

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