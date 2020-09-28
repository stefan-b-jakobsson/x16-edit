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
;Function name.......: clipboard_init
;Purpose.............: Initializes and clears clipboard
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc clipboard_init
    lda #>clipboard_mem
    sta clipboard_pointer
    sta clipboard_end

    stz clipboard_pointer+1
    stz clipboard_end+1

    rts
.endproc

;******************************************************************************
;Function name.......: clipboard_insert
;Purpose.............: Inserts a char into clipboard mem
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: C=1 if clipboard mem is full
.proc clipboard_insert
    ;Save char in X
    tax

    ;Check if clipboard mem is full
    sec
    lda clipboard_pointer
    sbc #>clipboard_mem
    cmp #8                  ;Allow clipboard mem to be 8x256 bytes = 2K
    bcc mem_not_full

    sec
    rts

mem_not_full:
    ;Set zero page vector
    lda #<clipboard_mem
    sta TMP1_ADR
    lda clipboard_pointer
    sta TMP1_ADR+1
    ldy clipboard_pointer+1

    ;Get char from X and save it
    txa
    sta (TMP1_ADR),y
    
    ;Increase pointer
    inc clipboard_pointer+1
    bne :+
    inc clipboard_pointer

    ;Increase end
:   inc clipboard_end+1
    bne exit
    inc clipboard_end

exit:
    clc
    rts

.endproc

;******************************************************************************
;Function name.......: clipboard_get
;Purpose.............: Gets a char from clipboard mem
;Input...............: Nothing
;Returns.............: A=char
;Error returns.......: C=1 if at end of clipboard mem
.proc clipboard_get
    ;Check if at end of clipboard mem
    sec
    lda clipboard_end
    sbc clipboard_pointer
    bne not_empty
    
    sec
    lda clipboard_end+1
    sbc clipboard_pointer+1
    bne not_empty

    sec
    rts

not_empty:
    ;Set zero page vector
    lda #<clipboard_mem
    sta TMP1_ADR
    lda clipboard_pointer
    sta TMP1_ADR+1
    ldy clipboard_pointer

    lda (TMP1_ADR),y

    inc clipboard_pointer+1
    bne :+
    inc clipboard_pointer

:   clc    
    rts
.endproc

;******************************************************************************
;Function name.......: clipboard_copy
;Purpose.............: Copies current line to clipboard. The cursor will be
;                      moved to end of line. Users of this function are
;                      responsible for backing up cursor position and
;                      restore it if wanted.
;Input...............: None
;Returns.............: Number of chars copied, 24 bit value, X=low, Y=mid, A=high
;Error returns.......: C=1: Clipboard mem full
.proc clipboard_copy
    stz count
    stz count+1
    stz count+2

    ;Move cursor to start of line
    jsr cmd_go_home

    ;Copy line
loop:
    lda CRS_BNK
    sta BNK_SEL
    ldy CRS_IDX
    lda #5
    sta CRS_ADR
    lda (CRS_ADR),y
    stz CRS_ADR
    jsr clipboard_insert
    bcs mem_full

    jsr cmd_go_right
    bcs eol
    cpx #1
    beq eof

    inc count
    bne :+
    inc count+1
    bne :+
    inc count+2

:   jmp loop

eol:
    inc count
    bne :+
    inc count+1
    bne :+
    inc count+2
:   jmp exit

eof:
    ;At EOF, replace last char in clipboard mem with a LF
    lda #LF
    lda #<clipboard_mem
    sta TMP1_ADR
    lda clipboard_end
    sta TMP1_ADR+1
    ldy clipboard_end+1
    dey
    cpy #$ff
    bne :+
    dec TMP1_ADR+1

:   lda #LF
    sta (TMP1_ADR),y

exit:
    clc
    ldx count
    ldy count+1
    lda count+2
    rts

mem_full:
    sec
    rts

count:
    .byt 0,0,0
.endproc

;******************************************************************************
;Function name.......: clipboard_cut
;Purpose.............: Cuts current line and copies it to clipboard
;Input...............: None
;Returns.............: Nothing
;Error returns.......: C=1: Clipboard mem full
.proc clipboard_cut
    jsr clipboard_copy
    bcc prepare
    rts

prepare:
    stx count
    sty count+1
    sta count+2

loop:
    lda count
    bne delete
    lda count+1
    bne delete
    lda count+2
    beq exit

delete:
    jsr cmd_delete
    
    dec count
    lda count
    cmp #$ff
    bne loop
    dec count+1
    lda count+1
    cmp #$ff
    bne loop
    dec count+2
    jmp loop

exit:
    clc
    rts

count:
    .byt 0,0,0
.endproc

;******************************************************************************
;Function name.......: clipboard_paste
;Purpose.............: Pastes clipboard memory into buffer; clipboard memory
;                      will be cleared
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc clipboard_paste
    ;Move pointer to start of clipboard mem
    lda #>clipboard_mem
    sta clipboard_pointer
    stz clipboard_pointer+1

loop:
    lda #<clipboard_mem
    sta TMP1_ADR
    lda clipboard_pointer
    sta TMP1_ADR+1

    ldy clipboard_pointer+1
    lda clipboard_pointer
    cmp clipboard_end
    bcc :+
    cpy clipboard_end+1
    bcs paste_done

:   lda (TMP1_ADR),y
    jsr cmd_insert

    inc clipboard_pointer+1
    bne loop
    inc clipboard_pointer
    jmp loop

paste_done:
    jmp clipboard_init

.endproc

clipboard_pointer:
    .byt 0,0

clipboard_end:
    .byt 0,0

clipboard_mem = mem_map+1024