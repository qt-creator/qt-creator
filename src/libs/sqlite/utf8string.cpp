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

#include "utf8string.h"

#include "utf8stringvector.h"

#include <QString>
#include <QDebug>

#include <ostream>

QDebug operator<<(QDebug debug, const Utf8String &text)
{
    debug << text.constData();

    return debug;
}

void PrintTo(const Utf8String &text, ::std::ostream* os)
{
    *os << "\"" << text.toByteArray().data() << "\"";
}

std::ostream& operator<<(std::ostream &os, const Utf8String &utf8String)
{
    using std::ostream;
    os << utf8String.constData();

    return os;
}

Utf8StringVector Utf8String::split(char separator) const
{
    Utf8StringVector utf8Vector;

    for (const QByteArray &byteArrayPart : byteArray.split(separator))
        utf8Vector.append(Utf8String::fromByteArray(byteArrayPart));

    return utf8Vector;
}
