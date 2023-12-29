;*******************************************************************************
;Copyright 2022-2023, Stefan Jakobsson
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

LINEBREAK_LF = 0
LINEBREAK_CR = 1
LINEBREAK_CRLF = 2

;******************************************************************************
;Function name.......: file_init
;Purpose.............: Initializes the file functions, and clears the current 
;                      file name
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: Nothing
.proc file_init
    ;Init disk status message
    ldx #0
:   lda msg,x
    beq :+
    sta file_disk_status,x
    inx
    bra :-

    ;Reset file name
:   stz file_cur_filename_len
    
    ;Set default device 8
    lda #8
    sta file_cur_device

    ;Set dir changed = false
    stz file_dir_changed

    rts

msg:
    .byt "drive status: ",0
.endproc

;******************************************************************************
;Function name.......: file_write
;Purpose.............: Writes current text to a file
;Input...............: Pointer to file name, X=LSB and Y=MSB
;                      Length of file name in A
;Returns.............: Nothing
;Error returns.......: Kernal I/O error code available in file_io_err
;Bank................: 2nd
.segment "CODE2"
.proc file_write
    ;Set default = no i/o error
    stz file_io_err

    ;Close file #1, and open file #1 for writing
    pha
    bridge_setaddr KERNAL_SETNAM
    pla
    bridge_call KERNAL_SETNAM

    bridge_setaddr KERNAL_CLOSE
    lda #1
    bridge_call KERNAL_CLOSE

    bridge_setaddr KERNAL_SETLFS
    lda #1
    ldx file_cur_device
    ldy #1
    bridge_call KERNAL_SETLFS

    jsr file_filename_copy

    bridge_setaddr KERNAL_OPEN
    bridge_call KERNAL_OPEN
    bcc :+
    jmp io_err

:   bridge_setaddr KERNAL_CHKOUT
    ldx #1
    bridge_call KERNAL_CHKOUT
    bcc :+
    jmp io_err

    ;Init temporary pointer to start of text; head of buffer is bank=mem_start+1, addressH=$a0
:   stz TMP1_ADR
    lda #$a0
    sta TMP1_ADR+1
    lda mem_start       
    ina
    sta BNK_SEL

    ;Len of first page
    ldy #4
    lda (TMP1_ADR),y
    sta page_len

    ;Offset to skip page metadata
    ldy #5
    sty TMP1_ADR
    
    ldy #0

loop:
    ;Check if page is full
    cpy page_len
    beq get_next_page
    lda (TMP1_ADR),y
    
    ;Check if line break
    cmp #LF
    bne :+
    
    ;Get line break encoding (0=LF, 1=CR, 2=CRLF)
    ldx file_linebreak_encoding
    beq :+ ;=LF

    lda #CR     ;Both other line break encodings start with CR
    cpx #1      ;=CR
    beq :+

    ;CRLF
    ldx index
    sta buf,x
    inc index
    lda #LF

    ;Store char in buffer
:   ldx index
    sta buf,x

    ;If buffer contains at least 254 chars, write out
    cpx #254
    bcc :+
    jsr writebuf
    bra loop
    
:   inc index
    iny
    bra loop

get_next_page:
    ;Reset to get page metadata
    stz TMP1_ADR

    ;Next bank, temp stored in X
    ldy #2
    lda (TMP1_ADR),y
    tax

    ;Next page
    ldy #3
    lda (TMP1_ADR),y
    beq eof                      ;Next page = 0 means (successfully) reached end of file
    
    ;Set pointer to next page
    sta TMP1_ADR+1
    stx BNK_SEL

    ;Get next page len
    ldy #4
    lda (TMP1_ADR),y
    sta page_len

    ;Set offset to skip page metadata again
    lda #5
    sta TMP1_ADR

    ;Prepare continue writing data
    ldy #0
    bra loop

eof:
    jsr writebuf
    bra close_file

io_err:
    sta file_io_err

close_file:
    bridge_setaddr KERNAL_CLOSE
    lda #1
    bridge_call KERNAL_CLOSE
    bridge_setaddr KERNAL_CLRCHN
    bridge_call KERNAL_CLRCHN
    
    jmp file_filename_restore

writebuf:
    phy
    bridge_setaddr KERNAL_MCIOUT
    ldx #<buf
    ldy #>buf
    lda index
    clc
    bridge_call KERNAL_MCIOUT
    bcs writebuf2 ;No MCIOUT support
    bra status

writebuf2:
    bridge_setaddr KERNAL_CHROUT
    ldx #0
:   lda buf,x
    bridge_call KERNAL_CHROUT
    inx
    cpx index
    bne :-

status:
    bridge_setaddr KERNAL_READST
    bridge_call KERNAL_READST
    ply
    cmp #0
    bne io_err
    stz index
    rts

page_len = tempvars     ;1 byte
index = tempvars+1
buf = file_cur_filename
.endproc
.CODE

;******************************************************************************
;Function name.......: file_read
;Purpose.............: Reads a file and replaces any current text in the editor
;Input...............: Pointer to file name, X=LSB and Y=MSB
;                      Length of file name in A
;Returns.............: Nothing
;Error returns.......: Kernal I/O error code available in file_io_err
;Bank................: 2nd
.segment "CODE2"
.proc file_read 
    ;Set default = no I/O error
    stz file_io_err

    ;Close file #1, and open file #1 for reading
    pha
    bridge_setaddr KERNAL_SETNAM
    pla
    bridge_call KERNAL_SETNAM

    bridge_setaddr KERNAL_CLOSE
    lda #1
    bridge_call KERNAL_CLOSE

    bridge_setaddr KERNAL_SETLFS
    lda #1
    ldx file_cur_device
    ldy #0
    bridge_call KERNAL_SETLFS

    jsr file_filename_copy

    bridge_setaddr KERNAL_OPEN
    bridge_call KERNAL_OPEN
    bcc :+
    jmp io_err

:   bridge_setaddr KERNAL_CHKIN
    ldx #1
    bridge_call KERNAL_CHKIN
    bcc :+
    jmp io_err

    ; Init
:   stz init_flag
    stz char

fill_buf:
    ;Try MACPTR
    bridge_setaddr KERNAL_MACPTR
    ldx #<buf
    ldy #>buf
    lda #$ff
    clc
    bridge_call KERNAL_MACPTR
    bcs fill_buf2 ;Block reading not supported, or file error
    stx buflen
    
    ;Read status
    bridge_setaddr KERNAL_READST
    bridge_call KERNAL_READST
    sta status
    bra prepare

fill_buf2:
    ;Fallback to using CHRIN
    ldx #0

:   cpx #$ff
    beq :+ ; Buffer full

    bridge_setaddr KERNAL_CHRIN
    bridge_call KERNAL_CHRIN
    sta buf,x
    
    bridge_setaddr KERNAL_READST
    bridge_call KERNAL_READST
    sta status
    bne prepare ; EOI or file error
    inx
    bra :-
:   stx buflen

prepare:
    ;Init text buffer if not done before
    lda init_flag
    bne :+
    inc init_flag
    call_firstbank mem_init
    stz curpage_len

:   ;Check file errors
    lda status
    and #%10000010
    beq :+
    stz CRS_ADR
    jmp close

:   ;Check empty buffer
    lda buflen
    bne :+
    jmp eof

:   stz index
    lda #5
    sta CRS_ADR
    
loop:
    ;Get next char from buffer
    ldx index
    lda buf,x

    ;Load previous char into X and remember the current char until next time
    ldx char
    sta char

    ;Handle line breaks
    cmp #CR                 ;Check if current char is CR
    bne :+
    lda #LINEBREAK_CR
    sta file_linebreak_encoding
    bra brk_insert

:   cpx #CR                 ;Check if previous char was CR
    bne :+
    
    cmp #LF                 ;Yes, check if current char is LF
    bne tab

    lda #LINEBREAK_CRLF     ;Line break is CRLF, ignore current char
    sta file_linebreak_encoding
    jmp movenext2

:   cmp #LF                 ;Previous char wasn't CR, check if current char is LF
    bne tab
    lda #LINEBREAK_LF
    sta file_linebreak_encoding

brk_insert:
    stz mem_cur_col
    stz mem_cur_col+1
    stz mem_cur_col+2
    lda #LF
    bra insert

tab:
    cmp #KEYVAL_TAB         ;Convert tab to blank spaces
    bne insert
    
    call_firstbank cmd_next_tab_stop
    stx tab_distance

:   ldy curpage_len
    cpy #251
    bcc :+
    jsr alloc

:   lda #32
    sta (CRS_ADR),y
    inc curpage_len
    
    dec tab_distance
    beq movenext

    call_firstbank mem_cur_col_inc
    bra :--

insert:
    ;Check if there is space left in the current page
    ldy curpage_len
    cpy #251
    bcc :+
    jsr alloc

:   ldy curpage_len
    sta (CRS_ADR),y
    inc curpage_len

movenext:
    call_firstbank mem_cur_col_inc

movenext2:
    inc index
    ldx index
    cpx buflen
    bcs :+
    jmp loop

:   lda status
    bne eof
    jmp fill_buf

io_err:
    ;Set kernal error
    sta file_io_err
    stz CRS_ADR
    bra close

eof:
    stz CRS_ADR
    
    ;Set last page len
    ldy #4
    lda curpage_len
    sta (CRS_ADR),y
      
    ;Set cursor mem pointer to start of text
goto_start:
    lda mem_start
    ina
    sta CRS_BNK

    lda #$a0
    sta CRS_ADR+1
    
    stz CRS_IDX

    stz mem_cur_col
    inc mem_cur_col
    stz mem_cur_col+1
    stz mem_cur_col+2

    stz mem_cur_line
    inc mem_cur_line
    stz mem_cur_line+1
    stz mem_cur_line+2
    
close:
    bridge_setaddr KERNAL_CLOSE
    lda #1
    bridge_call KERNAL_CLOSE

    bridge_setaddr KERNAL_CLRCHN
    bridge_call KERNAL_CLRCHN

    jmp file_filename_restore

mem_full:
    pla             ;Clean stack
    pla
    pla
    jsr eof
    lda #10         ;Set memory full error
    bra io_err

alloc:
    pha             ;Save char to insert on stack

    stz CRS_ADR

    lda CRS_BNK     ;Set current page length
    sta BNK_SEL
    ldy #4
    lda curpage_len
    sta (CRS_ADR),y

    call_firstbank mem_alloc   ;Y=bank, X=page
    cpx #0          ;Mem full?
    beq mem_full
    
    sty BNK_SEL     ;Select the newly allocated page
    sty CRS_BNK
    stx CRS_ADR+1
    
    lda #5          ;Offset to skip mem page metadata
    sta CRS_ADR

    ldy #0
    
    stz curpage_len ;Set new page len
    
    pla             ;Fetch char from stack
    rts

.segment "VARS"
    buf = file_cur_filename
    buflen: .res 1
    index: .res 1
    status: .res 1
    char: .res 1
    curpage_len: .res 1
    newpage: .res 2 
    tab_distance: .res 1
    init_flag: .res 1
.CODE
msg:
    .byt "memory full",0
.endproc

;******************************************************************************
;Function name.......: file_read_disk_status
;Purpose.............: Reads disk status channel
;Input...............: Nothing
;Returns.............: A=error code in decimal mode style
;                      Status message in file_disk_status
;Error returns.......: None
.proc file_read_disk_status
    lda #0
    ldx #0
    ldy #0
    bra file_disk_cmd
.endproc

;******************************************************************************
;Function name.......: file_disk_cmd
;Purpose.............: Invoke dos command
;Input...............: Pointer to command string, X=AddressL, Y=AddressH
;                      A=Command string length
;Returns.............: Kernal I/O Error returned in variable file_io_err
;                      Disk status code returned in A
;                      Status message returned in variable file_disk_status
;Error returns.......: None
.proc file_disk_cmd
    ;Set default = no kernal error
    stz file_io_err

    ;Check if change dir command
    stz is_cd

    sta len
    phy
    cmp #4
    bcc :++             ;Too short for CD command
    
    stx TMP1_ADR
    sty TMP1_ADR+1

    ldy #0
:   lda (TMP1_ADR),y
    and #%11011111      ;Convert upper/lower case
    cmp cd_cmd,y
    bne :+
    iny
    cpy #2
    bne :-

    inc is_cd

:   lda len
    ply

    ;Close file #15, and open file #15, secondary address 15
    pha
    bridge_setaddr KERNAL_SETNAM
    pla
    bridge_call KERNAL_SETNAM

    bridge_setaddr KERNAL_CLOSE
    lda #15
    bridge_call KERNAL_CLOSE

    bridge_setaddr KERNAL_SETLFS
    lda #15
    ldx file_cur_device
    ldy #15
    bridge_call KERNAL_SETLFS

    bridge_setaddr KERNAL_OPEN
    bridge_call KERNAL_OPEN
    bcs io_error

    bridge_setaddr KERNAL_CHKIN
    ldx #15
    bridge_call KERNAL_CHKIN
    bcs io_error

    ;Read status
    stz index
read_loop:
    bridge_setaddr KERNAL_READST
    bridge_call KERNAL_READST
    bne eof
    
    bridge_setaddr KERNAL_CHRIN
    bridge_call KERNAL_CHRIN
    
    cmp #$20
    bcc read_loop       ;Ignore control chars
    ldy index
    sta file_disk_status+14, y
    cpy #112            ;Last index = 127 - 14 - 1, where 14 is length of ingress "drive status: " and 1 byte is reserved for end of string marker (a zero)
    beq eof
    inc index
    bra read_loop

io_error:
    sta file_io_err
    stz file_disk_status+14
    bra close

eof:
    iny
    lda #0              
    sta file_disk_status+14,y   ;Set end of string marker

close:
    bridge_setaddr KERNAL_CLOSE
    lda #15
    bridge_call KERNAL_CLOSE
    bridge_setaddr KERNAL_CLRCHN
    bridge_call KERNAL_CLRCHN

    ;Return value
    sec
    lda file_disk_status+14     ;Error code, second digit
    sbc #48
    asl                         ;Shift 4 steps left
    asl
    asl
    asl
    sta index                   ;Just reusing index for temp storage.

    sec
    lda file_disk_status+15     ;Error code, first digit
    sbc #48
    ora index                   ;Adding them together

    ;Set dir_changed flag if successful directory change was made
    cmp #0
    bne :+                      ;Error => directory wasn't changed
    ldx is_cd                   ;Flag telling us if it was a CD command
    beq :+
    stx file_dir_changed

:   rts

cd_cmd:
    .byt "cd"

index = tempvars        ;1 byte
len = tempvars+1        ;1 byte
is_cd = tempvars+2      ;1 byte

.endproc

;******************************************************************************
;Function name.......: file_set_filename
;Purpose.............: Store current filename
;Input...............: Pointer to file name, X=LSB and Y=MSB
;                      Length of file name in A
;Returns.............: Nothing
;Error returns.......: None
.proc file_set_filename
    ;Clear dir changed flag
    stz file_dir_changed

    ;Copy file name
    stx TMP1_ADR
    sty TMP1_ADR+1
    
    sta file_cur_filename_len
    tay
    beq exit
    dey

:   lda (TMP1_ADR),y
    sta file_cur_filename,y
    dey
    cpy #$ff
    bne :-

exit:
    rts

.endproc

;******************************************************************************
;Function name.......: file_name_copy
;Purpose.............: Copies the file name buffer to banked RAM
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.segment "CODE2"
.proc file_filename_copy
    ; Select first RAM bank
    lda mem_start
    sta BNK_SEL

    ; Copy
    ldx file_cur_filename_len
    beq exit
loop:
    lda file_cur_filename-1,x
    sta filename_backup-1,x
    dex
    bne loop
exit:
    rts
.endproc
.CODE

;******************************************************************************
;Function name.......: file_name_restore
;Purpose.............: Restores the file name buffer from banked RAM
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.segment "CODE2"
.proc file_filename_restore
    ; Select first RAM bank
    lda mem_start
    sta BNK_SEL

    ; Copy
    ldx file_cur_filename_len
    beq exit
loop:
    lda filename_backup-1,x
    sta file_cur_filename-1,x
    dex
    bne loop

exit:
    rts
.endproc
.CODE


;Kernal error codes and their meaning (source: C64 PRG, 1:st ed, 9:th printing, 1987, page 306)
file_ioerr_L:
    .byt <file_ioerr_1, <file_ioerr_2, <file_ioerr_3, <file_ioerr_4, <file_ioerr_5, <file_ioerr_6, <file_ioerr_7, <file_ioerr_8, <file_ioerr_9 , <file_ioerr_10
file_ioerr_H:
    .byt >file_ioerr_1, >file_ioerr_2, >file_ioerr_3, >file_ioerr_4, >file_ioerr_5, >file_ioerr_6, >file_ioerr_7, >file_ioerr_8, >file_ioerr_9, >file_ioerr_10
file_ioerr_1:
    .byt "too many open files", 0
file_ioerr_2:
    .byt "file already open", 0
file_ioerr_3:
    .byt "file not open", 0
file_ioerr_4:
    .byt "file not found", 0
file_ioerr_5:
    .byt "device not present", 0
file_ioerr_6:
    .byt "file is not an input file", 0
file_ioerr_7:
    .byt "file is not an output file", 0
file_ioerr_8:
    .byt "file name is missing", 0
file_ioerr_9:
    .byt "illegal device number", 0
file_ioerr_10:
    .byt "memory full error", 0

.segment "VARS"
    file_cur_filename:      .res 256
    file_disk_status:       .res 127
    file_io_err:            .res 1              ;1 byte
    file_cur_filename_len:  .res 1              ;1 byte
    file_cur_device:        .res 1              ;1 byte
    file_dir_changed:       .res 1              ;1 byte, flag indicating whether directory was changed since current file was last read/saved
    file_linebreak_encoding: .res 1
.CODE