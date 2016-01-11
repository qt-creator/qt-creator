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

#include "cmbcodecompletedmessage.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

CodeCompletedMessage::CodeCompletedMessage(const CodeCompletions &codeCompletions,
                                           CompletionCorrection neededCorrection,
                                           quint64 ticketNumber)
    : codeCompletions_(codeCompletions),
      ticketNumber_(ticketNumber),
      neededCorrection_(neededCorrection)
{
}

const CodeCompletions &CodeCompletedMessage::codeCompletions() const
{
    return codeCompletions_;
}

CompletionCorrection CodeCompletedMessage::neededCorrection() const
{
    return neededCorrection_;
}

quint64 CodeCompletedMessage::ticketNumber() const
{
    return ticketNumber_;
}

quint32 &CodeCompletedMessage::neededCorrectionAsInt()
{
    return reinterpret_cast<quint32&>(neededCorrection_);
}

QDataStream &operator<<(QDataStream &out, const CodeCompletedMessage &message)
{
    out << message.codeCompletions_;
    out << quint32(message.neededCorrection_);
    out << message.ticketNumber_;

    return out;
}

QDataStream &operator>>(QDataStream &in, CodeCompletedMessage &message)
{
    in >> message.codeCompletions_;
    in >> message.neededCorrectionAsInt();
    in >> message.ticketNumber_;

    return in;
}

bool operator==(const CodeCompletedMessage &first, const CodeCompletedMessage &second)
{
    return first.ticketNumber_ == second.ticketNumber_
        && first.neededCorrection_ == second.neededCorrection_
        && first.codeCompletions_ == second.codeCompletions_;
}

bool operator<(const CodeCompletedMessage &first, const CodeCompletedMessage &second)
{
    return first.ticketNumber_ < second.ticketNumber_;
}

#define RETURN_TEXT_FOR_CASE(enumValue) case CompletionCorrection::enumValue: return #enumValue
static const char *completionCorrectionToText(CompletionCorrection correction)
{
    switch (correction) {
        RETURN_TEXT_FOR_CASE(NoCorrection);
        RETURN_TEXT_FOR_CASE(DotToArrowCorrection);
        default: return "UnhandledCompletionCorrection";
    }
}
#undef RETURN_TEXT_FOR_CASE

QDebug operator<<(QDebug debug, const CodeCompletedMessage &message)
{
    debug.nospace() << "CodeCompletedMessage(";

    debug.nospace() << message.codeCompletions_ << ", "
                    << completionCorrectionToText(message.neededCorrection()) << ", "
                    << message.ticketNumber_;

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const CodeCompletedMessage &message, ::std::ostream* os)
{
    QString output;
    QDebug debug(&output);

    debug << message;

    *os << output.toUtf8().constData();
}

} // namespace ClangBackEnd

