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

;*****************************************************************************
;KERNAL VECTORS
;*****************************************************************************

;File management
.define KERNAL_CLALL $ffe7
.define KERNAL_CLOSE $ffc3
.define KERNAL_CLRCHN $ffcc 
.define KERNAL_SETLFS $ffba
.define KERNAL_SETNAM $ffbd
.define KERNAL_OPEN $ffc0
.define KERNAL_CHKIN $ffc6
.define KERNAL_CHKOUT $ffc9
.define KERNAL_CHROUT $ffd2
.define KERNAL_CHRIN $ffcf
.define KERNAL_READST $ffb7
.define KERNAL_MEMTOP $ff99

;Other
.define KERNAL_SCREEN_SET_CHARSET $ff62
.define KERNAL_GETIN $ffe4


;*****************************************************************************
;VERA VECTORS
;*****************************************************************************
.define VERA_L $9f20
.define VERA_M $9f21
.define VERA_H $9f22

.define VERA_D0 $9f23
.define VERA_D1 $9f24

.define VERA_CTRL $9f25
.define VERA_VIDEO $9f29

.define VERA_L0_CONFIG $9f2d
.define VERA_L0_TILEBASE $9f2f

.define VERA_L1_CONFIG $9f34
.define VERA_L1_TILEBASE $9f36

;*****************************************************************************
;ZERO PAGE VECTORS
;*****************************************************************************

;Select RAM bank
.define BNK_SEL $9f61

;Pointer to memory location: Cursor position
.define CRS_BNK $22  ;Bank
.define CRS_ADR $23  ;Address, most significant byte (MSB)
.define CRS_IDX $25  ;Index

;Pointer to memory location: First visible character of the currently edited line
.define LNV_BNK $26  ;Bank
.define LNV_ADR $27  ;Address MSB
.define LNV_IDX $29  ;Byte index

;Pointer to memory location: First visible character of the screen (top left)
.define SCR_BNK $2a  ;Bank
.define SCR_ADR $2b  ;Address MSB
.define SCR_IDX $2d  ;Index

;Temporary pointers to memory locations, used for miscellaneous (short-term) purposes
;Warning: Any routine may use these temp pointers at any time and for whatever reason
.define TMP1_BNK $2e  ;Bank
.define TMP1_ADR $2f  ;Address MSB
.define TMP1_IDX $31  ;Index

.define TMP2_BNK $32 ;Bank
.define TMP2_ADR $33 ;Address MSB
.define TMP2_IDX $35 ;Index

;Cursor screen position
.define CRS_X $36   ;Cursor screen column position
.define CRS_Y $37   ;Cursor screen row position

;App screen mode
.define APP_MOD $38 ;Application mode

;App quit
.define APP_QUIT $39 ;Used to signal program exit

;*****************************************************************************
;OTHER STUFF
;*****************************************************************************
.define LF 10       ;New line separator
.define SCREEN_WIDTH 80
.define SCREEN_HEIGHT 54