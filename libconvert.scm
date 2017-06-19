(define copyright "// Copyright 2017, Jeffrey E. Bedard\n")
; The following prefix is applied to each entry
(define master-prefix "_NET_")
(define begin-array-definition (lambda (type name out)
	(display (string-append "static " type " " name " [] = {\n") out)))
(define begin-enum-definition (lambda (name out)
	(display (string-append "enum " name " {\n") out)))
(define end-c-definition (lambda (out) (display "};\n" out)))
(define get-array-line (lambda (prefix item)
	(string-append "\t\"" master-prefix prefix item "\",\n")))
(define print-each-array-element (lambda (prefix l)
	(map (lambda (item) (display (get-array-line prefix item))) l)))
(define get-enum-line (lambda (prefix item)
	(string-append "\t" master-prefix prefix item ",\n")))
(define print-enum-line (lambda (prefix item)
	(display (get-enum-line prefix item))))
(define print-each-enum (lambda (prefix l)
	(map (lambda (item) (print-enum-line prefix item)) l)))
(define guard-prefix "JBWM_")
(define get-guard (lambda (name) 
	(string-append guard-prefix name "_H\n")))
(define print-each (lambda (function data)
	(function (car data) (cdr data))))
(define add-c-include (lambda (name) (string-append "#include " name "\n")))
(define write-include-header (lambda (ig out)
	(display (string-append copyright
		"#ifndef " (get-guard ig)
		"#define " (get-guard ig)) out)))
(define write-include-fin (lambda (ig out) (display (string-append "#endif//!"
	(get-guard ig)) out)))
