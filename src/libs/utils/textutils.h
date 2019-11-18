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

#include "linecolumn.h"
#include "utils_global.h"

#include <QString>

QT_FORWARD_DECLARE_CLASS(QTextDocument)
QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace Utils {
namespace Text {

struct Replacement
{
    Replacement() = default;
    Replacement(int offset, int length, const QString &text)
        : offset(offset)
        , length(length)
        , text(text)
    {}

    int offset = -1;
    int length = -1;
    QString text;

    bool isValid() const { return offset >= 0 && length >= 0;  }
};
using Replacements = std::vector<Replacement>;

QTCREATOR_UTILS_EXPORT void applyReplacements(QTextDocument *doc, const Replacements &replacements);

// line is 1-based, column is 1-based
QTCREATOR_UTILS_EXPORT bool convertPosition(const QTextDocument *document,
                                            int pos,
                                            int *line, int *column);
QTCREATOR_UTILS_EXPORT
OptionalLineColumn convertPosition(const QTextDocument *document, int pos);

// line and column are 1-based
QTCREATOR_UTILS_EXPORT int positionInText(const QTextDocument *textDocument, int line, int column);

QTCREATOR_UTILS_EXPORT QString textAt(QTextCursor tc, int pos, int length);

QTCREATOR_UTILS_EXPORT QTextCursor selectAt(QTextCursor textCursor, int line, int column, uint length);

QTCREATOR_UTILS_EXPORT QTextCursor flippedCursor(const QTextCursor &cursor);

QTCREATOR_UTILS_EXPORT QTextCursor wordStartCursor(const QTextCursor &cursor);
QTCREATOR_UTILS_EXPORT QString wordUnderCursor(const QTextCursor &cursor);

QTCREATOR_UTILS_EXPORT bool utf8AdvanceCodePoint(const char *&current);
QTCREATOR_UTILS_EXPORT int utf8NthLineOffset(const QTextDocument *textDocument,
                                             const QByteArray &buffer,
                                             int line);

QTCREATOR_UTILS_EXPORT LineColumn utf16LineColumn(const QByteArray &utf8Buffer, int utf8Offset);
QTCREATOR_UTILS_EXPORT QString utf16LineTextInUtf8Buffer(const QByteArray &utf8Buffer,
                                                         int currentUtf8Offset);

} // Text
} // Utils
