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
;Function name.......: bridge_copy
;Purpose.............: Copies bridge code to RAM location $0780,
;                      necessary for making the program romable
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc bridge_copy
    ldx #0
:   lda bridge_kernal,x
    sta $0780,x
    inx
    cpx #(bridge_end-bridge_kernal)
    bne :-
    rts
.endproc

;******************************************************************************
;Function name.......: kernal_bridge
;Purpose.............: Bridge code to be copied to RAM on program startup.
;                      Necessary for making the program romable.
;Input...............: Use macro BRIDGE_SETADDR to set the Kernal address
;                      to call before using this function
;Returns.............: Nothing
;Error returns.......: None
bridge_kernal:
    stz ROM_SEL
    jsr $ffff       ;$ffff is just placeholder
    pha
    lda #7          ;Put possible ROM bank here
    sta ROM_SEL
    pla
    rts

bridge_irq:
    lda ROM_SEL
    pha
    lda #7
    sta ROM_SEL
    jsr irq_handler
    pla
    sta ROM_SEL
    jmp (irq_default_handler)

bridge_end:

.segment "IRQ"
    .byt $ff, $ff, $ff, $ff, $8b, $03
.CODE


