;*******************************************************************************
;Copyright 2022-2025, Stefan Jakobsson
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
    .byt "mps-801", 0
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
    ; Check option index
    cpx #OPTION_COUNT
    bcc :+
    sec
    lda #<option_not_exists
    sta message
    lda #>option_not_exists
    sta message+1
    rts

:   ; Check if type is int8 or list; int16 and string not used by this driver
    lda options_type,x
    pha
    beq int8

list:
    lda options_value,x
    asl
    tax
    lda options_string+1,x
    tay
    lda options_string,x
    tax
    bra exit

int8:
    lda options_value,x
    tax

exit:
    pla
    clc
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
    ; Check option index
    cpx #OPTION_COUNT
    bcc :+
    sec
    lda #<option_not_exists
    sta message
    lda #>option_not_exists
    sta message+1
    rts

:   ; Store string pointer
    sta VARZP
    sty VARZP+1
    cmp options_min,x

    ; Exit if type is list
    lda options_type,x
    cmp #TYPE_LIST
    beq err

    ; This driver only supports lists and int8, so it's int8 if not a list
    phx
    jsr util_str_to_bin
    plx
    bcs err

    cmp options_min,x
    bcc err
    cmp options_max,x
    beq :+
    bcs err

:   sta options_value,x
    clc
    rts

err:
    lda #<invalid_value
    sta message
    lda #>invalid_value
    sta message+1
    sec
    rts

val: .res 2
val2: .res 2

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
    ; Check option index
    cpx #OPTION_COUNT
    bcc :+
    sec
    lda #<option_not_exists
    sta message
    lda #>option_not_exists
    sta message+1
    rts

:   ; Check if we're incrementing och decrementing
    clc
    ora #0
    bmi decrement

increment:
    adc options_value,x
    bcs ovfinc
    cmp options_max,x
    beq setval
    bcc setval
ovfinc:
    lda options_min,x
    bra setval

decrement:
    adc options_value,x
    bcc ovfdec
    cmp options_min,x
    bcs setval
ovfdec:
    lda options_max,x
    
setval:
    sta options_value,x
    jmp get_option_value
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
    ldy charset
    cpy #1
    beq :+
    ldy #7 ; = PETSCII upper/lower case
    bra :++
:   ldy #0 ; = PETSCII upper case/graphics
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

    ; Init variables
    lda #6
    sta charsize
    jsr page_start

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

reset: .byt 15,146,16,'0','0' ; = Standard character mode + Turn off reverse field + Tab 0
reset_end:
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

    ; Store last char, convert LF to CR
    cmp #LF
    bne :+
    lda #CR
:   sta last_char

    ; Check if at end of page, assume returning from pause
    lda row_counter
    bne :+
    jsr page_start

:   ; Track printer position
    jsr track_position
    
    ; Check if at end of page
    lda row_counter
    bne :++ ; no, go on and print char

    jsr page_end
    lda OPTION_PAGE_BREAK
    bne :+ ; Continue on page break

    ; Pause at end of page
    pla
    sta ROM_SEL
    ply
    plx

    lda #1
    clc
    rts

:   ; Continue on page break
    jsr page_start

:   ; Print
    lda last_char
    ldx charset
    bne send_char

    ; Convert ASCII to PETSCII
    cmp #$41
    bcc send_char
    cmp #$5b
    bcs :+
    adc #$20
    bra send_char
:   cmp #$61
    bcc send_char
    cmp #$7b
    bcs send_char
    adc #$e0

send_char:
    jsr KERNAL_CHROUT
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
;Function name.......: page_start
;Purpose.............: Code to be executed at start of every new page
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc page_start
    ; Make top margin
    ldx OPTION_TOP_MARGIN
:   beq :+
    lda #CR
    jsr KERNAL_CHROUT
    dex
    bra :-

:   ; Set row count to start of bottom margin
    sec
    lda OPTION_PAGE_HEIGHT
    sbc OPTION_TOP_MARGIN
    sbc OPTION_BOTTOM_MARGIN
    sta row_counter

    ; Init settings
    stz dotaddress
    stz dotaddress+1
    stz quotes

    rts
.endproc

;******************************************************************************
;Function name.......: page_end
;Purpose.............: Code to be executed at end of every page
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc page_end
    ; Make bottom margin
    ldx OPTION_BOTTOM_MARGIN
:   beq :+
    lda #CR
    jsr KERNAL_CHROUT
    dex
    bra :-
:   rts
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

    ; Restore ROM bank
    pla
    sta ROM_SEL

    clc
    rts

fn:
    .byt "@//:x16editpd-mps801.drv"
fn_end:



.endproc

;******************************************************************************
;Function name.......: track_position
;Purpose.............: Tracks printer position based on characters sent
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc track_position
    ; Get last char
    lda last_char

    ; Get mode
    ldx mode
    beq :+
    jmp pos1

:   ; Line break?
    cmp #CR
    bne :+
    jmp line_break

:   ; Check quotes mode
    lda quotes
    and #1
    beq :+
    jmp step_right

:   ; Graphics mode?
    lda last_char ; Reload last char
    cmp #8
    bne :+
    lda #1
    sta charsize
    jmp exit

:   ; Double width?
    cmp #14
    bne :+
    lda #12
    sta charsize
    jmp exit

:   ; Single width?
    cmp #15
    bne :+
    lda #6
    sta charsize
    jmp exit

:   ; Set column position?
    cmp #16
    bne :+
    lda #MODE_POS1
    sta mode
    jmp exit

:   ; Repeat graphics?
    cmp #26
    bne :+
    lda #MODE_REPEAT
    sta mode
    jmp exit

:   ; Set dot address?
    cmp #27
    bne :+
    lda #MODE_ADDR1
    sta mode
    jmp exit

:   ; Double quote?
    cmp #34
    bne :+
    inc quotes

:   ; Control char?
    and #$7f
    cmp #$20
    bcc exit

step_right:
    ; Ignore if graphics mode and value is less than $80
    lda charsize
    cmp #1
    bne :+
    lda last_char
    bpl exit 

:   ; Increment horizontal position
    clc
    lda dotaddress
    adc charsize
    sta dotaddress
    lda dotaddress+1
    adc #0
    sta dotaddress+1

    ; Check line overflow
    lda dotaddress+1
    cmp #$01
    bcc exit
    bne :+
    lda dotaddress
    cmp #$e0
    bcc exit

:   ; Automatic line break happened due to overflow
    lda charsize
    sta dotaddress
    stz dotaddress+1
    dec row_counter
    stz quotes

exit:
    rts

line_break:
    stz dotaddress
    stz dotaddress+1
    dec row_counter
    stz quotes
    rts

pos1:
    ; Set position command: first digit (10^1)
    lda mode
    cmp #MODE_POS1
    bne pos2

    sec             ; Get digit from PETSCII
    lda last_char
    sbc #$30
    bcs :+
    jmp error
:   cmp #10
    bcc :+
    jmp error
:   eor #$ff
    inc
    
    asl             ; Digit * 10
    sta temp
    asl
    asl
    clc
    adc temp

    sta dotaddress  ; Store column
    inc mode
    rts

pos2:
    ; Set position command: second digit (10^0)
    cmp #MODE_POS2
    bne addr1

    sec             ; Get digit from PETSCII
    lda last_char
    sbc #$30
    bcs :+
    jmp error
:   cmp #10
    bcc :+
    jmp error
:   eor #$ff
    inc

    clc             ; Add column value
    adc dotaddress
    sta dotaddress

    asl dotaddress  ; Dot address = column * 6
    sta temp
    rol dotaddress+1
    asl dotaddress
    rol dotaddress+1
    clc
    lda dotaddress
    adc temp
    sta dotaddress
    lda dotaddress+1
    adc #0
    sta dotaddress+1
    stz mode
    rts

addr1: 
    ; Set dot address command: tab command ($10)
    cmp #MODE_ADDR1
    bne addr2

    lda last_char
    cmp #$10
    beq :+
    jmp error
:   inc mode
    rts

addr2:
    ; Set dot address command: MSB
    cmp #MODE_ADDR2
    bne addr3

    lda last_char
    sta dotaddress+1
    inc mode
    rts

addr3:
    ; Set dot address command: LSB
    cmp #MODE_ADDR3
    bne repeat

    lda last_char
    sta dotaddress
    stz mode
    rts

repeat:
    cmp #MODE_REPEAT
    bne error

    clc
    lda dotaddress
    adc last_char
    sta dotaddress
    lda dotaddress+1
    adc #0
    sta dotaddress+1
    stz mode
    rts

error:
    stz mode
    rts
.endproc

.include "util.inc"

;******************************************************************************
; Variables
mode: .res 1
charset: .res 1
row_counter: .res 1
dotaddress: .res 2
charsize: .res 1
quotes: .res 1
last_char: .res 1
temp: .res 1
message: .word null_message

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
    .byt 0, 0, 0, 0, 2

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
