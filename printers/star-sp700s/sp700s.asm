;*******************************************************************************
;Copyright 2026, Jason Hill, Stefan Jakobsson
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


;*******************************************************************************
; Notes:
;   This drivers is for a serially connect SP700/SP712/SP724 receipt printer,
;   including the SP700R series.
;
;   Ensure the following printer setting are correct via the on-board DIP
;   switches.
;     Data bits: 8
;     Parity: None
;     Stop bits: 1
;     Handshake: DTR
;     Baud: 38400 (optional - for best results)
;
;   3-inch wide paper required
;
;*******************************************************************************

; Option types
TYPE_INT8 = 0
TYPE_INT16 = 1
TYPE_LIST = 2
TYPE_STRING = 3

; Misc definitions
uart_base_addr = $9f60
LF = 10
CR = 13

; VERA registers (Commander X16)
VERA_ADDR_L = $9F20
VERA_ADDR_M = $9F21
VERA_ADDR_H = $9F22     ; bits 7:4 = auto-increment step, bit 0 = address bit 16
;VERA_DATA0  = $9F23	; Not needed in this configuration, using DATA1
VERA_DATA1  = $9F24		; 
VERA_CTRL   = $9F25     ; bit 0 = ADDRSEL (0=ADDR0, 1=ADDR1 for $9F20-$9F22)

; Charsets in VRAM (17-bit addresses, bit 16 = 1):
;   Upper/Graphics (ISO / PETSCII upper): $1F000, ADDR_M base = $F0
;   Upper/Lower   (PETSCII lowercase):    $1F800, ADDR_M base = $F8
; ROM defines 128 glyphs; chars $80-$FF are inverse-video fill (2048 bytes total)
CHARSET_BASE_M = $F0
CHARSET_BASE_H = $11    ; step=1, bit16=1

; Zero page variables (4 bytes)
VARZP = $32

.segment "JMPTBL"
;*******************************************************************************
; Driver jump table (16kb available for driver)
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

:   ; Check if we're incrementing or decrementing
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

	; Calculate UART offset and add to base addres
	lda OPTION_UART
	sec
	sbc options_min+1
	beq :+				;If zero, no offset
	lda #8
  : clc
	adc base_offset
	sta base_offset
	
	; Minimally test if UART exists, exit with error if not
	ldx base_offset
	lda #$5a
    sta uart_base_addr+7,x
	lda uart_base_addr+7,x
	cmp #$5a
	beq :+
		lda #<printing_error
		sta message
		lda #>printing_error
		sta message+1
		sec
	rts
	
    ; Set BAUD rate
  : ldx base_offset
    lda #$80            ; DLAB=1
    sta uart_base_addr+3,x

    sec
    lda OPTION_BAUDRATE
    sbc options_min+2
    tay
    lda baud_rates,y
    sta uart_base_addr,x
    lda #0
    sta uart_base_addr+1,x

    lda #$03            ; DLAB=0
    sta uart_base_addr+3,x
	
	;Enable FIFO, allows up to 16-bytes to be sent without checking overflow
    lda #$03
	sta uart_base_addr+2,x	; FCR
	
	;Use IBM Charset #2 for build-in printing (not strictly necessary with bit images)
    lda #$1b
    sta uart_base_addr,x
    lda #$36
    sta uart_base_addr,x
	
	; Set line spacing to 8 dots (ESC 3 24 = 24/216 inch = 8 pins at 72 DPI)
    ; This allows things to line up nicely on new lines for PETSCII art.
    lda #$1b
    sta uart_base_addr,x
    lda #$33
    sta uart_base_addr,x
    lda #24
    sta uart_base_addr,x
	
	;Two color printing selection
	lda OPTION_RBN
	sec
	sbc options_min+4	;Results to 1 if black 0 if red
	clc
	adc #52				;Default red value, add to 1 if black
    tay
    lda #$1b
	sta uart_base_addr,x
	tya
    sta uart_base_addr,x	

	;Determine word-wrapping size (from font size)
	;set to zero if using built-in printer font
	lda OPTION_SIZE
	sec
	sbc options_min+5	;Results for size selection
	clc
	;Accumulator now holds font size indicator
	;0=small(53), 1=large(27), 2=printe built-in
	;calculate line-wrap required based on font size
	beq sml_font
	
	cmp #$01
	beq lrg_font
	
	;using built-in printer font
	lda #0
	bra channel_open_exit
  
  lrg_font:	lda #27
			bra channel_open_exit
  sml_font: lda #53

  
 channel_open_exit:
	sta font_size
    ; Exit
    clc
    rts

baud_rates:
    .byt 48, 24, 16 ; 57600,38400,19200
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
	;Preserve Y and X before proceeding
    phy
    phx
	
	; determine if using built-in printer font to skip all
	; transposing logic and line-wrap logic.
	pha
	lda font_size ;if font_size=0 use built-in printer font
	bne :++
	
	;Check printer and UART are ready to send/receive
    ldx base_offset
  : lda uart_base_addr+6,x	;get MSR register
    and #$30            	;check CTS/DSR value (bits 4,5)
    beq :-					;non-zero is good, zero is bad
	
	lda uart_base_addr+5,x
    and #$40            	; wait for TEMT before auto-LF
    beq :-					;non-zero is good, zero is bad

    pla
    sta uart_base_addr,x
	jmp exit
	;----jump to exit from here ----------------------------
	
	;If here, we are using bit image fonts and line-wrapping
  :	pla
    cmp #$20            ; below space = control code, send raw
    bcc send_raw

    jsr transpose_8x8
    inc col_count
    lda col_count
	
	;Check font size
    cmp font_size
    bcc exit            ; under limit, no wrap needed
    stz col_count
    
    ;Check printer and UART are ready to send/receive
    ldx base_offset
  : lda uart_base_addr+6,x	;get MSR register
    and #$30            	;check CTS/DSR value (bits 4,5)
    beq :-					;non-zero is good, zero is bad
	
	lda uart_base_addr+5,x
    and #$40            	; wait for TEMT before auto-LF
    beq :-					;non-zero is good, zero is bad
    
	lda #LF
    sta uart_base_addr,x
    bra exit
    
send_raw:
    pha

    ;Check printer and UART are ready to send/receive
    ldx base_offset
  : lda uart_base_addr+6,x	;get MSR register
    and #$30            	;check CTS/DSR value (bits 4,5)
    beq :-					;non-zero is good, zero is bad
	
	lda uart_base_addr+5,x
    and #$40            	; wait for TEMT before auto-LF
    beq :-					;non-zero is good, zero is bad
    
    pla
    sta uart_base_addr,x
    cmp #LF
    beq :+
    cmp #CR
    bne exit
  : stz col_count
exit:
    plx
    ply
    lda #0
    clc
    rts
.endproc

;******************************************************************************
;Function name.......: uart_send
;Purpose.............: Bursts the complete ESC K bit image command to the
;                      UART: ESC K $08 $00 followed by all 8 bytes from
;                      rotbuf. 12 bytes total — fits within the 16-byte TX
;                      FIFO. Caller must ensure transmitter is empty first.
;Input...............: Nothing (reads rotbuf directly)
;Returns.............: Nothing
;Affected registers..: A, X, Y
.proc uart_send
    phx
    ldx base_offset
    ; ESC K n NUL header
    lda #$1B
    sta uart_base_addr,x

    ;Configure for large or small font in bit image command
	lda font_size
	cmp #27
	beq :+
	lda #$4C	;Small Font
	bra :++
  : lda #$4B	;Big Font
  
  : sta uart_base_addr,x
    lda #8
    sta uart_base_addr,x
    lda #0
    sta uart_base_addr,x
    
    ; Burst all 8 rotated column bytes
    ldy #0
  : lda rotbuf,y
    sta uart_base_addr,x
    iny
    cpy #8
    bne :-
    plx
    rts
.endproc

;******************************************************************************
;Function name.......: transpose_8x8
;Purpose.............: Reads 8-byte character glyph from VRAM, rotates it
;                      90 degrees, then sends it to the printer as a bit image
;                      using: ESC K n NUL d1 d2 .. dn  (n=8 columns)
;Input...............: A = character code
;Returns.............: Nothing
;Affected registers..: A, X, Y
;Notes...............: VERA address port 1 (DATA1) is used so port 0 is free
;                      for display DMA. VERA_CTRL and ADDR1 are saved and
;                      restored. Rotation: each output byte represents one
;                      vertical column (MSB=top pin, LSB=bottom pin).
;                      rotbuf[0] = leftmost column, rotbuf[7] = rightmost.
.proc transpose_8x8
    ; Translate ASCII char code to PETSCII font position.
    ; charset=0: ISO, no change
    ;		charset=1 PETSCII upper
    ;		charset=2 PETSCII upper/lower
    ;
    ; PETSCII upper rules ($20-$7E):
    ;   $20-$3F → identity  (space, digits, punctuation same)
    ;   $40-$5F → sub $40  (@→$00, A-Z→$01-$1A, [\]^_→$1B-$1F)
    ;   $60-$7E → sub $60  (a-z→$01-$1A, {|}~→$1B-$1E)
    ;
    ; PETSCII lower rules:
    ;   $20-$3F → identity  (space, digits, punctuation)
    ;   $40-$5A → sub $40   (@→$00, a-z→$01-$1A  — lowercase letters live here)
    ;   $61-$7A → sub $20   (A-Z→$41-$5A — uppercase letters live here)
    ;   $C1-$DA → sub $80   (A-Z→$41-$5A — alternate uppercase codes)
    ;
    ; Now I'm dizzy....
    
    sta VARZP
    lda charset
   
    cmp #1
    beq xlat_upper          ; charset=1: PETSCII upper
    cmp #2
    beq xlat_lower          ; charset=2: PETSCII lower

	;If not PETSCII, branch
    bra xlat_done           ; other: no translation
xlat_lower:
    lda #$F4                ; default: use Lower font for charset=2
    sta temp
    lda VARZP
    cmp #$C1
    bcs xlat_lower_altuc    ; $C1-$DA: SHIFT graphics → sub $80, UG font
    cmp #$A0
    bcs xlat_lower_cbm      ; $A0-$BF: CBM graphics → sub $40, UG font
    cmp #$61
    bcs xlat_lower_uc       ; $61-$7A: uppercase A-Z → sub $20, UL font
    cmp #$40
    bcs xlat_lower_lc       ; $40-$5A: @ and lowercase a-z → sub $40, UL font
    bra xlat_done           ; $20-$3F: identity
xlat_lower_altuc:
    lda #$F0                ; graphics live in Upper/Graphics font, not Lower font
    sta temp
    lda VARZP
    sec
    sbc #$80                ; $C1-$DA → UG $41-$5A (SHIFT graphics)
    sta VARZP
    bra xlat_done
xlat_lower_cbm:
    lda #$F0                ; CBM graphics also in Upper/Graphics font
    sta temp
    lda VARZP
    sec
    sbc #$40                ; $A0-$BF → UG $60-$7F (CBM graphics)
    sta VARZP
    bra xlat_done
xlat_lower_uc:
    sec
    sbc #$20                ; $61-$7A → $41-$5A (uppercase A-Z in Lower font)
    sta VARZP
    bra xlat_done
xlat_lower_lc:
    sec
    sbc #$40                ; $40-$5A → $00-$1A (@ and lowercase a-z in Lower font)
    sta VARZP
    bra xlat_done
xlat_upper:
    lda VARZP
    cmp #$C0
    bcs xlat_upper_shift    ; $C0-$DF: SHIFT+letter → sub $80 → UG $40-$5F
    cmp #$A0
    bcs xlat_upper_alt      ; $A0-$BF: CBM+letter → sub $40 → UG $60-$7F
    cmp #$60
    bcs xlat_sub60          ; $60-$9F: sub $60
    cmp #$40
    bcc xlat_done           ; $20-$3F: identity
    sec
    sbc #$40                ; $40-$5F: letters/@ → UG $00-$1F
    sta VARZP
    bra xlat_done
xlat_sub60:
    sec
    sbc #$60                ; $60-$9F → UG $00-$3F
    sta VARZP
    bra xlat_done
xlat_upper_alt:
    sec
    sbc #$40                ; $A0-$BF → UG $60-$7F (CBM graphics)
    sta VARZP
    bra xlat_done
xlat_upper_shift:
    sec
    sbc #$80                ; $C0-$DF → UG $40-$5F (SHIFT graphics)
    sta VARZP
xlat_done:
    ; Compute full VRAM address before the TEMT wait so an IRQ during the
    ; wait cannot corrupt VARZP+1 after it has been zeroed
    lda #0
    sta VARZP+1
    asl VARZP               ; VARZP:VARZP+1 = font_pos * 8  (16-bit, 3 shifts)
    rol VARZP+1
    asl VARZP
    rol VARZP+1
    asl VARZP
    rol VARZP+1

    ; Wait for transmitter empty (TEMT, LSR bit 6) — address already computed,
    ; rotation work will overlap with any remaining previous transmission
    phx
    ldx base_offset
  : lda uart_base_addr+6,x	;get MSR register
    and #$30            	;check CTS/DSR value (bits 4,5)
    beq :-					;non-zero is good, zero is bad
	
	lda uart_base_addr+5,x
    and #$40            	; wait for TEMT before auto-LF
    beq :-					;non-zero is good, zero is bad
    plx

    ; Save VERA_CTRL, then select ADDR1 (ADDRSEL=1) so $9F20-$9F22 expose
    ; port 1's address registers without disturbing port 0
    lda VERA_CTRL
    sta vera_save_ctrl
    ora #$01
    sta VERA_CTRL

    ; Save current ADDR1 registers
    lda VERA_ADDR_L
    sta vera_save_l
    lda VERA_ADDR_M
    sta vera_save_m
    lda VERA_ADDR_H
    sta vera_save_h

    ; Program ADDR1 using precomputed VARZP:VARZP+1
    lda VARZP
    sta VERA_ADDR_L
    lda #$F0                ; default: Upper/Graphics font ($1F000)
    ldx charset
    cpx #2
    bne set_addr_m          ; charset 0/1: always Upper/Graphics
    lda temp                ; charset=2: $F4 (Lower font) or $F0 (graphics)
set_addr_m:
    clc
    adc VARZP+1
    sta VERA_ADDR_M
    lda #CHARSET_BASE_H     ; step=1 auto-increment, bit16=1
    sta VERA_ADDR_H

    ; Read 8 character rows from VRAM via DATA1
    ldy #0
  : lda VERA_DATA1
    sta charbuf,y
    iny
    cpy #8
    bne :-

    ; UL charset ($F4) is stored with inverted bit polarity; correct it now.
    ; Only applies to charset=2 letters (temp=$F4); graphics use UG ($F0) and are fine.
    ldx charset
    cpx #2
    bne vera_restore
    lda temp
    cmp #$F4
    bne vera_restore
    ldy #7
invert_ul:
    lda charbuf,y
    eor #$FF
    sta charbuf,y
    dey
    bpl invert_ul

vera_restore:
    ; Restore ADDR1 registers, then restore VERA_CTRL (restores ADDRSEL)
    lda vera_save_l
    sta VERA_ADDR_L
    lda vera_save_m
    sta VERA_ADDR_M
    lda vera_save_h
    sta VERA_ADDR_H
    lda vera_save_ctrl
    sta VERA_CTRL

    ; Zero the rotation output buffer
    lda #0
    ldy #7
  : sta rotbuf,y
    dey
    bpl :-

    ; --- 90-degree rotation ---
    ; For each row r (0..7):
    ;   ASL the row byte to shift bits out MSB-first (leftmost pixel first).
    ;   ROL rotbuf[c] shifts rotbuf[c] left and catches carry in bit 0.
    ; After 8 rows, rotbuf[c] = {p(0,c), p(1,c), .., p(7,c)} in bits 7..0,
    ; i.e. column c of the glyph read top-to-bottom — exactly what the
    ; printer expects (MSB = top pin, LSB = bottom pin).
    ldy #0
row_loop:
    lda charbuf,y
    ldx #0
col_loop:
    asl                     ; bit 7 (next column's pixel) -> carry
    rol rotbuf,x            ; carry -> bit 0 of rotbuf[x], rotbuf[x] <<= 1
    inx
    cpx #8
    bne col_loop
    iny
    cpy #8
    bne row_loop

    ; Send ESC K bit image command: header + 8 column bytes in one FIFO burst
    jsr uart_send

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

    ; On print complete action
	ldy #14					; # of line feeds to get last printed line to tear or cut line
	lda #LF
  : sta uart_base_addr,x
	dey
	bne :-
	
    lda OPTION_ONCOMPLETE
    cmp options_min+2
    bne :+

    ; Cut paper at current position
    ; Full cut at the current position
    lda #$1b
    sta uart_base_addr,x
    lda #64
    sta uart_base_addr,x
    lda #0
    sta uart_base_addr,x

  : ; Exit
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
base_offset: .res 1			;
charset: .res 1				; stores current charset
quotes: .res 1				;
temp: .res 1				; Temporary storage for transpose 8x8
message: .word null_message		;
charbuf:    .res 8          		; raw 8-row glyph bytes from VRAM
rotbuf:     .res 8          		; 90-degree rotated column bytes for printer
vera_save_ctrl: .res 1      		; saved VERA_CTRL (preserves ADDRSEL and DCSEL)
vera_save_l: .res 1         		; saved VERA ADDR1 registers
vera_save_m: .res 1			; ...
vera_save_h: .res 1			; ...
col_count: .res 1			; Column count value for auto line-wrapping
font_size: .res 1			; Font size for word wrapping [27, or 53]
; *****************************************************************************
; Printing options

OPTION_COUNT = 6
OPTION_IOADDRESS = options_value+0
OPTION_UART = options_value+1
OPTION_BAUDRATE = options_value+2
OPTION_ONCOMPLETE = options_value+3
OPTION_RBN = options_value+4
OPTION_SIZE = options_value+5

options_label:
    .byt "io address.:", 13
	.byt "uart selection:", 13
    .byt "baud rate..:", 13
    .byt "on complete:", 13
	.byt "ribbon:", 13
	.byt "size:", 0

options_type:
    .byt TYPE_LIST, TYPE_LIST, TYPE_LIST, TYPE_LIST, TYPE_LIST, TYPE_LIST

options_value:
	.byt 8, 11, 13, 16, 18, 19

options_min:
    .byt 0, 10, 12, 15, 17, 19

options_max:
    .byt 9, 11, 14, 16, 18, 21

options_string:
    .word str_io3_low       ; List index 0 $9f60,$9f68
    .word str_io3_high      ; List index 1 $9f70,$9f78
    .word str_io4_low       ; List index 2 $9f80,$9f88
    .word str_io4_high      ; List index 3 $9f90,$9f98
    .word str_io5_low       ; List index 4 $9fa0,$9fa8
    .word str_io5_high      ; List index 5 $9fb0,$9fb8
    .word str_io6_low       ; List index 6 $9fc0,$9fc8
    .word str_io6_high      ; List index 7 $9fd0,$9fd8
    .word str_io7_low       ; List index 8 $9fe0,$9fe8
    .word str_io7_high      ; List index 9 $9ff0,$9ff8

    .word str_uart0			; List index 10
	.word str_uart1			; List index 11

    .word str_baud_19200    ; List index 12    
    .word str_baud_38400    ; List index 13
    .word str_baud_57600    ; List index 14

    .word str_cut           ; List index 15
    .word str_noaction      ; List index 16
	
	.word str_rb1			; List index 17
	.word str_rb2			; List index 18
	
	.word str_sz_sml		; List index 19
	.word str_sz_lrg		; List index 20
	.word str_sz_raw		; List index 21

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

str_uart0:
    .byt "uart0 (zimodem)", 0
str_uart1:
    .byt "uart1 (rs-232)", 0
	
str_baud_19200:
    .byt "19.2 kbaud", 0
str_baud_38400:
    .byt "38.4 kbaud", 0
str_baud_57600:
    .byt "57.6 kbaud", 0

str_cut:
    .byt "cut paper", 0
str_noaction:
    .byt "no action", 0

str_rb1:
    .byt "color (red)", 0
str_rb2:
    .byt "black", 0

str_sz_sml:
    .byt "small - 52 columns", 0
str_sz_lrg:
    .byt "large - 26 columns", 0
str_sz_raw:
    .byt "use printer built-in font", 0

; *****************************************************************************
; Common messages
null_message:
    .byt 0

option_not_exists:
    .byt "option does not exist", 0

invalid_charset:
    .byt "only iso/cp347 supported", 0

printing_error:
    .byt "printer communication error", 0
