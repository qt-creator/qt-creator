/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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


#ifndef QMLPROFILEROVERVIEWMODELPROXY_H
#define QMLPROFILEROVERVIEWMODELPROXY_H

#include <QObject>


namespace QmlProfiler {
namespace Internal {

class QmlProfilerModelManager;

class QmlProfilerOverviewModelProxy : public QObject
{
    Q_PROPERTY(bool empty READ isEmpty NOTIFY emptyChanged)

    Q_OBJECT
public:
    struct QmlOverviewEvent {
        int row;
        qint64 startTime;
        qint64 duration;
        double height;
        int eventId;
        int eventType;
        int bindingLoopHead;
    };

    QmlProfilerOverviewModelProxy(QmlProfilerModelManager *modelManager, QObject *parent = 0);
    ~QmlProfilerOverviewModelProxy();

    const QVector<QmlOverviewEvent> getData() const;

    void loadData();
    Q_INVOKABLE int count() const;
    Q_INVOKABLE qint64 traceStartTime() const;
    Q_INVOKABLE qint64 traceDuration() const;
    Q_INVOKABLE qint64 traceEndTime() const;
    Q_INVOKABLE qint64 getStartTime(int index) const;
    Q_INVOKABLE qint64 getDuration(int index) const;
    Q_INVOKABLE int getType(int index) const;
    Q_INVOKABLE int getEventId(int index) const;
    Q_INVOKABLE int getBindingLoopDest(int i) const;

    void clear();

    int rowCount() const;


// QML interface
    bool isEmpty() const;


signals:
    void countChanged();
    void dataAvailable();
//    void stateChanged();
    void emptyChanged();
//    void expandedChanged();

private slots:
    void dataChanged();

private:
    void detectBindingLoops();

private:
    class QmlProfilerOverviewModelProxyPrivate;
    QmlProfilerOverviewModelProxyPrivate *d;

};

}
}

#endif
