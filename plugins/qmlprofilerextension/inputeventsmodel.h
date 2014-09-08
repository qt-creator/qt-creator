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

#include "qmlprofiler/abstracttimelinemodel.h"

namespace QmlProfilerExtension {
namespace Internal {

class InputEventsModel : public QmlProfiler::AbstractTimelineModel
{
    Q_OBJECT
    class InputEventsModelPrivate;
    Q_DECLARE_PRIVATE(InputEventsModel)

public:
    InputEventsModel(QObject *parent = 0);
    quint64 features() const;

    int rowCount() const {return 0;}
    int eventId(int) const {return -1;}
    QColor color(int) const {return QColor();}
    QVariantList labels() const {return QVariantList();}
    QVariantMap details(int) const {return QVariantMap();}
    int row(int) const {return -1;}
    void loadData() {}
};

}
}
#endif // INPUTEVENTSMODEL_H
