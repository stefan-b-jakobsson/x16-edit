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
;Function name.......: cmd_file_save_prompt
;Purpose.............: Prompts user for file name to save buffer as
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_file_save_prompt
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
;Function name.......: cmd_file_save
;Purpose.............: Writes buffer to file. If file exists the operation will 
;                      fail, and the user is prompted to confirm overwriting.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_file_save
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
;Function name.......: cmd_file_save_overwrite
;Purpose.............: Writes buffer to file overwriting the file if it
;                      already exists.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_file_save_overwrite
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
;Function name.......: cmd_file_open_prompt
;Purpose.............: Prompt user for file name to open
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_file_open_prompt
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
;Function name.......: cmd_file_open
;Purpose.............: Reads file into buffer
;Preparatory routines: None
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cmd_file_open
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

.proc cmd_file_set_device
    lda #17
    sta APP_MOD
    
    ldx #<msg
    ldy #>msg
    lda #2
    jsr prompt_init

    ldx file_cur_device
    ldy #0
    lda #0
    jsr util_bin_to_bcd
    jsr util_bcd_to_str

    stx TMP1_ADR
    sty TMP1_ADR+1
    ldy #0
:   lda (TMP1_ADR),y
    beq foundend
    iny
    jmp :-

foundend:
    tya
    ldx TMP1_ADR
    ldy TMP1_ADR+1
    jmp prompt_default_input

msg:
    .byt "set file storage device number (1-15):",0
.endproc

.proc cmd_file_do_set_device
    ;Copy prompt val
    ldx #0
:   cpx prompt_len
    beq copydone
    lda prompt_input,x
    sta promptval,x
    inx
    jmp :-

copydone:
    stz promptval,x

    ;Convert to binary val
    ldx #<promptval
    ldy #>promptval
    jsr util_str_to_bcd
    bcs invalid
    jsr util_bcd_to_bin
    cpx #16
    bcs invalid
    cpx #1
    bcc invalid
    stx file_cur_device

    stz APP_MOD
    jmp prompt_close

    rts

invalid:
    jsr prompt_close
    ldx #<msg
    ldy #>msg
    lda #2
    sta APP_MOD
    jmp screen_print_status

promptval:
    .byt 0,0,0

msg:
    .byt "invalid device number, must be in range 1 to 15",0
.endproc

