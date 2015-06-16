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

#ifndef UTF8STRINGVECTOR_H
#define UTF8STRINGVECTOR_H

#include "sqliteglobal.h"
#include "utf8string.h"

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


SQLITE_EXPORT QDebug operator<<(QDebug debug, const Utf8StringVector &textVector);
SQLITE_EXPORT void PrintTo(const Utf8StringVector &textVector, ::std::ostream* os);

Q_DECLARE_METATYPE(Utf8StringVector)

#endif // UTF8STRINGVECTOR_H
