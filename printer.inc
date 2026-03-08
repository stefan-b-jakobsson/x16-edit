;*******************************************************************************
;Copyright 2022-2024, Stefan Jakobsson
;
;Redistribution and use in source and binary forms, with or without modification, 
;are permitted provided that the following conditions are met:
;
;1. Redistributions of source code must retain the above copyright notice, this 
;   list of conditions and the following disclaimer.
;
;2. Redistributions in binary form must reproduce the above copyright notice, 
;   this list of conditions and the following disclaimer in the documentation 
;   and/or other materials provided with the distribution.
;
;THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” 
;AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
;IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
;DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
;FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
;DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
;SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
;CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
;OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
;OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;*******************************************************************************

;Printer driver jump table
PRNDRV_INIT                    = $9efd
PRNDRV_API_VER                 = $9efa
PRNDRV_GET_DRIVER_NAME         = $9ef7
PRNDRV_SET_CHARSET             = $9ef4
PRNDRV_GET_OPTION_COUNT        = $9ef1
PRNDRV_GET_OPTION_LABELS       = $9eee
PRNDRV_GET_OPTION_VALUE        = $9eeb
PRNDRV_SET_OPTION_VALUE        = $9ee8
PRNDRV_ADD_OFFSET              = $9ee5
PRNDRV_CHANNEL_OPEN            = $9ee2
PRNDRV_PRINT_CHAR              = $9edf
PRNDRV_CHANNEL_CLOSE           = $9edc
PRNDRV_GET_MESSAGE             = $9ed9
PRNDRV_SAVE_DEFAULTS           = $9ed6

PRNDRV_API_MIN                 = 1
PRNDRV_API_MAX                 = 1

; Option types
PRINTER_TYPE_INT8 = 0
PRINTER_TYPE_INT16 = 1
PRINTER_TYPE_LIST = 2
PRINTER_TYPE_STRING = 3

.segment "CODE2"

;**********************************************************
;Function name.......: printer_load_driver
;Description.........: Loads driver from SD card into RAM
;Input...............: Nothing
;Returns.............: A = 0 if successfully loaded
;                      A = 1 if loading failed
;                      A = 2 if unsupported driver
.proc printer_load_driver
    lastbyte = tempvars
    bytecount = tempvars+1
    status = tempvars+2

    ; Close file 1
    bridge_setaddr KERNAL_CLOSE
    lda #1
    bridge_call KERNAL_CLOSE

    ; Copy file name to temp vars
    ldx #0
:   lda fn,x
    sta tempvars,x
    inx
    cpx #fn_end-fn
    bne :-

    ; Set file name
    bridge_setaddr KERNAL_SETNAM
    ldx #<tempvars
    ldy #>tempvars
    lda #fn_end-fn
    bridge_call KERNAL_SETNAM

    ; Set file params
    bridge_setaddr KERNAL_SETLFS
    lda #1
    ldx #8
    ldy #1
    bridge_call KERNAL_SETLFS

    ; Load driver
    sec
    jsr printer_suppress_messages ; Redirect/suppress Kernal messages
    bridge_setaddr KERNAL_LOAD
    lda #0 ; Load flag
    bridge_call KERNAL_LOAD
    php ; Save status on stack (C is error indicator)
    clc
    jsr printer_suppress_messages ; Restore Kernal messages

    ; Close file
    bridge_setaddr KERNAL_CLOSE
    lda #1
    bridge_call KERNAL_CLOSE

    ; Check disk status
    call_firstbank file_read_disk_status
    cmp #0
    beq :+
    plp
    bra loaderr

    ; Check Kernal error
:   plp
    bcs loaderr

    ; Check diver compatibilty
    jsr PRNDRV_API_VER
    cmp #PRNDRV_API_MIN
    bcc apierr
    cmp #PRNDRV_API_MAX+1
    bcs apierr

    ; Init driver
    jsr PRNDRV_INIT

    ; Return OK
    lda #0
    rts

loaderr:
    lda #1
    rts

apierr:
    lda #2
    rts

fn:
    .byt "//:x16editpd-*"
fn_end:
.endproc

;**********************************************************
;Function name.......: printer_dialog
;Description.........: Shows the print dialog
;Input...............: Nothing
;Prerequisites.......: printer_load_driver
;Error returns.......: Nothing
.proc printer_dialog
    ; Clear screen
    call_firstbank cursor_disable
    call_firstbank screen_clear_editor
    call_firstbank screen_clear_status

    ; Setup VERA to print from start of line 2
    stz VERA_L
    lda #VERA_BUFADR_M + 2
    sta VERA_M
    lda #VERA_BUFADR_H | %00100000
    sta VERA_H

    ; Output "PRINT TO " header
    ldy #0
:   lda header,y
    beq :+
    call_firstbank screen_put_uc_char
    iny
    bra :-

    ; Output name of driver
:   jsr PRNDRV_GET_DRIVER_NAME
    stx TMP1_ADR
    sty TMP1_ADR+1
    ldy #0
:   lda (TMP1_ADR),y
    beq :+
    call_firstbank screen_put_uc_char
    iny
    bra :-

    ; Setup VERA to print from start of line 4
:   stz VERA_L
    lda #VERA_BUFADR_M + 4
    sta VERA_M

    ; Clear column where values are displayed
    stz printer_value_start_column

    ; Get option count
    jsr PRNDRV_GET_OPTION_COUNT
    sta printer_option_count
    bne :+

    ; No options
    rts

    ; Print option labels
:   jsr PRNDRV_GET_OPTION_LABELS
    stx TMP1_ADR
    sty TMP1_ADR+1
    ldy #0
:   lda (TMP1_ADR),y
    beq exit
    cmp #13
    beq lf
    call_firstbank screen_put_uc_char
    iny
    bra :-

lf:
    ; Update max label max width
    jsr update_label_width
    
    ; Move pointer to start of next item
    iny
    clc
    tya
    adc TMP1_ADR
    sta TMP1_ADR
    lda TMP1_ADR+1
    adc #0
    sta TMP1_ADR+1

    ; Setup VERA to print from start of next line
    stz VERA_L
    inc VERA_M

    ; Print next line
    ldy #0
    bra :-

exit:
    ; Update max label max width
    jsr update_label_width

    ; Highlight first option
    stz printer_active_option
    sec
    ldx #0
    jsr printer_select

    ; Get current option values, and display them
    ldx #0
:   phx
    jsr printer_refresh_line
    plx
    inx
    cpx printer_option_count
    bne :-

    rts

update_label_width:
    cpy printer_value_start_column
    bcc :+
    sty printer_value_start_column
:   rts

header:
    .byt "print to ", 0
.endproc

;**********************************************************
;Function name.......: printer_key_handler
;Description.........: Key handler for the print dialog
;Input...............: A = char
;Error returns.......: Nothing
.proc printer_key_handler
    pha
    call_firstbank screen_clear_status
    pla

    cmp #KEYVAL_ESC
    beq esc

    ldx printer_option_count
    beq p

    cmp #KEYVAL_ENTER
    bne :+
    jmp enter

:   cmp #KEYVAL_DOWN
    bne :+
    jmp down

:   cmp #KEYVAL_UP
    bne :+
    jmp up

:   cmp #KEYVAL_LEFT
    bne :+
    jmp left

:   cmp #KEYVAL_RIGHT
    bne p
    jmp right

p:  ; Clear bit 5 to convert shifted char to unshifted char
    and #%11011111

    cmp #KEYVAL_P
    bne :+
    jmp printer_print

:   cmp #KEYVAL_S
    bne :+
    jmp printer_save_defaults

:   rts

esc:
    ; Exit printing dialog
    call_firstbank screen_clear_status
    call_firstbank screen_refresh
    call_firstbank screen_println
    call_firstbank screen_print_default_footer
    call_firstbank cursor_activate
    stz APP_MOD
    rts

enter:
    ldx printer_active_option
    jsr PRNDRV_GET_OPTION_VALUE
    cmp #PRINTER_TYPE_LIST
    bne :+
    jmp right

:   lda #25
    sta APP_MOD
    ldx #<msg
    ldy #>msg
    lda #255
    call_firstbank prompt_init
    call_firstbank cursor_activate
    rts
.segment "CODE"
msg: .byt "set option:",0
.segment "CODE2"

down:
    ; Select next option
    ldx printer_active_option
    inx
    cpx printer_option_count
    bcc :+
    rts
    
:   ; Save new active option
    stx printer_active_option

    ; Unselect current option
    clc
    dex
    jsr printer_select

    ; Select new option
    ldx printer_active_option
    sec
    jmp printer_select

up:
    ; Select previous option
    ldx printer_active_option
    bne :+
    rts

:   ; Unselect current option
    clc
    jsr printer_select

    ; Select new option
    dec printer_active_option
    ldx printer_active_option
    sec
    jmp printer_select

left:
    ; Check if option type is int or list (not string)
    ldx printer_active_option
    jsr PRNDRV_GET_OPTION_VALUE
    cmp #PRINTER_TYPE_STRING
    bcc :+
    rts

    ; Yes, decrement value
:   lda #255
left2:
    ldx printer_active_option
    jsr PRNDRV_ADD_OFFSET
    ldx printer_active_option
    jmp printer_refresh_line

right:
    ; Check if option type is int or list (not string)
    ldx printer_active_option
    jsr PRNDRV_GET_OPTION_VALUE
    cmp #PRINTER_TYPE_STRING
    bcc :+
    rts

    ; Yes, increment value
:   lda #1
    bra left2

print:
    jmp printer_print
.endproc

;**********************************************************
;Function name.......: printer_set_option
;Description.........: Key handler for printer option
;                      prompt
;Input...............: A = char
;Error returns.......: Nothing
.proc printer_set_option
    cmp #KEYVAL_ENTER
    beq enter
    cmp #KEYVAL_ESC
    beq hide
    call_firstbank prompt_keypress
    rts

enter:
    jsr hide
    ldx prompt_len
    lda #0
    sta prompt_input,x
    ldx printer_active_option
    lda #<prompt_input
    ldy #>prompt_input
    jsr PRNDRV_SET_OPTION_VALUE
    bcc :+ ; no error
    jsr PRNDRV_GET_MESSAGE
    call_firstbank screen_print_status
:   ldx printer_active_option
    jmp printer_refresh_line

hide:
    call_firstbank prompt_close
    call_firstbank cursor_disable
    lda #24
    sta APP_MOD
    rts
.endproc

;**********************************************************
;Function name.......: printer_select
;Description.........: Selects or unselects an option
;Input...............: X = option index
;                      C = 0: unselect
;                      C = 1: select
;Error returns.......: Nothing
.proc printer_select
    ; Load color, and save on stack
    lda screen_color
    bcc :+
    call_firstbank cmd_swap_colors
:   pha ; color
    txa ; option index

    ; Set VERA to start of line
    clc
    adc #VERA_BUFADR_M+4
    sta VERA_M
    lda #1
    sta VERA_L
    lda #(%00100000 | VERA_BUFADR_H)
    sta VERA_H

    ; Update color
    pla ; Get color from stack
    ldx screen_width
:   sta VERA_D0
    dex
    bne :-

    rts
.endproc

;**********************************************************
;Function name.......: printer_refresh_line
;Description.........: Gets option value from the driver
;                      and displays the value
;Input...............: X: Option index
;Error returns.......: Nothing
.proc printer_refresh_line
    ; Setup VERA
    clc
    txa
    adc #VERA_BUFADR_M+4
    sta VERA_M
    lda printer_value_start_column
    asl
    sta VERA_L
    lda #%00100001
    sta VERA_H
    
    ; Get value from driver
    jsr PRNDRV_GET_OPTION_VALUE
    
    ; Check type
    cmp #PRINTER_TYPE_INT8
    beq int8
    cmp #PRINTER_TYPE_INT16
    beq int16
    cmp #PRINTER_TYPE_LIST
    bcs string
    rts

int8:
    ldy #0 ; Clear bits 8-15 for int8 output
int16:
    lda #0 ; Clear bits 16-24
    call_firstbank util_bin_to_bcd
    call_firstbank util_bcd_to_str
    stx TMP1_ADR
    sty TMP1_ADR+1
    bra output

string:
    stx TMP1_ADR
    sty TMP1_ADR+1

output:
    ldy #0
:   lda (TMP1_ADR),y
    beq clear_end
    call_firstbank screen_put_uc_char
    iny
    bne :-

clear_end:
    ldx #32 ; blank space
:   lda VERA_L ; check if end of line
    lsr
    cmp screen_width
    bcs exit
    stx VERA_D0
    bra :- ; no, continue

exit:
    rts
.endproc

;**********************************************************
;Function name.......: printer_print
;Description.........: Prints the current document
;Input...............: Nothing
;Error returns.......: Nothing
.proc printer_print
    ; Send active charset to printer driver
    lda screen_mode
    jsr PRNDRV_SET_CHARSET

    ; Open printer
    jsr PRNDRV_CHANNEL_OPEN
    bcc :+
    jmp err

    ; Show message "PRINTING..."
:   ldx #<msg1
    ldy #>msg1
    call_firstbank screen_print_status 

    ; Setup pointer to start of text buffer
    lda mem_start
    inc
    sta next_bank
    stz TMP1_ADR
    lda #$a0
    sta next_page

next:
    ; Select next bank and page
    lda next_page
    beq eof
    sta TMP1_ADR+1
    
    lda next_bank
    sta BNK_SEL

    ldy #2
    lda (TMP1_ADR),y
    sta next_bank
    iny
    lda (TMP1_ADR),y
    sta next_page
    iny
    lda (TMP1_ADR),y
    beq next
    sta cur_len
    
loop:
    ; Pause printing if ESC key was pressed
    phy
    bridge_setaddr KERNAL_GETIN
    bridge_call KERNAL_GETIN
    ply
    cmp #KEYVAL_ESC
    beq pause

loop2:
    ; Print next char
    iny
    lda (TMP1_ADR),y
    jsr PRNDRV_PRINT_CHAR
    bcc :+
    jmp err
:   cmp #1
    beq pause ; A = 1 => Driver paused printing
    dec cur_len
    bne loop
    bra next

eof:
    ; End of file, send LF to clear printer buffer, and then close the channel
    lda #LF
    jsr PRNDRV_PRINT_CHAR
    jsr PRNDRV_CHANNEL_CLOSE

    ; Clear status row
    ldx #<msg3
    ldy #>msg3
    call_firstbank screen_print_status
    rts

pause:
    ; Save state, affected by other functions called below
    dey ; Decrement index to resend char after pause
    phy
    lda TMP1_ADR
    pha
    lda TMP1_ADR+1
    pha

    ; Show message "PRINTING PAUSED" in status row
    ldx #<msg2
    ldy #>msg2
    call_firstbank screen_print_status

    ; Wait for ESC or P key
    bridge_setaddr KERNAL_GETIN
:   bridge_call KERNAL_GETIN
    
    cmp #KEYVAL_ESC ; ESC key, abort printing
    bne :+

    pla
    sta TMP1_ADR+1
    pla
    sta TMP1_ADR
    ply
    bra eof

:   and #%11011111 ; convert shifted char to unshifted
    cmp #KEYVAL_P ; P key, continue printing
    bne :--

    ldx #<msg1
    ldy #>msg1
    call_firstbank screen_print_status

    pla
    sta TMP1_ADR+1
    pla
    sta TMP1_ADR
    ply
    jmp loop2

err:
    jsr PRNDRV_GET_MESSAGE
    call_firstbank screen_print_status
    rts

.segment "VARS"
next_bank: .res 1
next_page: .res 1
cur_len: .res 1

.CODE

msg1:
    .byt "printing...",0

msg2:
    .byt "printing paused",0

msg3:
    .byt "printing complete", 0
.segment "CODE2"
.endproc

;**********************************************************
;Function name.......: printer_save_defaults
;Description.........: Prints the current document
;Input...............: Nothing
;Error returns.......: Nothing
.proc printer_save_defaults
    jsr PRNDRV_SAVE_DEFAULTS
    bcs err

ok:
    ldx #<msg
    ldy #>msg
    call_firstbank screen_print_status
    rts

err:
    jsr PRNDRV_GET_MESSAGE
    call_firstbank screen_print_status
    rts

.CODE
    msg: .byt "settings saved",0
.segment "CODE2"
.endproc

;**********************************************************
;Function name.......: printer_suppress_messages
;Description.........: Suppresses Kernal messages by
;                      redirecting the CHROUT vector.
;                      Using the SETMSG function could
;                      also work, but there's no safe way
;                      to restore SETMSG to the state it
;                      had when the editor started.
;Input...............: C=0 restore messages
;                      C=1 suppress messages
;Error returns.......: Nothing
.proc printer_suppress_messages
    bcc restore

    ; Backup original vector
    lda $0326
    sta IBSOUT
    lda $0327
    sta IBSOUT+1

    ; Set new vector
    lda #$18 ; CLC
    sta suppress
    lda #$60 ; RTS
    sta suppress+1
    lda #<suppress
    sta $0326
    lda #>suppress
    sta $0327
    rts

restore:
    lda IBSOUT
    sta $0326
    lda IBSOUT+1
    sta $0327
    rts

.segment "VARS"
    IBSOUT: .res 2
    suppress:
        .res 2
.segment "CODE2"
.endproc

.segment "VARS"
printer_active_option: .res 1
printer_option_count: .res 1
printer_value_start_column: .res 1
.CODE

