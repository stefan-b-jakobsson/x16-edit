;******************************************************************************
;Function name.......: cmd_exit
;Purpose.............: Command: Initiate program exit
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_exit
    lda #1          ;APP_QUIT=1 signals to irq_handler to close down
    sta APP_QUIT
    rts
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
    jmp prompt_init
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
    lda #5
    sta APP_MOD     ;mode_file_open

    ldx #<msg
    ldy #>msg
    lda #250        ;maxlen
    jmp prompt_init
msg:
    .byt "open file name:",0
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