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

#ifndef DEBUGGER_WATCHDATA_H
#define DEBUGGER_WATCHDATA_H

#include <QtCore/QMetaType>
#include <QtCore/QtGlobal>
#include <QtCore/QObject>
#include <QtScript/QScriptValue>

namespace Debugger {
namespace Internal {

class WatchData
{
public:
    WatchData();

    enum State
    {
        Complete = 0,
        HasChildrenNeeded = 1,
        ValueNeeded = 2,
        TypeNeeded = 4,
        ChildrenNeeded = 8,

        NeededMask = ValueNeeded
            | TypeNeeded
            | ChildrenNeeded
            | HasChildrenNeeded,

        InitialState = ValueNeeded
            | TypeNeeded
            | ChildrenNeeded
            | HasChildrenNeeded
    };

    void setValue(const QString &);
    void setType(const QString &, bool guessChildrenFromType = true);
    void setValueToolTip(const QString &);
    void setError(const QString &);
    void setAddress(const QByteArray &);

    bool isSomethingNeeded() const { return state & NeededMask; }
    void setAllNeeded() { state = NeededMask; }
    void setAllUnneeded() { state = State(0); }

    bool isTypeNeeded() const { return state & TypeNeeded; }
    bool isTypeKnown() const { return !(state & TypeNeeded); }
    void setTypeNeeded() { state = State(state | TypeNeeded); }
    void setTypeUnneeded() { state = State(state & ~TypeNeeded); }

    bool isValueNeeded() const { return state & ValueNeeded; }
    bool isValueKnown() const { return !(state & ValueNeeded); }
    void setValueNeeded() { state = State(state | ValueNeeded); }
    void setValueUnneeded() { state = State(state & ~ValueNeeded); }

    bool isChildrenNeeded() const { return state & ChildrenNeeded; }
    bool isChildrenKnown() const { return !(state & ChildrenNeeded); }
    void setChildrenNeeded() { state = State(state | ChildrenNeeded); }
    void setChildrenUnneeded() { state = State(state & ~ChildrenNeeded); }

    bool isHasChildrenNeeded() const { return state & HasChildrenNeeded; }
    bool isHasChildrenKnown() const { return !(state & HasChildrenNeeded); }
    void setHasChildrenNeeded() { state = State(state | HasChildrenNeeded); }
    void setHasChildrenUnneeded() { state = State(state & ~HasChildrenNeeded); }
    void setHasChildren(bool c) { hasChildren = c; setHasChildrenUnneeded();
        if (!c) setChildrenUnneeded(); }

    QString toString() const;
    QString toToolTip() const;
    bool isLocal() const { return iname.startsWith("local."); }
    bool isWatcher() const { return iname.startsWith("watch."); }
    bool isValid() const { return !iname.isEmpty(); }

    bool isEqual(const WatchData &other) const;

    static QString msgNotInScope();
    static QString shadowedName(const QString &name, int seen);
    static const QString &shadowedNameFormat();

public:
    QByteArray iname;     // Internal name sth like 'local.baz.public.a'
    QByteArray exp;       // The expression
    QString name;         // Displayed name
    QString value;        // Displayed value
    QByteArray editvalue; // Displayed value
    int editformat;       // Format of displayed value
    QString valuetooltip; // Tooltip in value column
    QString typeFormats;  // Selection of formats of displayed value
    QString type;         // Type for further processing
    QString displayedType;// Displayed type (optional)
    QByteArray variable;  // Name of internal Gdb variable if created
    QByteArray addr;      // Displayed address
    QString framekey;     // Key for type cache
    QScriptValue scriptValue; // If needed...
    bool hasChildren;
    int generation;       // When updated?
    bool valueEnabled;    // Value will be greyed out or not
    bool valueEditable;   // Value will be editable
    bool error;

public:
    int source;  // Originated from dumper or symbol evaluation? (CDB only)
    int state;
    bool changed;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::WatchData);


#endif // DEBUGGER_WATCHDATA_H
