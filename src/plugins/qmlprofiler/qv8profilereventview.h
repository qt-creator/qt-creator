/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QV8PROFILEREVENTVIEW_H
#define QV8PROFILEREVENTVIEW_H

#include <QStandardItemModel>
#include <qmldebug/qmlprofilereventtypes.h>
#include "qmlprofilermodelmanager.h"
#include "qmlprofilereventsmodelproxy.h"
#include "qv8profilerdatamodel.h"
#include "qmlprofilertreeview.h"

#include <analyzerbase/ianalyzertool.h>

#include "qmlprofilerviewmanager.h"

namespace QmlProfiler {

namespace Internal {

class QV8ProfilerEventsMainView;
class QV8ProfilerEventRelativesView;


class QV8ProfilerEventsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QV8ProfilerEventsWidget(QWidget *parent,
                                     Analyzer::IAnalyzerTool *profilerTool,
                                     QmlProfilerViewManager *container,
                                     QmlProfilerModelManager *profilerModelManager );
    ~QV8ProfilerEventsWidget();

    void clear();

    void getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd);
    QModelIndex selectedItem() const;
    bool mouseOnTable(const QPoint &position) const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void resized();

public slots:
    void updateSelectedEvent(int eventId) const;
    void selectBySourceLocation(const QString &filename, int line, int column);
    void updateEnabledState();

protected:
    void contextMenuEvent(QContextMenuEvent *ev);
    virtual void resizeEvent(QResizeEvent *event);

private:
    class QV8ProfilerEventsWidgetPrivate;
    QV8ProfilerEventsWidgetPrivate *d;
};

class QV8ProfilerEventsMainView : public QmlProfilerTreeView
{
    Q_OBJECT
public:

    explicit QV8ProfilerEventsMainView(QWidget *parent,
                                       QV8ProfilerDataModel *v8Model);
    ~QV8ProfilerEventsMainView();

    void setFieldViewable(Fields field, bool show);
    void setShowAnonymousEvents( bool showThem );

    QModelIndex selectedItem() const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    int selectedEventId() const;

    void setShowExtendedStatistics(bool);
    bool showExtendedStatistics() const;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void eventSelected(int eventId);

public slots:
    void clear();
    void jumpToItem(const QModelIndex &index);
    void selectEvent(int eventId);
    void selectEventByLocation(const QString &filename, int line);
    void buildModel();

private:
    void setHeaderLabels();

private:
    class QV8ProfilerEventsMainViewPrivate;
    QV8ProfilerEventsMainViewPrivate *d;
};

class QV8ProfilerEventRelativesView : public QmlProfilerTreeView
{
    Q_OBJECT
public:
    enum SubViewType {
        ParentsView,
        ChildrenView
    };

    QV8ProfilerEventRelativesView(QV8ProfilerDataModel *model, SubViewType viewType,
                                  QWidget *parent);
    ~QV8ProfilerEventRelativesView();

signals:
    void eventClicked(int eventId);

public slots:
    void displayEvent(int eventId);
    void jumpToItem(const QModelIndex &);
    void clear();

private:
    void rebuildTree(QList<QV8ProfilerDataModel::QV8EventSub*> events);
    void updateHeader();

    QV8ProfilerEventRelativesView::SubViewType m_type;
    QV8ProfilerDataModel *m_v8Model;
    QStandardItemModel *m_model;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QV8PROFILEREVENTVIEW_H
