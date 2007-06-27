	name	dos
	page	55,80
	title	'DOS.ASM -- assembly routines for the teeny-shell under DOS'

_TEXT	segment byte public 'CODE'

        assume  cs:_TEXT

public	_fail_criterr
;
; If we get a critical error, just fail it - dos 3.0 and up only, please!
;
_fail_criterr proc far
	mov	al, 3
	iret
_fail_criterr endp

public _ignore_ctrlc
;
; If the user presses ^C, don't do any special handling of it.
;
_ignore_ctrlc proc far
	iret
_ignore_ctrlc endp
_pexec	endp

public _intr_on_ctrlc
;
; If the user presses ^C, terminate the current process.
;
_intr_on_ctrlc proc far
	mov	ah, 4ch
	mov	al, 0ffh
	int	21h
_intr_on_ctrlc endp

public _crawcin
;
; get a character from standard input without any sort of magical
; processing.
;
_crawcin proc far
	mov	ah, 07h
	int	21h
	ret
_crawcin endp

_TEXT	ends

	end
