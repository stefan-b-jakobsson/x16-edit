;******************************************************************************
;Copyright 2020-2023, Stefan Jakobsson.
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
;Purpose.............: Initializes and clears clipboard memory pointers
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc clipboard_init
    ;Pointer to end of clipboard content
    lda #>clipboard_mem         ;MSB
    sta clipboard_end
    stz clipboard_end+1         ;LSB

    ;Clear flags
    stz clipboard_flags

    rts
.endproc

;******************************************************************************
;Function name.......: clipboard_copy
;Purpose.............: Copy current line or active selection to clipboard
;Input...............: None
;Returns.............: Number of chars copied, in X:Y (low:high)
;Error returns.......: None
.proc clipboard_copy
    ;Check clipboard uncut flag, if set, clear clipboard
    lda clipboard_flags
    and #CLIPBOARD_UNCUT
    beq :+
    jsr clipboard_init

    ;Clear char counter
:   stz clipboard_count
    stz clipboard_count+1

    ;Setup pointer to clipboard memory
    lda clipboard_end+1
    sta TMP2_ADR
    lda clipboard_end
    sta TMP2_ADR+1

    ;Is there an active selection
    lda selection_active
    bne goto_selection
    
    ;Goto start of current line
    jsr cmd_go_home
    bra loop
    
    ;Goto selection start line
goto_selection:
    ldx selection_start_line
    ldy selection_start_line+1
    lda selection_start_line+2
    jsr cmd_goto_line

    ;Move cursor to selection start column
    ldx selection_start_col
    ldy selection_start_col+1
    lda selection_start_col+2
    jsr cmd_goto_col

loop:
    ldx selection_active
    beq :+
    
    sec
    lda mem_cur_line
    sbc selection_end_line
    lda mem_cur_line+1
    sbc selection_end_line+1
    lda mem_cur_line+2
    sbc selection_end_line+2
    bcc :+

    sec
    lda mem_cur_col
    sbc selection_end_col
    sta tempvars
    lda mem_cur_col+1
    sbc selection_end_col+1
    ora tempvars
    sta tempvars
    lda mem_cur_col+2
    sbc selection_end_col+2
    ora tempvars
    beq eol2
    bcs eol2
    
:   jsr mem_get_value
    ldx mem_start
    stx BNK_SEL
    sta (TMP2_ADR)

    ldx selection_active
    bne :+

    cmp #LF
    beq eol

:   jsr cmd_go_right
    cpx #0
    bne :+
    jsr incsize
    bra loop

:   ldx mem_start
    stx BNK_SEL
    lda #LF
    sta (TMP2_ADR)
    bra eol

incsize:
    inc clipboard_count
    bne :+
    inc clipboard_count+1

:   inc TMP2_ADR
    bne :+
    inc TMP2_ADR+1
:   ldx TMP2_ADR+1
    cpx #>clipboard_mem+CLIPBOARD_SIZE
    bcs full
    rts

eol:
    jsr incsize
    jsr cmd_go_right

eol2:
    clc
    lda clipboard_count
    adc clipboard_end+1
    sta clipboard_end+1
    lda clipboard_count+1
    adc clipboard_end
    sta clipboard_end

    ldx clipboard_count
    ldy clipboard_count+1
    clc
    rts

full:
    plx
    plx
    ldx #0
    ldy #0
    sec
    rts
.endproc

;******************************************************************************
;Function name.......: clipboard_cut
;Purpose.............: Cuts current line into clipboard
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: C=1: Clipboard mem full
.proc clipboard_cut
    ;Copy line, return with C=1 if clipboard mem is full
    jsr clipboard_copy
    bcs mem_full

    lda clipboard_count
    lda clipboard_count+1

:   lda clipboard_count
    ora clipboard_count+1
    beq exit
    jsr cmd_delete
    lda clipboard_count
    bne :+
    dec clipboard_count+1
:   dec clipboard_count
    bra :--

exit:
    clc
    rts

mem_full:
    rts             ;No need to set C=1 if we reach this, it's already set!
.endproc

;******************************************************************************
;Function name.......: clipboard_paste
;Purpose.............: Pastes clipboard memory into buffer
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc clipboard_paste
    ;Ensure auto-indent is disabled
    lda cmd_auto_indent_status
    pha
    stz cmd_auto_indent_status
    
    ;Set vector to start of clipboard memory
    stz TMP1_ADR
    lda #>clipboard_mem
    sta TMP1_ADR+1
    ldy #0

loop:
    ;Check if at end of clipboard content
    lda TMP1_ADR+1
    pha                     ;Need to store on stack, will be affected by subsequent call to cmd_insert
    cmp clipboard_end
    bcc :+
    cpy clipboard_end+1
    bcs exit

:   ;Get char from clipboard mem
    lda mem_start
    sta BNK_SEL
    lda (TMP1_ADR),y
    
    ;Insert char into text buffer
    phy                     ;Need to store on stack, will be affected by call to cmd_insert
    jsr cmd_insert
    ply                     ;Restore Y
    pla                     ;Restore TMP1_ADR
    sta TMP1_ADR+1
    stz TMP1_ADR

    iny
    bne loop

    inc TMP1_ADR+1
    bra loop

exit:
    ;We're done. Set uncut flag
    pla                     ;Clean stack
    lda clipboard_flags
    ora #CLIPBOARD_UNCUT
    sta clipboard_flags

    ;Restore auto-indent status
    pla
    sta cmd_auto_indent_status
    
    rts
.endproc

.segment "VARS"
    clipboard_end:          .res 2
    clipboard_flags:        .res 1
    clipboard_count:        .res 2
.CODE
.CODE

CLIPBOARD_SIZE = 12             ;In pages each of 256 bytes
CLIPBOARD_UNCUT = %00000001