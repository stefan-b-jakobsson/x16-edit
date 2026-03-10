;*******************************************************************************
;Copyright 2026, Stefan Jakobsson
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

; Kernal functions
ROM_SEL = $01
KERNAL_CLRCHN = $ffcc
KERNAL_CLOSE = $ffc3
KERNAL_OPEN = $ffc0
KERNAL_CHROUT = $ffd2
KERNAL_SETLFS = $ffba
KERNAL_SETNAM = $ffbd
KERNAL_CHKOUT = $ffc9
KERNAL_READST = $ffb7
KERNAL_SAVE = $ffd8
KERNAL_IBSOUT = $0326

; Multi-byte command states
MODE_DEFAULT = 0
MODE_POS1 = 1
MODE_POS2 = 2
MODE_ADDR1 = 3
MODE_ADDR2 = 4
MODE_ADDR3 = 5
MODE_REPEAT = 4

; Option types
TYPE_INT8 = 0
TYPE_INT16 = 1
TYPE_LIST = 2
TYPE_STRING = 3

; Misc definitions
LF = 10
CR = 13
FF = 12
PAGE_WIDTH = 480 ; 6 x 80 dots

; Zero page variables (4 bytes)
VARZP = $32

.segment "JMPTBL"
;*******************************************************************************
; Driver jump table
;*******************************************************************************

jmp save_defaults           ; $9ED6
jmp get_message             ; $9ED9
jmp channel_close           ; $9EDC
jmp print_char              ; $9EDF
jmp channel_open            ; $9EE2
jmp option_add_offset       ; $9EE5
jmp set_option_value        ; $9EE8
jmp get_option_value        ; $9EEB
jmp get_option_labels       ; $9EEE
jmp get_option_count        ; $9EF1
jmp set_charset             ; $9EF4
jmp get_driver_name         ; $9EF7
jmp get_api_version         ; $9EFA
jmp init                    ; $9EFD

.segment "CODE"
;******************************************************************************
;Function name.......: init
;Purpose.............: Initializes the printer driver and selects default
;                      printer settings
;Input...............: Nothing
;Returns.............: C = 1 on error
;Errors..............: None
;Affected registers..: A, X, Y
.proc init
    ; This driver requires no initializtion
    clc
    rts
.endproc

;******************************************************************************
;Function name.......: get_api_version
;Purpose.............: Returns printer driver API version
;Input...............: Nothing
;Returns.............: A = API version
;Errors..............: None
;Affected registers..: A
.proc get_api_version
    lda #1
    rts
.endproc

;******************************************************************************
;Function name.......: get_driver_name
;Purpose.............: Returns driver name
;Input...............: Nothing
;Returns.............: X/Y Pointer to null-terminated string
;Errors..............: None
;Affected registers..: A, X, Y
.proc get_driver_name
    ldx #<name
    ldy #>name
    rts
name: 
    .byt "generic esc/p printer @ iec port", 0
.endproc

;******************************************************************************
;Function name.......: set_charset
;Purpose.............: Sets charset encoding
;Input...............: A    0 = ISO
;                           1 = PETSCII upper case
;                           2 = PETSCII lower case
;                           Other input ignored
;Returns.............: Nothing
;Errors..............: C = 1 if invalid input
;Affected registers..: A, X, Y
.proc set_charset
    cmp #3
    bcs :+ ; Ignore invalid values
    sta charset
    clc
:   rts
.endproc

;******************************************************************************
;Function name.......: get_option_count
;Purpose.............: Returns the number of options supported by this driver
;Input...............: Nothing
;Returns.............: A = Option count
;Errors..............: None
;Affected registers..: A
.proc get_option_count
    lda #OPTION_COUNT
    rts
.endproc

;******************************************************************************
;Function name.......: get_option_labels
;Purpose.............: Returns a pointer to a null-terminated string containing
;                      one label per line that is terminated by CR
;Input...............: Nothing
;Returns.............: X/Y String pointer
;Errors..............: None
;Affected registers..: X, Y
.proc get_option_labels
    ldx #<options_label
    ldy #>options_label
    rts
.endproc

;******************************************************************************
;Function name.......: get_option_value
;Purpose.............: Returns option value referred to by index
;Input...............: X: Option index
;Returns.............: A: type (0=8 bit int, 1=16 bit integer, 2=list item, 3=null-terminated string)
;                      X: 8 bit int value
;                      X/Y: 16 bit int value (not used by this driver)
;                      X/Y: pointer to list item or string
;Errors..............: C = 1 on option index out of range
;Affected registers..: A, X, Y
.proc get_option_value
    ; Read int8 value into X
    cpx #0
    bne :+
    ldx OPTION_DEVICE
    bra exit

:   cpx #1
    bne :+
    ldx OPTION_PAGE_HEIGHT
    bra exit

:   cpx #2
    bne :+
    ldx OPTION_LEFT_MARGIN
    bra exit

:   cpx #3
    bne :+
    ldx OPTION_RIGHT_MARGIN
    bra exit

:   cpx #4
    bne err
    ldx OPTION_BOTTOM_MARGIN
exit:
    clc
    lda #0
    rts

err:
    ; Option doesn't exist
    sec
    lda #<option_not_exists
    sta message
    lda #>option_not_exists
    sta message+1
    rts
.endproc

;******************************************************************************
;Function name.......: set_option_value
;Purpose.............: Sets option value referred to by index
;Input...............: X: Option index
;                      A/Y: Pointer to null-terminated string
;Returns.............: Same as for get_option_value
;Errors..............: C = 1 on option index out of range or option value out
;                      of valid range
;Affected registers..: A, X, Y
.proc set_option_value
    phx

    ; Convert int8 input
    sta VARZP
    sty VARZP+1
    jsr util_str_to_bin

    ; Set int8 value
    plx
    cpx #0
    bne :+
    cmp #4
    bcc err
    cmp #5+1
    bcs err
    sta OPTION_DEVICE
    bra exit

:   cpx #1
    bne :+
    sta OPTION_PAGE_HEIGHT
    bra exit
    
:   cpx #2
    bne :+
    sta OPTION_LEFT_MARGIN
    bra exit
:   cpx #3
    bne :+
    sta OPTION_RIGHT_MARGIN
    bra exit

:   cpx #4
    bne err
    sta OPTION_BOTTOM_MARGIN
exit:
    tax
    lda #0
    clc
    rts

err:
    lda #<invalid_value
    sta message
    lda #>invalid_value
    sta message+1
    sec
    rts
.endproc

;******************************************************************************
;Function name.......: option_add_offset
;Purpose.............: Adds a signed 8 bit offset to the selected option. The
;                      option must any of the following types: int8, int16 or
;                      list.
;Input...............: X: Option index
;                      A: signed 8 bit offset value
;Returns.............: Same as for get_option_value
;Errors..............: C=1 if index of of range
;Affected registers..: A, X, Y
.proc option_add_offset
    cpx #0
    bne :++
    clc
    adc OPTION_DEVICE
    sta OPTION_DEVICE
    cmp #4
    bcc :+
    cmp #5+1
    bcc exit
    lda #4
    sta OPTION_DEVICE
    bra exit
:   lda #5
    sta OPTION_DEVICE
    bra exit
    
:   cpx #1
    bne :+
    clc
    adc OPTION_PAGE_HEIGHT
    sta OPTION_PAGE_HEIGHT
    bra exit
    
:   cpx #2
    bne :+
    clc
    adc OPTION_LEFT_MARGIN
    sta OPTION_LEFT_MARGIN
    bra exit
    
:   cpx #3
    bne :+
    clc
    adc OPTION_RIGHT_MARGIN
    sta OPTION_RIGHT_MARGIN
    bra exit

:   cpx #4
    bne err
    clc
    adc OPTION_BOTTOM_MARGIN
    sta OPTION_BOTTOM_MARGIN

exit:
    tax
    lda #0
    clc
    rts

err:
    lda #<invalid_value
    sta message
    lda #>invalid_value
    sta message+1
    sec
    rts
.endproc

;******************************************************************************
;Function name.......: channel_open
;Purpose.............: Opens printer channel, and sends commands to set
;                      initial printer options
;Input...............: Nothing
;Returns.............: Nothing
;Errors..............: C=1 on error
;Affected registers..: A, X, Y
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
    ldy #0
    jsr KERNAL_SETLFS

    ; Open file
    jsr KERNAL_OPEN
    bcs err

    ; Set file as std out
    ldx #1
    jsr KERNAL_CHKOUT
    bcs err
    
    ; Initialize printer
    ldx #0
:   lda init_seq,x
    jsr KERNAL_CHROUT
    jsr KERNAL_READST
    bne err
    inx
    cpx #init_seq_end-init_seq
    bne :-

    ; Restore ROM bank
    pla
    sta ROM_SEL
    clc
    rts

err:
    ; Restore ROM bank
    plx
    stx ROM_SEL
    jsr channel_close
    lda #<printing_error
    sta message
    lda #>printing_error
    sta message+1
    sec
    rts
.endproc

;******************************************************************************
;Function name.......: print_char
;Purpose.............: Sends one char to the printer
;Input...............: A = char
;Returns.............: A: Response code
;                         0 = OK
;                         1 = Paused before printing char (resend char)
;Error returns.......: C=1 on error
;Preserved registers.: X, Y
.proc print_char
    ; Backup X and Y
    phx
    phy

    ; Backup ROM bank, and select ROM bank 0
    ldx ROM_SEL
    phx
    stz ROM_SEL

    ; Convert LF to CRLF
    cmp #LF
    bne :+
    lda #CR
    jsr KERNAL_CHROUT
    jsr KERNAL_READST
    bne err
    lda #LF

    ; Print char
:   jsr KERNAL_CHROUT
    jsr KERNAL_READST
    bne err
    
exit:
    pla
    sta ROM_SEL
    ply
    plx

    lda #0
    clc
    rts

err:
    pla
    sta ROM_SEL
    ply
    plx
    lda #<printing_error
    sta message
    lda #>printing_error
    sta message+1
    sec
    rts

.endproc

;******************************************************************************
;Function name.......: channel_close
;Purpose.............: Closes printer channel
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
;Affected registers..: A, X, Y
.proc channel_close
    ; Backup ROM bank, and select ROM bank 0
    lda ROM_SEL
    pha
    stz ROM_SEL

    ; Send Form Feed
    lda #FF
    jsr KERNAL_CHROUT

    ; Reset printer
    lda #27
    jsr KERNAL_CHROUT
    lda #64
    jsr KERNAL_CHROUT

    ; Close file
    lda #1
    jsr KERNAL_CLOSE
    jsr KERNAL_CLRCHN

    ; Restore ROM bank
    pla
    sta ROM_SEL
    rts
.endproc

;******************************************************************************
;Function name.......: get_message
;Purpose.............: Returns pointer to null-terminated message string,
;                      mostly intended for error messages. The message is
;                      reset after calling this function.
;Input...............: Nothing
;Returns.............: X/Y String pointer
;Affected registers..: A,X,Y
.proc get_message
    ; Get pointer
    ldx message
    ldy message+1
    lda #<null_message
    
    ; Reset message
    sta message
    lda #>null_message
    sta message+1
    rts
.endproc

;******************************************************************************
;Function name.......: save_defaults
;Purpose.............: Save default settings
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: C = 1 on error
;Affected registers..: A, X, Y
.proc save_defaults
    ; Backup current ROM bank, and select ROM bank 0
    lda ROM_SEL
    pha
    stz ROM_SEL

    ; Redirect BSOUT vector to suppress Kernal messages
    lda KERNAL_IBSOUT
    sta ibsout_backup
    lda KERNAL_IBSOUT+1
    sta ibsout_backup+1
    lda #<bsout_suppress
    sta KERNAL_IBSOUT
    lda #>bsout_suppress
    sta KERNAL_IBSOUT+1

    ; Setup file name
    ldx #<fn
    ldy #>fn
    lda #fn_end-fn
    jsr KERNAL_SETNAM

    ; Setup file params
    lda #1
    ldx #8
    ldy #0
    jsr KERNAL_SETLFS

    ; Setup start address
    lda #$00
    sta VARZP
    lda #$90
    sta VARZP+1

    ; Save
    lda #<VARZP
    ldx #$00
    ldy #$9f
    jsr KERNAL_SAVE
    plx ; ROM bank
    php ; Store status on stack

    ; Restore BSOUT vector
    lda ibsout_backup
    sta KERNAL_IBSOUT
    lda ibsout_backup+1
    sta KERNAL_IBSOUT+1

    ; Read status
    jsr KERNAL_READST

    ; Restore ROM bank
    stx ROM_SEL

    plp ; Get status from stack
    bcs err
    cmp #0
    bne err
    clc
    rts

err:
    ldx #<save_err
    stx message
    ldx #>save_err
    stx message+1
    rts

bsout_suppress:
    clc
    rts

fn:
    .byt "@//:x16editpd-generic-escp.drv"
fn_end:

ibsout_backup:
    .res 2

save_err:
    .byt "saving default options failed", 0

.endproc

.include "util.inc"

;******************************************************************************
; Variables
charset: .res 1
message: .word null_message
cur_device: .byt 4

; *****************************************************************************
; Printing options

OPTION_COUNT = 5
OPTION_DEVICE = cur_device
OPTION_PAGE_HEIGHT = init_seq+4
OPTION_LEFT_MARGIN = init_seq+7
OPTION_RIGHT_MARGIN = init_seq+10
OPTION_BOTTOM_MARGIN = init_seq+13

init_seq:
    .byt 27, 64         ; ESC @     - reset
    .byt 27, 67, 66     ; ESC C nn  - Set page height
    .byt 27, 108, 0     ; ESC l nn  - Set left margin
    .byt 27, 81, 80     ; ESC Q nn  - Set right margin
    .byt 27, 78, 6      ; ESC N nn  - Set bottom margin
init_seq_end:

options_label:
    .byt "serial device:", 13
    .byt "page height..:", 13
    .byt "left margin..:", 13
    .byt "right margin.:", 13
    .byt "bottom margin:", 0

; *****************************************************************************
; Common messages
null_message:
    .byt 0

option_not_exists:
    .byt "option does not exist", 0

invalid_value:
    .byt "invalid value", 0

printing_error:
    .byt "printer communication error", 0
