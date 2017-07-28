#!/usr/bin/awk -f
BEGIN { FS=":"; print "// Copyright 2017, Jeffrey E. Bedard"; }
/:/ { printf("\tcase %d:\n\t\tLOG(\"%s--%s\");" \
	"\n\t\tm->%s = is_set;\n\t\tbreak;\n", $1, toupper($2), $3, $2);}
