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

; Hardware registers
ROM_SEL = $01

; Kernal functions
KERNAL_SETLFS = $ffba
KERNAL_SETNAM = $ffbd
KERNAL_READST = $ffb7
KERNAL_SAVE = $ffd8
KERNAL_IBSOUT = $0326

; Option types
TYPE_INT8 = 0
TYPE_INT16 = 1
TYPE_LIST = 2
TYPE_STRING = 3

; Misc definitions
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
    .byt "ipp/postscript", 0
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
;                      X/Y: string pointer (not used by this driver)
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

:   lda options_type,x
    pha

list:
    cmp #TYPE_LIST
    bne int8
    lda options_value,x
    asl
    tax
    lda options_string+1,x
    tay
    lda options_string,x
    tax
    bra exit

int8:
    cmp #TYPE_INT8
    bne string
    lda options_value,x
    tax
    bra exit

string:
    ldx #<path
    ldy #>path

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

    ; Check if path (option index #7)
    cpx #7
    bne :+++
    
    ldy #0          ; Copy string
:   lda (VARZP),y
    sta path,y
    beq :+
    iny
    bra :-
:   lda #TYPE_STRING
    ldx #<path
    ldy #>path
    rts

    ; Exit if type is list
:   lda options_type,x
    cmp #TYPE_LIST
    beq err

    ; If it's not a list, it's an int8
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

    ; Initialize
    stz is_new_line
    jsr uart_init
    bcs err

    ; Send AT+PRINT command
    ldx #<at_command
    ldy #>at_command
    lda #0
    jsr uart_write_string
    bcs err

    ; Send printer address + CR
    ldx #<path
    ldy #>path
    lda #0
    jsr uart_write_string
    bcs err

    lda #CR
    jsr uart_write_byte
    bcs err

    ; Prepare to send PostScript data stream
    stz curfield
    ldx #<ps_template
    ldy #>ps_template
loop:
    lda #'?' ; Fields are denoted by "?" in the PostScript template
    jsr uart_write_string
    bcs err
    cmp #'?' ; Is it a field? If not, fallthrough to exit
    beq isfield

exit:
    ; Restore ROM bank
    pla
    sta ROM_SEL

    ; Return with OK (C=0)
    clc
    rts

err:
    ; Restore ROM bank
    pla
    sta ROM_SEL

    ; Return with error (C=1)
    sec
    rts

isfield:
    ; Backup current position in the PostScript template
    stx template_pointer
    sty template_pointer+1

    ; Get current field and check that it's within bounds
    lda curfield
    cmp #8
    bcc :+

    ; Too many fields
    lda #<template_error
    sta message
    lda #>template_error
    sta message+1
    sec
    rts

    ; Jump to field handler
:   asl
    tax
    jmp (jmptbl,x)

jmptbl:
    .word field_ph          ; 0
    .word field_pw          ; 1
    .word field_mu          ; 2
    .word field_pm          ; 3
    .word field_fs          ; 4
    .word field_lh          ; 5
    .word field_encoding    ; 6
    .word field_glyphs      ; 7

field_ph:
    ; Page height
    ldx #<ph_a4
    ldy #>ph_a4
    lda OPTION_PAGE_SIZE
    cmp #17 ; A4
    beq :+
    ldx #<ph_letter
    ldy #>ph_letter
:   lda #0
    jsr uart_write_string
    jmp field_exit

ph_a4: 
    .byt "842", 0
ph_letter:
    .byt "792", 0

field_pw:
    ; Page width
    ldx #<pw_a4
    ldy #>pw_a4
    lda OPTION_PAGE_SIZE
    cmp #17 ; A4
    beq :+
    ldx #<pw_letter
    ldy #>pw_letter
:   lda #0
    jsr uart_write_string
    jmp field_exit

pw_a4: 
    .byt "595", 0
pw_letter:
    .byt "612", 0

field_mu:
    ; Measurment unit
    ldx #<mu_mm
    ldy #>mu_mm
    lda OPTION_UNIT
    cmp #19 ; Metric (mm)
    beq :+
    ldx #<mu_in
    ldy #>mu_in
:   lda #0
    jsr uart_write_string
    jmp field_exit

mu_mm: 
    .byt "25.4", 0
mu_in:
    .byt "32", 0

field_pm:
    ; Page margin
    lda OPTION_MARGINS
    jsr uart_write_int
    jmp field_exit

field_fs:
    ; Font size
    lda OPTION_FONT_SIZE
    jsr uart_write_int
    jmp field_exit

field_lh:
    ; Line height/spacing
    sec
    lda OPTION_SPACING
    sbc options_min+OPTION_SPACING-options_value
    tay

    cpy #2
    beq @2 ; Double line spacing

    lda #'1'
    jsr uart_write_byte
    bcc :+
    jmp err
    
:   cpy #1
    bne @3

    lda #'.'
    jsr uart_write_byte
    bcc :+
    jmp err
:   lda #'5'
    jsr uart_write_byte
    bra @3
    
@2: lda #'2'
    jsr uart_write_byte

@3: jmp field_exit

field_encoding:
    ; Custom character encoding
    lda charset
    beq @2
    cmp #1
    beq @3

@1: 
    ldx #<ps_encoding_pet_lcase
    ldy #>ps_encoding_pet_lcase
    bra @4

@2:
    ldx #<ps_encoding_latin9
    ldy #>ps_encoding_latin9
    bra @4

@3:
    ldx #<ps_encoding_pet_ucase
    ldy #>ps_encoding_pet_ucase

@4: lda #0
    jsr uart_write_string
    jmp field_exit

field_glyphs:
    ; Custom font glyphs
    lda charset
    beq @2

    ; Glyphs common to PETSCII upper and lower case
    ldx #<ps_glyphs_pet
    ldy #>ps_glyphs_pet
    lda #0
    jsr uart_write_string
    bcc :+
    jmp err

:   lda charset
    cmp #1
    beq @1

    ; PETSCII lower case glyphs
    ldx #<ps_glyphs_pet_lcase
    ldy #>ps_glyphs_pet_lcase
    bra @3

@1:
    ; PETSCII upper case glyphs
    ldx #<ps_glyphs_pet_ucase
    ldy #>ps_glyphs_pet_ucase
    bra @3

@2:
    ; ISO mode glyphs
    ldx #<ps_glyphs_latin9
    ldy #>ps_glyphs_latin9

@3: lda #0
    jsr uart_write_string

field_exit:
    bcc :+
    jmp err

:   inc curfield
    ldx template_pointer
    ldy template_pointer+1
    inx
    bne :+
    iny

:   jmp loop

template_pointer:
    .res 2

curfield:
    .res 1

; Resources

at_command:
    .byt "at+printr:",0

ps_template:
    .incbin "template.ps"
    .byt 0

ps_encoding_latin9:
    .incbin "encoding_latin9.ps"
    .byt 0

ps_encoding_pet_ucase:
    .incbin "encoding_petscii_ucase.ps"
    .byt 0

ps_encoding_pet_lcase:
    .incbin "encoding_petscii_lcase.ps"
    .byt 0

ps_glyphs_latin9:
    .incbin "glyphs_latin9.ps"
    .byt 0

ps_glyphs_pet:
    .incbin "glyphs_petscii.ps"
    .byt 0

ps_glyphs_pet_ucase:
    .incbin "glyphs_petscii_ucase.ps"
    .byt 0

ps_glyphs_pet_lcase:
    .incbin "glyphs_petscii_lcase.ps"
    .byt 0
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

    ; Store input
    sta lastchar

    ; Check if LF
    cmp #LF
    beq linebreak
    cmp #CR
    beq linebreak

    ; Check if start of new line
    ldx is_new_line
    bne high_nibble ; Not a new line, continue
    lda #'<' ; A new line, output start bracket
    jsr uart_write_byte
    bcs err

high_nibble:
    lda #1
    sta is_new_line
    
    lda lastchar
    lsr
    lsr
    lsr
    lsr
    jsr sendhexchar

low_nibble:
    lda lastchar
    and #$0f
    jsr sendhexchar

exit:
    ; Restore bank and registers
    pla
    sta ROM_SEL
    ply
    plx
    clc
    rts

linebreak:
    lda is_new_line
    beq :+
    ldx #<print
    ldy #>print
    lda #0
    jsr uart_write_string
    bcs err
    
:   stz is_new_line
    ldx #<br
    ldy #>br
    lda #0
    jsr uart_write_string
    bcs err
    bra exit

sendhexchar:
    ; Input: HEX nibble value in A
    clc
    adc #$30
    cmp #$3a
    bcc :+
    adc #$06
:   jsr uart_write_byte
    bcs :+
    rts

:   ; Eat return address
    pla
    pla
    ; Fallthrough to err

err:
    ; Restore registers
    pla
    sta ROM_SEL
    ply
    plx
    sec
    rts

br:
    .byt 98, 114, 10, 0 ; "br" + LF

print:
    .byt 62, 32, 112, 114, 10, 0 ; "> pr" + LF

lastchar:
    .res 1
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

    ; Send showpage command for last page
    ldx #<showpage
    ldy #>showpage
    lda #0
    jsr uart_write_string

    ; Set message
    lda #<transmission_ended
    sta message
    lda #>transmission_ended
    sta message+1

    ; Restore ROM bank 
    pla
    sta ROM_SEL
    rts

showpage:
    .byt 10, 115, 104, 111, 119, 112, 97, 103, 101, 10, 0 ; "showpage" + LF
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
    lda #$60
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
    .byt "@//:x16editpd-postscript.drv"
fn_end:

ibsout_backup:
    .res 2

save_err:
    .byt "saving default options failed", 0
.endproc

.include "util.inc"
.include "uart.inc"

;******************************************************************************
; Variables
charset: .res 1
message: .word null_message
is_new_line: .res 1

; *****************************************************************************
; Printing options

OPTION_COUNT = 8
OPTION_IOADDR = options_value+0
OPTION_BAUD = options_value+1
OPTION_PAGE_SIZE = options_value+2
OPTION_UNIT = options_value+3
OPTION_MARGINS = options_value+4
OPTION_FONT_SIZE = options_value+5
OPTION_SPACING = options_value+6
OPTION_PATH = options_value+7

options_label:
    .byt "i/o port....:", 13
    .byt "baud rate...:", 13
    .byt "paper size..:", 13
    .byt "units.......:", 13
    .byt "margins.....:", 13
    .byt "font size...:", 13
    .byt "line spacing:", 13
    .byt "ipp address.:", 0

options_type:
    .byt 2, 2, 2, 2, 0, 0, 2, 3

options_value:
    .byt 0, 15, 17, 19, 30, 12, 21

options_min:
    .byt 0, 10, 17, 19, 0, 1, 21

options_max:
    .byt 9, 16, 18, 20, 255, 255, 23

path:
    .byt 0
    .res 255

options_string:
    .word str_io3_low       ; 0
    .word str_io3_high      ; 1
    .word str_io4_low       ; 2
    .word str_io4_high      ; 3
    .word str_io5_low       ; 4
    .word str_io5_high      ; 5
    .word str_io6_low       ; 6
    .word str_io6_high      ; 7
    .word str_io7_low       ; 8
    .word str_io7_high      ; 9

    .word str_baud_2_4      ; 10
    .word str_baud_9_6      ; 11
    .word str_baud_19_2     ; 12
    .word str_baud_28_8     ; 13
    .word str_baud_57_6     ; 14
    .word str_baud_115_2    ; 15
    .word str_baud_921_6    ; 16

    .word str_a4            ; 17
    .word str_letter        ; 18

    .word str_mm            ; 19
    .word str_inch          ; 20

    .word str_spacing_1     ; 21
    .word str_spacing_1_5   ; 22
    .word str_spacing_2     ; 23

str_io3_low:    .byt "io3 low", 0
str_io3_high:   .byt "io3 high", 0
str_io4_low:    .byt "io4 low", 0
str_io4_high:   .byt "io4 high", 0
str_io5_low:    .byt "io5 low", 0
str_io5_high:   .byt "io5 high", 0
str_io6_low:    .byt "io6 low", 0
str_io6_high:   .byt "io6 high", 0
str_io7_low:    .byt "io7 low", 0
str_io7_high:   .byt "io7 high", 0

str_baud_2_4:   .byt "2.4 kbaud",0
str_baud_9_6:   .byt "9.6 kbaud",0
str_baud_19_2:  .byt "19.2 kbaud",0
str_baud_28_8:  .byt "28.8 kbaud",0
str_baud_57_6:  .byt "57.6 kbaud",0
str_baud_115_2: .byt "115.2 kbaud",0
str_baud_921_6: .byt "921.6 kbaud",0

str_a4:         .byt "a4",0
str_letter:     .byt "letter",0

str_mm:         .byt "millimeter", 0
str_inch:       .byt "1/32 inch", 0

str_spacing_1:  .byt "single", 0
str_spacing_1_5:.byt "1 1/2", 0
str_spacing_2:  .byt "double", 0

; *****************************************************************************
; Common messages
null_message:
    .byt 0

option_not_exists:
    .byt "option does not exist", 0

invalid_value:
    .byt "invalid value", 0

template_error:
    .byt "template error", 0

uart_port_error:
    .byt "uart error-check i/o port",0

uart_config_error:
    .byt "uart error-check configuration", 0

uart_comm_error:
    .byt "uart communication error", 0

transmission_ended:
    .byt "document sent to printer", 0
