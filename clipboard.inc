;*******************************************************************************
;Copyright 2022-2024, Stefan Jakobsson
;
;Redistribution and use in source and binary forms, with or without modification, 
;are permitted provided that the following conditions are met:
;
;1. Redistributions of source code must retain the above copyright notice, this 
;   list of conditions and the following disclaimer.
;
;2. Redistributions in binary form must reproduce the above copyright notice, 
;   this list of conditions and the following disclaimer in the documentation 
;   and/or other materials provided with the distribution.
;
;THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” 
;AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
;IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
;DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
;FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
;DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
;SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
;CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
;OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
;OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;*******************************************************************************

CLIPBOARD_UNCUT = %00000001

;******************************************************************************
;Function name.......: clipboard_init
;Purpose.............: Initializes clipboard memory pointers
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc clipboard_init
    ;Null pointer
    stz clipboard_head_page ; head page = NULL

    ;Clear flags
    stz clipboard_flags

    rts
.endproc

;******************************************************************************
;Function name.......: clipboard_has_data
;Purpose.............: Query if the clipboard has data or not
;Input...............: None
;Returns.............: C=0 if there is data in the clipboard else C=1
;Error returns.......: None
.proc clipboard_has_data
    clc
    lda clipboard_head_page
    bne :+
    sec
:   rts
.endproc

;******************************************************************************
;Function name.......: clipboard_clear
;Purpose.............: Clears clipboard memory
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc clipboard_clear
    ldy clipboard_head_bank
    ldx clipboard_head_page
    beq exit ; Clipboard already cleared, do nothing
    jsr mem_free_array
exit:
    stz clipboard_head_page
    stz clipboard_flags
    rts
.endproc

;******************************************************************************
;Function name.......: clipboard_copy
;Purpose.............: Copy current line or active selection to clipboard
;Input...............: None
;Returns.............: Number of chars copied, in X:Y (low:high)
;Error returns.......: C=1 on memory full
.proc clipboard_copy
    ;Check clipboard uncut flag: If set, clear clipboard
    lda clipboard_flags
    and #CLIPBOARD_UNCUT
    beq :+
    jsr clipboard_clear

    ;Clear copied char counter
:   stz clipboard_count
    stz clipboard_count+1

    ;Allocate first memory page if clipboard memory is NULL
    lda clipboard_head_page ; page = NULL?
    bne :+ ; No, contiune

    ldx #0 ; head of list
    jsr mem_alloc
    sty clipboard_head_bank
    sty clipboard_tail_bank
    stx clipboard_head_page
    stx clipboard_tail_page

    ;Is there an active selection?
:   lda selection_active
    bne goto_selection ; yes
    
    ;Goto start of current line
    jsr cmd_go_home
    jsr mem_at_eof
    bcc loop
    cpy #1
    bne loop
    jmp eol2
    
goto_selection:
    ;Goto selection start line
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
    ; Is there an active selection?
    ldx selection_active
    beq :++ ; No
    
    ; Check if current line < line where selection ends
    sec
    lda mem_cur_line
    sbc selection_end_line
    sta tempvars
    lda mem_cur_line+1
    sbc selection_end_line+1
    ora tempvars
    lda mem_cur_line+2
    sbc selection_end_line+2
    ora tempvars
    beq :+ ; current line = selection line end, check column
    bcc :++ ; current line < selection line end, continue
    bra eol2 ; we're done

    ; Check if current col <= column where selection ends
:   sec
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
    bcs eol2 ; greater than -> exit
    
:   ; Get char at current location
    jsr mem_get_value

    ; Append char to end of clipboard memory
    ldy clipboard_tail_bank
    ldx clipboard_tail_page
    jsr mem_append
    sty clipboard_tail_bank
    stx clipboard_tail_page

    ; Update counter
    jsr update_count

    ; Is there an active selection?
    ldx selection_active
    bne :+ ; yes

    ; Not an active selection: Are we at a line break?
    cmp #LF
    beq eol ; yes

:   ; Move cursor right, check if end of file
    jsr cmd_go_right
    jsr mem_at_eof
    cpy #1
    bne loop

eof:   
    ; End of file, append a line break to the clipboard, do not update counter as nothing was really copied
    ldx clipboard_tail_page
    ldy clipboard_tail_bank
    lda #LF
    jsr mem_append
    stx clipboard_tail_page
    sty clipboard_tail_bank
    bra eol2

eol:
    jsr cmd_go_right

eol2:
    ldx clipboard_count
    ldy clipboard_count+1
    clc
    rts

update_count:
    inc clipboard_count
    bne :+
    inc clipboard_count+1
:   rts

.endproc

;******************************************************************************
;Function name.......: clipboard_paste
;Purpose.............: Pastes clipboard memory into buffer
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc clipboard_paste
    index = clipboard_count

    ;Ensure auto-indent is disabled
    lda cmd_auto_indent_status
    pha
    stz cmd_auto_indent_status
    
    ;Set vector to start of clipboard memory
    lda clipboard_head_bank
    sta BNK_SEL
    lda clipboard_head_page
    beq exit ; page is NULL, clipboard empty
    sta TMP1_ADR+1
    stz index

loop:    
    ;Check if at end of clipboard
    stz TMP1_ADR
    ldy #4
    lda (TMP1_ADR),y
    beq :+ ; page is empty
    dec
    cmp index
    bcs :++ ; not at end of page

:   stz index
    ldy #2
    lda (TMP1_ADR),y ; next bank
    tax
    iny
    lda (TMP1_ADR),y ; next page
    beq exit ; next page is NULL, end of clipboard
    sta TMP1_ADR+1
    stx BNK_SEL
    bra loop

    ; Get char from clipboard
:   ldy #5
    sty TMP1_ADR
    ldy index
    lda (TMP1_ADR),y
    
    ;Insert char into text buffer
    ldx TMP1_ADR+1          ;Store on stack, affected by cmd_insert
    phx
    ldx BNK_SEL
    phx
    jsr cmd_insert
    pla
    sta BNK_SEL
    pla                     ;Restore TMP1_ADR
    sta TMP1_ADR+1

    inc index
    bra loop

exit:
    ;We're done. Set uncut flag
    lda clipboard_flags
    ora #CLIPBOARD_UNCUT
    sta clipboard_flags

    ;Restore auto-indent status
    pla
    sta cmd_auto_indent_status
    
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

    stz selection_active
    stx clipboard_count
    sty clipboard_count+1

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

.segment "VARS"
    clipboard_head_bank: .res 1
    clipboard_head_page: .res 1
    clipboard_tail_bank: .res 1
    clipboard_tail_page: .res 1
    clipboard_flags: .res 1
    clipboard_count: .res 2
.CODE
