/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

// This is essentially copied from Qt 5's
// qtbase/src/corelib/plugin/qelfparser_p.{h,cpp}

#ifndef UTILS_ELFREADER_H
#define UTILS_ELFREADER_H

#include "utils_global.h"

#include <qendian.h>
#include <qlist.h>
#include <qstring.h>

namespace Utils {

enum DebugSymbolsType
{
    UnknownSymbols,    // Unknown.
    NoSymbols,         // No usable symbols.
    SeparateSymbols,   // Symbols mentioned, but not in binary.
    PlainSymbols,      // Ordinary symbols available.
    FastSymbols        // Dwarf index available.
};

class QTCREATOR_UTILS_EXPORT ElfHeader
{
public:
    QByteArray name;
    quint32 index;
    quint32 type;
    quint64 offset;
    quint64 size;
    quint64 data;
};

class QTCREATOR_UTILS_EXPORT ElfHeaders : public QList<ElfHeader>
{
public:
    ElfHeaders() : symbolsType(UnknownSymbols) {}
    int indexOf(const QByteArray &name) const;

public:
    DebugSymbolsType symbolsType;
};

class QTCREATOR_UTILS_EXPORT ElfReader
{
public:
    explicit ElfReader(const QString &binary);
    enum Result { Ok, NotElf, Corrupt };

    enum ElfEndian { ElfLittleEndian = 0, ElfBigEndian = 1 };
    ElfHeaders readHeaders();
    QByteArray readSection(const QByteArray &sectionName);
    QString errorString() const { return m_errorString; }

private:
    friend class ElfMapper;
    Result readIt();

    QString m_binary;
    QString m_errorString;
    ElfEndian m_endian;
    ElfHeaders m_headers;
};

} // namespace Utils

#endif // UTILS_ELFREADER_H
