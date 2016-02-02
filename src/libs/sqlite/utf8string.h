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

#ifndef UTF8STRING_H
#define UTF8STRING_H

#include "sqliteglobal.h"

#include <QByteArray>
#include <QMetaType>

#include <iosfwd>

class Utf8StringVector;
class Utf8String;

class SQLITE_EXPORT Utf8String
{
    friend class Utf8StringVector;

    friend SQLITE_EXPORT const Utf8String operator+(const Utf8String &first, const Utf8String &second);

    friend SQLITE_EXPORT bool operator!=(const Utf8String &first, const Utf8String &second);
    friend SQLITE_EXPORT bool operator==(const Utf8String &first, const Utf8String &second);
    friend SQLITE_EXPORT bool operator==(const Utf8String &first, const char *second);
    friend SQLITE_EXPORT bool operator==(const char *first, const Utf8String &second);
    friend SQLITE_EXPORT bool operator==(const Utf8String &first, const QString &second);
    friend SQLITE_EXPORT bool operator<(const Utf8String &first, const Utf8String &second);

    friend SQLITE_EXPORT QDataStream &operator<<(QDataStream &datastream, const Utf8String &text);
    friend SQLITE_EXPORT QDataStream &operator>>(QDataStream &datastream, Utf8String &text);

    friend SQLITE_EXPORT uint qHash(const Utf8String &utf8String);

public:
    Utf8String() = default;
    explicit Utf8String(const char *utf8Text, int size);
    Utf8String(const QString &text);

    const char *constData() const;

    int byteSize() const;

    static Utf8String fromUtf8(const char *utf8Text);
    static Utf8String fromByteArray(const QByteArray &utf8ByteArray);
    const QByteArray &toByteArray() const;

    static Utf8String fromString(const QString &text);
    QString toString() const;

    Utf8String mid(int position, int length = -1) const;
    void replace(const Utf8String &before, const Utf8String &after);
    void replace(int position, int length, const Utf8String &after);
    Utf8StringVector split(char separator) const;

    void clear();

    void append(const Utf8String &textToAppend);
    bool contains(const Utf8String &text) const;
    bool contains(const char *text) const;
    bool contains(char character) const;
    bool startsWith(const Utf8String &text) const;
    bool startsWith(const char *text) const;
    bool startsWith(char character) const;
    bool endsWith(const Utf8String &text) const;
    bool isNull() const;
    bool isEmpty() const;
    bool hasContent() const;

    void reserve(int reserveSize);

    static Utf8String number(int number, int base=10);

    const Utf8String &operator+=(const Utf8String &text);

    static void registerType();

    operator QString () const;
    operator const QByteArray & () const;

protected:
    explicit Utf8String(const QByteArray &utf8ByteArray);

private:
    QByteArray byteArray;
};

SQLITE_EXPORT const Utf8String operator+(const Utf8String &first, const Utf8String &second);

SQLITE_EXPORT bool operator!=(const Utf8String &first, const Utf8String &second);
SQLITE_EXPORT bool operator==(const Utf8String &first, const Utf8String &second);
SQLITE_EXPORT bool operator==(const Utf8String &first, const char *second);
SQLITE_EXPORT bool operator==(const char *first, const Utf8String &second);
SQLITE_EXPORT bool operator==(const Utf8String &first, const QString &second);
SQLITE_EXPORT bool operator<(const Utf8String &first, const Utf8String &second);

SQLITE_EXPORT QDataStream &operator<<(QDataStream &datastream, const Utf8String &text);
SQLITE_EXPORT QDataStream &operator>>(QDataStream &datastream, Utf8String &text);
SQLITE_EXPORT QDebug operator<<(QDebug debug, const Utf8String &text);
SQLITE_EXPORT void PrintTo(const Utf8String &text, ::std::ostream* os);
SQLITE_EXPORT std::ostream& operator<<(std::ostream &os, const Utf8String &utf8String);

SQLITE_EXPORT uint qHash(const Utf8String &utf8String);

#define Utf8StringLiteral(str) Utf8String::fromByteArray(QByteArrayLiteral(str))

Q_DECLARE_METATYPE(Utf8String)

#endif // UTF8STRING_H
