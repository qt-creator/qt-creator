/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef DEBUGGER_CODA_UTILS
#define DEBUGGER_CODA_UTILS

#include "symbianutils_global.h"

#include <QByteArray>
#include <QHash>
#include <QStringList>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QDateTime;
QT_END_NAMESPACE

namespace Coda {

typedef unsigned char byte;
struct CodaResult;

enum SerialMultiplexor {
    MuxRaw = 0,
    MuxTextTrace = 0x0102,
    MuxCoda = 0x0190
};

inline byte extractByte(const char *data) { return *data; }
SYMBIANUTILS_EXPORT ushort extractShort(const char *data);
SYMBIANUTILS_EXPORT uint extractInt(const char *data);
SYMBIANUTILS_EXPORT quint64 extractInt64(const char *data);

SYMBIANUTILS_EXPORT QString quoteUnprintableLatin1(const QByteArray &ba);

// produces "xx xx xx "
SYMBIANUTILS_EXPORT QString stringFromArray(const QByteArray &ba, int maxLen = - 1);

enum Endianness
{
    LittleEndian,
    BigEndian,
    TargetByteOrder = BigEndian
};

SYMBIANUTILS_EXPORT void appendShort(QByteArray *ba, ushort s, Endianness = TargetByteOrder);
SYMBIANUTILS_EXPORT void appendInt(QByteArray *ba, uint i, Endianness = TargetByteOrder);
SYMBIANUTILS_EXPORT void appendString(QByteArray *ba, const QByteArray &str, Endianness = TargetByteOrder, bool appendNullTerminator = true);

struct SYMBIANUTILS_EXPORT Library
{
    Library();
    explicit Library(const CodaResult &r);

    QByteArray name;
    uint codeseg;
    uint dataseg;
     //library addresses are valid for a given process (depending on memory model, they might be loaded at the same address in all processes or not)
    uint pid;
};

struct SYMBIANUTILS_EXPORT CodaAppVersion
{
    CodaAppVersion();
    void reset();

    int codaMajor;
    int codaMinor;
    int protocolMajor;
    int protocolMinor;
};

struct SYMBIANUTILS_EXPORT Session
{
    Session();
    void reset();
    QString deviceDescription(unsigned verbose) const;
    QString toString() const;
    // Answer to qXfer::libraries
    QByteArray gdbLibraryList() const;
    // Answer to qsDllInfo, can be sent chunk-wise.
    QByteArray gdbQsDllInfo(int start = 0, int count = -1) const;

    // CODA feedback
    byte cpuMajor;
    byte cpuMinor;
    byte bigEndian;
    byte defaultTypeSize;
    byte fpTypeSize;
    byte extended1TypeSize;
    byte extended2TypeSize;
    CodaAppVersion codaAppVersion;
    uint pid;
    uint mainTid;
    uint tid;
    uint codeseg;
    uint dataseg;
    QHash<uint, uint> addressToBP;

    typedef QList<Library> Libraries;
    Libraries libraries;

    // Gdb request
    QStringList modules;
};

struct SYMBIANUTILS_EXPORT CodaResult
{
    CodaResult();
    void clear();
    QString toString() const;
    // 0 for no error.
    int errorCode() const;
    QString errorString() const;

    ushort multiplex;
    byte code;
    byte token;
    QByteArray data;
    QVariant cookie;
    bool isDebugOutput;
};

SYMBIANUTILS_EXPORT QByteArray errorMessage(byte code);
SYMBIANUTILS_EXPORT QByteArray hexNumber(uint n, int digits = 0);
SYMBIANUTILS_EXPORT QByteArray hexxNumber(uint n, int digits = 0); // prepends '0x', too
SYMBIANUTILS_EXPORT uint swapEndian(uint in);

} // namespace Coda

#endif // DEBUGGER_CODA_UTILS
