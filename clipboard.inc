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
;Purpose.............: Initializes and clears clipboard memory
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
;Purpose.............: Inserts a char into clipboard memory
;Input...............: A=char
;Returns.............: Nothing
;Error returns.......: C=1 if clipboard mem is full
.proc clipboard_insert
    ;Save char in X until we need it
    tax

    ;Check if clipboard mem is full
    sec
    lda clipboard_pointer
    sbc #>clipboard_mem
    cmp #12                  ;Allow clipboard mem to be max 12x256 bytes = 3K
    bcc mem_not_full

mem_full:
    sec
    rts

mem_not_full:
    ;Set vector to clipboard mem
    lda #<clipboard_mem
    sta TMP1_ADR
    lda clipboard_pointer
    sta TMP1_ADR+1
    ldy clipboard_pointer+1

    ;Get char from X and save it in clipboard mem
    txa
    
    ldx BNK_SEL         ;Save current RAM bank on stack
    phx

    ldx mem_start
    stx BNK_SEL
    sta (TMP1_ADR),y
    
    plx                 ;Restore RAM bank
    stx BNK_SEL
    
    ;Increase clipboard pointer
    inc clipboard_pointer+1
    bne :+
    inc clipboard_pointer

    ;Increase clipboard end pointer
:   inc clipboard_end+1
    bne exit
    inc clipboard_end

exit:
    clc
    rts

.endproc

;******************************************************************************
;Function name.......: clipboard_copy
;Purpose.............: Copies current line to clipboard. The cursor will be
;                      moved to end of line. Users of this function are
;                      responsible for backing up cursor position and
;                      restore it if that's needed
;Input...............: None
;Returns.............: Number of chars copied, 24 bit value, X=low, Y=mid, A=high
;Error returns.......: C=1: Clipboard mem full
.proc clipboard_copy
    ;Clear char counter
    stz count
    stz count+1
    stz count+2

    ;Backup clipboard pointer on the stack, should we need to restore on mem full
    lda clipboard_pointer
    pha
    lda clipboard_pointer+1
    pha
    lda clipboard_end
    pha
    lda clipboard_end+1
    pha

    ;Move cursor to start of line
    jsr cmd_go_home

    ;Copy line loop
loop:
    ;Setup vectors
    lda CRS_BNK
    sta BNK_SEL
    ldy CRS_IDX
    lda #5
    sta CRS_ADR
    
    ;Get char from text and store it in clipboard mem
    lda (CRS_ADR),y
    stz CRS_ADR
    jsr clipboard_insert
    bcs mem_full

    ;Move cursor right, exit if at end of line (C=1) or EOF (X=1)
    jsr cmd_go_right
    bcs eol
    cpx #1
    beq eof

    ;Increase counter
    inc count
    bne :+
    inc count+1
    bne :+
    inc count+2

:   jmp loop

eol:
    ;End of line
    inc count
    bne :+
    inc count+1
    bne :+
    inc count+2
:   jmp exit

eof:
    ;At EOF, replace last char in clipboard mem with a LF
    lda #<clipboard_mem
    sta TMP1_ADR
    lda clipboard_end
    sta TMP1_ADR+1
    ldy clipboard_end+1
    dey
    cpy #$ff
    bne :+
    dec TMP1_ADR+1

:   lda mem_start
    sta BNK_SEL
    lda #LF
    sta (TMP1_ADR),y

exit:
    ;Restore stack
    pla
    pla
    pla
    pla

    ;Return values
    clc
    ldx count
    ldy count+1
    lda count+2
    rts

mem_full:
    ;Restore pointers
    pla
    sta clipboard_end+1
    pla
    sta clipboard_end
    pla
    sta clipboard_pointer+1
    pla
    sta clipboard_pointer

    ;Exit with C=1 to indicate mem full
    sec
    rts

.segment "VARS"
    count: .res 3
.CODE
.endproc

;******************************************************************************
;Function name.......: clipboard_cut
;Purpose.............: Cuts current line and copies it to clipboard
;Input...............: None
;Returns.............: Nothing
;Error returns.......: C=1: Clipboard mem full
.proc clipboard_cut
    ;Copy line, return with C=1 if clipboard mem is full
    jsr clipboard_copy
    bcc prepare
    rts             ;No need to set C=1 if we reach this, it's already set!

prepare:
    ;Set counter to number of chars copied
    stx count
    sty count+1
    sta count+2

loop:
    ;Loop to delete the line
    ;First check if we are done, i.e. that the number of copied chars have been deleted
    lda count
    bne delete
    lda count+1
    bne delete
    lda count+2
    beq exit

delete:
    ;Delete one char (from the end of the line)
    jsr cmd_delete
    
    ;Decrease counter
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

.segment "VARS"
    count: .res 3   ;3 bytes
.CODE

.endproc

;******************************************************************************
;Function name.......: clipboard_paste
;Purpose.............: Pastes clipboard memory into buffer; clipboard memory
;                      will be cleared when done
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc clipboard_paste
    ;Move pointer to start of clipboard mem
    lda #>clipboard_mem
    sta clipboard_pointer
    stz clipboard_pointer+1

loop:
    ;Setup vectors to clipboard mem
    lda #<clipboard_mem
    sta TMP1_ADR
    lda clipboard_pointer
    sta TMP1_ADR+1
    ldy clipboard_pointer+1
    
    ;Check if we are at end of clipboard mem, exit if so
    lda clipboard_pointer
    cmp clipboard_end
    bcc :+
    cpy clipboard_end+1
    bcs paste_done

    ;Otherwise we insert char into buffer
:   lda mem_start
    sta BNK_SEL
    lda (TMP1_ADR),y
    jsr cmd_insert

    ;Increase clipboard pointer, and start over
    inc clipboard_pointer+1
    bne loop
    inc clipboard_pointer
    jmp loop

paste_done:
    ;We're done. Clear clipboard memory and exit
    jmp clipboard_init

.endproc

.segment "VARS"
    clipboard_pointer:      .res 2              ;2 bytes
    clipboard_end:          .res 2              ;2 bytes
.CODE