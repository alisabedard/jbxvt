// Copyright 2017, Jeffrey E. Bedard
    case 2:
        LOG("DECANM--VT52/ANSI mode");
        m->decanm = is_set;
        break;
    case 3:
        LOG("DECCOLM--80/132 column mode");
        m->deccolm = is_set;
        break;
    case 4:
        LOG("DECSCLM--slow scroll mode");
        m->decsclm = is_set;
        break;
    case 7:
        LOG("DECAWM--autowrap mode");
        m->decawm = is_set;
        break;
    case 9:
        LOG("MOUSE_X10--mouse tracking");
        m->mouse_x10 = is_set;
        break;
    case 12:
        LOG("ATT610--blinking cursor");
        m->att610 = is_set;
        break;
    case 18:
        LOG("DECPFF--print form feed mode");
        m->decpff = is_set;
        break;
    case 20:
        LOG("LNM--new line mode");
        m->lnm = is_set;
        break;
    case 40:
        LOG("ALLOW_DECCOLM--allow 132 column mode");
        m->allow_deccolm = is_set;
        break;
    case 45:
        LOG("DECAWM--autowrap mode");
        m->decawm = is_set;
        break;
    case 1000:
        LOG("MOUSE_VT200--mouse tracking");
        m->mouse_vt200 = is_set;
        break;
    case 1001:
        LOG("MOUSE_VT200HL--vt200 highlight mode");
        m->mouse_vt200hl = is_set;
        break;
    case 1002:
        LOG("MOUSE_BTN_EVT--mouse button event tracking");
        m->mouse_btn_evt = is_set;
        break;
    case 1003:
        LOG("MOUSE_ANY_EVT--track any mouse event");
        m->mouse_any_evt = is_set;
        break;
    case 1004:
        LOG("MOUSE_FOCUS_EVT--track mouse focus events");
        m->mouse_focus_evt = is_set;
        break;
    case 1005:
        LOG("MOUSE_EXT--utf-8 extended mouse mode");
        m->mouse_ext = is_set;
        break;
    case 1006:
        LOG("MOUSE_SGR--sgr extended mouse mode");
        m->mouse_sgr = is_set;
        break;
    case 1007:
        LOG("MOUSE_ALT_SCROLL--mouse alternate scroll");
        m->mouse_alt_scroll = is_set;
        break;
    case 1015:
        LOG("MOUSE_URXVT--urxvt extended mouse mode");
        m->mouse_urxvt = is_set;
        break;
    case 1034:
        LOG("S8C1T--turn on or off meta mode (8-bit mode)");
        m->s8c1t = is_set;
        break;
    case 2004:
        LOG("BPASTE--bracketed paste mode");
        m->bpaste = is_set;
        break;
