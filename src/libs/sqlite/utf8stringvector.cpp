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

#include "utf8stringvector.h"

#include <QStringList>
#include <QDebug>

#include <ostream>

Utf8StringVector::Utf8StringVector()
{
}

Utf8StringVector::Utf8StringVector(std::initializer_list<Utf8String> initializerList)
    : QVector<Utf8String>(initializerList)
{

}

Utf8StringVector::Utf8StringVector(const Utf8String &utf8String)
{
    append(utf8String);
}

Utf8StringVector::Utf8StringVector(const QVector<Utf8String> &vector)
    : QVector<Utf8String>(vector)
{

}

Utf8StringVector::Utf8StringVector(const QStringList &stringList)
{
    reserve(stringList.count());

    foreach (const QString &string, stringList)
        append(Utf8String(string));
}

Utf8StringVector::Utf8StringVector(int size, const Utf8String &text)
    : QVector<Utf8String>(size, text)
{
}

Utf8String Utf8StringVector::join(const Utf8String &separator) const
{
    Utf8String joindedString;

    joindedString.reserve(totalByteSize() + separator.byteSize() * count());

    for (auto position = begin(); position != end(); ++position) {
        joindedString.append(*position);
        if (std::next(position) != end())
            joindedString.append(separator);
    }

    return joindedString;
}

Utf8StringVector Utf8StringVector::fromIntegerVector(const QVector<int> &integerVector)
{
    Utf8StringVector utf8StringVector;
    utf8StringVector.reserve(integerVector.count());

    foreach (int integer, integerVector)
        utf8StringVector.append(Utf8String::number(integer));

    return utf8StringVector;
}

void Utf8StringVector::registerType()
{
    qRegisterMetaType<Utf8StringVector>("Utf8StringVector");
}

int Utf8StringVector::totalByteSize() const
{
    int totalSize = 0;

    for (const Utf8String &utf8String : *this)
        totalSize +=  utf8String.byteSize();

    return totalSize;
}

QDebug operator<<(QDebug debug, const Utf8StringVector &textVector)
{
    debug << "Utf8StringVector(" << textVector.join(Utf8StringLiteral(", ")).constData() << ")";

    return debug;
}

void PrintTo(const Utf8StringVector &textVector, ::std::ostream* os)
{
    *os << "Utf8StringVector(" << textVector.join(Utf8StringLiteral(", ")).constData() << ")";
}
