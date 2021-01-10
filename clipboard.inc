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

CLIPBOARD_SIZE = 12     ;In pages each of 256 bytes

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
;Returns.............: C=1 if clipboard mem is full
;Error returns.......: None
.proc clipboard_insert
    ;Save char in X until we need it
    tax

    ;Check if clipboard mem is full
    sec
    lda clipboard_pointer
    sbc #>clipboard_mem
    cmp #CLIPBOARD_SIZE
    bcc mem_not_full

mem_full:
    sec
    rts

mem_not_full:
    ;Prepare to insert char in clipboard mem
    lda #<clipboard_mem
    sta TMP1_ADR
    lda clipboard_pointer
    sta TMP1_ADR+1
    ldy clipboard_pointer+1
    
    ;Save current RAM bank on stack
    lda BNK_SEL
    pha

    ;Select RAM bank stack for clipboard mem
    lda mem_start
    sta BNK_SEL

    ;Store char
    txa
    sta (TMP1_ADR),y
    
    pla                 ;Restore RAM bank
    sta BNK_SEL
    
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

    ;Copy line
loop:
    ;Prepare reading a char from the text buffer
    lda CRS_BNK
    sta BNK_SEL
    ldy CRS_IDX
    lda #5
    sta CRS_ADR
    
    ;Get char from buffer and push it on the stack
    lda (CRS_ADR),y
    pha
    stz CRS_ADR

    ;Move cursor right, exit if at end of line (C=1) or EOF (X=1)
    jsr cmd_go_right
    bcs eol
    cpx #1
    beq eol

    ;Insert char
    pla
    jsr clipboard_insert
    bcs full

    ;Increase char counter
    inc count
    bne :+
    inc count+1
    bne :+
    inc count+2

:   jmp loop

eol:
    ;Pull last char from the stack, we will not use it though
    pla

    ;Insert LF
    lda #LF
    jsr clipboard_insert

    ;Increase char counter
    inc count
    bne exit
    inc count+1
    bne exit
    inc count+2

exit:
    ;Clean stack
    pla
    pla
    pla
    pla

    ;Set return values
    ldx count
    ldy count+1
    lda count+2
    clc             ;C=0, mem not full
    rts

full:
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
    bcc prepare_delete

mem_full:
    rts             ;No need to set C=1 if we reach this, it's already set!

prepare_delete:
    ;Set counter to number of chars copied
    stx count
    sty count+1
    sta count+2

;Loop to delete the line
loop:
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
    lda count
    bne :++
    lda count+1
    bne :+
    dec count+2
:   dec count+1
:   dec count
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