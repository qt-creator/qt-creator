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

#ifndef CLANGBACKEND_CODECOMPLETEDMESSAGE_H
#define CLANGBACKEND_CODECOMPLETEDMESSAGE_H

#include "codecompletion.h"

#include <QVector>

namespace ClangBackEnd {

class CMBIPC_EXPORT CodeCompletedMessage
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const CodeCompletedMessage &message);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, CodeCompletedMessage &message);
    friend CMBIPC_EXPORT bool operator==(const CodeCompletedMessage &first, const CodeCompletedMessage &second);
    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CodeCompletedMessage &message);
    friend void PrintTo(const CodeCompletedMessage &message, ::std::ostream* os);
public:
    CodeCompletedMessage() = default;
    CodeCompletedMessage(const CodeCompletions &codeCompletions,
                         CompletionCorrection neededCorrection,
                         quint64 ticketNumber);

    const CodeCompletions &codeCompletions() const;
    CompletionCorrection neededCorrection() const;

    quint64 ticketNumber() const;

private:
    quint32 &neededCorrectionAsInt();

private:
    CodeCompletions codeCompletions_;
    quint64 ticketNumber_ = 0;
    CompletionCorrection neededCorrection_ = CompletionCorrection::NoCorrection;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const CodeCompletedMessage &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, CodeCompletedMessage &message);
CMBIPC_EXPORT bool operator==(const CodeCompletedMessage &first, const CodeCompletedMessage &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CodeCompletedMessage &message);
void PrintTo(const CodeCompletedMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(CodeCompletedMessage)
} // namespace ClangBackEnd

#endif // CLANGBACKEND_CODECOMPLETEDMESSAGE_H
