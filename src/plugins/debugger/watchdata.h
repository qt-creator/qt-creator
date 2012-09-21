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

#ifndef DEBUGGER_WATCHDATA_H
#define DEBUGGER_WATCHDATA_H

#include <QMetaType>
#include <QtGlobal>
#include <QCoreApplication>

namespace Debugger {
namespace Internal {

class WatchData
{
public:
    WatchData();

    enum State
    {
        Complete          = 0,
        HasChildrenNeeded = 1,
        ValueNeeded       = 2,
        TypeNeeded        = 4,
        ChildrenNeeded    = 8,

        NeededMask = ValueNeeded
            | TypeNeeded
            | ChildrenNeeded
            | HasChildrenNeeded,

        InitialState = ValueNeeded
            | TypeNeeded
            | ChildrenNeeded
            | HasChildrenNeeded
    };

    bool isSomethingNeeded() const { return state & NeededMask; }
    void setAllNeeded()            { state = NeededMask; }
    void setAllUnneeded()          { state = State(0); }

    bool isTypeNeeded() const { return state & TypeNeeded; }
    bool isTypeKnown()  const { return !(state & TypeNeeded); }
    void setTypeNeeded()      { state = State(state | TypeNeeded); }
    void setTypeUnneeded()    { state = State(state & ~TypeNeeded); }

    bool isValueNeeded() const { return state & ValueNeeded; }
    bool isValueKnown()  const { return !(state & ValueNeeded); }
    void setValueNeeded()      { state = State(state | ValueNeeded); }
    void setValueUnneeded()    { state = State(state & ~ValueNeeded); }

    bool isChildrenNeeded() const { return state & ChildrenNeeded; }
    bool isChildrenKnown()  const { return !(state & ChildrenNeeded); }
    void setChildrenNeeded()   { state = State(state | ChildrenNeeded); }
    void setChildrenUnneeded() { state = State(state & ~ChildrenNeeded); }

    bool isHasChildrenNeeded() const { return state & HasChildrenNeeded; }
    bool isHasChildrenKnown()  const { return !(state & HasChildrenNeeded); }
    void setHasChildrenNeeded()   { state = State(state | HasChildrenNeeded); }
    void setHasChildrenUnneeded() { state = State(state & ~HasChildrenNeeded); }
    void setHasChildren(bool c)   { hasChildren = c; setHasChildrenUnneeded();
                                         if (!c) setChildrenUnneeded(); }

    bool isLocal()   const { return iname.startsWith("local."); }
    bool isWatcher() const { return iname.startsWith("watch."); }
    bool isInspect() const { return iname.startsWith("inspect."); }
    bool isValid()   const { return !iname.isEmpty(); }
    bool isVTablePointer() const;

    bool isEqual(const WatchData &other) const;

    void setError(const QString &);
    void setValue(const QString &);
    void setValueToolTip(const QString &);
    void setType(const QByteArray &, bool guessChildrenFromType = true);
    void setAddress(const quint64 &);
    void setHexAddress(const QByteArray &a);

    QString toString()  const;
    QString toToolTip() const;

    static QString msgNotInScope();
    static QString shadowedName(const QString &name, int seen);
    static const QString &shadowedNameFormat();

    quint64    coreAddress() const;
    QByteArray hexAddress()  const;
    QByteArray hexReferencingAddress()  const;

public:
    quint64    id;           // Token for the engine for internal mapping
    qint32     state;        // 'needed' flags;
    QByteArray iname;        // Internal name sth like 'local.baz.public.a'
    QByteArray exp;          // The expression
    QString    name;         // Displayed name
    QString    value;        // Displayed value
    QByteArray editvalue;    // Displayed value
    qint32     editformat;   // Format of displayed value
    QString    valuetooltip; // Tooltip in value column
    QString    typeFormats;  // Selection of formats of displayed value
    QByteArray type;         // Type for further processing
    QString    displayedType;// Displayed type (optional)
    quint64    address;      // Displayed address
    quint64    referencingAddress; // Address of the pointer referencing this item (gdb auto-deref)
    uint       size;         // Size
    uint       bitpos;       // Position within bit fields
    uint       bitsize;      // Size in case of bit fields
    bool hasChildren;
    bool valueEnabled;       // Value will be enabled or not
    bool valueEditable;      // Value will be editable
    bool error;
    qint32 sortId;
    QByteArray dumperFlags;

    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::WatchHandler)

public:
    // FIXME: this is engine specific data that should be mapped internally
    QByteArray variable;  // Name of internal Gdb variable if created
    qint32 source;  // Originated from dumper or symbol evaluation? (CDB only)
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::WatchData)


#endif // DEBUGGER_WATCHDATA_H
