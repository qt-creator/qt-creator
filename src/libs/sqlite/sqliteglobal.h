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

#ifndef SQLITEGLOBAL_H
#define SQLITEGLOBAL_H

#include <QtGlobal>

#if defined(BUILD_SQLITE_LIBRARY)
#  define SQLITE_EXPORT Q_DECL_EXPORT
#elif defined(BUILD_SQLITE_STATIC_LIBRARY)
#  define SQLITE_EXPORT
#else
#  define SQLITE_EXPORT Q_DECL_IMPORT
#endif


namespace Sqlite {
SQLITE_EXPORT void registerTypes();
}

enum class ColumnType {
    Numeric,
    Integer,
    Real,
    Text,
    None
};

enum class ColumnConstraint {
    PrimaryKey
};

enum class JournalMode {
    Delete,
    Truncate,
    Persist,
    Memory,
    Wal
};

enum TextEncoding {
    Utf8,
    Utf16le,
    Utf16be,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    Utf16 = Utf16le
#else
    Utf16 = Utf16be
#endif

};

QT_BEGIN_NAMESPACE
template <typename T> class QVector;
template <typename T> class QSet;
template <class Key, class T> class QHash;
template <class Key, class T> class QMap;
class QVariant;
QT_END_NAMESPACE

class Utf8String;
class Utf8StringVector;

typedef QMap<Utf8String, QVariant> RowDictionary;
typedef QVector<RowDictionary> RowDictionaries;


#endif // SQLITEGLOBAL_H
