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

; Option types
TYPE_INT8 = 0
TYPE_INT16 = 1
TYPE_LIST = 2
TYPE_STRING = 3

; Misc definitions
base_addr = $9f60
LF = 10
CR = 13

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
    .byt "star-sp700 @ serial", 0
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
    ; Not used by this driver
    clc
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
    ; Clear quote count
    stz quotes

    ; Calculate base address
    lda OPTION_IOADDRESS ; List index x 16
    asl
    asl
    asl
    asl
    sta base_offset

    ; Set BAUD rate
    ldx base_offset
    lda #$80            ; DLAB=1
    sta base_addr+3,x

    sec
    lda OPTION_BAUDRATE
    sbc options_min+1
    tay
    lda baud_rates,y
    sta base_addr,x
    lda #0
    sta base_addr+1,x

    lda #$03            ; DLAB=0
    sta base_addr+3,x
    
    ; Black text
    lda #$1b
    sta base_addr,x
    lda #$35
    sta base_addr,x

    ; Exit
    clc
    rts

baud_rates:
    .byt 48, 24, 16
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
    phx
    ldx base_offset

    cmp #LF
    bne :+
    lda #CR
    sta base_addr,x
    lda #LF
    sta base_addr,x
    bra exit

:   sta base_addr,x

exit:
    plx
    lda #0
    clc
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
    ldx base_offset

    ; Print extra line break
    lda #CR
    sta base_addr,x
    lda #LF
    sta base_addr,x

    ; On print complete action
    lda OPTION_ONCOMPLETE
    cmp options_min+2
    bne :+

    ; Cut paper
    lda #$1b
    sta base_addr,x
    lda #64
    sta base_addr,x
    lda #0
    sta base_addr,x

:   ; Exit
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
    ; Not supported, return error
    sec
    lda #<msg
    sta message
    lda #>msg
    sta message+1
    rts
msg:
    .byt "save settings not supported", 0
.endproc

;******************************************************************************
; Variables
base_offset: .res 1
charset: .res 1
quotes: .res 1
temp: .res 1
message: .word null_message

; *****************************************************************************
; Printing options

OPTION_COUNT = 3
OPTION_IOADDRESS = options_value+0
OPTION_BAUDRATE = options_value+1
OPTION_ONCOMPLETE = options_value+2

options_label:
    .byt "io address.:", 13
    .byt "baud rate..:", 13
    .byt "on complete:", 0

options_type:
    .byt TYPE_LIST, TYPE_LIST, TYPE_LIST

options_value:
    .byt 8, 11, 14

options_min:
    .byt 0, 10, 13

options_max:
    .byt 9, 12, 14

options_string:
    .word str_io3_low       ; List index 0
    .word str_io3_high      ; List index 1
    .word str_io4_low       ; List index 2
    .word str_io4_high      ; List index 3
    .word str_io5_low       ; List index 4
    .word str_io5_high      ; List index 5
    .word str_io6_low       ; List index 6
    .word str_io6_high      ; List index 7
    .word str_io7_low       ; List index 8
    .word str_io7_high      ; List index 9

    .word str_baud_19200    ; List index 10    
    .word str_baud_38400    ; List index 11
    .word str_baud_57600    ; List index 12

    .word str_cut           ; List index 13
    .word str_noaction      ; List index 14

str_io3_low:
    .byt "io3 low", 0
str_io3_high:
    .byt "io3 high", 0
str_io4_low:
    .byt "io4 low", 0
str_io4_high:
    .byt "io4 high", 0
str_io5_low:
    .byt "io5 low", 0
str_io5_high:
    .byt "io5 high", 0
str_io6_low:
    .byt "io6 low", 0
str_io6_high:
    .byt "io6 high", 0
str_io7_low:
    .byt "io7 low", 0
str_io7_high:
    .byt "io7 high", 0

str_baud_19200:
    .byt "19.2 kb/s", 0
str_baud_38400:
    .byt "38.4 kb/s", 0
str_baud_57600:
    .byt "57.6 kb/s", 0

str_cut:
    .byt "cut paper", 0
str_noaction:
    .byt "no action", 0

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
