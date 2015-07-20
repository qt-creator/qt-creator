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

#ifndef UTF8STRING_H
#define UTF8STRING_H

#include "sqliteglobal.h"

#include <QByteArray>
#include <QMetaType>

class Utf8StringVector;
class Utf8String;

class SQLITE_EXPORT Utf8String
{
    friend class Utf8StringVector;

    friend SQLITE_EXPORT const Utf8String operator+(const Utf8String &first, const Utf8String &second);

    friend SQLITE_EXPORT bool operator!=(const Utf8String &first, const Utf8String &second);
    friend SQLITE_EXPORT bool operator==(const Utf8String &first, const Utf8String &second);
    friend SQLITE_EXPORT bool operator==(const Utf8String &first, const char *second);
    friend SQLITE_EXPORT bool operator<(const Utf8String &first, const Utf8String &second);

    friend SQLITE_EXPORT QDataStream &operator<<(QDataStream &datastream, const Utf8String &text);
    friend SQLITE_EXPORT QDataStream &operator>>(QDataStream &datastream, Utf8String &text);

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
    Utf8StringVector split(char separator) const;

    void clear();

    void append(const Utf8String &textToAppend);
    bool contains(const Utf8String &text) const;
    bool contains(const char *text) const;
    bool contains(char character) const;
    bool startsWith(const Utf8String &text) const;
    bool startsWith(const char *text) const;
    bool startsWith(char character) const;
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
SQLITE_EXPORT bool operator<(const Utf8String &first, const Utf8String &second);

SQLITE_EXPORT QDataStream &operator<<(QDataStream &datastream, const Utf8String &text);
SQLITE_EXPORT QDataStream &operator>>(QDataStream &datastream, Utf8String &text);
SQLITE_EXPORT QDebug operator<<(QDebug debug, const Utf8String &text);
SQLITE_EXPORT void PrintTo(const Utf8String &text, ::std::ostream* os);

#define Utf8StringLiteral(str) Utf8String::fromByteArray(QByteArrayLiteral(str))

Q_DECLARE_METATYPE(Utf8String)

#endif // UTF8STRING_H
