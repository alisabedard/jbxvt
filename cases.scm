(define copyright "Copyright 2017, Jeffrey E. Bedard")
; Written for mit-scheme
; vim: sw=2
(define what_type?
 (lambda (type) ; map chars to symbols
  (cond ((char=? type #\l) 'logged)
   ((char=? type #\n) 'unlogged)
   ((char=? type #\s) 'stub)
   (else 'bad))))
; format components:
(define get_case (lambda (token) (string-append "case " token ":\n")))
(define get_log (lambda (message) (string-append "\tLOG(\"" message "\");\n")))
(define get_fixme
 (lambda (token) (get_log (string-append
			   "FIXME: " token " not implemented."))))
(define get_handler
 (lambda (token)
  (string-append "\tjbxvt_handle_" token "(xc, &token);\n")))
(define get_formatted
 (lambda (token type) ; map symbols to formats
  (string-append (get_case token )
   (cond ((equal? type 'logged) (begin
				 (string-append (get_log token)
				  (get_handler token))))
    ((equal? type 'unlogged) (get_handler token))
    (else (get_fixme token))) "\tbreak;\n")))
(define get_token
 (lambda (line i) (string-append "JBXVT_TOKEN_" (string-head line i))))
(define get_type
 (lambda (line i) (what_type? (string-ref (string-tail line (+ 1 i)) 0))))
(define parse
 (lambda (in out)
  (and-let*
   ((line (read-line in))
    ((not (eof-object? line))))
   (let ((i (string-find-next-char line #\:)))
    (let ((token (get_token line i))
	  (type (get_type line i)))
     (display (get_formatted token type) out)
     (parse in out))))))
(define casegen
 (lambda (in_file_name out_file_name)
  (let*
   ((i (open-input-file in_file_name))
    (o (open-output-file out_file_name)))
   (parse i o) (close-port i) (close-port o))))
(casegen "cases.txt" "cases.c")
