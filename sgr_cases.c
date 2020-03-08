    case 1: // Bold font
        jbxvt_add_rstyle(JBXVT_RS_BOLD);
        break;
    case 2:
        sgrc(250, true);
        break;
    case 3: // Italic font
        jbxvt_add_rstyle(JBXVT_RS_ITALIC);
        break;
    case 4: // Underline
        jbxvt_add_rstyle(JBXVT_RS_UNDERLINE);
        break;
    case 5: // Blinking
        jbxvt_add_rstyle(JBXVT_RS_BLINK);
        break;
    case 6: // Substituted for rapidly blinking
        jbxvt_add_rstyle(JBXVT_RS_BLINK);
        break;
    case 7: // Image negative
        jbxvt_add_rstyle(JBXVT_RS_RVID);
        break;
    case 8: // Invisible text
        jbxvt_add_rstyle(JBXVT_RS_INVISIBLE);
        break;
    case 9: // Crossed out
        jbxvt_add_rstyle(JBXVT_RS_CROSSED_OUT);
        break;
    case 17: // Alternative font
        jbxvt_add_rstyle(JBXVT_RS_BOLD);
        break;
    case 21: // Doubly Underlined
        jbxvt_add_rstyle(JBXVT_RS_DOUBLE_UNDERLINE);
        break;
    case 23: // Not italic
        jbxvt_del_rstyle(JBXVT_RS_ITALIC);
        break;
    case 27: // Image positive (rvid off)
        jbxvt_del_rstyle(JBXVT_RS_RVID);
        break;
    case 28: // Not invisible
        jbxvt_del_rstyle(JBXVT_RS_INVISIBLE);
        break;
    case 29: // Not crossed out
        jbxvt_del_rstyle(JBXVT_RS_CROSSED_OUT);
        break;
    case 30:
        sgrc(0, true);
        break;
    case 31:
        sgrc(011, true);
        break;
    case 32:
        sgrc(012, true);
        break;
    case 33:
        sgrc(013, true);
        break;
    case 34:
        sgrc(014, true);
        break;
    case 35:
        sgrc(015, true);
        break;
    case 36:
        sgrc(016, true);
        break;
    case 37:
        sgrc(017, true);
        break;
    case 39:
        sgrc(017, true);
        break;
    case 40:
        sgrc(0, false);
        break;
    case 41:
        sgrc(011, false);
        break;
    case 42:
        sgrc(012, false);
        break;
    case 43:
        sgrc(013, false);
        break;
    case 44:
        sgrc(014, false);
        break;
    case 45:
        sgrc(015, false);
        break;
    case 46:
        sgrc(016, false);
        break;
    case 47:
        sgrc(017, false);
        break;
    case 49:
        sgrc(0, false);
        break;
    case 90:
        sgrc(0, true);
        break;
    case 91:
        sgrc(011, true);
        break;
    case 92:
        sgrc(012, true);
        break;
    case 93:
        sgrc(013, true);
        break;
    case 94:
        sgrc(014, true);
        break;
    case 95:
        sgrc(015, true);
        break;
    case 96:
        sgrc(016, true);
        break;
    case 97:
        sgrc(017, true);
        break;
    case 100:
        sgrc(010, false);
        break;
    case 101:
        sgrc(011, false);
        break;
    case 102:
        sgrc(012, false);
        break;
    case 103:
        sgrc(013, false);
        break;
    case 104:
        sgrc(014, false);
        break;
    case 105:
        sgrc(015, false);
        break;
    case 106:
        sgrc(016, false);
        break;
    case 107:
        sgrc(017, false);
        break;
