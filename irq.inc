
;******************************************************************************
;Function name.......: irq_init
;Purpose.............: Initializes the interrupt handler
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
;Stack requirements..: None

.proc irq_init
    sei
    
    lda $0314
    sta irq_default_handler
    lda $0315
    sta irq_default_handler+1

    lda #<irq_handler
    sta $0314
    lda #>irq_handler
    sta $0315

    cli

    rts
.endproc

;******************************************************************************
;Function name.......: irq_handler
;Purpose.............: Interrupt handler called by the kernal, not to be called
;                      by the application. The irq handler is 
;Preparatory routines: None
;Input...............: None
;Returns.............: Nothing
;Error returns.......: None
;Stack requirements..: None
.proc irq_handler
    ;Read keyboard
    jsr KERNAL_SCNKEY
    jsr keyboard_read
exit:
    jmp (irq_default_handler)       ;Return control to kernal

.endproc

irq_default_handler:    .byt 0,0