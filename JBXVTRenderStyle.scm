; Copyright 2017, Jeffrey E. Bedard
(load "libjb/libconvert.scm")
(define parse (lambda (i o)
	(define parse-internal (lambda (n)
		(and-let* ((line (read-line i)) ((not (eof-object? line))))
		(if (< 0 (string-length line))
			; # at position 0 starts a comment
			(if (char=? #\# (string-ref line 0))
			(begin
				(display (string-append "\t//"
				(string-tail line 1) "\n") o)
				(set! n (- n 1)))
			(display (string-append "\tJBXVT_RS_"
				(string-upcase line) " = (1 << "
				(number->string n) "),\n") o)))
		(parse-internal (1+ n)))))
	(parse-internal 0)))
(define convert (lambda (input-filename output-filename)
		 (let ((i (open-input-file input-filename))
		       (o (open-output-file output-filename))
		       (tag "JBXVT_JBXVTRENDERSTYLE_H"))
		  (begin-include tag o)
		  (begin-enum-definition "JBXVTRenderStyle" o)
		  (parse i o)
		  (end-c-definition o)
		  (end-include tag o)
		  (close-port i)
		  (close-port o))))

(convert "JBXVTRenderStyle.txt" "JBXVTRenderStyle.h")
