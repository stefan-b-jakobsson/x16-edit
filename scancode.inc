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

    stz scancode_modifiers
    stz scancode_ext

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
;Returns:............: Modifier key status is stored in variable
;                      scancode_modifier, each bit representing one key
;                      (1=down, 0=up) as follows: Bit 0=Shift, bit 1=Alt,
;                      bit 2=Ctrl, bit 3=Windows key, bit 4=Caps lock
;
;                      The scan code handler supports several extra keys
;                      that are ignored by the Kernal. The last pressed
;                      extra key is stored in the variable scancode_ext.
;                      If it's a printable char the PETSCII/ASCII
;                      value is stored. The following non-printable
;                      keys are represented by the following non-standard
;                      values:
;
;                      Key        Value
;                      DELETE     = $15
;                      END        = $16
;                      PGUP       = $17
;                      PGDN       = $18
;                      KPD ENTER  = $19
;                      INSERT     = $1a
;Error returns:......: None
.proc scancode_handler
    ;Save input so we can restore values before exit
    pha
    phx
    php   

    ;Check scancode match with one of the supported extra keys
    ldy #(scancodes_L-scancodes_H)
loop:
    cmp scancodes_L-1,y
    beq checkprefix
loop_cont:
    dey
    bne loop
    
exit:
    ;No match, restore input and exit
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
    ;Handle scancode match
    pla
    lda bitmask-1,y
    plp
    php
    bcs keyup

keydown:
    ;Key down event
    cpy #16             ;Y>=16 => printable char
    bcs :++
    cpy #10             ;Y>=10 => control char
    bcs :+
                        ;Y<10 => modifier key

    ;Modifier key       
    ora scancode_modifiers
    sta scancode_modifiers
    cpy #3              ;If index < 3 we got one of the Shift keys which is transferred to the Kernal, other modifier keys are consumed and hidden from the Kernal
    bcc exit
    bra consume_and_exit

    ;Control key: set non-standard ASCII value (index+11)
:   clc
    tya
    adc #11
    sta scancode_ext
    cmp #$19
    bne consume_and_exit
    
    lda #KBD_EXT_ENT
    sta scancode_ext
    bra consume_and_exit

    ;Printable key: set ASCII code
:   lda printable-16,y
    sta scancode_ext
    bra consume_and_exit

keyup:
    ;Key up event
    cpy #10                 ;Y>10 => not modifier key, just consume and exit
    bcs consume_and_exit

    ;Modifier key, clear status bit
    eor #$ff
    and scancode_modifiers
    sta scancode_modifiers
    cpy #3                  ;If index < 3 we got one of the Shift keys which is transferred to the Kernal, other modifier keys are consumed and hidden from the Kernal
    bcc exit
    bra consume_and_exit

consume_and_exit:
    ;Consume key event preventing the Kernal from further handling it
    plp
    plx
    pla
    lda #0
    rts

scancodes_H:                             
         ;Modifier keys                                     ;Control keys                      ;Printable keys
    .byt $00, $00, $00, $e0, $00, $e0, $e0, $e0, $00,       $e0, $e0, $e0, $e0, $e0, $e0,      $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $e0, $00, $00, $00
scancodes_L:
    .byt $12, $59, $11, $11, $14, $14, $1f, $27, $58,       $71, $69, $7d, $7a, $5a, $70,      $70, $69, $72, $7a, $6b, $73, $74, $6c, $75, $7d, $71, $4a, $7c, $7b, $79
bitmask:
    .byt KBD_MODIFIER_SHIFT, KBD_MODIFIER_SHIFT, KBD_MODIFIER_ALT
    .byt KBD_MODIFIER_ALT, KBD_MODIFIER_CTRL, KBD_MODIFIER_CTRL
    .byt KBD_MODIFIER_WIN, KBD_MODIFIER_WIN, KBD_MODIFIER_CAPS
printable:
    .byt KBD_EXT_NUM_0, KBD_EXT_NUM_1, KBD_EXT_NUM_2, KBD_EXT_NUM_3
    .byt KBD_EXT_NUM_4, KBD_EXT_NUM_5, KBD_EXT_NUM_6, KBD_EXT_NUM_7
    .byt KBD_EXT_NUM_8, KBD_EXT_NUM_9, KBD_EXT_NUM_DP, KBD_EXT_NUM_DIV
    .byt KBD_EXT_NUM_MUL, KBD_EXT_NUM_MIN, KBD_EXT_NUM_PLS
.endproc

.segment "VARS"
    scancode_default_handler: .res 2
    scancode_modifiers: .res 1
    scancode_ext: .res 1
.CODE

;Modifier key bitmask
KBD_MODIFIER_SHIFT = 1 ; C64:  Shift
KBD_MODIFIER_ALT   = 2 ; C64:  Commodore
KBD_MODIFIER_CTRL  = 4 ; C64:  Ctrl
KBD_MODIFIER_WIN   = 8 ; C128: Alt
KBD_MODIFIER_CAPS  = 16; C128: Caps

;Control keys
KBD_EXT_DELETE     = $15
KBD_EXT_END        = $16
KBD_EXT_PGUP       = $17
KBD_EXT_PGDN       = $18
KBD_EXT_ENT        = $0d
KBD_EXT_INSERT     = $1a

;Printable keys: ASCII value
KBD_EXT_NUM_0       = 48
KBD_EXT_NUM_1      = 49
KBD_EXT_NUM_2      = 50
KBD_EXT_NUM_3      = 51
KBD_EXT_NUM_4      = 52
KBD_EXT_NUM_5      = 53
KBD_EXT_NUM_6      = 54
KBD_EXT_NUM_7      = 55
KBD_EXT_NUM_8      = 56
KBD_EXT_NUM_9      = 57
KBD_EXT_NUM_DP     = 46
KBD_EXT_NUM_DIV    = 47
KBD_EXT_NUM_MUL    = 42
KBD_EXT_NUM_MIN    = 45
KBD_EXT_NUM_PLS    = 43