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

#include "clangbackendipc_global.h"

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
                        const Utf8String &projectPartId)
        : filePath_(filePath),
          projectPartId_(projectPartId),
          ticketNumber_(++ticketCounter),
          line_(line),
          column_(column)
    {
    }

    const Utf8String &filePath() const
    {
        return filePath_;
    }

    const Utf8String &projectPartId() const
    {
        return projectPartId_;
    }

    quint32 line() const
    {
        return line_;
    }

    quint32 column() const
    {
        return column_;
    }

    quint64 ticketNumber() const
    {
        return ticketNumber_;
    }

    friend QDataStream &operator<<(QDataStream &out, const CompleteCodeMessage &message)
    {
        out << message.filePath_;
        out << message.projectPartId_;
        out << message.ticketNumber_;
        out << message.line_;
        out << message.column_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CompleteCodeMessage &message)
    {
        in >> message.filePath_;
        in >> message.projectPartId_;
        in >> message.ticketNumber_;
        in >> message.line_;
        in >> message.column_;

        return in;
    }

    friend bool operator==(const CompleteCodeMessage &first, const CompleteCodeMessage &second)
    {
        return first.ticketNumber_ == second.ticketNumber_
                && first.filePath_ == second.filePath_
                && first.projectPartId_ == second.projectPartId_
                && first.line_ == second.line_
                && first.column_ == second.column_;
    }

    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CompleteCodeMessage &message);
    friend void PrintTo(const CompleteCodeMessage &message, ::std::ostream* os);

private:
    Utf8String filePath_;
    Utf8String projectPartId_;
    static CMBIPC_EXPORT quint64 ticketCounter;
    quint64 ticketNumber_;
    quint32 line_ = 0;
    quint32 column_ = 0;
};

DECLARE_MESSAGE(CompleteCodeMessage);
} // namespace ClangBackEnd
