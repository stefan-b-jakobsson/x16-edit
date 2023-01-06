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

bridge_call = bridge_code
bridge_addr = bridge_call+3

.if target_mem=target_rom
    .macro bridge_setaddr addr
        lda #<addr
        sta bridge_addr
        lda #>addr
        sta bridge_addr+1
    .endmacro
.else
    .macro bridge_setaddr addr
        ;Do nothing
    .endmacro
.endif

.if target_mem=target_ram
    .macro bridge_call addr
        jsr addr
    .endmacro
.else
    .macro bridge_call addr
        jsr bridge_call
    .endmacro
.endif