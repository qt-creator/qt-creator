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

#ifndef DEBUGGER_WATCHDATA_H
#define DEBUGGER_WATCHDATA_H

#include "debuggerprotocol.h"

#include <QCoreApplication>
#include <QMetaType>

#include <functional>

namespace Debugger {
namespace Internal {

class GdbMi;

class WatchData
{
public:
    WatchData();

    enum State
    {
        HasChildrenNeeded = 1,
        ValueNeeded       = 2,
        ChildrenNeeded    = 8,

        NeededMask = ValueNeeded
            | ChildrenNeeded
            | HasChildrenNeeded,

        InitialState = ValueNeeded
            | ChildrenNeeded
            | HasChildrenNeeded
    };

    bool isSomethingNeeded() const { return state & NeededMask; }
    void setAllNeeded()            { state = NeededMask; }
    void setAllUnneeded()          { state = State(0); }

    bool isValueNeeded() const { return state & ValueNeeded; }
    void setValueNeeded()      { state = State(state | ValueNeeded); }
    void setValueUnneeded()    { state = State(state & ~ValueNeeded); }

    bool isChildrenNeeded() const { return state & ChildrenNeeded; }
    void setChildrenNeeded()   { state = State(state | ChildrenNeeded); }
    void setChildrenUnneeded() { state = State(state & ~ChildrenNeeded); }
    void setHasChildren(bool c)   { wantsChildren = c;  if (!c) setChildrenUnneeded(); }

    bool isLocal()   const { return iname.startsWith("local."); }
    bool isWatcher() const { return iname.startsWith("watch."); }
    bool isInspect() const { return iname.startsWith("inspect."); }
    bool isValid()   const { return !iname.isEmpty(); }
    bool isVTablePointer() const;

    bool isAncestorOf(const QByteArray &childIName) const;

    void setError(const QString &);
    void setValue(const QString &);
    void setType(const QByteArray &, bool guessChildrenFromType = true);
    void setHexAddress(const QByteArray &a);

    QString toString()  const;
    QString toToolTip() const;

    static QString msgNotInScope();
    static QString shadowedName(const QString &name, int seen);
    static const QString &shadowedNameFormat();

    QByteArray hexAddress()  const;

    // Protocol interaction.
    void updateValue(const GdbMi &item);
    void updateChildCount(const GdbMi &mi);
    void updateType(const GdbMi &item);
    void updateDisplayedType(const GdbMi &item);

public:
    quint64         id;            // Token for the engine for internal mapping
    qint32          state;         // 'needed' flags;
    QByteArray      iname;         // Internal name sth like 'local.baz.public.a'
    QByteArray      exp;           // The expression
    QString         name;          // Displayed name
    QString         value;         // Displayed value
    QByteArray      editvalue;     // Displayed value
    DebuggerDisplay editformat;    // Format of displayed value
    QByteArray      type;          // Type for further processing
    QString         displayedType; // Displayed type (optional)
    quint64         address;       // Displayed address of the actual object
    quint64         origaddr;      // Address of the pointer referencing this item (gdb auto-deref)
    uint            size;          // Size
    uint            bitpos;        // Position within bit fields
    uint            bitsize;       // Size in case of bit fields
    int             elided;        // Full size if value was cut off, -1 if cut on unknown size, 0 otherwise
    bool            wantsChildren;
    bool            valueEnabled;  // Value will be enabled or not
    bool            valueEditable; // Value will be editable
    qint32          sortId;

    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::WatchHandler)
};

void decodeArrayData(std::function<void(const WatchData &)> itemHandler,
                     const WatchData &tmplate,
                     const QByteArray &rawData,
                     int encoding);

void parseChildrenData(const WatchData &parent, const GdbMi &child,
                       std::function<void(const WatchData &)> itemHandler,
                       std::function<void(const WatchData &, const GdbMi &)> childHandler,
                       std::function<void(const WatchData &childTemplate,
                                          const QByteArray &encodedData,
                                          int encoding)> arrayDecoder);

void parseWatchData(const WatchData &parent, const GdbMi &child,
                    QList<WatchData> *insertions);

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::WatchData)


#endif // DEBUGGER_WATCHDATA_H
