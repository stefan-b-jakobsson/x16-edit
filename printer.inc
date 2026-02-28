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
PRNDRV_INIT                    = $9000
PRNDRV_GET_DRIVER_NAME         = $9003
PRNDRV_SET_CHARSET             = $9006
PRNDRV_GET_OPTION_COUNT        = $9009
PRNDRV_GET_OPTION_LABELS       = $900c
PRNDRV_GET_OPTION_VALUE        = $900f
PRNDRV_SET_OPTION_VALUE        = $9012
PRNDRV_CHANNEL_OPEN            = $9015
PRNDRV_PRINT_CHAR              = $9018
PRNDRV_CHANNEL_CLOSE           = $901b
PRNDRV_SAVE_DEFAULTS           = $901e

.segment "CODE2"

;**********************************************************
;Function name.......: printer_load_driver
;Description.........: Loads driver from SD card into RAM
;Input...............: Nothing
;Returns.............: A = 0 if successfully loaded
;                      A = 1 if loading failed
;                      A = 2 if unsupported driver
.proc printer_load_driver
    ; Close file 1
    bridge_setaddr KERNAL_CLOSE
    lda #1
    bridge_call KERNAL_CLOSE

    ; Copy name to temp vars
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
    bridge_setaddr KERNAL_LOAD
    lda #0
    bridge_call KERNAL_LOAD
    bcs loaderr

    ; Check for errors
    bridge_setaddr KERNAL_READST
    bridge_call KERNAL_READST
    and #2|128 ; read time out or device not present
    bne loaderr

    call_firstbank file_read_disk_status
    cmp #0
    bne loaderr

    ; Load successful, check diver compatibilty
    ; TODO

    ; Return OK
    lda #0
    rts

loaderr:
    lda #1
    rts

fn:
    .byt "x16editpd-*"
fn_end:
.endproc

;**********************************************************
;Function name.......: printer_dialog
;Description.........: Shows print dialog
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

    ; Output "PRINT TO "
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

    ; Clear column where settings are displayed
    stz printer_value_start_column

    ; Get option count
    jsr PRNDRV_GET_OPTION_COUNT
    sta printer_option_count

    ; Print option labels
    jsr PRNDRV_GET_OPTION_LABELS
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
    ; Update max label width
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
    ; Update max label width
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
    cmp #KEYVAL_ESC
    beq esc

    cmp #KEYVAL_DOWN
    bne :+
    jmp down

:   cmp #KEYVAL_UP
    bne :+
    jmp up

:   cmp #KEYVAL_LEFT
    bne :+
    jmp left

:   cmp #KEYVAL_RIGHT
    bne :+
    jmp right

:   ; Clear bit 5 to convert shifted char to unshifted char
    and #%11011111

    cmp #KEYVAL_P
    bne :+
    jmp print

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
    ; Check if option type is int or list
    ldx printer_active_option
    jsr PRNDRV_GET_OPTION_VALUE
    cmp #2
    bcc :+
    rts

    ; Yes, decrement value
:   lda #255
left2:
    ldx printer_active_option
    jsr PRNDRV_SET_OPTION_VALUE
    ldx printer_active_option
    jmp printer_refresh_line

right:
    ; Check if option type is int or list
    ldx printer_active_option
    jsr PRNDRV_GET_OPTION_VALUE
    cmp #2
    bcc :+
    rts

    ; Yes, increment value
:   lda #1
    bra left2

print:
    jmp printer_print
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
    cmp #0
    beq int
    cmp #3
    bcc string
    rts

int:
    ldy #0
    lda #0
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
    ; Init printer
    jsr PRNDRV_INIT
    
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
:   dec cur_len
    bne loop
    bra next

eof:
    ; End of file, send CR to clear printer buffer, and then close the channel
    lda #KEYVAL_ENTER
    jsr PRNDRV_PRINT_CHAR
    jsr PRNDRV_CHANNEL_CLOSE

    ; Clear status row
    call_firstbank screen_clear_status
    rts

pause:
    ; Save state, affected by other functions called below
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
    tax
    call_firstbank cmd_file_show_io_err
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

.endproc

.segment "VARS"
printer_active_option: .res 1
printer_option_count: .res 1
printer_value_start_column: .res 1
.CODE

