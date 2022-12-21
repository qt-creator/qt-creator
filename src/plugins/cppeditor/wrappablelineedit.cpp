// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "wrappablelineedit.h"

#include <QMimeData>

namespace CppEditor {

WrappableLineEdit::WrappableLineEdit(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setMaximumBlockCount(1); // Restrict to a single line.
}

void WrappableLineEdit::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        return; // Eat these to avoid new lines.
    case Qt::Key_Backtab:
    case Qt::Key_Tab:
        // Let the parent handle these because they might be used for navigation purposes.
        event->ignore();
        return;
    default:
        return QPlainTextEdit::keyPressEvent(event);
    }
}

void WrappableLineEdit::insertFromMimeData(const QMimeData *source)
{
    insertPlainText(source->text().simplified()); // Filter out new lines.
}

} // namespace CppEditor
