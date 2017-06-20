; Copyright 2017, Jeffrey E. Bedard
(load "libconvert.scm")
(define serial 1000)
(define get-value (lambda (in_value)
	(if (boolean/and (equal? (string-ref in_value 0) #\x)
		(= 1 (string-length in_value)))
		(number->string (set! serial (1+ serial)))
		in_value)))
(define get-comment (lambda (line) (let
	((value (string-cdr (string-cdr line))))
	(if (equal? value "")
		""
		(string-append " // " value)))))
(define parse (lambda (in out)
	(let* ((line (read-line in)))
	(if (not (eof-object? line))
		(let ((value (string-car (string-cdr line)))
			(comment (string-cdr (string-cdr line))))
			(display (string-append "\tJBXVT_TOKEN_"
				(string-car line) " = "
				(get-value value) ","
				(if (equal? "" comment) ""
					(string-append " // " comment))
				"\n") out)
			(parse in out))))))
(define convert_tokens (lambda (in_name out_name) (let*
	((ig "JBXVTTOKENINDEX")
	(i (open-input-file in_name))
	(o (open-output-file out_name)))
	(set! guard-prefix "JBXVT_")
	(begin-include ig o)
	(begin-enum-definition "JBXVTTokenIndex" o)
	(parse i o)
	(end-c-definition o)
	(end-include ig o)
	(close-port i)
	(close-port o))))
(convert_tokens "tokens.txt" "JBXVTTokenIndex.h")
