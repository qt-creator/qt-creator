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


#ifndef QMLPROFILEREVENTSMODELPROXY_H
#define QMLPROFILEREVENTSMODELPROXY_H

#include "qmlprofilerdatamodel.h"
#include <QObject>
#include <qmldebug/qmlprofilereventtypes.h>
#include <qmldebug/qmlprofilereventlocation.h>
#include <QHash>
#include <QVector>


namespace QmlProfiler {
class QmlProfilerModelManager;

namespace Internal {

class QmlProfilerEventsModelProxy : public QObject
{
    Q_OBJECT
public:
    struct QmlEventStats {
        QString displayName;
        QString eventHashStr;
        QString details;
        QmlDebug::QmlEventLocation location;
        int eventType;
        int bindingType;

        qint64 duration;
        qint64 calls;
        qint64 minTime;
        qint64 maxTime;
        qint64 timePerCall;
        double percentOfTime;
        qint64 medianTime;

        bool isBindingLoop;
    };

    QmlProfilerEventsModelProxy(QmlProfilerModelManager *modelManager, QObject *parent = 0);
    ~QmlProfilerEventsModelProxy();

    const QList<QmlEventStats> getData() const;
    int count() const;
    void clear();

    void limitToRange(qint64 rangeStart, qint64 rangeEnd);

signals:
    void dataAvailable();

private:
    void loadData(qint64 rangeStart = -1, qint64 rangeEnd = -1);

    QSet<QString> eventsInBindingLoop() const;

private slots:
    void dataChanged();

private:
    class QmlProfilerEventsModelProxyPrivate;
    QmlProfilerEventsModelProxyPrivate *d;

    friend class QmlProfilerEventParentsModelProxy;
    friend class QmlProfilerEventChildrenModelProxy;
};

class QmlProfilerEventRelativesModelProxy : public QObject
{
    Q_OBJECT
public:
    struct QmlEventRelativesData {
        QString displayName;
        int eventType;
        qint64 duration;
        qint64 calls;
        QString details;
        bool isBindingLoop;
    };
    typedef QHash <QString, QmlEventRelativesData> QmlEventRelativesMap;

    QmlProfilerEventRelativesModelProxy(QmlProfilerModelManager *modelManager,
                                        QmlProfilerEventsModelProxy *eventsModel,
                                        QObject *parent = 0);
    ~QmlProfilerEventRelativesModelProxy();


    int count() const;
    void clear();

    const QmlEventRelativesMap getData(const QString &hash) const;

protected:
    virtual void loadData() = 0;

signals:
    void dataAvailable();

protected slots:
    void dataChanged();

protected:
    QHash <QString, QmlEventRelativesMap> m_data;
    QmlProfilerModelManager *m_modelManager;
    QmlProfilerEventsModelProxy *m_eventsModel;
    QVector <int> m_acceptedTypes;
};

class QmlProfilerEventParentsModelProxy : public QmlProfilerEventRelativesModelProxy
{
    Q_OBJECT
public:
    QmlProfilerEventParentsModelProxy(QmlProfilerModelManager *modelManager,
                                      QmlProfilerEventsModelProxy *eventsModel,
                                      QObject *parent = 0);
    ~QmlProfilerEventParentsModelProxy();

protected:
    virtual void loadData();
signals:
    void dataAvailable();
};

class QmlProfilerEventChildrenModelProxy : public QmlProfilerEventRelativesModelProxy
{
    Q_OBJECT
public:
    QmlProfilerEventChildrenModelProxy(QmlProfilerModelManager *modelManager,
                                       QmlProfilerEventsModelProxy *eventsModel,
                                       QObject *parent = 0);
    ~QmlProfilerEventChildrenModelProxy();

protected:
    virtual void loadData();
signals:
    void dataAvailable();
};

}
}

#endif
