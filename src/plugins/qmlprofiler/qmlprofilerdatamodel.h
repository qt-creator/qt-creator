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

#ifndef QMLPROFILERDATAMODEL_H
#define QMLPROFILERDATAMODEL_H

#include <qmldebug/qmlprofilereventtypes.h>
#include "qmlprofilerbasemodel.h"

namespace QmlProfiler {

class QMLPROFILER_EXPORT QmlProfilerDataModel : public QmlProfilerBaseModel
{
    Q_OBJECT
public:
    struct QmlEventTypeData {
        QString displayName;
        QmlDebug::QmlEventLocation location;
        QmlDebug::Message message;
        QmlDebug::RangeType rangeType;
        int detailType; // can be EventType, BindingType, PixmapEventType or SceneGraphFrameType
        QString data;
    };

    struct QmlEventData {
        int typeIndex;
        qint64 startTime;
        qint64 duration;
        qint64 numericData1;
        qint64 numericData2;
        qint64 numericData3;
        qint64 numericData4;
        qint64 numericData5;
    };

    struct QmlEventNoteData {
        int typeIndex;
        qint64 startTime;
        qint64 duration;
        QString text;
    };

    explicit QmlProfilerDataModel(Utils::FileInProjectFinder *fileFinder, QmlProfilerModelManager *parent = 0);

    const QVector<QmlEventData> &getEvents() const;
    const QVector<QmlEventTypeData> &getEventTypes() const;
    const QVector<QmlEventNoteData> &getEventNotes() const;
    void setData(qint64 traceStart, qint64 traceEnd, const QVector<QmlEventTypeData> &types,
                 const QVector<QmlEventData> &events);
    void setNoteData(const QVector<QmlEventNoteData> &notes);

    int count() const;
    virtual void clear();
    virtual bool isEmpty() const;
    virtual void complete();
    void addQmlEvent(QmlDebug::Message message, QmlDebug::RangeType rangeType, int bindingType,
                     qint64 startTime, qint64 duration, const QString &data,
                     const QmlDebug::QmlEventLocation &location, qint64 ndata1, qint64 ndata2,
                     qint64 ndata3, qint64 ndata4, qint64 ndata5);
    qint64 lastTimeMark() const;

protected slots:
    void detailsChanged(int requestId, const QString &newString);

private:
    class QmlProfilerDataModelPrivate;
    Q_DECLARE_PRIVATE(QmlProfilerDataModel)
};

}

#endif
