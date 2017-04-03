#!/bin/sh
# Copyright 2017, Jeffrey E. Bedard
NAMES='ALN CHA CHT CPL CNL CUB CUD CUF CUP CUU DA DCH DL DSR DWL ECH'
NAMES="$NAMES ED EL ELR ENTGM52 EXTGM52 HOME HPA HPR HTS HVP ICH"
NAMES="$NAMES ID IL IND LL MC NEL PAM PM PNM RC REQTPARAM RESET RI"
NAMES="$NAMES RIS RQM S7C1T S8C1T SAVEPM SET"
STUBS='APC DHLT DHLB EPA OSC SOS SPA'
OUT=cases.c
rm -f $OUT
for NAME in $NAMES; do
	NAME="JBXVT_TOKEN_${NAME}"
	echo "case $NAME:\n\tLOG(\"$NAME\");" >> $OUT
	echo "\thandle_${NAME}(xc, &token);\n\tbreak;" >> $OUT
done
for STUB in $STUBS; do
	STUB="JBXVT_TOKEN_${STUB}"
	echo "case $STUB:" >> $OUT
	echo "\tLOG(\"FIXME: $STUB not implemented\");\n\tbreak;" >> $OUT
done
