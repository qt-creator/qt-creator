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
#include "utf8string.h"

#include <QDataStream>
#include <QVector>

class SQLITE_EXPORT Utf8StringVector : public QVector<Utf8String>
{
public:
    Utf8StringVector();
    Utf8StringVector(std::initializer_list<Utf8String> initializerList);
    explicit Utf8StringVector(const Utf8String &utf8String);
    Utf8StringVector(const QVector<Utf8String> &vector);
    explicit Utf8StringVector(const QStringList &stringList);
    explicit Utf8StringVector(int size, const Utf8String &text);

    Utf8String join(const Utf8String &separator) const;

    static Utf8StringVector fromIntegerVector(const QVector<int> &integerVector);

    static void registerType();

    inline bool removeFast(const Utf8String &valueToBeRemoved);

protected:
    int totalByteSize() const;
};

bool Utf8StringVector::removeFast(const Utf8String &valueToBeRemoved)
{
    auto position = std::remove(begin(), end(), valueToBeRemoved);

    bool hasEntry = position != end();

    erase(position, end());

    return hasEntry;
}

inline QDataStream &operator<<(QDataStream &s, const Utf8StringVector &v)
{ return s << static_cast<const QVector<Utf8String> &>(v); }

inline QDataStream &operator>>(QDataStream &s, Utf8StringVector &v)
{ return s >> static_cast<QVector<Utf8String> &>(v); }

SQLITE_EXPORT QDebug operator<<(QDebug debug, const Utf8StringVector &textVector);
SQLITE_EXPORT void PrintTo(const Utf8StringVector &textVector, ::std::ostream* os);

Q_DECLARE_METATYPE(Utf8StringVector)
