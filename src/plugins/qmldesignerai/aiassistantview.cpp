// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistantview.h"

#include "aiassistantwidget.h"

namespace QmlDesigner {

AiAssistantView::AiAssistantView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
{}

AiAssistantView::~AiAssistantView()
{}

bool AiAssistantView::hasWidget() const
{
    return true;
}

WidgetInfo AiAssistantView::widgetInfo()
{
    if (!m_widget)
        m_widget = Utils::makeUniqueObjectPtr<AiAssistantWidget>();

    return createWidgetInfo(m_widget.get(),
                            "AiAssistant",
                            WidgetInfo::LeftPane,
                            tr("AI Assistant"),
                            tr("AI Assistant view"));
}

void AiAssistantView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearAttachedImage();
}

} // namespace QmlDesigner
