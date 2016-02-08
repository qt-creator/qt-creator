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

