;KERNAL JUMP TABLE
.define KERNAL_SETLFS $ffba
.define KERNAL_SETNAM $ffbd
.define KERNAL_OPEN $ffc0
.define KERNAL_CHKIN $ffc6
.define KERNAL_CHRIN $ffcf

.define KERNAL_SCNKEY $ff9f
.define KERNAL_GETIN $ffe4

;VERA
.define VERA_L $9f20
.define VERA_M $9f21
.define VERA_H $9f22

.define VERA_D0 $9f23
.define VERA_D1 $9f24

.define VERA_CTRL $9f25
.define VERA_VIDEO $9f29

;ZERO PAGE VECTORS

;Select RAM bank ($01-$FF, $00 reserved by Kernal)
.define BNK_SEL $00

;Pointer to memory location corresponding to the cursor
.define CRS_BNK $22  ;Memory bank ($01-$FF, $00 reserved by Kernal)
.define CRS_ADR $23  ;Memory page ($A0-$BF)
.define CRS_IDX $25  ;Byte index ($00-$FB)

;Pointer to first visible memory location of the currently edited line
.define DSP_BNK $26  ;Memory bank
.define DSP_ADR $27  ;Memory page
.define DSP_IDX $29  ;Byte index

;Temporary pointers to memory locations, used for miscellaneous purposes
.define TMP1_BNK $2a  ;Memory bank
.define TMP1_ADR $2b  ;Memory page
.define TMP1_IDX $2d  ;Byte index

.define TMP2_BNK $2e ;Memory bank
.define TMP2_ADR $2f ;Memory page
.define TMP2_IDX $31 ;Byte index