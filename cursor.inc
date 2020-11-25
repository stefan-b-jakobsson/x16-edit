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
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_init
    ;Prepare writing sprite graphics data at VERA address $4000
    lda #$00
    sta VERA_L
    lda #$40
    sta VERA_M
    lda #(1<<4)
    sta VERA_H

    ;First write 6 blank lines (48 bytes) with color 0
    lda #$00
    ldx #48
    :   sta VERA_D0
        dex
        bne :-
    
    ;Then write 2 lines (16 bytes) with color 1 (=white)
    lda #$01
    ldx #16
    :   sta VERA_D0
        dex
        bne :-
    
    ;Prepare writing attributes for sprite #0 (VERA address 01fc00) + autoincrement +1
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
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_show
    ;Prepare writing sprite attributes at VERA address 01fc06
    lda #$06
    sta VERA_L
    lda #$fc
    sta VERA_M
    lda #$01 
    sta VERA_H

    ;Set sprite #0 visible
    lda #(3 << 2)
    sta VERA_D0

    rts
.endproc

;******************************************************************************
;Function name.......: cursor_hide
;Purpose.............: Hides the cursor sprite
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_hide
    ;Prepare writing sprite attributes at VERA address 01fc06
    lda #$06
    sta VERA_L
    lda #$fc
    sta VERA_M
    lda #$01 
    sta VERA_H

    ;Set sprite #0 not visible
    lda #0
    sta VERA_D0

    rts
.endproc

;******************************************************************************
;Function name.......: cursor_move
;Purpose.............: Moves the cursor
;Input...............: X=cursor column, Y=cursor row
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_move
    ;Prepare writing sprite attributes at VERA address 01fc02
    lda #$02
    sta VERA_L
    lda #$fc
    sta VERA_M
    lda #($01 | (1 << 4))
    sta VERA_H

    ;Set X position, bits 0:7 (X register multiplied by 8)
    txa
    asl
    asl
    asl
    sta VERA_D0

    ;Set X position, bits 8:9 (X register divided by 32)
    txa
    lsr
    lsr
    lsr
    lsr
    lsr
    sta VERA_D0

    ;Set Y position, bits 0:7 (Y register multiplied by 8)
    tya
    asl
    asl
    asl
    sta VERA_D0

    ;Set Y position, bits 8:9 (Y register divided by 32)
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
;Purpose.............: Moves cursor one step right, but not beyond rightmost
;                      column of the screen
;Input...............: Nothing
;Returns.............: C=1 if at rightmost position of screen, else C=0
;Error returns.......: None
.proc cursor_move_right
    lda CRS_X
    cmp #(SCREEN_WIDTH-1)
    bcs rightmost

    inc CRS_X
    ldx CRS_X
    ldy CRS_Y
    jsr cursor_move

    clc             ;We're not at rightmost position, return C=0
    rts

rightmost:
    sec             ;We're at rightmost position, return C=1
    rts

.endproc

;******************************************************************************
;Function name.......: cursor_move_left
;Purpose.............: Moves cursor one step left, but not beyond leftmost
;                      column of the screen
;Input...............: Nothing
;Returns.............: C=1 if at leftmost position of screen, else C=0
;Error returns.......: None
.proc cursor_move_left
    lda CRS_X
    cmp #0
    beq leftmost

    dec CRS_X

    ldx CRS_X
    ldy CRS_Y
    jsr cursor_move

    clc             ;We're not at leftmost position, return C=0
    rts

leftmost:
    sec             ;We're at leftmost position, return C=1
    rts
.endproc

;******************************************************************************
;Function name.......: cursor_move_up
;Purpose.............: Moves the cursor one row up, but not beyond the first
;                      user editable row
;Input...............: Nothing
;Returns.............: C=1 if at top of screen, else C=0
;Error returns.......: None
.proc cursor_move_up
    ldy CRS_Y
    cpy #2      ;Row 2 is the first editable screen row, program header above
    beq top

    dey
    ldx CRS_X
    jsr cursor_move

    clc         ;We're not at top of screen, return C=0
    rts

top:
    sec         ;We're at top of screen, return C=1
    rts
.endproc

;******************************************************************************
;Function name.......: cursor_move_down
;Purpose.............: Moves the cursor one row down, but not beyond the last
;                      user editable row
;Input...............: Nothing
;Returns.............: C=1 if at bottom of screen, else C=0
;Error returns.......: None
.proc cursor_move_down
    ldy CRS_Y
    cpy #(SCREEN_HEIGHT+1)  ;The last editable row
    beq bottom

    iny
    ldx CRS_X
    jsr cursor_move
    
    clc             ;We're not at bottom of screen, return C=0
    rts

bottom:
    sec             ;We're at bottom of screen, return C=1
    rts
.endproc

;******************************************************************************
;Function name.......: cursor_move_crlf
;Purpose.............: Moves the cursor to the first column of next row, but 
;                      the cursor will not be moved below the last editable
;                      row
;Input...............: Nothing
;Returns.............: C=1 if at bottom of screen, else C=0
;Error returns.......: None
.proc cursor_move_crlf
    ldy CRS_Y
    cpy #(SCREEN_HEIGHT+1)         ;The last editable row
    beq bottom
    iny
    ldx #0
    jsr cursor_move
    
    clc             ;We're not at bottom of screen, return C=0
    rts

bottom:
    ;Move cursor to start of row
    ldy CRS_Y       
    ldx #0
    jsr cursor_move

    sec             ;We're at bottom of screen, return C=1
    rts
.endproc

;******************************************************************************
;Function name.......: cursor_toggle
;Purpose.............: Controls cursor toggle. The cursor will change state
;                      (hidden or visible) for every 30 invocations of this
;                      function. When called once every interrupt, the cursor
;                      toggle period will be 1 second.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_toggle
    ;Check if cursor is disabled, cursor_toggle_counter = 0
    lda cursor_toggle_counter
    bne active
    
    ;Cursor is disabled
    rts

active:
    ;Cursor is active, decrease counter
    dec cursor_toggle_counter
    
    lda cursor_toggle_counter
    cmp #30                 ;After 30 invocations we have 1/2 a second, show cursor
    beq show
    cmp #0                  ;After another 30 invocations we have 1/2 a second again, hide cursor
    bne exit                ;If counter value is not 30 or 0, do nothing

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

;******************************************************************************
;Function name.......: cursor_activate
;Purpose.............: Acivates the cursor, making it visible and enabling
;                      cursor toggle
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_activate
    jsr cursor_show
    lda #29
    sta cursor_toggle_counter
    rts
.endproc

;******************************************************************************
;Function name.......: cursor_disable
;Purpose.............: Disables and hides the cursor
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc cursor_disable
    jsr cursor_hide
    stz cursor_toggle_counter
    rts
.endproc