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

#ifndef ABSTRACTTIMELINEMODEL_H
#define ABSTRACTTIMELINEMODEL_H


#include "qmlprofiler_global.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilersimplemodel.h"
#include <QObject>
#include <QVariant>
#include <QColor>

namespace QmlProfiler {

class QMLPROFILER_EXPORT AbstractTimelineModel : public QObject
{
    Q_OBJECT

public:
    explicit AbstractTimelineModel(QObject *parent = 0);
    ~AbstractTimelineModel();

    void setModelManager(QmlProfilerModelManager *modelManager);

    virtual int categories() const = 0;
    virtual QStringList categoryTitles() const = 0;
    virtual QString name() const = 0;
    virtual int count() const = 0;

    virtual bool isEmpty() const = 0;

    virtual bool eventAccepted(const QmlProfilerSimpleModel::QmlEventData &event) const = 0;

    Q_INVOKABLE virtual qint64 lastTimeMark() const = 0;
    Q_INVOKABLE qint64 traceStartTime() const;
    Q_INVOKABLE qint64 traceEndTime() const;
    Q_INVOKABLE qint64 traceDuration() const;
    Q_INVOKABLE int getState() const;

    Q_INVOKABLE virtual bool expanded(int category) const = 0;
    Q_INVOKABLE virtual void setExpanded(int category, bool expanded) = 0;
    Q_INVOKABLE virtual int categoryDepth(int categoryIndex) const = 0;
    Q_INVOKABLE virtual int categoryCount() const = 0;
    Q_INVOKABLE virtual int rowCount() const;
    Q_INVOKABLE virtual const QString categoryLabel(int categoryIndex) const = 0;

    virtual int findFirstIndex(qint64 startTime) const = 0;
    virtual int findFirstIndexNoParents(qint64 startTime) const = 0;
    virtual int findLastIndex(qint64 endTime) const = 0;

    virtual int getEventType(int index) const = 0;
    virtual int getEventCategory(int index) const = 0;
    virtual int getEventRow(int index) const = 0;

    virtual void loadData() = 0;
    virtual void clear() = 0;

    Q_INVOKABLE virtual qint64 getDuration(int index) const = 0;
    Q_INVOKABLE virtual qint64 getStartTime(int index) const = 0;
    Q_INVOKABLE virtual qint64 getEndTime(int index) const = 0;
    Q_INVOKABLE virtual int getEventId(int index) const = 0;
    Q_INVOKABLE virtual int getBindingLoopDest(int index) const;
    Q_INVOKABLE virtual QColor getColor(int index) const = 0;
    Q_INVOKABLE virtual float getHeight(int index) const = 0;

    Q_INVOKABLE virtual const QVariantList getLabelsForCategory(int category) const = 0;

    Q_INVOKABLE virtual const QVariantList getEventDetails(int index) const = 0;

    // returned map should contain "file", "line", "column" properties, or be empty
    Q_INVOKABLE virtual const QVariantMap getEventLocation(int index) const = 0;
    Q_INVOKABLE virtual int getEventIdForHash(const QString &eventHash) const = 0;
    Q_INVOKABLE virtual int getEventIdForLocation(const QString &filename, int line, int column) const = 0;

signals:
    void dataAvailable();
    void stateChanged();
    void emptyChanged();
    void expandedChanged();

protected:
    QmlProfilerModelManager *m_modelManager;
    int m_modelId;

protected slots:
    void dataChanged();
};

}

#endif // ABSTRACTTIMELINEMODEL_H
