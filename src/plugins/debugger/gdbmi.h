/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_GDBMI_H
#define DEBUGGER_GDBMI_H

#include <QtCore/QByteArray>
#include <QtCore/QList>

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
    explicit GdbMi(const QByteArray &str) { fromString(str); }

    QByteArray m_name;
    QByteArray m_data;
    QList<GdbMi> m_children;

    enum Type {
        Invalid,
        Const,
        Tuple,
        List,
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
    GdbMi findChild(const QByteArray &name) const;
    GdbMi findChild(const QByteArray &name, const QByteArray &defaultString) const;

    QByteArray toString(bool multiline = false, int indent = 0) const;
    void fromString(const QByteArray &str);
    void setStreamOutput(const QByteArray &name, const QByteArray &content);

private:
    friend class GdbResultRecord;
    friend class GdbEngine;

    static QByteArray parseCString(const char *&from, const char *to);
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
    GdbResultCustomDone,
    GdbResultRunning,
    GdbResultConnected,
    GdbResultError,
    GdbResultExit,
};

class GdbResultRecord
{
public:
    GdbResultRecord() : token(-1), resultClass(GdbResultUnknown) {}
    QByteArray toString() const;

    int            token;
    GdbResultClass resultClass;
    GdbMi          data;
};

} // namespace Internal
} // namespace Debugger

//Q_DECLARE_METATYPE(GdbDebugger::Internal::GdbMi);

#endif // DEBUGGER_GDBMI_H
