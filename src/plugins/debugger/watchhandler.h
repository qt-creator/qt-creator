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

#ifndef DEBUGGER_WATCHHANDLER_H
#define DEBUGGER_WATCHHANDLER_H

#include "watchdata.h"

#include <QHash>
#include <QSet>
#include <QStringList>
#include <QAbstractItemModel>

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class WatchModel;

class UpdateParameters
{
public:
    UpdateParameters() { tryPartial = tooltipOnly = false; }

    bool tryPartial;
    bool tooltipOnly;
    QByteArray varList;
};

typedef QHash<QString, QStringList> TypeFormats;

enum WatchType
{
    LocalsType,
    InspectType,
    WatchersType,
    ReturnType,
    TooltipType
};

enum IntegerFormat
{
    DecimalFormat = 0, // Keep that at 0 as default.
    HexadecimalFormat,
    BinaryFormat,
    OctalFormat
};

class WatchHandler : public QObject
{
    Q_OBJECT

public:
    explicit WatchHandler(DebuggerEngine *engine);
    ~WatchHandler();

    QAbstractItemModel *model() const;

    void cleanup();
    void watchExpression(const QString &exp);
    Q_SLOT void clearWatches();

    void updateWatchers(); // Called after locals are fetched

    void showEditValue(const WatchData &data);

    const WatchData *watchData(const QModelIndex &) const;
    const WatchData *findData(const QByteArray &iname) const;
    QString displayForAutoTest(const QByteArray &iname) const;
    bool hasItem(const QByteArray &iname) const;

    void loadSessionData();
    void saveSessionData();
    void removeTooltip();
    void rebuildModel();

    bool isExpandedIName(const QByteArray &iname) const;
    QSet<QByteArray> expandedINames() const;

    static QStringList watchedExpressions();
    static QHash<QByteArray, int> watcherNames();

    QByteArray expansionRequests() const;
    QByteArray typeFormatRequests() const;
    QByteArray individualFormatRequests() const;

    int format(const QByteArray &iname) const;

    void addTypeFormats(const QByteArray &type, const QStringList &formats);
    void setTypeFormats(const TypeFormats &typeFormats);
    TypeFormats typeFormats() const;

    void setUnprintableBase(int base);
    static int unprintableBase();

    QByteArray watcherName(const QByteArray &exp);
    void synchronizeWatchers();
    QString editorContents();
    void editTypeFormats(bool includeLocals, const QByteArray &iname);

    void scheduleResetLocation();
    void resetLocation();
    bool isValidToolTip(const QByteArray &iname) const;

    void setCurrentItem(const QByteArray &iname);
    void updateWatchersWindow();

    void insertData(const WatchData &data); // Convenience.
    void insertData(const QList<WatchData> &list);
    void insertIncompleteData(const WatchData &data);
    void removeData(const QByteArray &iname);
    void removeChildren(const QByteArray &iname);
    void removeAllData();
    void markAllUnchanged();

private:
    friend class WatchModel;

    void saveWatchers();
    static void loadTypeFormats();
    static void saveTypeFormats();

    void setFormat(const QByteArray &type, int format);

    WatchModel *m_model;
    DebuggerEngine *m_engine;

    int m_watcherCounter;

    bool m_contentsValid;
    bool m_resetLocationScheduled;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::UpdateParameters)

#endif // DEBUGGER_WATCHHANDLER_H
