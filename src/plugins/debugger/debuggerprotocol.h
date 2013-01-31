/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_PROTOCOL_H
#define DEBUGGER_PROTOCOL_H

#include "debugger_global.h"

#include <QByteArray>
#include <QList>
#include <QVariant>

namespace Debugger {
namespace Internal {

/*

output ==>
    ( out-of-band-record )* [ result-record ] "(gdb)" nl
result-record ==>
     [ token ] "^" result-class ( "," result )* nl
out-of-band-record ==>
    async-record | stream-record
async-record ==>
    exec-async-output | status-async-output | notify-async-output
exec-async-output ==>
    [ token ] "*" async-output
status-async-output ==>
    [ token ] "+" async-output
notify-async-output ==>
    [ token ] "=" async-output
async-output ==>
    async-class ( "," result )* nl
result-class ==>
    "done" | "running" | "connected" | "error" | "exit"
async-class ==>
    "stopped" | others (where others will be added depending on the needs--this is still in development).
result ==>
     variable "=" value
variable ==>
     string
value ==>
     const | tuple | list
const ==>
    c-string
tuple ==>
     "{}" | "{" result ( "," result )* "}"
list ==>
     "[]" | "[" value ( "," value )* "]" | "[" result ( "," result )* "]"
stream-record ==>
    console-stream-output | target-stream-output | log-stream-output
console-stream-output ==>
    "~" c-string
target-stream-output ==>
    "@" c-string
log-stream-output ==>
    "&" c-string
nl ==>
    CR | CR-LF
token ==>
    any sequence of digits.

 */

// FIXME: rename into GdbMiValue
class GdbMi
{
public:
    GdbMi() : m_type(Invalid) {}

    QByteArray m_name;
    QByteArray m_data;
    QList<GdbMi> m_children;

    enum Type {
        Invalid,
        Const,
        Tuple,
        List
    };

    Type m_type;

    inline Type type() const { return m_type; }
    inline QByteArray name() const { return m_name; }
    inline bool hasName(const char *name) const { return m_name == name; }

    inline bool isValid() const { return m_type != Invalid; }
    inline bool isConst() const { return m_type == Const; }
    inline bool isTuple() const { return m_type == Tuple; }
    inline bool isList() const { return m_type == List; }


    inline QByteArray data() const { return m_data; }
    inline const QList<GdbMi> &children() const { return m_children; }
    inline int childCount() const { return m_children.size(); }

    const GdbMi &childAt(int index) const { return m_children[index]; }
    GdbMi &childAt(int index) { return m_children[index]; }
    GdbMi findChild(const char *name) const;

    QByteArray toString(bool multiline = false, int indent = 0) const;
    qulonglong toAddress() const;
    void fromString(const QByteArray &str);
    void fromStringMultiple(const QByteArray &str);

private:
    friend class GdbResponse;
    friend class GdbEngine;

    static QByteArray parseCString(const char *&from, const char *to);
    static QByteArray escapeCString(const QByteArray &ba);
    void parseResultOrValue(const char *&from, const char *to);
    void parseValue(const char *&from, const char *to);
    void parseTuple(const char *&from, const char *to);
    void parseTuple_helper(const char *&from, const char *to);
    void parseList(const char *&from, const char *to);

    void dumpChildren(QByteArray *str, bool multiline, int indent) const;
};

enum GdbResultClass
{
    // "done" | "running" | "connected" | "error" | "exit"
    GdbResultUnknown,
    GdbResultDone,
    GdbResultRunning,
    GdbResultConnected,
    GdbResultError,
    GdbResultExit
};

class GdbResponse
{
public:
    GdbResponse() : token(-1), resultClass(GdbResultUnknown) {}
    QByteArray toString() const;
    static QByteArray stringFromResultClass(GdbResultClass resultClass);

    int            token;
    GdbResultClass resultClass;
    GdbMi          data;
    QVariant       cookie;
    QByteArray     logStreamOutput;
    QByteArray     consoleStreamOutput;
};

void extractGdbVersion(const QString &msg,
    int *gdbVersion, int *gdbBuildVersion, bool *isMacGdb, bool *isQnxGdb);

// Keep in sync with dumper.py
enum DebuggerEncoding
{
    Unencoded8Bit                          =  0,
    Base64Encoded8BitWithQuotes            =  1,
    Base64Encoded16BitWithQuotes           =  2,
    Base64Encoded32BitWithQuotes           =  3,
    Base64Encoded16Bit                     =  4,
    Base64Encoded8Bit                      =  5,
    Hex2EncodedLatin1WithQuotes            =  6,
    Hex4EncodedLittleEndianWithQuotes      =  7,
    Hex8EncodedLittleEndianWithQuotes      =  8,
    Hex2EncodedUtf8WithQuotes              =  9,
    Hex8EncodedBigEndian                   = 10,
    Hex4EncodedBigEndianWithQuotes         = 11,
    Hex4EncodedLittleEndianWithoutQuotes   = 12,
    Hex2EncodedLocal8BitWithQuotes         = 13,
    JulianDate                             = 14,
    MillisecondsSinceMidnight              = 15,
    JulianDateAndMillisecondsSinceMidnight = 16,
    Hex2EncodedInt1                        = 17,
    Hex2EncodedInt2                        = 18,
    Hex2EncodedInt4                        = 19,
    Hex2EncodedInt8                        = 20,
    Hex2EncodedUInt1                       = 21,
    Hex2EncodedUInt2                       = 22,
    Hex2EncodedUInt4                       = 23,
    Hex2EncodedUInt8                       = 24,
    Hex2EncodedFloat4                      = 25,
    Hex2EncodedFloat8                      = 26
};

// Keep in sync with dumper.py, symbolgroupvalue.cpp of CDB
enum DebuggerDisplay {
    StopDisplay                            = 0,
    DisplayImageData                       = 1,
    DisplayUtf16String                     = 2,
    DisplayImageFile                       = 3,
    DisplayProcess                         = 4,
    DisplayLatin1String                    = 5,
    DisplayUtf8String                      = 6
};
// Decode string data as returned by the dumper helpers.
QString decodeData(const QByteArray &baIn, int encoding);

} // namespace Internal
} // namespace Debugger


#endif // DEBUGGER_PROTOCOL_H
