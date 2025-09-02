// Copyright (C) 2019 The Qt Company Ltd.
// Copyright (C) 2019 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "camelcasecursor.h"

#include "multitextcursor.h"

#include <QLineEdit>

namespace Utils {
namespace {

template<typename C>
bool moveCursor(C *cursor, QTextCursor::MoveOperation direction, QTextCursor::MoveMode mode);

template<>
bool moveCursor(QTextCursor *cursor, QTextCursor::MoveOperation direction,
                QTextCursor::MoveMode mode)
{
    return cursor->movePosition(direction, mode);
}

template<>
bool moveCursor(QLineEdit *edit, QTextCursor::MoveOperation direction, QTextCursor::MoveMode mode)
{
    bool mark = (mode == QTextCursor::KeepAnchor);
    switch (direction) {
    case QTextCursor::Left:
        edit->cursorBackward(mark);
        break;
    case QTextCursor::WordLeft:
        edit->cursorWordBackward(mark);
        break;
    case QTextCursor::Right:
        edit->cursorForward(mark);
        break;
    case QTextCursor::WordRight:
        edit->cursorWordForward(mark);
        break;
    default:
        return false;
    }
    return edit->cursorPosition() > 0 && edit->cursorPosition() < edit->text().size();
}

template<typename C>
QChar charUnderCursor(C *cursor);

template<>
QChar charUnderCursor(QTextCursor *cursor)
{
    return cursor->document()->characterAt(cursor->position());
}

template<>
QChar charUnderCursor(QLineEdit *edit)
{
    const int pos = edit->cursorPosition();
    if (pos < 0 || pos >= edit->text().length())
        return QChar::Null;

    return edit->text().at(pos);
};

template<typename C>
int position(C *cursor);

template<>
int position(QTextCursor *cursor)
{
    return cursor->position();
}

template<>
int position(QLineEdit *edit)
{
    return edit->cursorPosition();
}

enum class Input {
    Upper,
    Lower,
    Underscore,
    Space,
    Other
};

static Input inputFromChar(QChar c)
{
    Input input = Input::Other;
    if (c.isUpper())
        input = Input::Upper;
    else if (c.isLower() || c.isDigit())
        input = Input::Lower;
    else if (c == '_')
        input = Input::Underscore;
    else if (c.isSpace() && c != QChar::ParagraphSeparator)
        input = Input::Space;
    else
        input = Input::Other;
    return input;
}

template<typename C>
bool camelCaseLeft(C *cursor, QTextCursor::MoveMode mode)
{
    int state = 0;

    if (!moveCursor(cursor, QTextCursor::Left, mode))
        return false;

    for (;;) {
        const Input input = inputFromChar(charUnderCursor(cursor));

        switch (state) {
        case 0:
            switch (input) {
            case Input::Upper:
                state = 1;
                break;
            case Input::Lower:
                state = 2;
                break;
            case Input::Underscore:
                state = 3;
                break;
            case Input::Space:
                state = 4;
                break;
            default:
                moveCursor(cursor, QTextCursor::Right, mode);
                return moveCursor(cursor, QTextCursor::WordLeft, mode);
            }
            break;
        case 1:
            switch (input) {
            case Input::Upper:
                break;
            default:
                moveCursor(cursor, QTextCursor::Right, mode);
                return true;
            }
            break;
        case 2:
            switch (input) {
            case Input::Upper:
                return true;
            case Input::Lower:
                break;
            default:
                moveCursor(cursor, QTextCursor::Right, mode);
                return true;
            }
            break;
        case 3:
            switch (input) {
            case Input::Underscore:
                break;
            case Input::Upper:
                state = 1;
                break;
            case Input::Lower:
                state = 2;
                break;
            default:
                moveCursor(cursor, QTextCursor::Right, mode);
                return true;
            }
            break;
        case 4:
            switch (input) {
            case Input::Space:
                break;
            case Input::Upper:
                state = 1;
                break;
            case Input::Lower:
                state = 2;
                break;
            case Input::Underscore:
                state = 3;
                break;
            default:
                moveCursor(cursor, QTextCursor::Right, mode);
                if (position(cursor) == 0)
                    return true;
                return moveCursor(cursor, QTextCursor::WordLeft, mode);
            }
        }

        if (!moveCursor(cursor, QTextCursor::Left, mode))
            return true;
    }
}

template<typename C>
bool camelCaseRight(C *cursor, QTextCursor::MoveMode mark)
{
    int state = 0;

    for (;;) {
        const Input input = inputFromChar(charUnderCursor(cursor));

        switch (state) {
        case 0:
            switch (input) {
            case Input::Upper:
                state = 4;
                break;
            case Input::Lower:
                state = 1;
                break;
            case Input::Underscore:
                state = 6;
                break;
            default:
                return moveCursor(cursor, QTextCursor::WordRight, mark);
            }
            break;
        case 1:
            switch (input) {
            case Input::Upper:
                return true;
            case Input::Lower:
                break;
            case Input::Underscore:
                state = 6;
                break;
            case Input::Space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 2:
            switch (input) {
            case Input::Upper:
                break;
            case Input::Lower:
                moveCursor(cursor, QTextCursor::Left, mark);
                return true;
            case Input::Underscore:
                state = 6;
                break;
            case Input::Space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 4:
            switch (input) {
            case Input::Upper:
                state = 2;
                break;
            case Input::Lower:
                state = 1;
                break;
            case Input::Underscore:
                state = 6;
                break;
            case Input::Space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 6:
            switch (input) {
            case Input::Underscore:
                break;
            case Input::Space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 7:
            switch (input) {
            case Input::Space:
                break;
            default:
                return true;
            }
            break;
        }
        if (!moveCursor(cursor, QTextCursor::Right, mark))
            return false;
    }
}

} // anonymous namespace

bool CamelCaseCursor::left(QTextCursor *cursor, QTextCursor::MoveMode mode)
{
    return camelCaseLeft(cursor, mode);
}

bool CamelCaseCursor::left(MultiTextCursor *cursor, QTextCursor::MoveMode mode)
{
    bool result = false;
    for (QTextCursor &c : *cursor)
        result |= CamelCaseCursor::left(&c, mode);
    cursor->mergeCursors();
    return result;
}

bool CamelCaseCursor::left(QLineEdit *edit, QTextCursor::MoveMode mode)
{
    return camelCaseLeft(edit, mode);
}

bool CamelCaseCursor::right(QTextCursor *cursor, QTextCursor::MoveMode mode)
{
    return camelCaseRight(cursor, mode);
}

bool CamelCaseCursor::right(MultiTextCursor *cursor, QTextCursor::MoveMode mode)
{
    bool result = false;
    for (QTextCursor &c : *cursor)
        result |= CamelCaseCursor::right(&c, mode);
    cursor->mergeCursors();
    return result;
}

bool CamelCaseCursor::right(QLineEdit *edit, QTextCursor::MoveMode mode)
{
    return camelCaseRight(edit, mode);
}

} // namespace Utils

