/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QMLPROFILEREVENTVIEW_H
#define QMLPROFILEREVENTVIEW_H

#include <QTreeView>
#include <qmljsdebugclient/qmlprofilereventtypes.h>
#include <qmljsdebugclient/qmlprofilereventlist.h>

namespace QmlProfiler {
namespace Internal {

typedef QHash<QString, QmlJsDebugClient::QmlEventData *> QmlEventHash;
typedef QList<QmlJsDebugClient::QmlEventData *> QmlEventList;

enum ItemRole {
    LocationRole = Qt::UserRole+1,
    FilenameRole = Qt::UserRole+2,
    LineRole = Qt::UserRole+3
};

class QmlProfilerEventsView : public QTreeView
{
    Q_OBJECT
public:
    enum Fields {
        Name,
        Type,
        Percent,
        TotalDuration,
        CallCount,
        TimePerCall,
        MaxTime,
        MinTime,
        MedianTime,
        Details,
        Parents,
        Children,

        MaxFields
    };

    enum ViewTypes {
        EventsView,
        CallersView,
        CalleesView,

        MaxViewTypes
    };

    explicit QmlProfilerEventsView(QWidget *parent, QmlJsDebugClient::QmlProfilerEventList *model);
    ~QmlProfilerEventsView();

    void setEventStatisticsModel(QmlJsDebugClient::QmlProfilerEventList *model);
    void setFieldViewable(Fields field, bool show);
    void setViewType(ViewTypes type);
    void setShowAnonymousEvents( bool showThem );

    QModelIndex selectedItem() const;
    void copyTableToClipboard();
    void copyRowToClipboard();

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber);
    void contextMenuRequested(const QPoint &position);

public slots:
    void clear();
    void jumpToItem(const QModelIndex &index);
    void buildModel();

private:
    void setHeaderLabels();
    void contextMenuEvent(QContextMenuEvent *ev);

private:
    class QmlProfilerEventsViewPrivate;
    QmlProfilerEventsViewPrivate *d;

};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILEREVENTVIEW_H
