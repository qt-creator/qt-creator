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

#pragma once

#include "clangsupport_global.h"

#include "filecontainer.h"

#include <QDataStream>

namespace ClangBackEnd {

class RequestFollowSymbolMessage
{
public:
    RequestFollowSymbolMessage() = default;
    RequestFollowSymbolMessage(const FileContainer &fileContainer,
                               const QVector<Utf8String> &dependentFiles,
                               quint32 line,
                               quint32 column)
        : m_fileContainer(fileContainer)
        , m_ticketNumber(++ticketCounter)
        , m_line(line)
        , m_column(column)
        , m_dependentFiles(dependentFiles)
    {
    }

    const FileContainer &fileContainer() const
    {
        return m_fileContainer;
    }

    const QVector<Utf8String> &dependentFiles() const
    {
        return m_dependentFiles;
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

    friend QDataStream &operator<<(QDataStream &out, const RequestFollowSymbolMessage &message)
    {
        out << message.m_fileContainer;
        out << message.m_dependentFiles;
        out << message.m_ticketNumber;
        out << message.m_line;
        out << message.m_column;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestFollowSymbolMessage &message)
    {
        in >> message.m_fileContainer;
        in >> message.m_dependentFiles;
        in >> message.m_ticketNumber;
        in >> message.m_line;
        in >> message.m_column;

        return in;
    }

    friend bool operator==(const RequestFollowSymbolMessage &first,
                           const RequestFollowSymbolMessage &second)
    {
        return first.m_ticketNumber == second.m_ticketNumber
            && first.m_line == second.m_line
            && first.m_column == second.m_column
            && first.m_fileContainer == second.m_fileContainer
            && first.m_dependentFiles == second.m_dependentFiles;
    }

    friend CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RequestFollowSymbolMessage &message);
    friend std::ostream &operator<<(std::ostream &os, const RequestFollowSymbolMessage &message);
private:
    FileContainer m_fileContainer;
    quint64 m_ticketNumber = 0;
    quint32 m_line = 0;
    quint32 m_column = 0;
    QVector<Utf8String> m_dependentFiles;
    static CLANGSUPPORT_EXPORT quint64 ticketCounter;
};

DECLARE_MESSAGE(RequestFollowSymbolMessage);
} // namespace ClangBackEnd
