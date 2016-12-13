/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "watchdata.h"
#include "debuggerengine.h"

#include <QVector>

namespace Debugger {
namespace Internal {

class DebuggerCommand;
class DebuggerEngine;
class WatchModel;

typedef QVector<DisplayFormat> DisplayFormats;

class WatchModelBase : public Utils::TreeModel<WatchItem, WatchItem>
{
    Q_OBJECT

public:
    WatchModelBase() {}

signals:
    void currentIndexRequested(const QModelIndex &idx);
    void itemIsExpanded(const QModelIndex &idx);
    void inameIsExpanded(const QString &iname);
    void columnAdjustmentRequested();
    void updateStarted();
    void updateFinished();
};

class WatchHandler : public QObject
{
    Q_OBJECT

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

    void loadSessionData();
    void saveSessionData();

    bool isExpandedIName(const QString &iname) const;
    QSet<QString> expandedINames() const;

    static QStringList watchedExpressions();
    static QMap<QString, int> watcherNames();

    void appendFormatRequests(DebuggerCommand *cmd);
    void appendWatchersAndTooltipRequests(DebuggerCommand *cmd);

    QString typeFormatRequests() const;
    QString individualFormatRequests() const;

    int format(const QString &iname) const;
    static QString nameForFormat(int format);

    void addDumpers(const GdbMi &dumpers);
    void addTypeFormats(const QString &type, const DisplayFormats &formats);

    QString watcherName(const QString &exp);

    void scheduleResetLocation();
    void resetLocation();

    void setCurrentItem(const QString &iname);
    void updateWatchersWindow();

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

private:
    WatchModel *m_model; // Owned.
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::DisplayFormat)
