/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <texteditor/texteditor_global.h>

QT_BEGIN_NAMESPACE
class QChar;
class QString;
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT TextDocumentManipulatorInterface
{
public:
    virtual ~TextDocumentManipulatorInterface() = default;

    virtual int currentPosition() const = 0;
    virtual int positionAt(TextPositionOperation textPositionOperation) const = 0;
    virtual QChar characterAt(int position) const = 0;
    virtual QString textAt(int position, int length) const = 0;
    virtual QTextCursor textCursorAt(int position) const = 0;

    virtual void setCursorPosition(int position) = 0;
    virtual void setAutoCompleteSkipPosition(int position) = 0;
    virtual bool replace(int position, int length, const QString &text) = 0;
    virtual void insertCodeSnippet(int position, const QString &text) = 0;
    virtual void paste() = 0;
    virtual void encourageApply() = 0;
    virtual void autoIndent(int position, int length) = 0;
};

} // namespace TextEditor
