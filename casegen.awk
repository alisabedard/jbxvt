#!/usr/bin/env awk -f
BEGIN{
    FS=":"
}
{
    if($2=="s"){
        print("case JBXVT_TOKEN_" $1 ":\n    "\
            "LOG(\"FIXME: JBXVT_TOKEN_" $1 " not implemented.\");\n"\
            "    break;");
    }else{
        tok="JBXVT_TOKEN_" $1
        dbg=$2=="l"?"    LOG(\"" tok "\");\n":"";
        cmd="    jbxvt_handle_" tok "(xc, &token);";
        printf("case %s:\n%s%s\n    break;\n",tok,dbg,cmd);
    }
}
