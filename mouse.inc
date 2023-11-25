MOUSE_LEFT_BUTTON = 1
MOUSE_RIGHT_BUTTON = 2
MOUSE_MIDDLE_BUTTON = 4
MOUSE_X_OFFSET = 3
MOUSE_Y_OFFSET = 6

;**********************************************************
;Function name.......: mouse_init
;Description.........: Initializes mouse state
;Input...............: Nothing
;Error returns.......: Nothing
.proc mouse_init    
    ; Copy mouse custom pointer graphics to VERA
    lda #(VERA_POINTER_ADDR & $ff)
    sta VERA_L
    lda #((VERA_POINTER_ADDR >> 8) & $ff)
    sta VERA_M
    lda #(((VERA_POINTER_ADDR >> 16) & $ff) | %00010000)    ; With auto-increment +1
    sta VERA_H

    ldx #0
    ldy #8
loop1:
    lda sprite_black_mask,x
    sta black
    lda sprite_white_mask,x
    sta white

loop2:
    asl black
    bcc :+
    lda #16
    sta VERA_D0
    asl white
    bra next
    
:   asl white
    bcc :+
    lda #31
    sta VERA_D0
    bra next

:   stz VERA_D0

next:
    dey
    bne loop2

    ldy #8
:   stz VERA_D0
    dey
    bne :-

    inx
    cpx #13
    beq :+
    ldy #8
    bra loop1

:   ldx #48
:   stz VERA_D0
    dex
    bne :-

    ; Configure sprite #0
    lda #(VERA_SPRITE_ATTR & $ff)
    sta VERA_L
    lda #((VERA_SPRITE_ATTR >> 8) & $ff)
    sta VERA_M
    lda #(((VERA_SPRITE_ATTR >> 16) & $ff) | %00010000)    ; With auto-increment +1
    sta VERA_H
    ldx #0
:       lda sprite_attr,x
    sta VERA_D0
    inx
    cpx #8
    bne :-

    ; Enable mouse
    bridge_setaddr KERNAL_MOUSE_CONFIG
    ldx screen_width
    ldy screen_height
    lda #$ff
    bridge_call KERNAL_MOUSE_CONFIG

    ; Init state
    stz mouse_button_state
    stz mouse_last_pos
    stz mouse_last_pos+1
    lda #1

    rts

sprite_black_mask:
    .byt %00000000
    .byt %01111100
    .byt %00010000
    .byt %00010000
    .byt %00010000
    .byt %00010000
    .byt %00111000
    .byt %00010000
    .byt %00010000
    .byt %00010000
    .byt %00010000
    .byt %01111100
    .byt %00000000

sprite_white_mask:
    .byt %11111110
    .byt %10000010
    .byt %11101110
    .byt %00101000
    .byt %00101000
    .byt %00101000
    .byt %01000100
    .byt %00101000
    .byt %00101000
    .byt %00101000
    .byt %11101110
    .byt %10000010
    .byt %11111110

sprite_attr:
    ; Address VERA_POINTER_ADDR, Mode 8bpp, Height 16 px, Width 16 px
    .byt (VERA_POINTER_ADDR>>5) & $ff, ((VERA_POINTER_ADDR>>13) & $ff) | $80, 0, 0, 0, 0, 0, $50

black = tempvars
white = tempvars+1

.endproc

;**********************************************************
;Function name.......: mouse_get
;Description.........: Fetches mouse position and button
;                      status; intended to be called as
;                      part of the program main loop
;Input...............: Nothing
;Error returns.......: Nothing
.proc mouse_get
    ; Abort if APP_MOD != 0
    lda APP_MOD
    beq :+
    cmp #2 ; Mode=Status message
    beq :+
    rts

    ; Get mouse status
:   bridge_setaddr KERNAL_MOUSE_GET
    ldx #TMP1_BNK
    bridge_call KERNAL_MOUSE_GET

    ; Store wheel
    stx mouse_wheel

    ; Offset to center of mouse pointer
    pha

    clc
    lda TMP1_BNK
    adc #MOUSE_X_OFFSET
    sta TMP1_BNK
    lda TMP1_BNK+1
    adc #0
    sta TMP1_BNK+1

    clc
    lda TMP1_BNK+2
    adc #MOUSE_Y_OFFSET
    sta TMP1_BNK+2
    lda TMP1_BNK+3
    adc #0
    sta TMP1_BNK+3

    pla

    ; Divide mouse pointer X and Y coordinates by 8; translate from pixel to character
    ldx #3
:   lsr TMP1_BNK+1
    ror TMP1_BNK
    lsr TMP1_BNK+3
    ror TMP1_BNK+2
    dex
    bne :-

    ; Load mouse pointer X/Y coordinates
    ldx TMP1_BNK
    ldy TMP1_BNK+2

    ; Store values on stack
    phy
    phx
    pha

    ; Check button status
    lsr ; Left button state -> C
    bcs down

up:
    lda mouse_button_state
    and #MOUSE_LEFT_BUTTON
    bne update_state
    jsr mouse_on_left_release
    bra update_state

down:
    lda mouse_button_state
    and #MOUSE_LEFT_BUTTON
    bne :+
    jsr mouse_on_left_click
    stz selection_active
    bra update_state

:   cpx mouse_last_pos
    bne :+
    cpy mouse_last_pos+1
    beq update_state
:   jsr mouse_on_left_drag

update_state:
    pla
    plx
    ply
    sta mouse_button_state
    stx mouse_last_pos
    sty mouse_last_pos+1

wheel:
    lda mouse_wheel
    beq exit
    bmi wheel_up

:   jsr cmd_go_down
    dec mouse_wheel
    bne :-
    bra refresh

wheel_up:
    lda mouse_wheel
    eor #$ff
    inc
    sta mouse_wheel

:   jsr cmd_go_up
    dec mouse_wheel
    bne :-

refresh:
    jsr screen_refresh

exit:
    rts
.endproc

;**********************************************************
;Function name.......: mouse_on_left_click
;Description.........: Called when left button is clicked
;Input...............: X = Mouse pointer X coordinate
;                      Y = Mouse pointer Y coordinate
;Error returns.......: Nothing
.proc mouse_on_left_click
    ; Collapse any previous selection
    stz selection_active
    
    ; Place cursor
    jsr cursor_disable
    jsr mouse_place_cursor

    ; Refresh
    jsr screen_refresh
    jmp cursor_activate
.endproc

;**********************************************************
;Function name.......: mouse_on_left_release
;Description.........: Called when left button is released
;Input...............: X = Mouse pointer X coordinate
;                      Y = Mouse pointer Y coordinate
;Error returns.......: Nothing
.proc mouse_on_left_release
    rts
.endproc

;**********************************************************
;Function name.......: mouse_on_left_drag
;Description.........: Called when mouse pointer has moved 
;                      while the left button is down
;Input...............: X = Mouse pointer X coordinate
;                      Y = Mouse pointer Y coordinate
;Error returns.......: Nothing
.proc mouse_on_left_drag
    jsr cursor_disable
    ; Set selection active flag
    lda selection_active
    bne :+

    phx
    jsr selection_mark_origin
    plx

    ; Place cursor
:   jsr mouse_place_cursor

    ; Grow selection to cursor
    jsr selection_grow

refresh:
    jsr screen_refresh
    jmp cursor_activate
.endproc

;**********************************************************
;Function name.......: mouse_translate_x
;Description.........: Translates screen X coordinate to
;                      document column
;Input...............: X = Mouse pointer X coordinate
;Error returns.......: Nothing
.proc mouse_translate_x
    sec
    lda mem_cur_col
    sbc CRS_X
    sta tempvars
    lda mem_cur_col+1
    sbc #0
    sta tempvars+1
    lda mem_cur_col+2
    sbc #0
    sta tempvars+2

    clc
    txa
    adc tempvars
    tax
    lda tempvars+1
    adc #0
    tay
    lda tempvars+2
    adc #0
    rts
.endproc

;**********************************************************
;Function name.......: mouse_place_cursor
;Description.........: Moves the cursor to specified screen
;                      coordinates
;Input...............: X = column
;                      Y = row
;Error returns.......: Nothing
.proc mouse_place_cursor
    ; Store corrdinates on stack
    phx
    phy

    ; Current line?
    ply
    cpy CRS_Y
    beq :+

    ; No, goto line
    jsr mouse_translate_y
    jsr cmd_goto_line

    ; Goto column
:   plx
    jsr mouse_translate_x
    jmp cmd_goto_col
.endproc


;**********************************************************
;Function name.......: mouse_translate_y
;Description.........: Translates screen Y coordinate to
;                      document line
;Input...............: Y = Mouse pointer Y coordinate
;Error returns.......: Nothing
.proc mouse_translate_y
    sec
    lda mem_cur_line
    sbc CRS_Y
    sta tempvars
    lda mem_cur_line+1
    sbc #0
    sta tempvars+1
    lda mem_cur_line+2
    sbc #0
    sta tempvars+2

    clc
    tya
    adc tempvars
    tax
    lda tempvars+1
    adc #0
    tay
    lda tempvars+2
    adc #0
    rts
.endproc

.segment "VARS"
mouse_button_state: .res 1
mouse_last_pos: .res 2
mouse_wheel: .res 1
.CODE