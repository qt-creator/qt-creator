// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "insightview.h"
#include "insightmodel.h"
#include "insightwidget.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QTextDocument>

namespace QmlDesigner {

InsightView::InsightView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_insightModel(std::make_unique<InsightModel>(this, externalDependencies))
{
    Q_ASSERT(m_insightModel);
}

InsightView::~InsightView()
{
    delete m_insightWidget.data();
}

void InsightView::modelAttached(Model *model)
{
    if (model == AbstractView::model())
        return;

    QTC_ASSERT(model, return );
    AbstractView::modelAttached(model);

    m_insightModel->setup();
}

WidgetInfo InsightView::widgetInfo()
{
    if (!m_insightWidget)
        m_insightWidget = new InsightWidget(this, m_insightModel.get());

    return createWidgetInfo(m_insightWidget.data(),
                            "QtInsight",
                            WidgetInfo::RightPane,
                            0,
                            tr("Qt Insight"));
}

bool InsightView::hasWidget() const
{
    return true;
}

} // namespace QmlDesigner
