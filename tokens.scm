; Copyright 2017, Jeffrey E. Bedard
; token code x assigns a self-incrementing serial number to the token
; case logging codes:
;	""	do not generate a case
;	l	log normally
;	n	do not log
;	s	generate a stub
(load "libjb/libconvert.scm")
(define serial 1000)
(define debug-tokens #f)
(define get-value (lambda (in_value)
	(if (boolean/and (equal? (string-ref in_value 0) #\x)
		(= 1 (string-length in_value)))
		(number->string (set! serial (1+ serial)))
		in_value)))
(define get-comment (lambda (line)
	(if (equal? line "") "" (string-append " // " line))))
(define make-case (lambda (name logging port)
	(if (not (equal? logging ""))
		(begin
		(if debug-tokens
			(display (string-append name ":" logging "\n")))
		(display (string-append name ":" logging "\n") port)))))
(define parse (lambda (in out case_out_port)
	(let* ((line (read-line in)))
	(if (not (eof-object? line))
		; Define the database file format:
		(let* ( (name (string-car line))
			(namecdr (string-cdr line))
			(value (string-car namecdr))
			(valuecdr (string-cdr namecdr))
			(logging (string-car valuecdr))
			(loggingcdr (string-cdr valuecdr))
			(comment (string-car loggingcdr))
			(commentcdr (string-cdr loggingcdr)))
			(display (string-append "\tJBXVT_TOKEN_" name
				" = " (get-value value) ","
				(get-comment comment) "\n") out)
			(make-case name logging case_out_port)
			(parse in out case_out_port))))))
(define convert_tokens (lambda (in_name out_name cases_out_name) (let*
	((guard "JBXVT_JBXVTTOKENINDEX")
	(i (open-input-file in_name))
	(o (open-output-file out_name))
	(c (open-output-file cases_out_name)))
	(begin-include guard o)
	(begin-enum-definition "JBXVTTokenIndex" o)
	(parse i o c)
	(end-c-definition o)
	(end-include guard o)
	(close-port i)
	(close-port o)
	(close-port c))))
(convert_tokens "tokens.txt" "JBXVTTokenIndex.h" "cases.txt")
(load "casegen.scm")
