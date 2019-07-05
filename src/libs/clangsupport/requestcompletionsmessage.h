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

#include "clangsupport_global.h"

#include <utf8string.h>

#include <QDataStream>

namespace ClangBackEnd {

class RequestCompletionsMessage
{
public:
    RequestCompletionsMessage() = default;
    RequestCompletionsMessage(const Utf8String &filePath,
                              quint32 line,
                              quint32 column,
                              qint32 funcNameStartLine = -1,
                              qint32 funcNameStartColumn = -1)
        : filePath(filePath)
        , ticketNumber(++ticketCounter)
        , line(line)
        , column(column)
        , funcNameStartLine(funcNameStartLine)
        , funcNameStartColumn(funcNameStartColumn)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const RequestCompletionsMessage &message)
    {
        out << message.filePath;
        out << message.ticketNumber;
        out << message.line;
        out << message.column;
        out << message.funcNameStartLine;
        out << message.funcNameStartColumn;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestCompletionsMessage &message)
    {
        in >> message.filePath;
        in >> message.ticketNumber;
        in >> message.line;
        in >> message.column;
        in >> message.funcNameStartLine;
        in >> message.funcNameStartColumn;

        return in;
    }

    friend bool operator==(const RequestCompletionsMessage &first,
                           const RequestCompletionsMessage &second)
    {
        return first.ticketNumber == second.ticketNumber
                && first.filePath == second.filePath
                && first.line == second.line
                && first.column == second.column
                && first.funcNameStartLine == second.funcNameStartLine
                && first.funcNameStartColumn == second.funcNameStartColumn;
    }

public:
    Utf8String filePath;
    static CLANGSUPPORT_EXPORT quint64 ticketCounter;
    quint64 ticketNumber = 0;
    quint32 line = 0;
    quint32 column = 0;
    qint32 funcNameStartLine = -1;
    qint32 funcNameStartColumn = -1;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RequestCompletionsMessage &message);

DECLARE_MESSAGE(RequestCompletionsMessage);
} // namespace ClangBackEnd
