; Copyright 2017, Jeffrey E. Bedard
(load "libjb/libconvert.scm")

(define format-value
 (lambda (value op)
  (let ((c (string-ref op 0)))
   (cond ((or (char=? c #\a) (char=? c #\d))
	  (string-append "JBXVT_RS_" value))
    (else value)))))

(define format-sgrc (lambda (comment op)
		     (if (char=? #\c (string-ref op 0))
		      (string-append ", " comment) "")))

(define format-comment (lambda (cmt op)
 (cond ((or (string=? op "c") (string=? cmt "")) "")
 (else (string-append " // " cmt)))))

(define get-op
 (lambda (op)
  (let ((c (string-ref op 0)))
   (cond
    ((char=? c #\a) "jbxvt_add_rstyle")
    ((char=? c #\c) "sgrc")
    ((char=? c #\d) "jbxvt_del_rstyle")
    (else "unknown")))))

(define format-line
 (lambda (case-id op value comment out-port)
    (display (string-append "\tcase " case-id ":"
    (format-comment comment op)
    "\n\t\t"
    (get-op op) "("
    (format-value value op)
    (format-sgrc comment op)
    ");\n\t\tbreak;\n") out-port)))

(define parse-sgr
 (lambda (i o)
  (and-let* ((line (read-line i))
	     ((not (eof-object? line))) ; validate
	     ; Define the database file format:
	     (case-id (string-car line))
	     (case-id-cdr (string-cdr line))
	     (op (string-car case-id-cdr))
	     (op-cdr (string-cdr case-id-cdr))
	     (value (string-car op-cdr))
	     (value-cdr (string-cdr op-cdr))
	     (comment (string-car value-cdr))
	     (comment-cdr (string-cdr value-cdr)))
   (if (> (string-length line) 0) ; not a blank line
    (format-line case-id op value comment o))
   (parse-sgr i o))))

(define convert-sgr 
 (lambda (in-filename out-filename)
  (let ((i (open-input-file in-filename))
	(o (open-output-file out-filename)))
   (parse-sgr i o)
   (close-port i)
   (close-port o))))

(convert-sgr "sgr.txt" "sgr_cases.c")
