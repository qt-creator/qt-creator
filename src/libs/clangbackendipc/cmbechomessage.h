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

#ifndef CLANGBACKEND_ECHOMESSAGE_H
#define CLANGBACKEND_ECHOMESSAGE_H

#include "clangbackendipc_global.h"

#include "messageenvelop.h"

namespace ClangBackEnd {

class CMBIPC_EXPORT EchoMessage
{
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, EchoMessage &message);
    friend CMBIPC_EXPORT bool operator==(const EchoMessage &first, const EchoMessage &second);
public:
    EchoMessage() = default;
    explicit EchoMessage(const MessageEnvelop &message);

    const MessageEnvelop &message() const;

private:
    MessageEnvelop message_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const EchoMessage &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, EchoMessage &message);
CMBIPC_EXPORT bool operator==(const EchoMessage &first, const EchoMessage &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const EchoMessage &message);
void PrintTo(const EchoMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(EchoMessage)
} // namespace ClangBackEnd

#endif // CLANGBACKEND_ECHOMESSAGE_H
