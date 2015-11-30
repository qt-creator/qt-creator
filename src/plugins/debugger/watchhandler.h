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

#ifndef DEBUGGER_WATCHHANDLER_H
#define DEBUGGER_WATCHHANDLER_H

#include "watchdata.h"

#include <utils/treemodel.h>

#include <QVector>

namespace Debugger {
namespace Internal {

class DebuggerCommand;
class DebuggerEngine;
class WatchModel;

typedef QVector<DisplayFormat> DisplayFormats;

class WatchItem : public Utils::TreeItem, public WatchData
{
public:
    WatchItem() {}
    WatchItem(const QByteArray &i, const QString &n);
    explicit WatchItem(const WatchData &data);
    explicit WatchItem(const GdbMi &data);

    void fetchMore();

    QString displayName() const;
    QString displayType() const;
    QString displayValue() const;
    QString formattedValue() const;
    QString expression() const;

    int itemFormat() const;

    QVariant editValue() const;
    int editType() const;
    QColor valueColor(int column) const;

    int requestedFormat() const;
    WatchItem *findItem(const QByteArray &iname);

private:
    WatchItem *parentItem() const;
    const WatchModel *watchModel() const;
    WatchModel *watchModel();
    DisplayFormats typeFormatList() const;

    bool canFetchMore() const;
    QVariant data(int column, int role) const;
    Qt::ItemFlags flags(int column) const;

    void parseWatchData(const GdbMi &input);
};

class WatchModelBase : public Utils::TreeModel
{
    Q_OBJECT

public:
    WatchModelBase() {}

signals:
    void currentIndexRequested(const QModelIndex &idx);
    void itemIsExpanded(const QModelIndex &idx);
    void inameIsExpanded(const QByteArray &iname);
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
    void watchExpression(const QString &exp, const QString &name = QString());
    void updateWatchExpression(WatchItem *item, const QByteArray &newExp);
    void watchVariable(const QString &exp);
    Q_SLOT void clearWatches();

    const WatchItem *watchItem(const QModelIndex &) const;
    void fetchMore(const QByteArray &iname) const;
    WatchItem *findItem(const QByteArray &iname) const;
    const WatchItem *findCppLocalVariable(const QString &name) const;

    void loadSessionData();
    void saveSessionData();

    bool isExpandedIName(const QByteArray &iname) const;
    QSet<QByteArray> expandedINames() const;

    static QStringList watchedExpressions();
    static QHash<QByteArray, int> watcherNames();

    void appendFormatRequests(DebuggerCommand *cmd);
    void appendWatchersAndTooltipRequests(DebuggerCommand *cmd);

    QByteArray typeFormatRequests() const;
    QByteArray individualFormatRequests() const;

    int format(const QByteArray &iname) const;
    static QString nameForFormat(int format);

    void addDumpers(const GdbMi &dumpers);
    void addTypeFormats(const QByteArray &type, const DisplayFormats &formats);

    void setUnprintableBase(int base);
    static int unprintableBase();

    QByteArray watcherName(const QByteArray &exp);
    QString editorContents();

    void scheduleResetLocation();
    void resetLocation();

    void setCurrentItem(const QByteArray &iname);
    void updateWatchersWindow();

    void insertItem(WatchItem *item); // Takes ownership.
    void removeItemByIName(const QByteArray &iname);
    void removeAllData(bool includeInspectData = false);
    void resetValueCache();
    void resetWatchers();

    void notifyUpdateStarted(const QList<QByteArray> &inames = {});
    void notifyUpdateFinished();

    void reexpandItems();

private:
    WatchModel *m_model; // Owned.
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::DisplayFormat)

#endif // DEBUGGER_WATCHHANDLER_H
