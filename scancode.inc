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
;Function name.......: scancode_init
;Purpose.............: Initializes keyboard scan code handler
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc scancode_init
    sei
    lda #<scancode_handler
    sta $032e
    lda #>scancode_handler
    sta $032f
    cli
    rts
.endproc

;******************************************************************************
;Function name.......: scancode_handler
;Purpose.............: Custom keyboard scan code handler
;Input...............: Input set by Kernal, X=PS/2 prefix, A=PS/2 scan code,
;                      carry clear=keydown, and carry set=keyup
.proc scancode_handler
    pha
    phx
    php

    ldy #9
loop:
    cmp scancodes_L-1,y
    beq checkprefix
loop_cont:
    dey
    bne loop
    
exit:
    plp
    plx
    pla
    rts

checkprefix:
    pha
    txa
    cmp scancodes_H-1,y
    beq match
    pla
    bra loop_cont

match:
    pla
    lda scancode_modifiers
    plp
    php
    bcs keyup

keydown:
    ora bitmask-1,y
    sta scancode_modifiers
    bra exit

keyup:
    eor #$ff
    and scancode_modifiers
    sta scancode_modifiers
    bra exit

scancodes_H:
    .byt  $0,  $0,  $0, $e0,  $0, $e0, $e0, $e0,  $0      ;Left Shift, Right Shift, Left Alt, Right Alt, Left Ctrl, Right Ctrl, Left Win, Right Win, Caps Lock
scancodes_L:
    .byt $12, $59, $11, $11, $14, $14, $1f, $27, $58      ;Left Shift, Right Shift, Left Alt, Right Alt, Left Ctrl, Right Ctrl, Left Win, Right Win, Caps Lock
bitmask:
    .byt KBD_MODIFIER_SHIFT, KBD_MODIFIER_SHIFT, KBD_MODIFIER_ALT, KBD_MODIFIER_ALT, KBD_MODIFIER_CTRL, KBD_MODIFIER_CTRL, KBD_MODIFIER_WIN, KBD_MODIFIER_WIN, KBD_MODIFIER_CAPS
.endproc

KBD_MODIFIER_SHIFT = 1 ; C64:  Shift
KBD_MODIFIER_ALT   = 2 ; C64:  Commodore
KBD_MODIFIER_CTRL  = 4 ; C64:  Ctrl
KBD_MODIFIER_WIN   = 8 ; C128: Alt
KBD_MODIFIER_CAPS  = 16; C128: Caps
