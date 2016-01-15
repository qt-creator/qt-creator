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

#include "cmbechomessage.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

EchoMessage::EchoMessage(const QVariant &message)
    : message_(message)
{

}

const QVariant &EchoMessage::message() const
{
    return message_;
}

QDataStream &operator<<(QDataStream &out, const EchoMessage &message)
{
    out << message.message();

    return out;
}

QDataStream &operator>>(QDataStream &in, EchoMessage &message)
{
    in >> message.message_;

    return in;
}

bool operator==(const EchoMessage &first, const EchoMessage &second)
{
    return first.message_ == second.message_;
}

bool operator<(const EchoMessage &first, const EchoMessage &second)
{
    return first.message_ < second.message_;
}

QDebug operator<<(QDebug debug, const EchoMessage &message)
{
    return debug.nospace() << "EchoMessage(" << message.message() << ")";
}

void PrintTo(const EchoMessage &message, ::std::ostream* os)
{
    QString output;
    QDebug debug(&output);

    debug << message;

    *os << output.toUtf8().constData();
}

} // namespace ClangBackEnd

