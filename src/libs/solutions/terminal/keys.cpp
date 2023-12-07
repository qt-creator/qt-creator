// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "keys.h"

namespace TerminalSolution {

VTermModifier qtModifierToVTerm(Qt::KeyboardModifiers mod)
{
    int ret = VTERM_MOD_NONE;

    if (mod & Qt::ShiftModifier)
        ret |= VTERM_MOD_SHIFT;

    if (mod & Qt::AltModifier)
        ret |= VTERM_MOD_ALT;

#ifdef Q_OS_DARWIN
    if (mod & Qt::MetaModifier)
        ret |= VTERM_MOD_CTRL;
#else
    if (mod & Qt::ControlModifier)
        ret |= VTERM_MOD_CTRL;
#endif

    return static_cast<VTermModifier>(ret);
}

VTermKey qtKeyToVTerm(Qt::Key key, bool keypad)
{
    if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
        return static_cast<VTermKey>(int(VTERM_KEY_FUNCTION_0) + key - Qt::Key_F1 + 1);

    switch (key) {
    case Qt::Key_Return:
        return VTERM_KEY_ENTER;
    case Qt::Key_Tab:
        return VTERM_KEY_TAB;
    case Qt::Key_Backspace:
        return VTERM_KEY_BACKSPACE;
    case Qt::Key_Escape:
        return VTERM_KEY_ESCAPE;
    case Qt::Key_Up:
        return VTERM_KEY_UP;
    case Qt::Key_Down:
        return VTERM_KEY_DOWN;
    case Qt::Key_Left:
        return VTERM_KEY_LEFT;
    case Qt::Key_Right:
        return VTERM_KEY_RIGHT;
    case Qt::Key_Insert:
        return VTERM_KEY_INS;
    case Qt::Key_Delete:
        return VTERM_KEY_DEL;
    case Qt::Key_Home:
        return VTERM_KEY_HOME;
    case Qt::Key_End:
        return VTERM_KEY_END;
    case Qt::Key_PageUp:
        return VTERM_KEY_PAGEUP;
    case Qt::Key_PageDown:
        return VTERM_KEY_PAGEDOWN;
    case Qt::Key_multiply:
        return keypad ? VTERM_KEY_KP_MULT : VTERM_KEY_NONE;
    case Qt::Key_Plus:
        return keypad ? VTERM_KEY_KP_PLUS : VTERM_KEY_NONE;
    case Qt::Key_Comma:
        return keypad ? VTERM_KEY_KP_COMMA : VTERM_KEY_NONE;
    case Qt::Key_Minus:
        return keypad ? VTERM_KEY_KP_MINUS : VTERM_KEY_NONE;
    case Qt::Key_Period:
        return keypad ? VTERM_KEY_KP_PERIOD : VTERM_KEY_NONE;
    case Qt::Key_Slash:
        return keypad ? VTERM_KEY_KP_DIVIDE : VTERM_KEY_NONE;
    case Qt::Key_Enter: {
        VTermKey enterKey = VTERM_KEY_KP_ENTER;

#ifdef Q_OS_WIN
        enterKey = VTERM_KEY_ENTER;
#endif

        return keypad ? enterKey : VTERM_KEY_NONE;
    }
    case Qt::Key_Equal:
        return keypad ? VTERM_KEY_KP_EQUAL : VTERM_KEY_NONE;
    default:
        return VTERM_KEY_NONE;
    }
}
} // namespace TerminalSolution
