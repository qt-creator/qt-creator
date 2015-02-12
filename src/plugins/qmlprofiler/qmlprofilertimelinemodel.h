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

#ifndef QMLPROFILERTIMELINEMODEL_H
#define QMLPROFILERTIMELINEMODEL_H

#include "qmlprofiler_global.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"
#include "timeline/timelinemodel.h"

namespace QmlProfiler {

class QMLPROFILER_EXPORT QmlProfilerTimelineModel : public Timeline::TimelineModel {
    Q_OBJECT
    Q_PROPERTY(QmlDebug::RangeType rangeType READ rangeType CONSTANT)
    Q_PROPERTY(QmlDebug::Message message READ message CONSTANT)
    Q_PROPERTY(QmlProfilerModelManager *modelManager READ modelManager CONSTANT)

public:
    QmlProfilerTimelineModel(QmlProfilerModelManager *modelManager, const QString &displayName,
                             QmlDebug::Message message, QmlDebug::RangeType rangeType,
                             QObject *parent);

    QmlProfilerModelManager *modelManager() const;

    QmlDebug::RangeType rangeType() const;
    QmlDebug::Message message() const;

    virtual bool accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const;
    bool handlesTypeId(int typeId) const;
    Q_INVOKABLE virtual int bindingLoopDest(int index) const;

    virtual void loadData() = 0;
    void clear();

private slots:
    void dataChanged();

protected:
    void updateProgress(qint64 count, qint64 max) const;
    void announceFeatures(quint64 features) const;

private:
    const QmlDebug::Message m_message;
    const QmlDebug::RangeType m_rangeType;
    QmlProfilerModelManager *const m_modelManager;
};

}

#endif // QMLPROFILERTIMELINEMODEL_H
