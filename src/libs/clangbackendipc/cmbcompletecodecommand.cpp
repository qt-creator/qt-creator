/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmbcompletecodecommand.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

quint64 CompleteCodeCommand::ticketCounter = 0;

CompleteCodeCommand::CompleteCodeCommand(const Utf8String &filePath, quint32 line, quint32 column, const Utf8String &projectPartId)
    : filePath_(filePath),
      projectPartId_(projectPartId),
      ticketNumber_(++ticketCounter),
      line_(line),
      column_(column)
{
}

const Utf8String &CompleteCodeCommand::filePath() const
{
    return filePath_;
}

const Utf8String &CompleteCodeCommand::projectPartId() const
{
    return projectPartId_;
}

quint32 CompleteCodeCommand::line() const
{
    return line_;
}

quint32 CompleteCodeCommand::column() const
{
    return column_;
}

quint64 CompleteCodeCommand::ticketNumber() const
{
    return ticketNumber_;
}

QDataStream &operator<<(QDataStream &out, const CompleteCodeCommand &command)
{
    out << command.filePath_;
    out << command.projectPartId_;
    out << command.ticketNumber_;
    out << command.line_;
    out << command.column_;

    return out;
}

QDataStream &operator>>(QDataStream &in, CompleteCodeCommand &command)
{
    in >> command.filePath_;
    in >> command.projectPartId_;
    in >> command.ticketNumber_;
    in >> command.line_;
    in >> command.column_;

    return in;
}

bool operator==(const CompleteCodeCommand &first, const CompleteCodeCommand &second)
{
    return first.ticketNumber_ == second.ticketNumber_
            && first.filePath_ == second.filePath_
            && first.projectPartId_ == second.projectPartId_
            && first.line_ == second.line_
            && first.column_ == second.column_;
}

bool operator<(const CompleteCodeCommand &first, const CompleteCodeCommand &second)
{
    return first.ticketNumber_ < second.ticketNumber_
            && first.filePath_ < second.filePath_
            && first.projectPartId_ < second.projectPartId_
            && first.line_ < second.line_
            && first.column_ < second.column_;
}

QDebug operator<<(QDebug debug, const CompleteCodeCommand &command)
{
    debug.nospace() << "CompleteCodeCommand(";

    debug.nospace() << command.filePath_ << ", ";
    debug.nospace() << command.line_<< ", ";
    debug.nospace() << command.column_<< ", ";
    debug.nospace() << command.projectPartId_ << ", ";
    debug.nospace() << command.ticketNumber_;

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const CompleteCodeCommand &command, ::std::ostream* os)
{
    *os << "CompleteCodeCommand(";

    *os << command.filePath_.constData() << ", ";
    *os << command.line_ << ", ";
    *os << command.column_ << ", ";
    *os << command.projectPartId_.constData() << ", ";
    *os << command.ticketNumber_;

    *os << ")";
}

} // namespace ClangBackEnd

