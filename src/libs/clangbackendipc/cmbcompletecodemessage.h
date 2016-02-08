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

#ifndef CLANGBACKEND_COMPLETECODEMESSAGE_H
#define CLANGBACKEND_COMPLETECODEMESSAGE_H

#include "clangbackendipc_global.h"

#include <utf8string.h>

namespace ClangBackEnd {

class CMBIPC_EXPORT CompleteCodeMessage
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const CompleteCodeMessage &message);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, CompleteCodeMessage &message);
    friend CMBIPC_EXPORT bool operator==(const CompleteCodeMessage &first, const CompleteCodeMessage &second);
    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CompleteCodeMessage &message);
    friend void PrintTo(const CompleteCodeMessage &message, ::std::ostream* os);

public:
    CompleteCodeMessage() = default;
    CompleteCodeMessage(const Utf8String &filePath,
                        quint32 line,
                        quint32 column,
                        const Utf8String &projectPartId);

    const Utf8String &filePath() const;
    const Utf8String &projectPartId() const;

    quint32 line() const;
    quint32 column() const;

    quint64 ticketNumber() const;

private:
    Utf8String filePath_;
    Utf8String projectPartId_;
    static quint64 ticketCounter;
    quint64 ticketNumber_;
    quint32 line_ = 0;
    quint32 column_ = 0;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const CompleteCodeMessage &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, CompleteCodeMessage &message);
CMBIPC_EXPORT bool operator==(const CompleteCodeMessage &first, const CompleteCodeMessage &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const CompleteCodeMessage &message);
void PrintTo(const CompleteCodeMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(CompleteCodeMessage);
} // namespace ClangBackEnd

#endif // CLANGBACKEND_COMPLETECODEMESSAGE_H
