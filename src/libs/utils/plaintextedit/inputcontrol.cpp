// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "inputcontrol.h"

#include <QKeyEvent>

namespace Utils {

InputControl::InputControl(Type type, QObject *parent)
    : QObject(parent)
    , m_type(type)
{
}

bool InputControl::isAcceptableInput(const QKeyEvent *event) const
{
    const QString text = event->text();
    if (text.isEmpty())
        return false;

    const QChar c = text.at(0);

    // Formatting characters such as ZWNJ, ZWJ, RLM, etc. This needs to go before the
    // next test, since CTRL+SHIFT is sometimes used to input it on Windows.
    if (c.category() == QChar::Other_Format)
        return true;

    // QTBUG-35734: ignore Ctrl/Ctrl+Shift; accept only AltGr (Alt+Ctrl) on German keyboards
    if (event->modifiers() == Qt::ControlModifier
        || event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier)) {
        return false;
    }

    if (c.isPrint())
        return true;

    if (c.category() == QChar::Other_PrivateUse)
        return true;

    if (c.isHighSurrogate() && text.size() > 1 && text.at(1).isLowSurrogate())
        return true;

    if (m_type == TextEdit && c == u'\t')
        return true;

    return false;
}

bool InputControl::isCommonTextEditShortcut(const QKeyEvent *ke)
{
    if (ke->modifiers() == Qt::NoModifier
        || ke->modifiers() == Qt::ShiftModifier
        || ke->modifiers() == Qt::KeypadModifier) {
        if (ke->key() < Qt::Key_Escape) {
            return true;
        } else {
            switch (ke->key()) {
            case Qt::Key_Return:
            case Qt::Key_Enter:
            case Qt::Key_Delete:
            case Qt::Key_Home:
            case Qt::Key_End:
            case Qt::Key_Backspace:
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Tab:
                return true;
            default:
                break;
            }
        }
#if QT_CONFIG(shortcut)
    } else if (ke->matches(QKeySequence::Copy)
               || ke->matches(QKeySequence::Paste)
               || ke->matches(QKeySequence::Cut)
               || ke->matches(QKeySequence::Redo)
               || ke->matches(QKeySequence::Undo)
               || ke->matches(QKeySequence::MoveToNextWord)
               || ke->matches(QKeySequence::MoveToPreviousWord)
               || ke->matches(QKeySequence::MoveToStartOfDocument)
               || ke->matches(QKeySequence::MoveToEndOfDocument)
               || ke->matches(QKeySequence::SelectNextWord)
               || ke->matches(QKeySequence::SelectPreviousWord)
               || ke->matches(QKeySequence::SelectStartOfLine)
               || ke->matches(QKeySequence::SelectEndOfLine)
               || ke->matches(QKeySequence::SelectStartOfBlock)
               || ke->matches(QKeySequence::SelectEndOfBlock)
               || ke->matches(QKeySequence::SelectStartOfDocument)
               || ke->matches(QKeySequence::SelectEndOfDocument)
               || ke->matches(QKeySequence::SelectAll)
               ) {
        return true;
#endif
    }
    return false;
}

} // namespace Utils
