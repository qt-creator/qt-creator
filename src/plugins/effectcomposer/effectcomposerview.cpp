// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposerview.h"

#include "effectcomposermodel.h"
#include "effectcomposernodesmodel.h"
#include "effectcomposerwidget.h"

#include <designermcumanager.h>
#include <documentmanager.h>
#include <modelnodeoperations.h>
#include <qmldesignerconstants.h>

#include <coreplugin/icore.h>

namespace EffectComposer {

EffectComposerContext::EffectComposerContext(QWidget *widget)
    : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(QmlDesigner::Constants::C_QMLEFFECTCOMPOSER,
                             QmlDesigner::Constants::C_QT_QUICK_TOOLS_MENU));
}

void EffectComposerContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<EffectComposerWidget *>(m_widget)->contextHelp(callback);
}

EffectComposerView::EffectComposerView(QmlDesigner::ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{
}

EffectComposerView::~EffectComposerView()
{}

bool EffectComposerView::hasWidget() const
{
    return true;
}

QmlDesigner::WidgetInfo EffectComposerView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new EffectComposerWidget{this};

        connect(m_widget->effectComposerModel(), &EffectComposerModel::assignToSelectedTriggered, this,
                [&] (const QString &effectPath) {
            executeInTransaction("EffectComposerView::widgetInfo", [&] {
                const QList<QmlDesigner::ModelNode> selectedNodes = selectedModelNodes();
                for (const QmlDesigner::ModelNode &node : selectedNodes)
                    QmlDesigner::ModelNodeOperations::handleItemLibraryEffectDrop(effectPath, node);
            });
        });

        auto context = new EffectComposerContext(m_widget.data());
        Core::ICore::addContextObject(context);
    }

    return createWidgetInfo(m_widget.data(), "EffectComposer",
                            QmlDesigner::WidgetInfo::LeftPane, 0, tr("Effect Composer [beta]"));
}

void EffectComposerView::customNotification([[maybe_unused]] const AbstractView *view,
                                         const QString &identifier,
                                         [[maybe_unused]] const QList<QmlDesigner::ModelNode> &nodeList,
                                         const QList<QVariant> &data)
{
    if (identifier == "open_effectcomposer_composition" && data.count() > 0) {
        const QString compositionPath = data[0].toString();
        m_widget->openComposition(compositionPath);
    }
}

void EffectComposerView::modelAttached(QmlDesigner::Model *model)
{
    AbstractView::modelAttached(model);


    QString currProjectPath = QmlDesigner::DocumentManager::currentProjectDirPath().toString();

    if (m_currProjectPath != currProjectPath) { // starting a new project
        m_widget->effectComposerNodesModel()->loadModel();
        m_widget->effectComposerModel()->clear(true);
        m_widget->effectComposerModel()->setIsEnabled(
            !QmlDesigner::DesignerMcuManager::instance().isMCUProject());
        m_widget->initView();
    }

    m_currProjectPath = currProjectPath;

}

void EffectComposerView::modelAboutToBeDetached(QmlDesigner::Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

} // namespace EffectComposer
