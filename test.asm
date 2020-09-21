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


.include "common.inc"

cld

;SELECT TEST
;jsr test_ram
;jsr test_mem_init
;jsr test_mem_alloc
;jsr test_mem_alloc2
;jsr test_mem_alloc3
;jsr test_mem_push
;jsr test_mem_step_right
;jsr test_mem_insert
;jsr test_screen_println
;jsr test_initial
;jsr test_keyboard
;jsr test_prompt
;jsr test_mem_free
;jsr test_decimal
;jsr test_mem_step
jsr test_util_bin

rts

.proc test_util_bin
    ldx #$99
    ldy #$10
    lda #$23
    jsr util_convert_to_binary
    stx $5000
    sty $5001
    sta $5002
    rts
.endproc

.proc test_mem_step
    jsr mem_init
    ldy #4
    lda #251
    sta (CRS_ADR),y

    lda #251
    sta counter
    
:   jsr mem_crs_step_right
    dec counter
    bne :-

    lda CRS_BNK
    sta $5000
    lda CRS_ADR
    sta $5001   
    lda CRS_ADR+1
    sta $5002
    lda CRS_IDX
    sta $5003

    lda #251
    sta counter

:   jsr mem_crs_step_left
    dec counter
    bne :-

    lda CRS_BNK
    sta $5004
    lda CRS_ADR
    sta $5005   
    lda CRS_ADR+1
    sta $5006
    lda CRS_IDX
    sta $5007

    rts

counter:
    .byt 0
.endproc

.proc test_ram
    lda #3
    sta BNK_SEL
    lda #$ff
    sta $a000
    rts
.endproc

.proc test_mem_init
    ;Scramble first mem page content
    lda #1
    sta BNK_SEL

    lda #$a0
    sta CRS_ADR+1
    stz CRS_ADR

    ldy #0
:   lda #$ff
    sta (CRS_ADR),y
    iny
    cpy #5
    bne :-

    ;Scramble zero page vectors
    lda #$ff
    
    sta CRS_BNK
    sta CRS_ADR
    sta CRS_ADR+1
    sta CRS_IDX
    
    sta LNV_BNK
    sta LNV_ADR
    sta LNV_ADR+1
    sta LNV_IDX

    ;Init test output   
    ldy #0
    lda #$ff
:   sta $5000,y
    iny
    cpy #$0c
    bne :-

    ;Call procedure we're testing
    jsr mem_init

    ;Assert CRS_BNK = 1
    lda CRS_BNK
    cmp #1
    beq :+
    stz $5000

    ;Assert CRS_ADR = 0
:   lda CRS_ADR
    beq :+
    stz $5001

    ;Assert CRS_ADR+1 = $a0
:   lda CRS_ADR+1
    cmp #$a0
    beq :+
    stz $5002

    ;Assert CRS_IDX = 0
:   lda CRS_IDX
    beq :+
    stz $5003

    ;Assert LNV_BNK = 1
:   lda LNV_BNK
    cmp #1
    beq :+
    stz $5004

    ;Assert LNV_ADR = 0
:   lda LNV_ADR
    beq :+
    stz $5005

    ;Assert LNV_ADR+1 = $a0
:   lda LNV_ADR+1
    cmp #$a0
    beq :+
    stz $5006

    ;Asser LNV_IDX = 0
:   lda LNV_IDX
    beq :+
    stz $5007

    ;Assert mem_map to mem_map+3 = $ff
:   lda mem_map
    cmp #255
    beq :+
    stz $5008

:   lda mem_map+1
    cmp #255
    beq :+
    stz $5008

:   lda mem_map+2
    cmp #255
    beq :+
    stz $5008

:   lda mem_map+3
    cmp #255
    beq :+
    stz $5008

    ;Assert mem_map+4 = 1
:   lda mem_map+4
    cmp #1
    beq :+
    stz $5009

    ;Assert mem_map+5 to mem_map+1023 = 0
    ldy #0

    lda mem_map+5,y
    beq :+
    stz $500a
    
:   lda mem_map+256,y
    beq :+
    stz $500a
 
:   lda mem_map+512,y
    beq :+
    stz $500a

:   lda mem_map+768,y
    beq :+
    stz $500a

    ;Assert content of bank 1, page $a0, first 5 bytes should be 0
:   lda #1
    sta BNK_SEL

    ldy #0
    lda (CRS_ADR),y
    beq :+
    stz $500b

    ldy #1
    lda (CRS_ADR),y
    beq :+
    stz $500b

    ldy #2
    lda (CRS_ADR),y
    beq :+
    stz $500b

    ldy #3
    lda (CRS_ADR),y
    beq :+
    stz $500b

    ldy #4
    lda (CRS_ADR),y
    beq :+
    stz $500b

:   rts
.endproc

.proc test_mem_alloc
    jsr mem_init

    lda #0
    sta counter

    ;Assert 1 call => bank 01, page $a1
    jsr mem_alloc
    sty $5010
    stx $5011
    rts
    cpy #1
    beq :+
    stz $5010
:   stx $5010
    cpx #$a1
    beq :+
    stx $5010

    ;Assert 31 more calls => bank 02, page $a0
:   lda #31
    sta counter
    
:   jsr mem_alloc
    dec counter
    bne :-

    cpy #2
    beq :+
    stz $5011
    cpx #$a0
    beq :+
    stz $5011

    ;Assert 8127 more calls => bank $ff, page $bf
:   lda #0
    sta counter
    lda #31
    sta counter+1

:   jsr mem_alloc
    dec counter
    bne :-

    dec counter+1
    bne :-

    lda #191
    sta counter

:   jsr mem_alloc
    dec counter
    bne :-

    sty $5012
    stx $5013

    ;Assert 1 more call => bank 0, page 0, i.e. mem full
    jsr mem_alloc
    sty $5014
    stx $5015

    rts

counter:
    .byt 0, 0
.endproc

.proc test_mem_alloc2
    jsr mem_init
    
    ;Call 33 times to go over bank boundary
    lda #32
    sta counter

    :jsr mem_alloc
    inc CRS_ADR+1

    dec counter
    bne :-

    rts

counter:
    .byt 0
.endproc


.proc test_mem_push
    lda #1
    sta $5020
    sta $5021
    sta $5022

    jsr mem_init

    lda #1
    sta BNK_SEL
    lda #$a0
    sta TMP1_ADR+1
    stz TMP1_ADR
    ldy #5
    lda #49
    sta (TMP1_ADR),y
    iny
    lda #50
    sta (TMP1_ADR),y

    ldy #1
    ldx #$a0
    lda #0

    ;Assert push at index 0, len 0
    jsr mem_push
    ldy #6
    lda (TMP1_ADR),y
    cmp #49
    beq :+
    stz $5020

    ;Assert 250 more pushes
:   ldx #250
    stx counter

:   ldy #1
    ldx #$a0
    lda #0
    jsr mem_push
    dec counter
    bne :-

    ;Assert one more push, going over the page boundary
    ldy #1
    ldx #$a0
    lda #0

    jsr mem_push
    bcs :+
    stz $5021

:   sta $5022

    rts

counter: .byt 0

.endproc

.proc test_mem_alloc3
    jsr mem_init
    jsr mem_alloc
    jsr mem_alloc
    rts

    ldx #251
    stx counter

:   lda #48
    jsr mem_insert
    dec counter
    bne:-

    ldx #100
    stx counter

:   lda #49
    jsr mem_insert
    dec counter
    bne:-

    jsr mem_alloc

    rts


counter:
    .byt 0
.endproc

.proc test_mem_step_right
    jsr mem_init
    
    ldy #4
    lda #250
    sta (CRS_ADR),y

    lda #250
    sta CRS_IDX

    jsr mem_crs_step_right

    lda CRS_IDX
    sta $5030
    lda CRS_ADR+1
    sta $5031

    rts
.endproc

.proc test_mem_insert
    jsr mem_init
    jsr mem_alloc

    lda #0
    sta counter
    lda #4
    sta counter+1

    :lda counter
    jsr mem_insert
    stz CRS_IDX
    dec counter
    bne :-

    dec counter+1
    bne :-

    ;Verify
    stz $5000
    stz $5001

    stz TMP1_ADR
    lda #$a0
    sta TMP1_ADR+1
    lda #1
    sta BNK_SEL

    ldy #4
    lda (TMP1_ADR),y
    sta len

    lda #$ff
    sta counter
    lda #5
    sta TMP1_ADR

    ldy #0

:   cpy len
    beq :+
    inc counter
    lda (TMP1_ADR),y
    cmp counter
    bne fail
    iny
    bne :-

:   stz TMP1_ADR
    ldy #3
    lda (TMP1_ADR),y
    beq exit
    tax
    ldy #2
    lda (TMP1_ADR),y
    sta BNK_SEL
    stx TMP1_ADR+1
    ldy #4
    lda (TMP1_ADR),y
    sta len

    lda #5
    sta TMP1_ADR

    ldy #0
    
    jmp :--

exit:
    lda #1
    sta CRS_BNK
    lda #$a0
    sta CRS_ADR+1
    stz CRS_ADR
    lda #250
    sta CRS_IDX
    lda #251
    ldy #4
    sta (CRS_ADR),y
    lda #255
    ldy #255
    sta (CRS_ADR),y

    jsr mem_alloc
    lda #$a6
    sta CRS_ADR+1
    lda #1
    ldy #5
    sta (CRS_ADR),y
    ldy #6
    lda #2
    sta (CRS_ADR),y
    ldy #7
    lda #3
    sta (CRS_ADR),y
    lda #$a0
    sta CRS_ADR+1
    
    lda #$ff
    jsr mem_insert

    rts

fail:
    lda TMP1_ADR+1
    sta $5000
    sty $5001
    rts

counter:
    .byt 0,0

len:
    .byt 0
.endproc

.proc test_screen_println
    jsr mem_init
    
    lda #255
    sta counter

    lda #240
    sta LNV_IDX

:   lda counter
    jsr mem_insert
    dec counter
    bne :-

    jsr screen_println

    rts

counter: .byt 0
.endproc

.proc test_prompt
    jsr prompt_init
    lda #65
    jsr prompt_keypress
    rts
.endproc

.proc test_initial
    lda #0
    sta CRS_X
    sta CRS_Y
    sta APP_MOD

    jsr mem_init
    jsr screen_init
    jsr cursor_init
    jsr irq_init

    stz APP_QUIT
:   lda APP_QUIT
    cmp #2
    bne :-
    
    rts
.endproc

.proc test_keyboard
    :jsr KERNAL_GETIN
    beq :-
.endproc

.proc test_mem_free
    jsr mem_init

    lda #32
    sta counter

:   jsr mem_alloc
    sty CRS_BNK
    sty CRS_BNK
    stx CRS_ADR+1
    dec counter
    bne :-
    
    ldy #1
    ldx #$bf
    jsr mem_free

    ldy #2
    ldx #$a0
    jsr mem_free
    
    ldx #16
:   lda mem_map+3,x
    sta $5000-1,x
    dex
    bne :-
    
    rts

counter:
    .byt 32

.endproc

.proc test_decimal
    lda #0
    sta VERA_L
    lda #2
    sta VERA_M
    lda #(2<<4)
    sta VERA_H

    ldx #5
    ldy #100
    lda #100
    jsr util_convert_to_decimal

    stx TMP1_ADR
    sty TMP1_ADR+1
    ldy #0
:   lda (TMP1_ADR),y
    beq exit
    sta VERA_D0
    iny
    jmp :-
exit:
    rts
.endproc

.include "screen.inc"
.include "keyboard.inc"
.include "cmd.inc"
.include "prompt.inc"
.include "irq.inc"
.include "cursor.inc"
.include "file.inc"
.include "util.inc"
.include "mem.inc"