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
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.if target_mem=target_ram
.proc scancode_init
    lda $032e
    sta scancode_default_handler
    lda $032f
    sta scancode_default_handler+1
    
    stz scancode_modifiers
    stz scancode_ext

    sei
    lda #<scancode_handler
    sta $032e
    lda #>scancode_handler
    sta $032f
    cli
    
    rts
.endproc
.endif

.if target_mem=target_rom
.proc scancode_init
    lda $032e
    sta scancode_default_handler
    lda $032f
    sta scancode_default_handler+1

    sei
    lda #<(bridge_code+bridge_scancode-bridge_kernal)
    sta $032e
    lda #>(bridge_code+bridge_scancode-bridge_kernal)
    sta $032f
    cli
    
    rts
.endproc
.endif


;******************************************************************************
;Function name.......: scancode_restore
;Purpose.............: Restores scan code handler to default value
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc scancode_restore
    sei
    lda scancode_default_handler
    sta $032e
    lda scancode_default_handler+1
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

    ldy #14
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
    lda bitmask-1,y
    plp
    php
    bcs keyup

keydown:
    cpy #10
    bcs :+      ;y>10 => not modifier key

    ora scancode_modifiers
    sta scancode_modifiers
    bra exit

:   ora scancode_ext
    sta scancode_ext
    bra exit

keyup:
    cpy #10     ;y>10 => not modifier, ignore key up
    bcs exit

    eor #$ff
    and scancode_modifiers
    sta scancode_modifiers
    bra exit

scancodes_H:
    .byt  $0,  $0,  $0, $e0,  $0, $e0, $e0, $e0,  $0, $e0, $e0, $e0, $e0, $e0
scancodes_L:
    .byt $12, $59, $11, $11, $14, $14, $1f, $27, $58, $71, $70, $69, $7d, $7a
bitmask:
    .byt KBD_MODIFIER_SHIFT, KBD_MODIFIER_SHIFT, KBD_MODIFIER_ALT, KBD_MODIFIER_ALT, KBD_MODIFIER_CTRL, KBD_MODIFIER_CTRL, KBD_MODIFIER_WIN, KBD_MODIFIER_WIN, KBD_MODIFIER_CAPS
    .byt KBD_EXT_DELETE, KBD_EXT_INSERT, KBD_EXT_END, KBD_EXT_PGUP, KBD_EXT_PGDN
.endproc

.segment "VARS"
    scancode_default_handler: .res 2
    scancode_modifiers: .res 1
    scancode_ext: .res 1
.CODE

KBD_MODIFIER_SHIFT = 1 ; C64:  Shift
KBD_MODIFIER_ALT   = 2 ; C64:  Commodore
KBD_MODIFIER_CTRL  = 4 ; C64:  Ctrl
KBD_MODIFIER_WIN   = 8 ; C128: Alt
KBD_MODIFIER_CAPS  = 16; C128: Caps

KBD_EXT_DELETE     = 1
KBD_EXT_INSERT     = 2
KBD_EXT_END        = 4
KBD_EXT_PGUP       = 8
KBD_EXT_PGDN       = 16
