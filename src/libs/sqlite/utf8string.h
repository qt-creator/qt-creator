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

#pragma once

#include "sqliteglobal.h"

#include <QByteArray>
#include <QDataStream>
#include <QHashFunctions>
#include <QMetaType>
#include <QString>

#include <iosfwd>

class Utf8StringVector;
class Utf8String;

class Utf8String
{
    friend class Utf8StringVector;

public:
    Utf8String() = default;

    explicit Utf8String(const char *utf8Text, int size)
        : byteArray(utf8Text, size)
    {
    }

    Utf8String(const QString &text)
        : byteArray(text.toUtf8())
    {
    }

    const char *constData() const
    {
        return byteArray.constData();
    }

    int byteSize() const
    {
        return byteArray.size();
    }

    static Utf8String fromUtf8(const char *utf8Text)
    {
        return Utf8String(utf8Text, -1);
    }

    static Utf8String fromByteArray(const QByteArray &utf8ByteArray)
    {
        return Utf8String(utf8ByteArray);
    }
    const QByteArray &toByteArray() const
    {
        return byteArray;
    }

    static Utf8String fromString(const QString &text)
    {
        return Utf8String::fromByteArray(text.toUtf8());
    }

    QString toString() const
    {
        return QString::fromUtf8(byteArray, byteArray.size());
    }

    Utf8String mid(int position, int length = -1) const
    {
        return Utf8String(byteArray.mid(position, length));
    }

    void replace(const Utf8String &before, const Utf8String &after)
    {
        byteArray.replace(before.byteArray, after.byteArray);
    }

    void replace(int position, int length, const Utf8String &after)
    {
        byteArray.replace(position, length, after.byteArray);
    }

    SQLITE_EXPORT Utf8StringVector split(char separator) const;

    void clear()
    {
        byteArray.clear();
    }

    void append(const Utf8String &textToAppend)
    {
        byteArray.append(textToAppend.byteArray);
    }

    bool contains(const Utf8String &text) const
    {
        return byteArray.contains(text.byteArray);
    }

    bool contains(const char *text) const
    {
        return byteArray.contains(text);
    }

    bool contains(char character) const
    {
        return byteArray.contains(character);
    }

    bool startsWith(const Utf8String &text) const
    {
        return byteArray.startsWith(text.byteArray);
    }

    bool startsWith(const char *text) const
    {
        return byteArray.startsWith(text);
    }

    bool startsWith(char character) const
    {
        return byteArray.startsWith(character);
    }

    bool endsWith(const Utf8String &text) const
    {
        return byteArray.endsWith(text.byteArray);
    }

    bool isNull() const
    {
        return byteArray.isNull();
    }

    bool isEmpty() const
    {
        return byteArray.isEmpty();
    }

    bool hasContent() const
    {
        return !isEmpty();
    }

    void reserve(int reserveSize)
    {
        byteArray.reserve(reserveSize);
    }

    static Utf8String number(int number, int base=10)
    {
        return Utf8String::fromByteArray(QByteArray::number(number, base));
    }

    const Utf8String &operator+=(const Utf8String &text)
    {
        byteArray += text.byteArray;

        return *this;
    }

    static void registerType()
    {
        qRegisterMetaType<Utf8String>("Utf8String");
    }

    operator QString() const
    {
        return toString();
    }

    operator const QByteArray &() const
    {
        return byteArray;
    }

    friend const Utf8String operator+(const Utf8String &first, const Utf8String &second)
    {
        return Utf8String(first.byteArray + second.byteArray);
    }

    friend bool operator!=(const Utf8String &first, const Utf8String &second)
    {
        return first.byteArray != second.byteArray;
    }

    friend bool operator==(const Utf8String &first, const Utf8String &second)
    {
        return first.byteArray == second.byteArray;
    }

    friend bool operator==(const Utf8String &first, const char *second)
    {
        return first.byteArray == second;
    }

    friend bool operator==(const char *first, const Utf8String &second)
    {
        return second == first;
    }

    friend bool operator==(const Utf8String &first, const QString &second)
    {
        return first.byteArray == second.toUtf8();
    }

    friend bool operator<(const Utf8String &first, const Utf8String &second)
    {
        if (first.byteSize() == second.byteSize())
            return first.byteArray < second.byteArray;

        return first.byteSize() < second.byteSize();
    }

    friend  QDataStream &operator<<(QDataStream &datastream, const Utf8String &text)
    {
        datastream << text.byteArray;

        return datastream;
    }

    friend QDataStream &operator>>(QDataStream &datastream, Utf8String &text)
    {
        datastream >> text.byteArray;

        return datastream;
    }

    friend uint qHash(const Utf8String &utf8String)
    {
        return qHash(utf8String.byteArray);
    }

protected:
    explicit Utf8String(const QByteArray &utf8ByteArray)
        : byteArray(utf8ByteArray)
    {
    }

private:
    QByteArray byteArray;
};

SQLITE_EXPORT QDebug operator<<(QDebug debug, const Utf8String &text);
SQLITE_EXPORT void PrintTo(const Utf8String &text, ::std::ostream* os);
SQLITE_EXPORT std::ostream& operator<<(std::ostream &os, const Utf8String &utf8String);

#define Utf8StringLiteral(str) Utf8String::fromByteArray(QByteArrayLiteral(str))
