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

#include "cmbcompletecodemessage.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

quint64 CompleteCodeMessage::ticketCounter = 0;

CompleteCodeMessage::CompleteCodeMessage(const Utf8String &filePath, quint32 line, quint32 column, const Utf8String &projectPartId)
    : filePath_(filePath),
      projectPartId_(projectPartId),
      ticketNumber_(++ticketCounter),
      line_(line),
      column_(column)
{
}

const Utf8String &CompleteCodeMessage::filePath() const
{
    return filePath_;
}

const Utf8String &CompleteCodeMessage::projectPartId() const
{
    return projectPartId_;
}

quint32 CompleteCodeMessage::line() const
{
    return line_;
}

quint32 CompleteCodeMessage::column() const
{
    return column_;
}

quint64 CompleteCodeMessage::ticketNumber() const
{
    return ticketNumber_;
}

QDataStream &operator<<(QDataStream &out, const CompleteCodeMessage &message)
{
    out << message.filePath_;
    out << message.projectPartId_;
    out << message.ticketNumber_;
    out << message.line_;
    out << message.column_;

    return out;
}

QDataStream &operator>>(QDataStream &in, CompleteCodeMessage &message)
{
    in >> message.filePath_;
    in >> message.projectPartId_;
    in >> message.ticketNumber_;
    in >> message.line_;
    in >> message.column_;

    return in;
}

bool operator==(const CompleteCodeMessage &first, const CompleteCodeMessage &second)
{
    return first.ticketNumber_ == second.ticketNumber_
            && first.filePath_ == second.filePath_
            && first.projectPartId_ == second.projectPartId_
            && first.line_ == second.line_
            && first.column_ == second.column_;
}

QDebug operator<<(QDebug debug, const CompleteCodeMessage &message)
{
    debug.nospace() << "CompleteCodeMessage(";

    debug.nospace() << message.filePath_ << ", ";
    debug.nospace() << message.line_<< ", ";
    debug.nospace() << message.column_<< ", ";
    debug.nospace() << message.projectPartId_ << ", ";
    debug.nospace() << message.ticketNumber_;

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const CompleteCodeMessage &message, ::std::ostream* os)
{
    *os << "CompleteCodeMessage(";

    *os << message.filePath_.constData() << ", ";
    *os << message.line_ << ", ";
    *os << message.column_ << ", ";
    *os << message.projectPartId_.constData() << ", ";
    *os << message.ticketNumber_;

    *os << ")";
}

} // namespace ClangBackEnd

