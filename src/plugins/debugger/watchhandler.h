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

#ifndef DEBUGGER_WATCHHANDLER_H
#define DEBUGGER_WATCHHANDLER_H

#include <QtCore/QPointer>
#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtGui/QTreeView>
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
        ChildCountNeeded = 1,
        ValueNeeded = 2,
        TypeNeeded = 4,
        ChildrenNeeded = 8,

        NeededMask = ValueNeeded
            | TypeNeeded
            | ChildrenNeeded
            | ChildCountNeeded,

        InitialState = ValueNeeded
            | TypeNeeded
            | ChildrenNeeded
            | ChildCountNeeded
    };

    void setValue(const QByteArray &);
    void setType(const QString &);
    void setValueToolTip(const QString &);
    void setError(const QString &);
    void setAddress(const QString &address);

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

    bool isChildCountNeeded() const { return state & ChildCountNeeded; }
    bool isChildCountKnown() const { return !(state & ChildCountNeeded); }
    void setChildCountNeeded() { state = State(state | ChildCountNeeded); }
    void setChildCountUnneeded() { state = State(state & ~ChildCountNeeded); }
    void setChildCount(int n) { childCount = n; setChildCountUnneeded();
        if (n == 0) setChildrenUnneeded(); }

    QString toString() const;
    bool isLocal() const { return iname.startsWith(QLatin1String("local.")); }
    bool isWatcher() const { return iname.startsWith(QLatin1String("watch.")); };
    bool isValid() const { return !iname.isEmpty(); }

public:
    QString iname;        // internal name sth like 'local.baz.public.a'
    QString exp;          // the expression
    QString name;         // displayed name
    QString value;        // displayed value
    QByteArray editvalue; // displayed value
    QString valuetooltip; // tooltip in value column
    QString type;         // displayed type
    QString variable;     // name of internal Gdb variable if created
    QString addr;         // displayed adress
    QString framekey;     // key for type cache
    QScriptValue scriptValue; // if needed...
    int childCount;
    bool valuedisabled;   // value will be greyed out

private:

public:
    int state;

    // Model
    int parentIndex;
    int row;
    int level;
    QList<int> childIndex;
    bool changed;
};

enum { INameRole = Qt::UserRole, VisualRole, ExpandedRole };


class WatchHandler : public QAbstractItemModel
{
    Q_OBJECT

public:
    WatchHandler();
    QAbstractItemModel *model() { return this; }

    //
    //  QAbstractItemModel
    //
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int, int, const QModelIndex &idx) const;
    QModelIndex parent(const QModelIndex &idx) const;
    int rowCount(const QModelIndex &idx) const;
    int columnCount(const QModelIndex &idx) const;
    bool hasChildren(const QModelIndex &idx) const;
    Qt::ItemFlags flags(const QModelIndex &idx) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;
    bool checkIndex(int id) const;
    
//public slots:
    void cleanup();
    void watchExpression(const QString &exp);
    void removeWatchExpression(const QString &exp);
    void reinitializeWatchers();

    void collapseChildren(const QModelIndex &idx);
    void expandChildren(const QModelIndex &idx);

    void rebuildModel(); // unconditionally version of above
    void showEditValue(const WatchData &data);

    bool isDisplayedIName(const QString &iname) const
        { return m_displayedINames.contains(iname); }
    bool isExpandedIName(const QString &iname) const
        { return m_expandedINames.contains(iname); }

    void insertData(const WatchData &data);
    QList<WatchData> takeCurrentIncompletes();

    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);

    WatchData *findData(const QString &iname);

    void loadSessionData();
    void saveSessionData();

signals:
    void watchModelUpdateRequested();

    void sessionValueRequested(const QString &name, QVariant *value);
    void setSessionValueRequested(const QString &name, const QVariant &value);

private:
    void reinitializeWatchersHelper();
    WatchData takeData(const QString &iname);
    QString toString() const;

    void loadWatchers();
    void saveWatchers();

    bool m_expandPointers;
    bool m_inChange;

    typedef QMap<QString, QPointer<QWidget> > EditWindows;
    EditWindows m_editWindows;

    QList<WatchData> m_incompleteSet;
    QList<WatchData> m_completeSet;
    QList<WatchData> m_oldSet;
    QList<WatchData> m_displaySet;
    QStringList m_watchers;

    void setDisplayedIName(const QString &iname, bool on);
    QSet<QString> m_expandedINames;  // those expanded in the treeview
    QSet<QString> m_displayedINames; // those with "external" viewers

    bool m_inFetchMore;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::WatchData);

#endif // DEBUGGER_WATCHHANDLER_H
