/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "wrappablelineedit.h"

#include <QMimeData>

namespace CppTools {

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

} // namespace CppTools
