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


#ifndef QMLPROFILERPAINTEVENTSMODELPROXY_H
#define QMLPROFILERPAINTEVENTSMODELPROXY_H

#include <QObject>
#include "abstracttimelinemodel.h"
#include <qmldebug/qmlprofilereventtypes.h>
#include <qmldebug/qmlprofilereventlocation.h>
//#include <QHash>
//#include <QVector>
#include <QVariantList>
//#include <QVariantMap>
#include "qmlprofilersimplemodel.h"
#include <QColor>


namespace QmlProfiler {
class QmlProfilerModelManager;

namespace Internal {

class PaintEventsModelProxy : public AbstractTimelineModel
{
//    Q_PROPERTY(bool empty READ isEmpty NOTIFY emptyChanged)

    Q_OBJECT
public:

    struct QmlPaintEventData {
        int framerate;
        int animationcount;
    };

    PaintEventsModelProxy(QObject *parent = 0);
    ~PaintEventsModelProxy();


    int categories() const;
    QString name() const;

    void loadData();
    Q_INVOKABLE int count() const;
    void clear();

    bool isEmpty() const;

    Q_INVOKABLE qint64 lastTimeMark() const;

    Q_INVOKABLE bool expanded(int category) const;
    Q_INVOKABLE void setExpanded(int category, bool expanded);
    Q_INVOKABLE int categoryDepth(int categoryIndex) const;
    Q_INVOKABLE int categoryCount() const;
    Q_INVOKABLE const QString categoryLabel(int categoryIndex) const;

    int findFirstIndex(qint64 startTime) const;
    int findFirstIndexNoParents(qint64 startTime) const;
    int findLastIndex(qint64 endTime) const;

    int getEventType(int index) const;
    Q_INVOKABLE int getEventCategory(int index) const;
    int getEventRow(int index) const;
    Q_INVOKABLE qint64 getDuration(int index) const;
    Q_INVOKABLE qint64 getStartTime(int index) const;
    Q_INVOKABLE qint64 getEndTime(int index) const;
    Q_INVOKABLE int getEventId(int index) const;
    Q_INVOKABLE QColor getColor(int index) const;
    Q_INVOKABLE float getHeight(int index) const;

    Q_INVOKABLE const QVariantList getLabelsForCategory(int category) const;
    Q_INVOKABLE const QVariantList getEventDetails(int index) const;
    Q_INVOKABLE const QVariantMap getEventLocation(int index) const;
    Q_INVOKABLE int getEventIdForHash(const QString &eventHash) const;
    Q_INVOKABLE int getEventIdForLocation(const QString &filename, int line, int column) const;

private slots:
    bool eventAccepted(const QmlProfilerSimpleModel::QmlEventData &event) const;

private:
    class PaintEventsModelProxyPrivate;
    PaintEventsModelProxyPrivate *d;

};

}
}

#endif
