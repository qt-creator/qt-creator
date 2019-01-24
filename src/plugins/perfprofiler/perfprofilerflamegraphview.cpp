/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "perfprofilerflamegraphmodel.h"
#include "perfprofilerflamegraphview.h"
#include "perfprofilertool.h"
#include "perftimelinemodel.h"

#include <tracing/flamegraph.h>
#include <tracing/timelinetheme.h>
#include <utils/theme/theme.h>

#include <QQmlContext>

namespace PerfProfiler {
namespace Internal {

PerfProfilerFlameGraphView::PerfProfilerFlameGraphView(QWidget *parent, PerfProfilerTool *tool) :
    QQuickWidget(parent)
{
    setObjectName(QLatin1String("PerfProfilerFlameGraphView"));

    PerfProfilerTraceManager *manager = tool->traceManager();
    m_model = new PerfProfilerFlameGraphModel(manager);

    qmlRegisterType<FlameGraph::FlameGraph>("FlameGraph", 1, 0, "FlameGraph");
    qmlRegisterUncreatableType<PerfProfilerFlameGraphModel>(
                "PerfProfilerFlameGraphModel", 1, 0, "PerfProfilerFlameGraphModel",
                QLatin1String("use the context property"));

    Timeline::TimelineTheme::setupTheme(engine());

    rootContext()->setContextProperty(QStringLiteral("flameGraphModel"), m_model);
    setSource(QUrl(QStringLiteral("qrc:/perfprofiler/PerfProfilerFlameGraphView.qml")));
    setClearColor(Utils::creatorTheme()->color(Utils::Theme::Timeline_BackgroundColor1));

    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(rootObject(), SIGNAL(typeSelected(int)), this, SIGNAL(typeSelected(int)));
    connect(m_model, &PerfProfilerFlameGraphModel::gotoSourceLocation,
            this, &PerfProfilerFlameGraphView::gotoSourceLocation);
}

PerfProfilerFlameGraphView::~PerfProfilerFlameGraphView()
{
    delete m_model;
}

void PerfProfilerFlameGraphView::selectByTypeId(int typeId)
{
    rootObject()->setProperty("selectedTypeId", typeId);
}

void PerfProfilerFlameGraphView::resetRoot()
{
    QMetaObject::invokeMethod(rootObject(), "resetRoot");
}

bool PerfProfilerFlameGraphView::isZoomed() const
{
    return rootObject()->property("zoomed").toBool();
}

} // namespace Internal
} // namespace PerfProfiler
