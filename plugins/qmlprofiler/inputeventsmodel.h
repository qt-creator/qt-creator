/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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
