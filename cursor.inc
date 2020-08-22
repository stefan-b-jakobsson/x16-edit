;******************************************************************************
;Copyright 2020, Stefan Jakobsson.
;
;This file is part of X16 Edit.
;
;X16 Edit is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.
;
;X16 Edit is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.
;
;You should have received a copy of the GNU General Public License
;along with X16 Edit.  If not, see <https://www.gnu.org/licenses/>.
;******************************************************************************


;******************************************************************************
;Function name.......: cursor_init
;Purpose.............: Initializes the cursor
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_init
    ;Prepare saving sprite data at VERA address $4000
    lda #$00
    sta VERA_L
    lda #$40
    sta VERA_M
    lda #(1<<4)
    sta VERA_H

    ;Save 6 blank lines, color=0 (6x8=48 bytes)
    lda #$00
    ldx #48
    :   sta VERA_D0
        dex
        bne :-
    
    ;Save 2 white lines, color=1 (2x8=16 bytes)
    lda #$01
    ldx #16
    :   sta VERA_D0
        dex
        bne :-
    
    ;Prepare sprite attributes setup (sprite #0)
    lda #$00
    sta VERA_L
    lda #$fc
    sta VERA_M
    lda #($01 | (1 << 4)) 
    sta VERA_H

    ;Set graphics address = $4000 and graphics mode 1 (8 bpp)
    lda #(($4000 << 5) & 255)
    sta VERA_D0
    lda #(($4000 >> 13) | (1 << 7))
    sta VERA_D0

    ;Set cursor screen x position = 0
    lda #0
    sta VERA_D0
    sta VERA_D0
    
    lda #0
    sta CRS_X

    ;Set cursor screen y position = 2
    lda #16
    sta VERA_D0
    lda #0
    sta VERA_D0
    lda #2
    sta CRS_Y

    ;Set Z-depth = 0 => hidden, and V-flip & H-flip = false
    lda #0
    sta VERA_D0

    ;Set sprite height=8, and sprite width=8
    lda #0
    sta VERA_D0

    ;Enable sprites
    lda #%01000000
    ora VERA_VIDEO
    sta VERA_VIDEO

    ;Init cursor toggle counter
    lda #60
    sta cursor_toggle_counter

    rts

.endproc

;******************************************************************************
;Function name.......: cursor_show
;Purpose.............: Shows the cursor sprite
;Preparatory routines: cursor_init (only once)
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_show
    lda #$06
    sta VERA_L
    lda #$fc
    sta VERA_M
    lda #$01 
    sta VERA_H

    lda #(3 << 2)
    sta VERA_D0

    rts

.endproc

;******************************************************************************
;Function name.......: cursor_hide
;Purpose.............: Hides the cursor sprite
;Preparatory routines: cursor_init (only once)
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_hide
    lda #$06
    sta VERA_L
    lda #$fc
    sta VERA_M
    lda #$01 
    sta VERA_H

    lda #0
    sta VERA_D0

    rts

.endproc

;******************************************************************************
;Function name.......: cursor_move
;Purpose.............: Moves the cursor
;Preparatory routines: cursor_init (only once)
;Input...............: .X = new cursor column, .Y = new cursor row
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_move
    lda #$02
    sta VERA_L
    lda #$fc
    sta VERA_M
    lda #($01 | (1 << 4))
    sta VERA_H

    ;Set X position, bits 0:7
    txa
    asl
    asl
    asl
    sta VERA_D0

    ;Set X position, bits 8:9
    txa
    lsr
    lsr
    lsr
    lsr
    lsr
    sta VERA_D0

    ;Set Y position, bits 0:7
    tya
    asl
    asl
    asl
    sta VERA_D0

    ;Set Y position, bits 8:9
    tya
    lsr
    lsr
    lsr
    lsr
    lsr
    sta VERA_D0

    ;Finally, save position to zero page vectors
    stx CRS_X
    sty CRS_Y

    rts

.endproc

;******************************************************************************
;Function name.......: cursor_move_right
;Purpose.............: Moves the cursor one step right
;Preparatory routines: cursor_init (only once)
;Input...............: Nothing
;Returns.............: .C = 1 if at rightmost position, otherwise .C = 0
;Error returns.......: None
.proc cursor_move_right
    lda CRS_X
    cmp #79
    bcs rightmost

    inc CRS_X

    ldx CRS_X
    ldy CRS_Y
    jsr cursor_move

    clc

    rts

rightmost:
    sec
    rts

.endproc

;******************************************************************************
;Function name.......: cursor_move_left
;Purpose.............: Moves the cursor one step left
;Preparatory routines: cursor_init (only once)
;Input...............: Nothing
;Returns.............: .C = 1 if at leftmost position, otherwise .C = 0
;Error returns.......: None
.proc cursor_move_left
    lda CRS_X
    cmp #0
    beq leftmost

    dec CRS_X

    ldx CRS_X
    ldy CRS_Y
    jsr cursor_move

    clc
    rts

leftmost:
    sec
    rts
.endproc

;******************************************************************************
;Function name.......: cursor_move_up
;Purpose.............: Moves the cursor one step up
;Preparatory routines: cursor_init (only once)
;Input...............: Nothing
;Returns.............: .C = 1 if at top of screen, otherwise .C = 0
;Error returns.......: None
.proc cursor_move_up
    ldy CRS_Y
    cpy #2
    beq top
    dey
    ldx CRS_X
    jsr cursor_move
    clc
    rts

top:
    sec
    rts
.endproc

;******************************************************************************
;Function name.......: cursor_move_down
;Purpose.............: Moves the cursor one step down
;Preparatory routines: cursor_init (only once)
;Input...............: Nothing
;Returns.............: .C = 1 if at bottom of screen position, otherwise .C = 0
;Error returns.......: None
.proc cursor_move_down
    ldy CRS_Y
    cpy #55
    beq bottom

    iny
    ldx CRS_X
    jsr cursor_move
    clc
    rts

bottom:
    sec
    rts
.endproc

;******************************************************************************
;Function name.......: cursor_move_crlf
;Purpose.............: Moves the cursor to the first column of next row
;Preparatory routines: cursor_init (only once)
;Input...............: Nothing
;Returns.............: .C = 1 if at bottom of screen, otherwise .C = 0
;Error returns.......: None
.proc cursor_move_crlf
    ldy CRS_Y
    cpy #55
    beq bottom
    iny
    ldx #0
    jsr cursor_move
    clc
    rts

bottom:
    ldy CRS_Y
    ldx #0
    jsr cursor_move
    sec
    rts
.endproc

;******************************************************************************
;Function name.......: cursor_toggle
;Purpose.............: Toggles the cursor on and off
;Preparatory routines: cursor_init (only once)
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_toggle
    ;Check if cursor is disabled, cursor_toggle_counter = 0
    lda cursor_toggle_counter
    bne active
    
    ;Cursor is disabled, ensure hidden
    jsr cursor_hide
    rts

active:
    dec cursor_toggle_counter
    
    lda cursor_toggle_counter
    cmp #30                 ;After 30 invocations we have 1/2 a second, show cursor
    beq show
    cmp #0                  ;After another 30 invocations we have 1/2 a second again, hide cursor
    bne exit

hide:
    jsr cursor_hide
    lda #60                 ;Restore counter to start value
    sta cursor_toggle_counter

    rts
    
show:
    jsr cursor_show

exit:
    rts
.endproc

.proc cursor_activate
    jsr cursor_show
    lda #29
    sta cursor_toggle_counter
    rts
.endproc

.proc cursor_disable
    jsr cursor_hide
    stz cursor_toggle_counter
    rts
.endproc

cursor_toggle_counter: .byt 0