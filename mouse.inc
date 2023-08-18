MOUSE_LEFT_BUTTON = 1
MOUSE_RIGHT_BUTTON = 2
MOUSE_MIDDLE_BUTTON = 4

;**********************************************************
;Function name.......: mouse_init
;Description.........: Initializes mouse state
;Input...............: Nothing
;Error returns.......: Nothing
.proc mouse_init
    stz mouse_button_state
    stz mouse_last_pos
    stz mouse_last_pos+1
    rts
.endproc

;**********************************************************
;Function name.......: mouse_get
;Description.........: Fetches mouse position and button
;                      status; intended to be called as
;                      part of the program main loop
;Input...............: Nothing
;Error returns.......: Nothing
.proc mouse_get
    ; Get mouse status
    ldx #TMP1_BNK
    jsr KERNAL_MOUSE_GET

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
    rts
.endproc

;**********************************************************
;Function name.......: mouse_on_left_click
;Description.........: Called when left button is clicked
;Input...............: X = Mouse pointer X coordinate
;                      Y = Mouse pointer Y coordinate
;Error returns.......: Nothing
.proc mouse_on_left_click
    ; Store corrdinates on stack
    phx
    phy

    ; Hide cursor to avoid tearing
    jsr cursor_disable

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
    jsr cmd_goto_col

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
    lda selection_active
    bne :+

    inc selection_active

    lda mem_cur_col
    sta selection_start_col
    lda mem_cur_col+1
    sta selection_start_col+1
    lda mem_cur_col+2
    sta selection_start_col+2

    lda mem_cur_line
    sta selection_start_line
    lda mem_cur_line+1
    sta selection_start_line+1
    lda mem_cur_line+2
    sta selection_start_line+2

:   jsr mouse_on_left_click
    
    lda mem_cur_col
    sta selection_end_col
    lda mem_cur_col+1
    sta selection_end_col+1
    lda mem_cur_col+2
    sta selection_end_col+2

    lda mem_cur_line
    sta selection_end_line
    lda mem_cur_line+1
    sta selection_end_line+1
    lda mem_cur_line+2
    sta selection_end_line+2

    rts
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
.CODE