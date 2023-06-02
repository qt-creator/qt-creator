// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "watchdata.h"
#include "debuggerengine.h"

#include <QVector>

namespace Debugger {
namespace Internal {

class DebuggerCommand;
class DebuggerEngine;
class WatchModel;

using DisplayFormats = QVector<DisplayFormat>;

class WatchModelBase : public Utils::TreeModel<WatchItem, WatchItem>
{
    Q_OBJECT

public:
    WatchModelBase() = default;

    enum { NameColumn, TimeColumn, ValueColumn, TypeColumn };

signals:
    void currentIndexRequested(const QModelIndex &idx);
    void itemIsExpanded(const QModelIndex &idx);
    void inameIsExpanded(const QString &iname);
    void updateStarted();
    void updateFinished();
};

class WatchHandler
{
    Q_DISABLE_COPY_MOVE(WatchHandler)

public:
    explicit WatchHandler(DebuggerEngine *engine);
    ~WatchHandler();

    WatchModelBase *model() const;

    void cleanup();
    void grabWidget(QWidget *viewParent);
    void watchExpression(const QString &exp, const QString &name = QString(),
                         bool temporary = false);
    void updateWatchExpression(WatchItem *item, const QString &newExp);
    void watchVariable(const QString &exp);

    const WatchItem *watchItem(const QModelIndex &) const;
    void fetchMore(const QString &iname) const;
    WatchItem *findItem(const QString &iname) const;
    const WatchItem *findCppLocalVariable(const QString &name) const;

    void loadSessionDataForEngine();

    bool isExpandedIName(const QString &iname) const;
    QSet<QString> expandedINames() const;
    int maxArrayCount(const QString &iname) const;

    static QStringList watchedExpressions();
    static QMap<QString, int> watcherNames();

    void appendFormatRequests(DebuggerCommand *cmd) const;
    void appendWatchersAndTooltipRequests(DebuggerCommand *cmd) const;

    QString typeFormatRequests() const;
    QString individualFormatRequests() const;

    int format(const QString &iname) const;
    static QString nameForFormat(int format);

    void addDumpers(const GdbMi &dumpers);
    void addTypeFormats(const QString &type, const DisplayFormats &formats);

    QString watcherName(const QString &exp);

    void scheduleResetLocation();

    void setCurrentItem(const QString &iname);
    void updateLocalsWindow();

    bool insertItem(WatchItem *item); // Takes ownership, returns whether item was added, not overwritten.
    void insertItems(const GdbMi &data);

    void removeItemByIName(const QString &iname);
    void removeAllData(bool includeInspectData = false);
    void resetValueCache();
    void resetWatchers();

    void notifyUpdateStarted(const UpdateParameters &updateParameters = UpdateParameters());
    void notifyUpdateFinished();

    void reexpandItems();
    void recordTypeInfo(const GdbMi &typeInfo);

    void setLocation(const Location &loc);

private:
    DebuggerEngine * const m_engine; // Not owned
    WatchModel *m_model; // Owned.
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::DisplayFormat)
