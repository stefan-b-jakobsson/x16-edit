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

    rts

msg:
    .byt "disk status: ",0
.endproc

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
    
    ;Append ",s,w" to file name
    stx TMP1_ADR
    sty TMP1_ADR+1
    
    phy
    
    tay
    lda #','
    sta (TMP1_ADR),y
    iny
    lda #'s'
    sta (TMP1_ADR),y
    iny
    lda #','
    sta (TMP1_ADR),y
    iny
    lda #'w'
    sta (TMP1_ADR),y
    iny
    
    tya
    ply

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
    ldy #3
    bridge_call KERNAL_SETLFS
    bcc :+
    bra io_err

:   bridge_setaddr KERNAL_OPEN
    bridge_call KERNAL_OPEN
    bcs io_err

    bridge_setaddr KERNAL_CHKOUT
    ldx #1
    bridge_call KERNAL_CHKOUT
    bcs io_err

    ;Init temporary pointer to start of text; head of buffer is bank=mem_start+1, addressH=$a0
    stz TMP1_ADR
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

    bridge_setaddr KERNAL_CHROUT
write_loop:

    cpy page_len
    beq get_next_page
    lda (TMP1_ADR),y
    
    ldx screen_mode     ;Convert LF to CR line breaks if PETSCII mode (screenmode>0)
    beq :+
    cmp #LF
    bne :+
    lda #CR

:   bridge_call KERNAL_CHROUT
    bcs io_err
    iny
    bra write_loop

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
    beq close_file          ;Next page = 0 means end of file
    
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

    ldy #0
    bra write_loop

io_err:
    sta file_io_err

close_file:
    bridge_setaddr KERNAL_CLOSE
    lda #1
    bridge_call KERNAL_CLOSE
    bridge_setaddr KERNAL_CLRCHN
    bridge_call KERNAL_CLRCHN

    rts

page_len = tempvars     ;1 byte

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

    ;Append ",s,r" to file name
    stx TMP1_ADR
    sty TMP1_ADR+1
    
    phy
    
    tay
    lda #','
    sta (TMP1_ADR),y
    iny
    lda #'s'
    sta (TMP1_ADR),y
    iny
    lda #','
    sta (TMP1_ADR),y
    iny
    lda #'r'
    sta (TMP1_ADR),y
    iny
    
    tya
    ply

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
    ldy #3
    bridge_call KERNAL_SETLFS

    bridge_setaddr KERNAL_OPEN
    bridge_call KERNAL_OPEN
    bcc :+
    jmp io_err

:   bridge_setaddr KERNAL_CHKIN
    ldx #1
    bridge_call KERNAL_CHKIN
    bcc :+
    jmp io_err

    ;Get one byte to see if there are any disk errors, in particular file not found
:   bridge_setaddr KERNAL_CHRIN
    bridge_call KERNAL_CHRIN
    sta first_char
    bridge_setaddr KERNAL_READST
    bridge_call KERNAL_READST
    beq continue
    jmp close

    ;Clear and initialize memory
continue:
    jsr mem_init
    lda #5
    sta CRS_ADR
        
    ;Store first char
    ldy #0
    lda first_char
    
    cmp #CR             ;Convert CR to LF line break
    bne :+
    lda #LF

:   sta (CRS_ADR),y
    
    lda #1
    sta curpage_len

read_loop:
    bridge_setaddr KERNAL_READST
    bridge_call KERNAL_READST
    bne eof
    bridge_setaddr KERNAL_CHRIN
    bridge_call KERNAL_CHRIN

    cmp #CR         ;Concert CR to LF line break
    bne :+
    lda #LF

:   ldy curpage_len
    cpy #251
    bcs alloc_new_page

    sta (CRS_ADR),y
    inc curpage_len
    bra read_loop

alloc_new_page:
    pha             ;Save char to insert on stack

    stz CRS_ADR

    jsr mem_alloc   ;Y=bank, X=page
    cpx #0          ;Mem full?
    beq mem_full
    sty newpage
    stx newpage+1

    lda CRS_BNK
    sta BNK_SEL

    tya
    ldy #2
    sta (CRS_ADR),y ;Next bank
    
    txa
    ldy #3
    sta (CRS_ADR),y ;Next page
    
    ldy #4
    lda curpage_len
    sta (CRS_ADR),y ;Next len
    
    lda newpage
    sta BNK_SEL
    sta CRS_BNK

    lda newpage+1
    sta CRS_ADR+1
    
    lda #5          ;Offset to skip mem page metadata
    sta CRS_ADR

    ldy #0
    
    pla             ;Insert the char that was read from memory
    sta (CRS_ADR),y
    
    ldy #1          ;Set page len = 1
    sty curpage_len

    bra read_loop

io_err:
    ;Set kernal error
    sta file_io_err
    bra close

eof:
    stz CRS_ADR
    
    ;Set last page len
    ldy #4
    lda curpage_len
    sta (CRS_ADR),y
      
    ;Set cursor mem pointer to start of text
    lda mem_start
    ina
    sta CRS_BNK

    lda #$a0
    sta CRS_ADR+1
    
    stz CRS_IDX
    
close:
    bridge_setaddr KERNAL_CLOSE
    lda #1
    bridge_call KERNAL_CLOSE

    bridge_setaddr KERNAL_CLRCHN
    bridge_call KERNAL_CLRCHN
    
    rts

mem_full:
    pla             ;Clean stack

    jsr eof

    lda #10         ;Set memory full error
    bra io_err      

.segment "VARS"
    first_char: .res 1
    curpage_len: .res 1
    newpage: .res 2 
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
;Input...............: Nothing
;Returns.............: Kernal I/O Error returned in variable file_io_err
;                      Disk status code returned in A
;                      Status message returned in variable file_disk_status
;Error returns.......: None
.proc file_disk_cmd
    ;Set default = no kernal error
    stz file_io_err

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
    bcs io_error

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
    sta file_disk_status+13, y
    cpy #113            ;Last index = 127 - 13 - 1, where 13 is length of ingress "disk status: " and 1 byte is reserved for end of string marker (a zero)
    beq eof
    inc index
    bra read_loop

io_error:
    sta file_io_err
    stz file_disk_status+13
    bra close

eof:
    iny
    lda #0              
    sta file_disk_status+13,y   ;Set end of string marker

close:
    bridge_setaddr KERNAL_CLOSE
    lda #15
    bridge_call KERNAL_CLOSE
    bridge_setaddr KERNAL_CLRCHN
    bridge_call KERNAL_CLRCHN

    ;Return value
    sec
    lda file_disk_status+13     ;Error code, second digit
    sbc #48
    asl                         ;Shift 4 steps left
    asl
    asl
    asl
    sta index                   ;Just reusing index for temp storage.

    sec
    lda file_disk_status+14     ;Error code, first digit
    sbc #48
    ora index                   ;Adding them together

    rts

index = tempvars        ;1 byte

.endproc

;******************************************************************************
;Function name.......: file_set_filename
;Purpose.............: Store current filename
;Input...............: Pointer to file name, X=LSB and Y=MSB
;                      Length of file name in A
;Returns.............: Nothing
;Error returns.......: None
.proc file_set_filename
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

;Kernal error codes and their meaning (source: C64 PRG, 1:st ed, 9:th printing, 1987, page 306)
file_ioerr_L:
    .byt <file_ioerr_1, <file_ioerr_2, <file_ioerr_3, <file_ioerr_4, <file_ioerr_5, <file_ioerr_6, <file_ioerr_7, <file_ioerr_8, <file_ioerr_9 , <file_ioerr_10
file_ioerr_H:
    .byt >file_ioerr_1, >file_ioerr_2, >file_ioerr_3, >file_ioerr_4, >file_ioerr_5, >file_ioerr_6, >file_ioerr_7, >file_ioerr_8, >file_ioerr_9, >file_ioerr_10
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
file_ioerr_10:
    .byt "memory full error", 0

.segment "VARS"
    file_cur_filename:      .res 256
    file_disk_status:       .res 127
    file_io_err:            .res 1              ;1 byte
    file_cur_filename_len:  .res 1              ;1 byte
    file_cur_device:        .res 1              ;1 byte
    file_format:            .res 1              ;1 byte
.CODE