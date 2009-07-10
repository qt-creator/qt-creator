/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_WATCHHANDLER_H
#define DEBUGGER_WATCHHANDLER_H

#include <QtCore/QPointer>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QAbstractItemModel>
#include <QtScript/QScriptValue>

QT_BEGIN_NAMESPACE
class QTreeView;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class WatchHandler;
enum WatchType { LocalsWatch, WatchersWatch, TooltipsWatch, WatchModelCount };

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

    bool isHasChildrenNeeded() const { return state & HasChildrenNeeded; }
    bool isHasChildrenKnown() const { return !(state & HasChildrenNeeded); }
    void setHasChildrenNeeded() { state = State(state | HasChildrenNeeded); }
    void setHasChildrenUnneeded() { state = State(state & ~HasChildrenNeeded); }
    void setHasChildren(bool c) { hasChildren = c; setHasChildrenUnneeded();
        if (!c) setChildrenUnneeded(); }

    WatchData pointerChildPlaceHolder() const;

    QString toString() const;
    QString toToolTip() const;
    bool isLocal() const { return iname.startsWith(QLatin1String("local.")); }
    bool isWatcher() const { return iname.startsWith(QLatin1String("watch.")); }
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
    QString saddr;        // stored address (pointer in container)
    QString framekey;     // key for type cache
    QScriptValue scriptValue; // if needed...
    bool hasChildren;
    int generation;       // when updated?
    bool valuedisabled;   // value will be greyed out

private:

public:
    int source;  // Used by some debuggers (CDB) to tell where it originates from (dumper or symbol evaluation)
    int state;
    bool changed;
};

enum WatchRoles
{
    INameRole = Qt::UserRole,
    ExpressionRole,
    ExpandedRole,    // used to communicate prefered expanded state to the view
    ActiveDataRole,  // used for tooltip
    TypeFormatListRole,
    TypeFormatRole,  // used to communicate alternative formats to the view
    IndividualFormatRole
};

enum IntegerFormat
{
    DecimalFormat = 0, // keep that at 0 as default
    HexadecimalFormat,
    BinaryFormat,
    OctalFormat,
};

enum DumpableFormat
{
    PrettyFormat = 0, // keep that at 0 as default
    PlainFomat, 
};

class WatchHandler;

// Item used by the model.
class WatchItem : public WatchData
{
    Q_DISABLE_COPY(WatchItem)
public:
    WatchItem();
    explicit WatchItem(const WatchData &data);

    ~WatchItem();

    void setData(const WatchData &data);
    void addChild(WatchItem *child);
    void removeChildren();

    WatchItem *parent;
    bool fetchTriggered;
    QList<WatchItem*> children;
};

/* WatchModel: To be used by synchonous debuggers.
 * Implements all of WatchModel and provides new virtuals for
 * the debugger to complete items. */

class WatchModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit WatchModel(WatchHandler *handler, WatchType type, QObject *parent = 0);
    virtual ~WatchModel();

    virtual QModelIndex index(int, int, const QModelIndex &idx = QModelIndex()) const;

    virtual int columnCount(const QModelIndex &idx= QModelIndex()) const;
    virtual int rowCount(const QModelIndex &idx) const;
    virtual bool hasChildren(const QModelIndex &idx) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

    virtual Qt::ItemFlags flags(const QModelIndex &idx) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;

    virtual QModelIndex parent(const QModelIndex &idx) const;

    WatchItem *watchItem(const QModelIndex &) const;
    QModelIndex watchIndex(const WatchItem *needle) const;
    QModelIndex watchIndexHelper(const WatchItem *needle,
        const WatchItem *parentItem, const QModelIndex &parentIndex) const;

    WatchItem *findItemByIName(const QString &iname, WatchItem *root) const;
    WatchItem *findItemByExpression(const QString &iname, WatchItem *root) const;
    void reinitialize();

    void removeItem(WatchItem *item);
    WatchItem *root() const;

    WatchItem *dummyRoot() const;

    void setValueByIName(const QString &iname, const QString &value);
    void setValueByExpression(const QString &iname, const QString &value);

    void emitDataChanged(int column, const QModelIndex &parentIndex = QModelIndex());

protected:
    WatchHandler *handler() const { return m_handler; }
    QVariant data(const WatchData &data, int column, int role) const;

    static QString parentName(const QString &iname);

private:
    WatchItem *m_root;
    WatchHandler *m_handler;
    const WatchType m_type;
};

QDebug operator<<(QDebug d, const WatchModel &wm);

/* A helper predicate for implementing model find routines */
class WatchPredicate {
public:
    enum Mode { INameMatch, ExpressionMatch };
    explicit WatchPredicate(Mode m, const QString &pattern);
    bool operator()(const WatchData &w) const;

private:
    const QString &m_pattern;
    const Mode m_mode;
};

class WatchHandler : public QObject
{
    Q_OBJECT

public:
    WatchHandler();
    void init(QTreeView *localsView);

    WatchModel *model(WatchType type) const;
    static WatchType watchTypeOfIName(const QString &iname);
    WatchModel *modelForIName(const QString &data) const;

    QStringList watcherExpressions() const;
    QString watcherName(const QString &exp);

    void setModel(WatchType type, WatchModel *model);

//public slots:
    void cleanup();
    Q_SLOT void watchExpression(); // data in action->data().toString()
    Q_SLOT void watchExpression(const QString &exp);
    Q_SLOT void removeWatchExpression();
    Q_SLOT void removeWatchExpression(const QString &exp);
    void showEditValue(const WatchData &data);

    void removeData(const QString &iname);
    WatchData *findItem(const QString &iname) const;

    void loadSessionData();
    void saveSessionData();

    bool isDisplayedIName(const QString &iname) const
        { return m_displayedINames.contains(iname); }
    bool isExpandedIName(const QString &iname) const
        { return m_expandedINames.contains(iname); }
    QSet<QString> expandedINames() const
        { return m_expandedINames; }

    // For debuggers that clean out the locals models between frames:
    // save/restore the expansion state.
    Q_SLOT void restoreLocalsViewState(int frame = 0);
    Q_SLOT void saveLocalsViewState(int frame = 0);

    static QString watcherEditPlaceHolder();

signals:
    void watcherInserted(const WatchData &data);
    void sessionValueRequested(const QString &name, QVariant *value);
    void setSessionValueRequested(const QString &name, const QVariant &value);

private:
    friend class WatchModel; // needs formats, expanded inames

    // Per stack-frame locals state
    struct LocalsViewState {
        LocalsViewState() : firstVisibleRow(0) {}
        int firstVisibleRow;
        QSet<QString> expandedINames;
    };
    typedef QMap<int, LocalsViewState> LocalsViewStateMap;

    void loadWatchers();
    void saveWatchers();
    void initWatchModel();

    void loadTypeFormats();
    void saveTypeFormats();
    void setFormat(const QString &type, int format);

    bool m_expandPointers;
    bool m_inChange;

    typedef QMap<QString, QPointer<QWidget> > EditWindows;
    EditWindows m_editWindows;

    QHash<QString, int> m_watcherNames;
    QHash<QString, int> m_typeFormats;
    QHash<QString, int> m_individualFormats;

    void setDisplayedIName(const QString &iname, bool on);
    QSet<QString> m_expandedINames;  // those expanded in the treeview
    QSet<QString> m_displayedINames; // those with "external" viewers
    LocalsViewStateMap m_localsViewState;

    WatchModel *m_models[WatchModelCount];
    QPointer<QTreeView> m_localsView;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::WatchData);

#endif // DEBUGGER_WATCHHANDLER_H
