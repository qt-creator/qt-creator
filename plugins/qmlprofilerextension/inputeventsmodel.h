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

#include "qmlprofiler/abstracttimelinemodel.h"

namespace QmlProfilerExtension {
namespace Internal {

class InputEventsModel : public QmlProfiler::AbstractTimelineModel
{
    Q_OBJECT
    class InputEventsModelPrivate;
    Q_DECLARE_PRIVATE(InputEventsModel)

protected:
    bool accepted(const QmlProfiler::QmlProfilerDataModel::QmlEventTypeData &event) const;

public:
    InputEventsModel(QObject *parent = 0);
    quint64 features() const;

    int selectionId(int index) const;
    QColor color(int index) const;
    QVariantList labels() const;
    QVariantMap details(int index) const;
    int row(int index) const;
    void loadData();
};

}
}
#endif // INPUTEVENTSMODEL_H
