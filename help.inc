;******************************************************************************
;Function name.......: help_decompress
;Purpose.............: Decompresses the lzsa compressed help text.
;                      The compressed binary should be created with the lzsa
;                      utility that you may get here:
;                      https://github.com/emmanuel-marty/lzsa
;                      Compress with the following command:
;                      lzsa -r -f2 help.txt helptxt.bin
;                      The decompressed text is stored in banked RAM.
;Input...............: Nothing
;Returns.............: Nothing
;Error returns.......: None
.proc help_decompress
    lda mem_start       ;Select first RAM bank used by the program
    sta BNK_SEL

    lda #<help_bin      ;Vector to compressed text (input)
    sta r0
    lda #>help_bin
    sta r0+1

    lda #<help_txt      ;Vector to decompressed text (output)
    sta r1
    lda #>help_txt
    sta r1+1

    jmp KERNAL_DECOMPRESS

help_bin: 
    .incbin "helptxt.bin"

.endproc