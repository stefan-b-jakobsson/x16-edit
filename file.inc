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
;Function name.......: file_write
;Purpose.............: Writes current text to a file
;Input...............: Pointer to file name, X=LSB and Y=MSB
;                      Length of file name in A
;Returns.............: Nothing
;Error returns.......: Kernal I/O error code available in file_io_err
.proc file_write
    ;Set default = no i/o error
    stz file_io_err
    
    ;Open 1, 8, 1, "@:filename, s, w"
    jsr KERNAL_SETNAM

    lda #1
    jsr KERNAL_CLOSE

    lda #1
    ldx #8
    ldy #1
    jsr KERNAL_SETLFS
    bcs io_err

    jsr KERNAL_OPEN
    bcs io_err

    ldx #1
    jsr KERNAL_CHKOUT
    bcs io_err

    ;Init temporary pointer to start of text
    stz TMP1_ADR
    lda #$a0
    sta TMP1_ADR+1
    lda #1
    sta BNK_SEL

    ;Len of first page
    ldy #4
    lda (TMP1_ADR),y
    sta page_len

    ;Offset to skip page metadata
    ldy #5
    sty TMP1_ADR
    ldy #0
    
write_loop:
    cpy page_len
    beq get_next_page
    lda (TMP1_ADR),y
    jsr KERNAL_CHROUT
    bcs io_err
    iny
    jmp write_loop

get_next_page:
    ;Reset to get page metadata
    stz TMP1_ADR

    ;Next bank
    ldy #2
    lda (TMP1_ADR),y
    tax

    ;Next page
    ldy #3
    lda (TMP1_ADR),y
    beq close_file          ;Next page = 0 means end of file
    
    ;Set pointer to next page
    sta TMP1_ADR+1
    stx BNK_SEL

    ;Get next page len
    ldy #4
    lda (TMP1_ADR),y
    sta page_len

    ;Offset to skip page metadata
    lda #5
    sta TMP1_ADR

    ldy #0
    jmp write_loop

io_err:
    sta file_io_err

close_file:
    lda #1
    jsr KERNAL_CLOSE
    jsr KERNAL_CLRCHN

    rts

page_len:
    .byt 0

.endproc

;******************************************************************************
;Function name.......: file_read
;Purpose.............: Reads a file and replaces any current text in the editor
;Input...............: Pointer to file name, X=LSB and Y=MSB
;                      Length of file name in A
;Returns.............: Nothing
;Error returns.......: Kernal I/O error code available in file_io_err
.proc file_read
    ;Set default = no I/O error
    stz file_io_err

    ;Open 1,8,2,"filename"
    jsr KERNAL_SETNAM

    lda #1
    jsr KERNAL_CLOSE

    lda #1
    ldx #8
    ldy #2
    jsr KERNAL_SETLFS

    jsr KERNAL_OPEN
    bcs io_err

    ldx #1
    jsr KERNAL_CHKIN
    bcs io_err

    ;Get one byte to see if there are any disk errors, in particular file not found
    jsr KERNAL_CHRIN
    sta first_char
    jsr KERNAL_READST
    beq continue
    jmp close

    ;Clear and initialize memory
continue:
    jsr mem_init
    lda #5
    sta CRS_ADR
        
    ;Store first char
    lda first_char
    ldy #0
    sta (CRS_ADR),y
    
    lda #1
    sta curpage_len

read_loop:
    jsr KERNAL_READST
    bne eof
    jsr KERNAL_CHRIN
    
    ldy curpage_len
    cpy #251
    bcs alloc_new_page
    sta (CRS_ADR),y
    inc curpage_len
    jmp read_loop

alloc_new_page:
    pha             ;Save char to insert on stack

    stz CRS_ADR

    jsr mem_alloc   ;Y=bank, X=page
    sty newpage
    stx newpage+1

    tya
    ldy #2
    sta (CRS_ADR),y
    
    txa
    ldy #3
    sta (CRS_ADR),y
    
    ldy #4
    lda curpage_len
    sta (CRS_ADR),y
    
    lda newpage
    sta BNK_SEL
    sta CRS_BNK

    lda newpage+1
    sta CRS_ADR+1
    
    lda #5
    sta CRS_ADR
    
    ldy #0
    pla
    sta (CRS_ADR),y
    lda #1
    sta curpage_len

    jmp read_loop

io_err:
    sta file_io_err
    jmp close

eof:
    stz CRS_ADR
    ldy #4
    lda curpage_len
    sta (CRS_ADR),y

    lda #1
    sta CRS_BNK
    sta LNV_BNK
    sta SCR_BNK

    lda #$a0
    sta CRS_ADR+1
    sta LNV_ADR+1
    sta SCR_ADR+1

    stz CRS_IDX
    stz LNV_IDX
    stz SCR_IDX

close:
    lda #1
    jsr KERNAL_CLOSE
    jsr KERNAL_CLRCHN
    
    rts

first_char:
    .byt 0

curpage_len:
    .byt 0

newpage:
    .byt 0,0

.endproc

;******************************************************************************
;Function name.......: file_read_disk_status
;Purpose.............: Reads disk status channel
;Input...............: Nothing
;Returns.............: Message returned in file_disk_status
;Error returns.......: None
.proc file_read_disk_status
    lda #0
    ldx #0
    ldy #0
    jsr KERNAL_SETNAM

    lda #15
    jsr KERNAL_CLOSE

    lda #15
    ldx #8
    ldy #15
    jsr KERNAL_SETLFS
    bcs io_error

    jsr KERNAL_OPEN
    bcs io_error

    ldx #15
    jsr KERNAL_CHKIN
    bcs io_error

    lda #13
    sta index
read_loop:
    jsr KERNAL_READST
    bne eof
    jsr KERNAL_CHRIN
    cmp #$20
    bcc read_loop       ;Ignore contron chars
    ldy index
    sta file_disk_status, y
    cpy #254
    beq eof
    inc index
    jmp read_loop

eof:
    iny
    lda #0
    sta file_disk_status,y

    stz file_io_err

    lda #15
    jsr KERNAL_CLOSE
    jsr KERNAL_CLRCHN

    rts

io_error:
    ldy #13
    lda #0
    sta file_disk_status,y
    rts

index:
    .byt 0

.endproc

file_disk_status:
    .byt "disk status: "
    .res 243, 0

;Kernal error codes and their meaning (source: C64 PRG, 1:st ed, 9:th printing, 1987, page 306)
file_io_err:
    .byt 0
file_ioerr_L:
    .byt <file_ioerr_1, <file_ioerr_2, <file_ioerr_3, <file_ioerr_4, <file_ioerr_5, <file_ioerr_6, <file_ioerr_7, <file_ioerr_8, <file_ioerr_9 
file_ioerr_H:
    .byt >file_ioerr_1, >file_ioerr_2, >file_ioerr_3, >file_ioerr_4, >file_ioerr_5, >file_ioerr_6, >file_ioerr_7, >file_ioerr_8, >file_ioerr_9
file_ioerr_1:
    .byt "i/o error: too many open files", 0
file_ioerr_2:
    .byt "i/o error: file already open", 0
file_ioerr_3:
    .byt "i/o error: file not open", 0
file_ioerr_4:
    .byt "i/o error: file not found", 0
file_ioerr_5:
    .byt "i/o error: device not present", 0
file_ioerr_6:
    .byt "i/o error: file is not an input file", 0
file_ioerr_7:
    .byt "i/o error: file is not an output file", 0
file_ioerr_8:
    .byt "i/o error: file name is missing", 0
file_ioerr_9:
    .byt "i/o error: illegal device number", 0
