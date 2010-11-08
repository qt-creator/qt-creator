/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "debuggerstreamops.h"

namespace Debugger {
namespace Internal {

QDataStream &operator<<(QDataStream &stream, const ThreadData &d)
{
    stream << (qint64)d.id;
    stream << d.address;
    stream << d.function;
    stream << d.fileName;
    stream << d.state;
    stream << d.lineNumber;
    stream << d.name;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, ThreadData &d)
{
    qint64 id;
    stream >> id;
    d.id = id;
    stream >> d.address;
    stream >> d.function;
    stream >> d.fileName;
    stream >> d.state;
    stream >> d.lineNumber;
    stream >> d.name;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const Threads &threads)
{
    stream << (quint64)threads.count();
    for (int i = 0; i < threads.count(); i++)
    {
        const ThreadData &d = threads.at(i);
        stream << d;
    }
    return stream;
}

QDataStream &operator>>(QDataStream &stream, Threads &threads)
{
    quint64 count;
    stream >> count;
    threads.clear();
    for (quint64 i = 0; i < count; i++)
    {
        ThreadData d;
        stream >> d;
        threads.append(d);
    }
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const StackFrame &s)
{
    stream << (quint64)s.level;
    stream << s.function;
    stream << s.file;
    stream << s.from;
    stream << s.to;
    stream << s.line;
    stream << s.address;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, StackFrame &s)
{
    quint64 level;
    stream >> level;
    s.level = level;
    stream >> s.function;
    stream >> s.file;
    stream >> s.from;
    stream >> s.to;
    stream >> s.line;
    stream >> s.address;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const StackFrames &frames)
{
    stream << (quint64)frames.count();
    for (int i = 0; i < frames.count(); i++)
    {
        const StackFrame &s = frames.at(i);
        stream << s;
    }
    return stream;
}

QDataStream &operator>>(QDataStream &stream, StackFrames &frames)
{
    quint64 count;
    stream >> count;
    frames.clear();
    for (quint64 i = 0; i < count; i++)
    {
        StackFrame s;
        stream >> s;
        frames.append(s);
    }
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const BreakpointData &s)
{
    stream << s.fileName;
    stream << s.condition;
    stream << (quint64)s.ignoreCount;
    stream << (quint64)s.lineNumber;
    stream << s.address;
    stream << s.funcName;
    stream << s.useFullPath;

    stream << s.bpNumber;
    stream << s.bpCondition;
    stream << s.bpIgnoreCount;
    stream << s.bpFileName;
    stream << s.bpFullName;
    stream << s.bpLineNumber;
    stream << s.bpCorrectedLineNumber;
    stream << s.bpThreadSpec;
    stream << s.bpFuncName;
    stream << s.bpAddress;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, BreakpointData &s)
{
    quint64 t;
    stream >> s.fileName;
    stream >> s.condition;
    stream >> t;
    s.ignoreCount = t;
    stream >> t;
    s.lineNumber = t;
    stream >> s.address;
    stream >> s.funcName;
    stream >> s.useFullPath;

    stream >> s.bpNumber;
    stream >> s.bpCondition;
    stream >> s.bpIgnoreCount;
    stream >> s.bpFileName;
    stream >> s.bpFullName;
    stream >> s.bpLineNumber;
    stream >> s.bpCorrectedLineNumber;
    stream >> s.bpThreadSpec;
    stream >> s.bpFuncName;
    stream >> s.bpAddress;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const WatchData &wd)
{
    stream << wd.id;
    stream << wd.iname;
    stream << wd.exp;
    stream << wd.name;
    stream << wd.value;
    stream << wd.editvalue;
    stream << wd.editformat;
    stream << wd.valuetooltip;
    stream << wd.typeFormats;
    stream << wd.type;
    stream << wd.displayedType;
    stream << wd.variable;
    stream << wd.address;
    stream << wd.hasChildren;
    stream << wd.generation;
    stream << wd.valueEnabled;
    stream << wd.valueEditable;
    stream << wd.error;
    stream << wd.state;
    stream << wd.changed;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, WatchData &wd)
{
    stream >> wd.id;
    stream >> wd.iname;
    stream >> wd.exp;
    stream >> wd.name;
    stream >> wd.value;
    stream >> wd.editvalue;
    stream >> wd.editformat;
    stream >> wd.valuetooltip;
    stream >> wd.typeFormats;
    stream >> wd.type;
    stream >> wd.displayedType;
    stream >> wd.variable;
    stream >> wd.address;
    stream >> wd.hasChildren;
    stream >> wd.generation;
    stream >> wd.valueEnabled;
    stream >> wd.valueEditable;
    stream >> wd.error;
    stream >> wd.state;
    stream >> wd.changed;
    return stream;
}

} // namespace Internal
} // namespace Debugger


