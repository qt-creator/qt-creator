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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_PROTOCOL_H
#define DEBUGGER_PROTOCOL_H

#include <QByteArray>
#include <QList>
#include <QString>

#include <functional>
#include <vector>

namespace Debugger {
namespace Internal {

class DebuggerResponse;

// Convenience structure to build up backend commands.
class DebuggerCommand
{
public:
    typedef std::function<void(const DebuggerResponse &)> Callback;

    DebuggerCommand() : flags(0) {}
    DebuggerCommand(const char *f, int flags = 0, Callback cb = Callback())
        : function(f), callback(cb), flags(flags)
    {}
    DebuggerCommand(const char *f, Callback cb)
        : function(f), callback(cb), flags(0)
    {}
    DebuggerCommand(const QByteArray &f) : function(f), flags(0) {}

    void arg(const char *name);
    void arg(const char *name, int value);
    void arg(const char *name, qlonglong value);
    void arg(const char *name, qulonglong value);
    void arg(const char *name, const QString &value);
    void arg(const char *name, const QByteArray &value);
    void arg(const char *name, const char *value);
    void arg(const char *name, const QList<int> &list);

    void beginList(const char *name = 0);
    void endList();
    void beginGroup(const char *name = 0);
    void endGroup();
    QByteArray arguments() const;

    QByteArray function;
    QByteArray args;
    Callback callback;
    uint postTime; // msecsSinceStartOfDay
    int flags;

private:
    void argHelper(const char *name, const QByteArray &value);
};

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
    std::vector<GdbMi> m_children;

    enum Type { Invalid, Const, Tuple, List };

    Type m_type;

    Type type() const { return m_type; }
    const QByteArray &name() const { return m_name; }
    bool hasName(const char *name) const { return m_name == name; }

    bool isValid() const { return m_type != Invalid; }
    bool isList() const { return m_type == List; }

    const QByteArray &data() const { return m_data; }
    const std::vector<GdbMi> &children() const { return m_children; }
    int childCount() const { return int(m_children.size()); }

    const GdbMi &childAt(int index) const { return m_children[index]; }
    const GdbMi &operator[](const char *name) const;

    QByteArray toString(bool multiline = false, int indent = 0) const;
    qulonglong toAddress() const;
    int toInt() const { return m_data.toInt(); }
    QString toUtf8() const { return QString::fromUtf8(m_data); }
    QString toLatin1() const { return QString::fromLatin1(m_data); }
    void fromString(const QByteArray &str);
    void fromStringMultiple(const QByteArray &str);

    static QByteArray parseCString(const char *&from, const char *to);
    static QByteArray escapeCString(const QByteArray &ba);
    void parseResultOrValue(const char *&from, const char *to);
    void parseValue(const char *&from, const char *to);
    void parseTuple(const char *&from, const char *to);
    void parseTuple_helper(const char *&from, const char *to);
    void parseList(const char *&from, const char *to);

private:
    void dumpChildren(QByteArray *str, bool multiline, int indent) const;
};

enum ResultClass
{
    // "done" | "running" | "connected" | "error" | "exit"
    ResultUnknown,
    ResultDone,
    ResultRunning,
    ResultConnected,
    ResultError,
    ResultExit
};

class DebuggerResponse
{
public:
    DebuggerResponse() : token(-1), resultClass(ResultUnknown) {}
    QByteArray toString() const;
    static QByteArray stringFromResultClass(ResultClass resultClass);

    int            token;
    ResultClass    resultClass;
    GdbMi          data;
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
    Hex2EncodedFloat8                      = 26,
    IPv6AddressAndHexScopeId               = 27,
    Hex2EncodedUtf8WithoutQuotes           = 28,
    DateTimeInternal                       = 29,
    SpecialEmptyValue                      = 30,
    SpecialUninitializedValue              = 31,
    SpecialInvalidValue                    = 32,
    SpecialNotAccessibleValue              = 33,
    SpecialItemCountValue                  = 34,
    SpecialMinimumItemCountValue           = 35,
    SpecialNotCallableValue                = 36,
    SpecialNullReferenceValue              = 37,
    SpecialOptimizedOutValue               = 38,
    SpecialEmptyStructureValue             = 39
};

// Keep in sync with dumper.py, symbolgroupvalue.cpp of CDB
enum DebuggerDisplay {
    StopDisplay                            = 0,
    DisplayImageData                       = 1,
    DisplayUtf16String                     = 2,
    DisplayImageFile                       = 3,
    DisplayLatin1String                    = 4,
    DisplayUtf8String                      = 5,
    DisplayPlotData                        = 6
};
// Decode string data as returned by the dumper helpers.
QString decodeData(const QByteArray &baIn, int encoding);

} // namespace Internal
} // namespace Debugger


#endif // DEBUGGER_PROTOCOL_H
