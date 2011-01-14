/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_WATCHHANDLER_H
#define DEBUGGER_WATCHHANDLER_H

#include "watchdata.h"

#include <QtCore/QPointer>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QAbstractItemModel>

namespace Debugger {
class DebuggerEngine;

namespace Internal {

class WatchItem;
class WatchHandler;

enum WatchType
{
    ReturnWatch,
    LocalsWatch,
    WatchersWatch,
    TooltipsWatch
};

enum IntegerFormat
{
    DecimalFormat = 0, // Keep that at 0 as default.
    HexadecimalFormat,
    BinaryFormat,
    OctalFormat
};

class WatchModel : public QAbstractItemModel
{
    Q_OBJECT

private:
    explicit WatchModel(WatchHandler *handler, WatchType type);
    virtual ~WatchModel();

public:
    int rowCount(const QModelIndex &idx) const;
    int columnCount(const QModelIndex &idx) const;

private:
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QModelIndex index(int, int, const QModelIndex &idx) const;
    QModelIndex parent(const QModelIndex &idx) const;
    bool hasChildren(const QModelIndex &idx) const;
    Qt::ItemFlags flags(const QModelIndex &idx) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;
    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);

    friend class WatchHandler;

    WatchItem *watchItem(const QModelIndex &) const;
    QModelIndex watchIndex(const WatchItem *needle) const;
    QModelIndex watchIndexHelper(const WatchItem *needle,
        const WatchItem *parentItem, const QModelIndex &parentIndex) const;

    void insertData(const WatchData &data);
    void insertBulkData(const QList<WatchData> &data);
    WatchItem *findItem(const QByteArray &iname, WatchItem *root) const;
    void reinitialize();
    void removeOutdated();
    void removeOutdatedHelper(WatchItem *item);
    WatchItem *rootItem() const;
    void destroyItem(WatchItem *item);

    void emitDataChanged(int column,
        const QModelIndex &parentIndex = QModelIndex());
    void beginCycle(); // Called at begin of updateLocals() cycle.
    void endCycle(); // Called after all results have been received.

    friend QDebug operator<<(QDebug d, const WatchModel &m);

    void dump();
    void dumpHelper(WatchItem *item);
    void emitAllChanged();

signals:
    void enableUpdates(bool);

private:
    QString displayType(const WatchData &typeIn) const;
    void formatRequests(QByteArray *out, const WatchItem *item) const;
    DebuggerEngine *engine() const;
    int itemFormat(const WatchData &data) const;

    WatchHandler *m_handler;
    WatchType m_type;
    WatchItem *m_root;
    QSet<QByteArray> m_fetchTriggered;
};

class WatchHandler : public QObject
{
    Q_OBJECT

public:
    explicit WatchHandler(DebuggerEngine *engine);
    WatchModel *model(WatchType type) const;
    WatchModel *modelForIName(const QByteArray &iname) const;

    void cleanup();
    void watchExpression(const QString &exp);
    void removeWatchExpression(const QString &exp);
    Q_SLOT void clearWatches();
    Q_SLOT void emitAllChanged();

    void beginCycle(bool fullCycle = true); // Called at begin of updateLocals() cycle
    void updateWatchers(); // Called after locals are fetched
    void endCycle(bool fullCycle = true); // Called after all results have been received
    void showEditValue(const WatchData &data);

    void insertData(const WatchData &data);
    void insertBulkData(const QList<WatchData> &data);
    void removeData(const QByteArray &iname);

    const WatchData *watchData(WatchType type, const QModelIndex &) const;
    const WatchData *findItem(const QByteArray &iname) const;
    QModelIndex itemIndex(const QByteArray &iname) const;

    void loadSessionData();
    void saveSessionData();
    void removeTooltip();

    bool isExpandedIName(const QByteArray &iname) const
        { return m_expandedINames.contains(iname); }
    QSet<QByteArray> expandedINames() const
        { return m_expandedINames; }
    static QStringList watchedExpressions();
    static QHash<QByteArray, int> watcherNames()
        { return m_watcherNames; }

    QByteArray expansionRequests() const;
    QByteArray typeFormatRequests() const;
    QByteArray individualFormatRequests() const;

    int format(const QByteArray &iname) const;

    void addTypeFormats(const QByteArray &type, const QStringList &formats);

    QByteArray watcherName(const QByteArray &exp);
    void synchronizeWatchers();
    QString editorContents();

private:
    friend class WatchModel;

    void saveWatchers();
    static void loadTypeFormats();
    static void saveTypeFormats();

    void setFormat(const QByteArray &type, int format);
    void updateWatchersWindow();
    void showInEditorHelper(QString *contents, WatchItem *item, int level);

    bool m_inChange;

    // QWidgets and QProcesses taking care of special displays.
    typedef QMap<QString, QPointer<QObject> > EditHandlers;
    EditHandlers m_editHandlers;

    static QHash<QByteArray, int> m_watcherNames;
    static QHash<QByteArray, int> m_typeFormats;
    QHash<QByteArray, int> m_individualFormats; // Indexed by iname.
    QHash<QString, QStringList> m_reportedTypeFormats;

    // Items expanded in the Locals & Watchers view.
    QSet<QByteArray> m_expandedINames;

    WatchModel *m_return;
    WatchModel *m_locals;
    WatchModel *m_watchers;
    WatchModel *m_tooltips;
    DebuggerEngine *m_engine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHHANDLER_H
