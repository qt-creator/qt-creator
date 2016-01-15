/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef QMLPROFILERANIMATIONSMODEL_H
#define QMLPROFILERANIMATIONSMODEL_H

#include <QObject>
#include "qmlprofilertimelinemodel.h"
#include <qmldebug/qmlprofilereventtypes.h>
#include <qmldebug/qmlprofilereventlocation.h>
//#include <QHash>
//#include <QVector>
#include <QVariantList>
//#include <QVariantMap>
#include "qmlprofilerdatamodel.h"
#include <QColor>


namespace QmlProfiler {
class QmlProfilerModelManager;

namespace Internal {

class QmlProfilerAnimationsModel : public QmlProfilerTimelineModel
{
    Q_OBJECT
public:

    struct QmlPaintEventData {
        int framerate;
        int animationcount;
        int typeId;
    };

    QmlProfilerAnimationsModel(QmlProfilerModelManager *manager, QObject *parent = 0);

    int rowMaxValue(int rowNumber) const;

    int typeId(int index) const;
    Q_INVOKABLE int expandedRow(int index) const;
    Q_INVOKABLE int collapsedRow(int index) const;

    QColor color(int index) const;
    float relativeHeight(int index) const;

    QVariantList labels() const;
    QVariantMap details(int index) const;

    bool accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const;

protected:
    void loadData();
    void clear();

private:
    QVector<QmlProfilerAnimationsModel::QmlPaintEventData> m_data;
    int m_maxGuiThreadAnimations;
    int m_maxRenderThreadAnimations;
    int rowFromThreadId(int threadId) const;
};

}
}

#endif // QMLPROFILERANIMATIONSMODEL_H
