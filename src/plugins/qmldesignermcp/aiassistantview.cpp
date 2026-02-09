// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistantview.h"

#include "aiassistantwidget.h"

#include <projectexplorer/projecttree.h>

#include <qmldesignerplugin.h>

namespace QmlDesigner {

AiAssistantView::AiAssistantView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{}

AiAssistantView::~AiAssistantView()
{}

void AiAssistantView::updateAiModelConfig()
{
    m_widget->updateModelConfig();
}

bool AiAssistantView::hasWidget() const
{
    return true;
}

WidgetInfo AiAssistantView::widgetInfo()
{
    if (!m_widget) {
        m_widget = Utils::makeUniqueObjectPtr<AiAssistantWidget>(this);

        using ProjectExplorer::ProjectTree;
        connect(
            ProjectTree::instance(),
            &ProjectTree::treeChanged,
            this,
            &AiAssistantView::handleProjectTreeChanges);
    }

    return createWidgetInfo(m_widget.get(),
                            "AiAssistant",
                            WidgetInfo::LeftPane,
                            tr("AI Assistant"),
                            tr("AI Assistant view"),
                            DesignerWidgetFlags::IgnoreErrors);
}

void AiAssistantView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->initManifest();
    m_widget->clear();
    m_widget->setProjectPath(
        QmlDesignerPlugin::instance()->currentDesignDocument()->projectFolder().toFSPathString());
}

void AiAssistantView::handleProjectTreeChanges()
{
    m_widget->removeMissingAttachedImage();
}

} // namespace QmlDesigner
