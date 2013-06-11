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
****************************************************************************/

#ifndef SCENEGRAPHTIMELINEMODEL_H
#define SCENEGRAPHTIMELINEMODEL_H

#include "qmlprofiler/abstracttimelinemodel.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"
#include "qmlprofiler/qmlprofilersimplemodel.h"

#include <QStringList>
#include <QColor>

namespace QmlProfilerExtended {
namespace Internal {

#define timingFieldCount 16

class SceneGraphTimelineModel : public QmlProfiler::AbstractTimelineModel
{
    Q_OBJECT
public:

    struct SceneGraphEvent {
        qint64 startTime;
        qint64 duration;
        int sgEventType;
        qint64 timing[timingFieldCount];
    };


    SceneGraphTimelineModel(QObject *parent = 0);
    ~SceneGraphTimelineModel();


//    void setModelManager(QmlProfiler::Internal::QmlProfilerModelManager *modelManager);

    int categories() const;
    QStringList categoryTitles() const;
    QString name() const;
    int count() const;

    bool isEmpty() const;

    bool eventAccepted(const QmlProfiler::Internal::QmlProfilerSimpleModel::QmlEventData &event) const;

    Q_INVOKABLE qint64 lastTimeMark() const;

    Q_INVOKABLE void setExpanded(int category, bool expanded);
    Q_INVOKABLE int categoryDepth(int categoryIndex) const;
    Q_INVOKABLE int categoryCount() const;
    Q_INVOKABLE const QString categoryLabel(int categoryIndex) const;

    int findFirstIndex(qint64 startTime) const;
    int findFirstIndexNoParents(qint64 startTime) const;
    int findLastIndex(qint64 endTime) const;

    int getEventType(int index) const;
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

    void loadData();
    void clear();
//signals:
//    void countChanged();
//    void dataAvailable();
//    void stateChanged();
//    void emptyChanged();
//    void expandedChanged();

protected slots:
    void dataChanged();

private:
    class SceneGraphTimelineModelPrivate;
    SceneGraphTimelineModelPrivate *d;

};

} // namespace Internal
} // namespace QmlProfilerExtended

#endif // SCENEGRAPHTIMELINEMODEL_H
