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
;Function name.......: irq_init
;Purpose.............: Initializes the interrupt handler
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc irq_init
    sei
    
    ;Backup default interrupt handler
    lda $0314
    sta irq_default_handler
    lda $0315
    sta irq_default_handler+1

    ;Set vectors to our custom handler
    lda #<irq_handler
    sta $0314
    lda #>irq_handler
    sta $0315

    cli

    rts
.endproc

;******************************************************************************
;Function name.......: irq_handler
;Purpose.............: Custom interrupt handler
;Preparatory routines: irq_init
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
.proc irq_handler
    lda APP_QUIT
    bne shutdown

    jsr keyboard_read
    jsr cursor_toggle
exit:
    ;Return control to default handler
    jmp (irq_default_handler)

shutdown:
    sei

    ;Restore irq handler
    lda irq_default_handler
    sta $0314
    lda irq_default_handler+1
    sta $0315

    jsr cursor_hide

    ;Clear screen
    lda #147
    jsr KERNAL_PRINT

    ;Set app quit = 2 => signal to exit main loop
    lda #2
    sta APP_QUIT
    
    cli

    jmp (irq_default_handler)
.endproc

irq_default_handler:    .byt 0,0