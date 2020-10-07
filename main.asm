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

;Include global defines
.include "common.inc"
.include "charset.inc"

;******************************************************************************
;Function name.......: main
;Purpose.............: Program entry function
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc main
    ;Set program mode to default
    stz APP_MOD

    ;Set program in running state
    stz APP_QUIT

    ;Initialize base functions
    jsr mem_init
    jsr file_init
    jsr screen_init
    jsr cursor_init
    jsr irq_init
    jsr clipboard_init
    
    ;Wait for the program to terminate    
:   lda APP_QUIT
    cmp #2
    bne :-
    
    rts

count:
    .byt 32
.endproc

;A routine to feed test characters to the editor 
;for debugging purposes
.proc feed_test
    lda #<testtext
    sta $40
    lda #>testtext
    sta $41
    stz index

:   ldy index
    lda ($40),y
    beq eof
    jsr keyboard_mode_default
    inc index
    bne :-
    inc $41
    jmp :-
    
eof:
    jsr screen_refresh
    rts

index:
    .byt 0

testtext:
    .res 59, 65
    .byt 13

    .res 59, 66
    .byt 13

    .res 59, 67
    .byt 13

    .res 59, 68
    .byt 13

    .res 11, 69
    .byt 0

.endproc

.include "screen.inc"
.include "keyboard.inc"
.include "cmd.inc"
.include "prompt.inc"
.include "irq.inc"
.include "cursor.inc"
.include "file.inc"
.include "util.inc"
.include "clipboard.inc"
.include "mem.inc"