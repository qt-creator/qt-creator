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

class CompleteCodeMessage
{

public:
    CompleteCodeMessage() = default;
    CompleteCodeMessage(const Utf8String &filePath,
                        quint32 line,
                        quint32 column,
                        const Utf8String &projectPartId,
                        qint32 funcNameStartLine = -1,
                        qint32 funcNameStartColumn = -1)
        : m_filePath(filePath),
          m_projectPartId(projectPartId),
          m_ticketNumber(++ticketCounter),
          m_line(line),
          m_column(column),
          m_funcNameStartLine(funcNameStartLine),
          m_funcNameStartColumn(funcNameStartColumn)
    {
    }

    const Utf8String &filePath() const
    {
        return m_filePath;
    }

    const Utf8String &projectPartId() const
    {
        return m_projectPartId;
    }

    quint32 line() const
    {
        return m_line;
    }

    quint32 column() const
    {
        return m_column;
    }

    quint64 ticketNumber() const
    {
        return m_ticketNumber;
    }

    qint32 funcNameStartLine() const
    {
        return m_funcNameStartLine;
    }

    qint32 funcNameStartColumn() const
    {
        return m_funcNameStartColumn;
    }

    friend QDataStream &operator<<(QDataStream &out, const CompleteCodeMessage &message)
    {
        out << message.m_filePath;
        out << message.m_projectPartId;
        out << message.m_ticketNumber;
        out << message.m_line;
        out << message.m_column;
        out << message.m_funcNameStartLine;
        out << message.m_funcNameStartColumn;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CompleteCodeMessage &message)
    {
        in >> message.m_filePath;
        in >> message.m_projectPartId;
        in >> message.m_ticketNumber;
        in >> message.m_line;
        in >> message.m_column;
        in >> message.m_funcNameStartLine;
        in >> message.m_funcNameStartColumn;

        return in;
    }

    friend bool operator==(const CompleteCodeMessage &first, const CompleteCodeMessage &second)
    {
        return first.m_ticketNumber == second.m_ticketNumber
                && first.m_filePath == second.m_filePath
                && first.m_projectPartId == second.m_projectPartId
                && first.m_line == second.m_line
                && first.m_column == second.m_column
                && first.m_funcNameStartLine == second.m_funcNameStartLine
                && first.m_funcNameStartColumn == second.m_funcNameStartColumn;
    }

    friend CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const CompleteCodeMessage &message);
    friend std::ostream &operator<<(std::ostream &os, const CompleteCodeMessage &message);

private:
    Utf8String m_filePath;
    Utf8String m_projectPartId;
    static CLANGSUPPORT_EXPORT quint64 ticketCounter;
    quint64 m_ticketNumber = 0;
    quint32 m_line = 0;
    quint32 m_column = 0;
    qint32 m_funcNameStartLine = -1;
    qint32 m_funcNameStartColumn = -1;
};

DECLARE_MESSAGE(CompleteCodeMessage);
} // namespace ClangBackEnd
