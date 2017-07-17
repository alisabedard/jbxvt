; Copyright 2017, Jeffrey E. Bedard
(load "libjb/libconvert.scm")
(letrec
	((parse (lambda (in out) (let*
		((line (read-line in))
		(format-line (lambda (index value) (string-append
			"\t[" index "] = 0x" value ",\n"))))
		(if (eof-object? line) #f
			(begin (if (> (string-length line) 1)
				(let* ((i (string-find-next-char line #\:))
					(index (string-car line))
					(value (string-cdr line)))
					(display (format-line
						index value) out)
					(flush-output out)))
				(parse in out))))))
	(convert_colors (lambda (in_file_name out_file_name) (let
		((tag "JBXVT_COLOR_INDEX_H")
		(i (open-input-file in_file_name))
		(o (open-output-file out_file_name)))
		(begin-include tag o)
		(display "#include <stdint.h>\n" o)
		(begin-array-definition "uint32_t" "jbxvt_color_index" o)
		(parse i o)
		(end-c-definition o)
		(end-include tag o)
		(close-port i)
		(close-port o)))))
	(convert_colors "color_index.txt" "color_index.h"))
