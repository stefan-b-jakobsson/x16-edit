;******************************************************************************
;Function name.......: prompt_init
;Purpose.............: Initializes input prompt and displays message
;Input...............: Pointer to message (X=LSB, Y=MSB)
;                      A = input max length
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_init
    ;Set max len
    sta prompt_maxlen

    ;Backup cursor pos
    lda CRS_X
    sta prompt_cursor_backup
    sta $5200
    lda CRS_Y
    sta prompt_cursor_backup+1
    sta $5201
    
    ;Print message
    stz VERA_L
    lda #57
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    stx TMP1_ADR
    sty TMP1_ADR+1
    ldy #0

:   lda (TMP1_ADR),y
    beq set_bgcolor
    sta VERA_D0
    iny
    jmp :-
    
set_bgcolor:
    iny
    sty prompt_start

    ;Set background color
    lda #1
    sta VERA_L
    
    ldx #80
    lda #160

:   sta VERA_D0
    dex
    bne :-

    ;Set start values
    stz prompt_curpos
    stz prompt_fvc
    stz prompt_len

    ;Move cursor
    ldx prompt_start
    ldy #57
    jmp cursor_move

.endproc

;******************************************************************************
;Function name.......: prompt_refresh
;Purpose.............: Refreshes user input
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_refresh
    ;Prepare to print
    lda prompt_start
    asl
    sta VERA_L
    lda #57
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ldy prompt_fvc
    ldx prompt_start

:   cpy prompt_len
    bcs clear_line
    lda prompt_input,y
    sta VERA_D0
    iny
    inx
    cpx #SCREEN_WIDTH
    bne :-

    rts

clear_line:
    lda #32
:   sta VERA_D0
    inx
    cpx #SCREEN_WIDTH
    bne :-

    rts
.endproc

;******************************************************************************
;Function name.......: prompt_close
;Purpose.............: Close and clean up prompt when done
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_close
    ;Clear status line
    jsr screen_clear_status

    ;Restore cursor pos
    ldx prompt_cursor_backup
    ldy prompt_cursor_backup+1
    jsr cursor_move

    rts
.endproc

;******************************************************************************
;Function name.......: prompt_keypress
;Purpose.............: Handle keypess in prompt
;Input...............: A=ascii value
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_keypress
    ;Delete key
    cmp #$14
    beq delete

    ;Left arrow key
    cmp #$9d
    beq left_arrow

    ;Right arrow key
    cmp #$1d
    beq right_arrow

    ;Home key
    cmp #$13
    beq home

    ;Shift+Home = End key
    cmp #$93
    beq end

    ;Other
    cmp #$20
    bcc exit

    jmp prompt_insert

delete:
    jmp prompt_delete

left_arrow:
    jsr prompt_go_left
    jmp prompt_refresh

right_arrow:
    jsr prompt_go_right
    jmp prompt_refresh

home:
    rts

end:
    rts

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: prompt_insert
;Purpose.............: Inserts one char in prompt input string
;Input...............: A=char to insert
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_insert
    ldx prompt_len
    cpx prompt_maxlen
    bcs exit

    pha

    ldy #254

:   lda prompt_input,y
    sta prompt_input+1,y
    cpy prompt_curpos
    beq next
    dey
    jmp :-

next:
    pla
    sta prompt_input,y

    inc prompt_len

    jsr prompt_go_right  
    jmp prompt_refresh

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: prompt_delete
;Purpose.............: Deletes one char in prompt input string
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_delete
    ldy prompt_curpos
    beq exit

:   lda prompt_input,y
    sta prompt_input-1,y
    iny
    bne :-

    dec prompt_len
    jsr prompt_go_left
    
    jmp prompt_refresh

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: prompt_go_right
;Purpose.............: Moves current editing position one step right
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_go_right
    lda prompt_curpos
    cmp prompt_len
    bcs exit

    inc prompt_curpos
    
    lda CRS_X
    cmp #SCREEN_WIDTH-1
    beq scroll
    inc CRS_X
    ldy #57
    ldx CRS_X
    jmp cursor_move

scroll:
    inc prompt_fvc

exit:
    rts
.endproc

;******************************************************************************
;Function name.......: prompt_go_left
;Purpose.............: Moves current editing position one step left
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc prompt_go_left
    lda prompt_curpos
    beq exit

    dec prompt_curpos

    lda prompt_fvc
    beq :+
    dec prompt_fvc
    rts

:   ldx CRS_X
    dex
    ldy CRS_Y
    jsr cursor_move

exit:
    rts

.endproc


prompt_input:
    .res 256,1

prompt_start:
    .byt 0

prompt_fvc:
    .byt 0

prompt_curpos:
    .byt 0

prompt_len:
    .byt 0

prompt_maxlen:
    .byt 0

prompt_cursor_backup:
    .byt 0,0