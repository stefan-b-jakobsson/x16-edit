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

; Build: cl65 -o MPS801.DRV -t cx16 -C conf/printer_driver.cfg mps801.asm

ROM_SEL = $01
KERNAL_CLRCHN = $ffcc
KERNAL_CLOSE = $ffc3
KERNAL_OPEN = $ffc0
KERNAL_CHROUT = $ffd2
KERNAL_SETLFS = $ffba
KERNAL_SETNAM = $ffbd
KERNAL_CHKOUT = $ffc9
KERNAL_READST = $ffb7

PAGE_WIDTH = 80
LF = 10
CR = 13

;*******************************************************************************
; Driver jump table
;*******************************************************************************
jmp init                    ; $9000
jmp get_driver_name         ; $9003
jmp set_charset             ; $9006
jmp get_option_count        ; $9009
jmp get_option_labels       ; $900c
jmp get_option_value        ; $900f
jmp set_option_value        ; $9012
jmp channel_open            ; $9015
jmp print_char              ; $9018
jmp channel_close           ; $901b
jmp save_defaults           ; $901e

;******************************************************************************
;Function name.......: init
;Purpose.............: Initializes the printer driver and selects default
;                      printer settings
;Input...............: Nothing
;Returns.............: C = 1 on error
;Errors..............: None
;Preserved registers.: None
.proc init
    ; This driver requires no initializtion
    clc
    rts
.endproc

;******************************************************************************
;Function name.......: get_driver_name
;Purpose.............: Returns driver name
;Input...............: Nothing
;Returns.............: X/Y Pointer to null-terminated string
;Errors..............: None
;Preserved registers.: None
.proc get_driver_name
    ldx #<name
    ldy #>name
    rts
name: 
    .byt "mps-801", 0
.endproc

;******************************************************************************
;Function name.......: set_charset
;Purpose.............: Sets charset encoding
;Input...............: A:   0 = ISO
;                           1 = PETSCII upper case
;                           2 = PETSCII lower case
;                           Other input ignored
;Returns.............: Nothing
;Errors..............: None
;Preserved registers.: None
.proc set_charset
    cmp #3
    bcs :+ ; Ignore invalid values
    sta charset
:   rts
.endproc

.proc get_option_count
    lda #OPTION_COUNT
    rts
.endproc

.proc get_option_labels
    ldx #<options_label
    ldy #>options_label
    rts
.endproc

;******************************************************************************
;Function name.......: get_option_value
;Purpose.............: Returns option value
;Input...............: X: Option index
;Returns.............: A: type (0=8 bit int, 1=list item, 2=null-terminated string)
;                      X: int value
;                      X/Y: string pointer
;Errors..............: C = 1 on option index out of range
;Preserved registers.: None
.proc get_option_value
    cpx #OPTION_COUNT
    bcc :+
    sec
    rts

:   ; Check if type is int or list, string not used in this driver
    lda options_type,x
    pha
    beq int

list:
    lda options_value,x
    asl
    tax
    lda options_string+1,x
    tay
    lda options_string,x
    tax
    bra exit

int:
    lda options_value,x
    tax

exit:
    pla
    clc
    rts
.endproc

;******************************************************************************
;Function name.......: set_option_value
;Purpose.............: Sets option value
;Input...............: X: Option index
;                      A: Value to be added to current value
;                      A/Y: String pointer
;Returns.............: Same as for get_option_value
;Errors..............: C = 1 on option index out of range
;Preserved registers.: None
.proc set_option_value
    cpx #OPTION_COUNT
    bcc :+
    sec
    rts

:   ; Update int or list, string not used in this driver
    clc
    adc options_value,x

    cmp options_min,x
    bcc err
    cmp options_max,x
    beq :+
    bcs err

:   sta options_value,x
    jmp get_option_value

err:
    sec
    rts
.endproc

;******************************************************************************
;Function name.......: channel_open
;Purpose.............: Opens printer channel, and sends commands to set
;                      printer options
;Input...............: Nothing
;Returns.............: Nothing
;Errors..............: C=1 on error, error code in A
;Preserved registers.: None
.proc channel_open
    ; Backup current ROM bank, and select ROM bank 0
    lda ROM_SEL
    pha
    stz ROM_SEL

    ; Ensure file #1 is closed
    lda #1
    jsr KERNAL_CLOSE

    ; Set empty name
    lda #0
    jsr KERNAL_SETNAM

    ; Set file params
    lda #1
    ldx OPTION_DEVICE
    ldy charset
    cpy #1
    beq :+
    ldy #7 ; = upper/lower case
    bra :++
:   ldy #0 ; = upper case/graphics
:   jsr KERNAL_SETLFS

    ; Open file
    jsr KERNAL_OPEN
    bcs err

    ; Set file as std out
    ldx #1
    jsr KERNAL_CHKOUT
    bcs err
    
    ; Send commands for default printer settings
    ldx #0
:   lda reset,x
    jsr KERNAL_CHROUT
    bcs err
    inx
    cpx #reset_end-reset
    bne :-

    ; Clear row_counter to force new page when receiving the first character
    stz row_counter

    ; Restore ROM bank
    pla
    sta ROM_SEL
    clc

    rts

err:
    ; Restore ROM bank
    plx
    stx ROM_SEL
    pha
    jsr channel_close
    pla
    sec
    rts

reset: .byt 15,146,16,0,0 ; = Standard character mode + Turn off reverse field + Tab 0
reset_end:
.endproc

;******************************************************************************
;Function name.......: print_char
;Purpose.............: Sends one char to the printer
;Input...............: A = char
;Returns.............: A: Response code
;                         0 = OK
;                         1 = Stop on page break
;Error returns.......: C=1 on error
;Preserved registers.: X, Y
.proc print_char
    ; Backup X and Y
    phx
    phy

    ; Store input
    sta temp

    ; Backup ROM bank, and select ROM bank 0
    ldx ROM_SEL
    phx
    stz ROM_SEL

    ; If row_counter = 0 then page start
    ldx row_counter
    bne :+
    jsr page_start
    lda temp

    ; Print
:   ldx charset
    bne @2

    ; Convert ASCII to PETSCII
    cmp #$41
    bcc @2
    cmp #$5b
    bcs @1
    adc #$20
    bra @2
@1: cmp #$61
    bcc @2
    cmp #$7b
    bcs @2
    adc #$e0

@2: ; Send to printer
    jsr KERNAL_CHROUT
    jsr KERNAL_READST
    bne err

    ; Line break?
    lda temp
    cmp #LF
    beq @3
    cmp #CR
    beq @3

    ; Decrement column counter
    dec column_counter
    bne @4
    lda #LF
    jsr KERNAL_CHROUT
    
@3: ; Handle end of line
    lda #PAGE_WIDTH
    sta column_counter

    dec row_counter
    bne @4

    jsr page_end

    ; Return, at end of page
    pla
    sta ROM_SEL
    lda #1
    clc
   
    ; Restore registers
    ply
    plx
   
    rts

@4: ; Return, not end of page
    pla
    sta ROM_SEL
    lda #0
    clc

    ; Restore registers
    ply
    plx

    rts

err:
    pla
    sta ROM_SEL
    sec

    ; Restore registers
    ply
    plx

    rts

.endproc

;******************************************************************************
;Function name.......: page_start
;Purpose.............: Code to be executed at start of every new page
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc page_start
    ldx OPTION_TOP_MARGIN
:   lda #LF
    jsr KERNAL_CHROUT
    cpx #0
    beq :+
    dex
    bne :-

:   sec
    lda OPTION_PAGE_HEIGHT
    sbc OPTION_TOP_MARGIN
    sbc OPTION_BOTTOM_MARGIN
    sta row_counter

    lda #PAGE_WIDTH
    sta column_counter

    rts
.endproc

;******************************************************************************
;Function name.......: page_end
;Purpose.............: Code to be executed at end of every page
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc page_end
    ldx OPTION_BOTTOM_MARGIN
:   lda #LF
    jsr KERNAL_CHROUT
    cpx #0
    beq :+
    dex
    bne :-
:   rts
.endproc

;******************************************************************************
;Function name.......: channel_close
;Purpose.............: Closes printer channel
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
;Preserved registers.: None
.proc channel_close
    ; Backup ROM bank, and select ROM bank 0
    lda ROM_SEL
    pha
    stz ROM_SEL

    lda #1
    jsr KERNAL_CLOSE
    jsr KERNAL_CLRCHN

    ; Restore ROM bank
    pla
    sta ROM_SEL
    rts
.endproc

.proc save_defaults
    rts
.endproc

;******************************************************************************
; Variables
charset: .res 1
row_counter: .res 1
column_counter: .res 1
temp: .res 1

; *****************************************************************************
; Printing options

OPTION_COUNT = 5
OPTION_DEVICE = options_value+0
OPTION_PAGE_HEIGHT = options_value+1
OPTION_TOP_MARGIN = options_value+2
OPTION_BOTTOM_MARGIN = options_value+3
OPTION_PAGE_BREAK = options_value+4

options_label:
    .byt "serial device:", 13
    .byt "page height..:", 13
    .byt "top margin...:", 13
    .byt "bottom margin:", 13
    .byt "on page break:", 0

options_type:
    .byt 0, 0, 0, 0, 1

options_value:
    .byt 4, 66, 3, 3, 0

options_min:
    .byt 4, 1, 0, 0, 0

options_max:
    .byt 5, 255, 255, 255, 1

options_string:
    .word str_pause         ; List index 0
    .word str_continue      ; List index 1

str_pause:
    .byt "pause", 0

str_continue:
    .byt "continue", 0
