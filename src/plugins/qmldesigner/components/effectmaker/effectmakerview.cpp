// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakerview.h"

#include "effectmakerwidget.h"
#include "effectmakernodesmodel.h"
#include "designmodecontext.h"
#include "nodeinstanceview.h"

#include <coreplugin/icore.h>

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>

namespace QmlDesigner {

EffectMakerView::EffectMakerView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{
}

EffectMakerView::~EffectMakerView()
{}

bool EffectMakerView::hasWidget() const
{
    return true;
}

WidgetInfo EffectMakerView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new EffectMakerWidget{this};

        auto context = new Internal::EffectMakerContext(m_widget.data());
        Core::ICore::addContextObject(context);
    }

    return createWidgetInfo(m_widget.data(), "Effect Maker", WidgetInfo::LeftPane, 0, tr("Effect Maker"));
}

void EffectMakerView::customNotification(const AbstractView * /*view*/,
                                           const QString & /*identifier*/,
                                           const QList<ModelNode> & /*nodeList*/,
                                           const QList<QVariant> & /*data*/)
{
    // TODO
}

void EffectMakerView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    // Add some dummy effects data
    //m_widget->effectMakerModel()->setEffects({"Drop Shadow", "Colorize", "Fast Blue"}); // TODO
    m_widget->effectMakerNodesModel()->loadModel();
    m_widget->initView();
}

void EffectMakerView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

} // namespace QmlDesigner
