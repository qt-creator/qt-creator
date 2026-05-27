// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilerflamegraphmodel.h"
#include "perfprofilerflamegraphview.h"

namespace PerfProfiler::Internal {

PerfProfilerFlameGraphView::PerfProfilerFlameGraphView(QWidget *parent)
    : Timeline::FlameGraphWidget(
          new PerfProfilerFlameGraphModel(&traceManager()),
          QUrl(QStringLiteral("qrc:/qt/qml/QtCreator/PerfProfiler/PerfProfilerFlameGraphView.qml")),
          parent)
{
    setObjectName(QLatin1String("PerfProfilerFlameGraphView"));

    auto *m = static_cast<PerfProfilerFlameGraphModel *>(model());
    m->setParent(this);
    connect(m, &PerfProfilerFlameGraphModel::gotoSourceLocation,
            this, &PerfProfilerFlameGraphView::gotoSourceLocation);
}

} // namespace PerfProfiler::Internal
