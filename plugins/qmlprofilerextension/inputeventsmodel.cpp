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

#include "inputeventsmodel.h"
#include "qmldebug/qmlprofilereventtypes.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"
#include "qmlprofiler/abstracttimelinemodel_p.h"

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler;

class InputEventsModel::InputEventsModelPrivate : public AbstractTimelineModelPrivate
{
    Q_DECLARE_PUBLIC(InputEventsModel)
};

InputEventsModel::InputEventsModel(QObject *parent)
    : AbstractTimelineModel(new InputEventsModelPrivate(),
                            tr(QmlProfilerModelManager::featureName(QmlDebug::ProfileInputEvents)),
                            QmlDebug::Event, QmlDebug::MaximumRangeType, parent)
{
}

quint64 InputEventsModel::features() const
{
    return 1 << QmlDebug::ProfileInputEvents;
}

}
}
