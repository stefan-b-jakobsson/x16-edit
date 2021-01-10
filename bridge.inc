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
;Function name.......: bridge_copy
;Purpose.............: Copies bridge code to RAM, necessary to make the program 
;                      run in ROM
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc bridge_copy
    ldx #bridge_end-bridge_kernal
:   lda bridge_kernal-1,x
    sta bridge_code-1,x
    dex
    bne :-
    rts
.endproc

;******************************************************************************
;Function name.......: kernal_bridge
;Purpose.............: Bridge code to be copied to RAM on program startup.
;                      Necessary to make the program run in ROM
;Input...............: Use macro BRIDGE_SETADDR to set the Kernal address
;                      to call before using this function
;Returns.............: Nothing
;Error returns.......: None

;Bridge for Kernal function calls
bridge_kernal:
    stz ROM_SEL        ;Kernal is ROM bank 0
    jsr $ffff          ;$ffff is just placeholder
    pha
    lda #ROM_BNK       ;Set ROM select to our bank again
    sta ROM_SEL
    pla
    rts                 ;14 bytes

;Bridge for interrupt handling, called by the Kernal
;on interrupt, switches to X16 Edit's ROM bank and then
;calls the custom interrupt handler stored in ROM
bridge_irq:
    lda ROM_SEL
    pha
    lda #ROM_BNK
    sta ROM_SEL
    jsr irq_handler
    pla
    sta ROM_SEL
    jmp (irq_default_handler)   ;19 bytes

bridge_end:


.segment "IRQ"
    .byt $ff, $ff, $ff, $ff, $8b, $03
.CODE