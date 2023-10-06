// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakerview.h"

#include "effectmakerwidget.h"
#include "effectmakernodesmodel.h"

#include "nodeinstanceview.h"
#include "qmldesignerconstants.h"

#include <coreplugin/icore.h>

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>

namespace EffectMaker {

EffectMakerContext::EffectMakerContext(QWidget *widget)
    : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(QmlDesigner::Constants::C_QMLEFFECTMAKER,
                             QmlDesigner::Constants::C_QT_QUICK_TOOLS_MENU));
}

void EffectMakerContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<EffectMakerWidget *>(m_widget)->contextHelp(callback);
}

EffectMakerView::EffectMakerView(QmlDesigner::ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{
}

EffectMakerView::~EffectMakerView()
{}

bool EffectMakerView::hasWidget() const
{
    return true;
}

QmlDesigner::WidgetInfo EffectMakerView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new EffectMakerWidget{this};

        auto context = new EffectMakerContext(m_widget.data());
        Core::ICore::addContextObject(context);
    }

    return createWidgetInfo(m_widget.data(), "Effect Maker",
                            QmlDesigner::WidgetInfo::LeftPane, 0, tr("Effect Maker"));
}

void EffectMakerView::customNotification(const AbstractView * /*view*/,
                                         const QString & /*identifier*/,
                                         const QList<QmlDesigner::ModelNode> & /*nodeList*/,
                                         const QList<QVariant> & /*data*/)
{
    // TODO
}

void EffectMakerView::modelAttached(QmlDesigner::Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->effectMakerNodesModel()->loadModel();
    m_widget->initView();
}

void EffectMakerView::modelAboutToBeDetached(QmlDesigner::Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

} // namespace EffectMaker

