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

#ifdef QT_TESTLIB_LIB
#include <QTest>
#endif

#include <QDataStream>

#include <ostream>

Utf8String::Utf8String(const char *utf8Text, int size)
    : byteArray(utf8Text, size)
{
}

Utf8String::Utf8String(const QString &text)
    : byteArray(text.toUtf8())
{
}

const char *Utf8String::constData() const
{
    return byteArray.constData();
}

int Utf8String::byteSize() const
{
    return byteArray.size();
}

Utf8String Utf8String::fromUtf8(const char *utf8Text)
{
    return Utf8String(utf8Text, -1);
}

Utf8String::Utf8String(const QByteArray &utf8ByteArray)
    : byteArray(utf8ByteArray)
{
}

Utf8String Utf8String::fromByteArray(const QByteArray &utf8ByteArray)
{
    return Utf8String(utf8ByteArray);
}

const QByteArray &Utf8String::toByteArray() const
{
    return byteArray;
}

Utf8String Utf8String::fromString(const QString &text)
{
    return Utf8String::fromByteArray(text.toUtf8());
}

QString Utf8String::toString() const
{
    return QString::fromUtf8(byteArray, byteArray.size());
}

Utf8String Utf8String::mid(int position, int length) const
{
    return Utf8String(byteArray.mid(position, length));
}

void Utf8String::replace(const Utf8String &before, const Utf8String &after)
{
    byteArray.replace(before.byteArray, after.byteArray);
}

void Utf8String::replace(int position, int length, const Utf8String &after)
{
    byteArray.replace(position, length, after.byteArray);
}

Utf8StringVector Utf8String::split(char separator) const
{
    Utf8StringVector utf8Vector;

    foreach (const QByteArray &byteArray, byteArray.split(separator))
        utf8Vector.append(Utf8String::fromByteArray(byteArray));

   return utf8Vector;
}

void Utf8String::clear()
{
    byteArray.clear();
}

void Utf8String::append(const Utf8String &textToAppend)
{
    byteArray.append(textToAppend.byteArray);
}

bool Utf8String::contains(const Utf8String &text) const
{
    return byteArray.contains(text.byteArray);
}

bool Utf8String::contains(const char *text) const
{
    return byteArray.contains(text);
}

bool Utf8String::contains(char character) const
{
    return byteArray.contains(character);
}

bool Utf8String::startsWith(const Utf8String &text) const
{
    return byteArray.startsWith(text.byteArray);
}

bool Utf8String::startsWith(const char *text) const
{
    return byteArray.startsWith(text);
}

bool Utf8String::startsWith(char character) const
{
    return byteArray.startsWith(character);
}

bool Utf8String::endsWith(const Utf8String &text) const
{
    return byteArray.endsWith(text.byteArray);
}

bool Utf8String::isNull() const
{
    return byteArray.isNull();
}

bool Utf8String::isEmpty() const
{
    return byteArray.isEmpty();
}

bool Utf8String::hasContent() const
{
    return !isEmpty();
}

void Utf8String::reserve(int reserveSize)
{
    byteArray.reserve(reserveSize);
}

Utf8String Utf8String::number(int number, int base)
{
    return Utf8String::fromByteArray(QByteArray::number(number, base));
}

const Utf8String &Utf8String::operator+=(const Utf8String &text)
{
    byteArray += text.byteArray;

    return *this;
}

void Utf8String::registerType()
{
    qRegisterMetaType<Utf8String>("Utf8String");
}

Utf8String::operator const QByteArray &() const
{
    return byteArray;
}

Utf8String::operator QString() const
{
    return toString();
}

const Utf8String operator+(const Utf8String &first, const Utf8String &second)
{
    return Utf8String(first.byteArray + second.byteArray);
}


bool operator!=(const Utf8String &first, const Utf8String &second)
{
    return first.byteArray != second.byteArray;
}


bool operator==(const Utf8String &first, const Utf8String &second)
{
    return first.byteArray == second.byteArray;
}

bool operator==(const Utf8String &first, const char *second)
{
    return first.byteArray == second;
}

bool operator==(const char *first, const Utf8String &second)
{
    return second == first;
}

bool operator==(const Utf8String &first, const QString &second)
{
    return first.byteArray == second.toUtf8();
}

bool operator<(const Utf8String &first, const Utf8String &second)
{
    if (first.byteSize() == second.byteSize())
        return first.byteArray < second.byteArray;

    return first.byteSize() < second.byteSize();
}


QDataStream &operator<<(QDataStream &datastream, const Utf8String &text)
{
    datastream << text.byteArray;

    return datastream;
}


QDataStream &operator>>(QDataStream &datastream, Utf8String &text)
{
    datastream >> text.byteArray;

    return datastream;
}


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

uint qHash(const Utf8String &utf8String)
{
    return qHash(utf8String.byteArray);
}
