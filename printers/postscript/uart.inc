UART_BASE = $9f60
UART_FCR = 2
UART_LCR = 3
UART_MCR = 4
UART_LSR = 5
UART_THR = 0
UART_RBR = 0
UART_MSR = 6
UART_SCRATCH = 7

;******************************************************************************
;Function name.......: uart_init
;Purpose.............: Initializes UART communication
;Input...............: Nothing
;Returns.............: Nothing
;Errors..............: C=1, error message in "message" buffer
;Affected registers..: All
.proc uart_init
    ; Set UART port offset
    sec
    lda OPTION_IOADDR
    sbc options_min+OPTION_IOADDR-options_value
    asl
    asl
    asl
    asl
    sta uart_offset
    tax

    ; UART port check
    lda #$55
    sta UART_BASE+UART_SCRATCH,x
    cmp UART_BASE+UART_SCRATCH,x
    beq :+

    ; Set error message
    lda #<uart_port_error
    sta message
    lda #>uart_port_error
    sta message+1
    sec
    rts

    ; DLAB=1
:   lda #$80
    sta UART_BASE+UART_LCR,x

    ; Set BAUD rate
    sec
    lda OPTION_BAUD
    sbc options_min+OPTION_BAUD-options_value
    tay
    
    lda baud_divisor,y
    sta UART_BASE,x
    
    lda #0
    cpy #0
    bne :+
    inc
:   sta UART_BASE+1,x

    ; DLAB=0 + 8 data bits, 1 stop bit, no parity
    lda #$03
    sta UART_BASE+UART_LCR,x

    ; Disable interrupts
    stz UART_BASE+1,x

    ; Enable and clear both transmitter and receiver FIFOs + 14 bytes trigger level
    lda #$07
    sta UART_BASE+UART_FCR,x

    lda #$03
    sta UART_BASE+UART_MCR,x

    ; Read to clear stale status
    lda UART_BASE+UART_LSR,x

    ; Wait for modem ready (CTS and DSR active)
:   lda UART_BASE+UART_MSR,x
    and #$30
    cmp #$30
    bne :-

    ; Ensure the FIFO input buffer is empty; read until we get a timeout
:   lda #1 ; Timeout approx 0.3 seconds
    jsr uart_get_response
    bcc :-

    ; Testing comms by sending the "AT" command and reading the response
    ldx #<at_command
    ldy #>at_command
    lda #0
    jsr uart_write_string

    lda #2 ; = timeout after approx 0.7 seconds
    jsr uart_get_response
    bcs timeout_err
    cmp #0
    bne comm_err
    clc ; OK
    rts

comm_err:
    ; This error happens if ZiModem responds with
    ; "error" to the AT command. This should not happen...
    lda #<uart_comm_error
    sta message
    lda #>uart_comm_error
    sta message+1
    sec
    rts

timeout_err:
    ; The timeout error usually happens if selecting the wrong BAUD rate
    ; It could possibly be caused by other misconfigurations as well
    lda #<uart_config_error
    sta message
    lda #>uart_config_error
    sta message+1
    sec
    rts

at_command:
    .byt "at",13,0
.endproc

;******************************************************************************
;Function name.......: uart_write_byte
;Purpose.............: Writes one byte to the UART
;Input...............: A = byte value
;Returns.............: Nothing
;Errors..............: C=1, error message in "message" buffer
;Affected registers..: None
.proc uart_write_byte
    ; Preserve registers
    phx
    pha

    ; Get port offset
    ldx uart_offset

    ; Setup timeout counter
    stz timer
    stz timer+1
    lda #224
    sta timer+2
    
wait:
    ; Increment timeout counter
    inc timer
    bne :+
    inc timer+1
    bne :+
    inc timer+2
    beq timeout

    ; Wait for TEMT = HIGH
:   lda UART_BASE+UART_LSR,x
    and #$40
    beq wait

    ; Wait for CTS and DSR = HIGH
    lda UART_BASE+UART_MSR,x
    and #$30
    cmp #$30
    bne wait

    ; Push data
    pla
    sta UART_BASE,x

    ; Restore registers
    plx

    ; Return
    clc
    rts

timeout:
    ; Set error message
    lda #<uart_config_error
    sta message
    lda #>uart_config_error
    sta message+1

    ; Restore registers
    pla
    plx

    ; Return
    sec
    rts

timer:
    .res 3
.endproc

;******************************************************************************
;Function name.......: uart_write_int
;Purpose.............: Converts a binary value to a decimal string that is
;                      written to the UART
;Input...............: A = binary value
;Returns.............: Nothing
;Errors..............: C=1, error message in "message" buffer
;Affected registers..: None
.proc uart_write_int
    ; Preserve registers
    phx
    pha

    ; Prepare
    sta tmp
    stz out
    stz out+1
    ldx #8
    sed

loop:
    asl tmp
    lda out
    adc out
    sta out
    lda out+1
    adc out+1
    sta out+1
    dex
    bne loop
    cld

    stz tmp

    ; Send digits
    lda out+1
    jsr send_digit
    lda out
    lsr
    lsr
    lsr
    lsr
    jsr send_digit
    lda out
    and #15
    jsr send_digit

    ; Restore registers and return
    pla
    plx
    clc
    rts

send_digit:
    cmp #0
    bne :+
    ldx tmp
    bne :+
    rts
:   inc tmp
    clc
    adc #'0'
    jsr uart_write_byte
    bcs err
    rts

err:
    pla
    plx
    sec

    tmp: .res 1
    out: .res 2
.endproc

;******************************************************************************
;Function name.......: uart_write_string
;Purpose.............: Writes a NULL terminated string to the UART. The
;                      function breaks on NULL or a possible other 
;                      char specified by the A register.
;Input...............: X/Y = string pointer
;                      A = break char
;Returns.............: X/Y = pointer to current position in string
;                      A = current char
;Errors..............: C=1, error message in "message" buffer
;Affected registers..: A, X, Y
.proc uart_write_string
    ; Store string pointer
    stx VARZP
    sty VARZP+1

    ; Store break char
    sta breakchar

loop:
    ; Get char from buffer
    lda (VARZP)
    beq exit
    cmp breakchar
    beq exit

    ; Write to UART
    jsr uart_write_byte
    bcs err

    ; Increment buffer pointer
    inc VARZP
    bne loop
    inc VARZP+1
    bra loop

exit:
    ldx VARZP
    ldy VARZP+1
    clc
    rts

err:
    ldx VARZP
    ldy VARZP+1
    rts

breakchar:
    .res 1
.endproc

;******************************************************************************
;Function name.......: uart_get_response
;Purpose.............: Reads and returns the modem's response to the last
;                      AT command.
;Input...............: A = timeout threshold, approx value x 0.34 s
;Returns.............: A = response code (0=OK, 1=Error)
;Errors..............: C=1, if timeout error, message stored in "message" buffer
;Affected registers..: A, X, Y
.proc uart_get_response
    ; Store timeout threshold
    sta threshold

    ; Setup timeout counter
    stz counter
    stz counter+1
    stz counter+2

    ; Get uart offset
    ldx uart_offset

    ldy #0
loop:
    ; Increment timeout counter
    inc counter
    bne :+
    inc counter+1
    bne :+
    inc counter+2
    
    ; Check if timeout
    lda counter+2
    cmp threshold
    bcc :+
    jmp err

:   ; Check data ready
    lda UART_BASE+UART_LSR,x
    and #$01
    beq loop

    ; Reset timout counter
    stz counter
    stz counter+1
    stz counter+2

    ; Get character
    lda UART_BASE+UART_RBR,x

    ; Line break?
    cmp #CR
    beq linebreak
    cmp #LF
    beq linebreak

    ; Ignore overflow?
    cpy #6
    bcs loop

    ; Else store input in buffer
    sta buffer,y
    iny
    bra loop

linebreak:
    cpy #5
    beq iserror

    cpy #2
    bne cont

isok:
    ldy #0
:   lda buffer,y
    cmp msg_ok,y
    bne cont
    iny
    cpy #2
    bcc :-
    ; Return OK
    lda #0
    clc
    rts

iserror:
    ldy #0
:   lda buffer,y
    cmp msg_error,y
    bne cont
    iny
    cpy #5
    bne :-
    ; Return error code
    lda #<uart_port_error
    sta message
    lda #>uart_port_error
    sta message+1
    lda #1
    clc
    rts

cont:
    ldy #0
    jmp loop

err:
    sec
    rts

counter:
    .res 3
threshold:
    .res 1
buffer:
    .res 6
msg_ok:
    .byt "ok"
msg_error:
    .byt "error"
.endproc

baud_divisor:
    .byt 128, 96, 48, 32, 16, 8, 1  ; LSB for 2.4, 9.6, 19.2, 28.8, 57.6, 115.2 and 921.6 kbaud

uart_offset:
    .byt 0
