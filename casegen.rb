#!/usr/bin/env ruby
out="cases.c"
File.delete(out)
file = File.new(out, "w")
tokens=%w(ALN CHA CHT CPL CNL CS_ALT_G1 CS_ALT_G2 CS_ALT_G3 CS_G0 CS_G1 CS_G2 CS_G3 CUB CUD CUF CUP CUU DA DCH DL DSR DWL ECH ED EL ELR ENTGM52 EXTGM52 HOME HPA HPR HTS HVP ICH ID IL IND LL MC NEL PAM PM PNM RC REQTPARAM RESET RI RIS RQM S7C1T S8C1T SAVEPM SBGOTO SBSWITCH SC SD SET SELINSRT SGR SS2 SS3 ST STBM SU SWL TBC TXTPAR VPA VPR)
nolog_tokens=%w(CHAR STRING)
stubs=%w(APC DHLT DHLB EPA OSC SOS SPA)
tokens.each do |token|
	token = "JBXVT_TOKEN_#{token}"
	file.write "case #{token}:\n\tLOG(\"#{token}\");\n" +
		"\tjbxvt_handle_#{token}(xc, &token);\n\tbreak;\n"
end
nolog_tokens.each do |token|
	token = "JBXVT_TOKEN_#{token}"
	file.write "case #{token}:\n" +
		"\tjbxvt_handle_#{token}(xc, &token);\n\tbreak;\n"
end
stubs.each do |token|
	token = "JBXVT_TOKEN_#{token}"
	file.write "case #{token}:\n" +
		"\tLOG(\"FIXME: #{token} not implemented\");\n\tbreak;\n" 
end

