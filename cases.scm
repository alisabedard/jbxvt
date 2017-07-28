(define copyright "Copyright 2017, Jeffrey E. Bedard")
; Written for mit-scheme
; vim: sw=2
(load-option 'format)
(define what_type?
 (lambda (type) ; map chars to symbols
  (cond ((char=? type #\l) 'logged)
   ((char=? type #\n) 'unlogged)
   ((char=? type #\s) 'stub)
   (else 'bad))))
; format components:
(define (get-case tkn) (format #f "case ~A:~%" tkn))
(define (get-log msg) (format #f "\tLOG(\"~A\");~%" msg))
(define (get-fixme tkn)
 (get-log (format #f "FIXME: ~A not implemented." tkn)))
(define (get-handler tkn) (format #f "\tjbxvt_handle_~A(xc, &token);~%" tkn))
(define get-formatted
 (lambda (token type) ; map symbols to formats
  (string-append (get-case token )
   (cond ((equal? type 'logged) (begin
				 (string-append (get-log token)
				  (get-handler token))))
    ((equal? type 'unlogged) (get-handler token))
    (else (get-fixme token))) "\tbreak;\n")))
(define get-token
 (lambda (line i) (string-append "JBXVT_TOKEN_" (string-head line i))))
(define get-type
 (lambda (line i) (what_type? (string-ref (string-tail line (+ 1 i)) 0))))
(define parse
 (lambda (in out)
  (and-let*
   ((line (read-line in))
    ((not (eof-object? line))))
   (let ((i (string-find-next-char line #\:)))
    (let ((token (get-token line i))
	  (type (get-type line i)))
     (display (get-formatted token type) out)
     (parse in out))))))
(define casegen
 (lambda (in_file_name out_file_name)
  (let*
   ((i (open-input-file in_file_name))
    (o (open-output-file out_file_name)))
   (parse i o)
   (close-port i)
   (close-port o))))
(casegen "cases.txt" "cases.c")
