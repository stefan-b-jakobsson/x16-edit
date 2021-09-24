;******************************************************************************
;Copyright 2020-2021, Stefan Jakobsson.
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
;Function name.......: progress_init
;Purpose.............: Initializes progress indicator functions and sets up 
;                      the three sprites used as indicators
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc progress_init
    ;Sprite #1 config
    lda #$08
    sta VERA_L
    lda #$fc
    sta VERA_M
    lda #1 + (1<<4)
    sta VERA_H
    jsr config

    ;Sprite #2 config
    jsr config

    ;Sprite #3 config
    jsr config

    ;Sprite #2, set final X position
    lda #$12
    sta VERA_L
    lda #56
    sta VERA_D0
    
    ;Sprite #3, set final X position
    lda #$1a
    sta VERA_L
    lda #72
    sta VERA_D0

    ;Write sprite data to VERA address $4000
    stz VERA_L
    lda #$40
    sta VERA_M
    lda #1<<4
    sta VERA_H
    
    ldx #0
:   lda sprite_data,x
    sta VERA_D0
    inx
    cpx #32
    bne :-
    rts

config:
    ;Write config data to VERA
    ldx #0
:   lda config_data,x
    sta VERA_D0
    inx
    cpx #8
    bne :-
    rts

config_data:
    .byt ($4000>>5) & 255
    .byt ($4000>>13) & 255
    .byt 40
    .byt 1
    .byt 200
    .byt 1
    .byt %00000000
    .byt 0

sprite_data:
    .byt 0,85,85,0,0,85,85,0,0,85,85,0,0,85,85,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
       
.segment "VARS"
    temp: .res 1
.CODE
.endproc

;******************************************************************************
;Function name.......: progress_show
;Purpose.............: Set animation state to 0 activates sprites, but all three 
;                      indicator sprites hidden (Z-value=00)
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc progress_show
    stz progress_state
    jsr progress_tick

    lda VERA_VIDEO
    ora #%01000000
    sta VERA_VIDEO
    
    stz progress_state
    rts
.endproc

;******************************************************************************
;Function name.......: progress_hide
;Purpose.............: Hides all sprites
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc progress_hide
    lda VERA_VIDEO
    and #%10111111
    sta VERA_VIDEO
    rts
.endproc

;******************************************************************************
;Function name.......: progress_tick
;Purpose.............: Advances progress animation one step; the animation
;                      consists of four steps: all indicators hidden, 
;                      first shown, first and second show, and finally all 
;                      shown
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc progress_tick
    lda #(8+6)
    sta VERA_L
    lda #$fc
    sta VERA_M
    lda #1 + (4<<4)
    sta VERA_H
    
    ldy #12
    lda progress_state

    ldx #3
loop:
    lsr
    bcs :+
    stz VERA_D0
    bra :++
:   sty VERA_D0
:   dex
    bne loop

    sec
    rol progress_state
    lda progress_state
    and #%11111000
    beq :+
    stz progress_state
:   rts
.endproc

.segment "VARS"
    progress_state: .res 1
.CODE