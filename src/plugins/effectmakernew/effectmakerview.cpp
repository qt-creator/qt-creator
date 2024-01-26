// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakerview.h"

#include "effectmakermodel.h"
#include "effectmakernodesmodel.h"
#include "effectmakerwidget.h"

#include <documentmanager.h>
#include <modelnodeoperations.h>
#include <qmldesignerconstants.h>

#include <coreplugin/icore.h>

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

        connect(m_widget->effectMakerModel(), &EffectMakerModel::assignToSelectedTriggered, this,
                [&] (const QString &effectPath) {
            executeInTransaction("EffectMakerView::widgetInfo", [&] {
                const QList<QmlDesigner::ModelNode> selectedNodes = selectedModelNodes();
                for (const QmlDesigner::ModelNode &node : selectedNodes)
                    QmlDesigner::ModelNodeOperations::handleItemLibraryEffectDrop(effectPath, node);
            });
        });

        auto context = new EffectMakerContext(m_widget.data());
        Core::ICore::addContextObject(context);
    }

    return createWidgetInfo(m_widget.data(), "Effect Composer",
                            QmlDesigner::WidgetInfo::LeftPane, 0, tr("Effect Composer [beta]"));
}

void EffectMakerView::customNotification([[maybe_unused]] const AbstractView *view,
                                         const QString &identifier,
                                         [[maybe_unused]] const QList<QmlDesigner::ModelNode> &nodeList,
                                         const QList<QVariant> &data)
{
    if (identifier == "open_effectmaker_composition" && data.count() > 0) {
        const QString compositionPath = data[0].toString();
        m_widget->openComposition(compositionPath);
    }
}

void EffectMakerView::modelAttached(QmlDesigner::Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->effectMakerNodesModel()->loadModel();

    QString currProjectPath = QmlDesigner::DocumentManager::currentProjectDirPath().toString();

    // if starting a new project, clear the effect composer
    if (m_currProjectPath != currProjectPath)
        m_widget->effectMakerModel()->clear();

    m_currProjectPath = currProjectPath;

    m_widget->initView();
}

void EffectMakerView::modelAboutToBeDetached(QmlDesigner::Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

} // namespace EffectMaker
