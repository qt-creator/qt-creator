/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef INPUTEVENTSMODEL_H
#define INPUTEVENTSMODEL_H

#include "qmlprofiler/qmlprofilertimelinemodel.h"

namespace QmlProfilerExtension {
namespace Internal {

class InputEventsModel : public QmlProfiler::QmlProfilerTimelineModel
{
    Q_OBJECT

protected:
    bool accepted(const QmlProfiler::QmlProfilerDataModel::QmlEventTypeData &event) const;

public:
    InputEventsModel(QmlProfiler::QmlProfilerModelManager *manager, QObject *parent = 0);

    int typeId(int index) const;
    QColor color(int index) const;
    QVariantList labels() const;
    QVariantMap details(int index) const;
    int expandedRow(int index) const;
    int collapsedRow(int index) const;
    void loadData();
    void clear();

private:
    int m_keyTypeId;
    int m_mouseTypeId;

};

}
}
#endif // INPUTEVENTSMODEL_H
