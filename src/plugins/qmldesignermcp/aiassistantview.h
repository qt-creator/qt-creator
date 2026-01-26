// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"

#include <utils/uniqueobjectptr.h>

namespace QmlDesigner {

class AiAssistantWidget;

class AiAssistantView : public AbstractView
{
    Q_OBJECT

public:
    AiAssistantView(ExternalDependenciesInterface &externalDependencies);
    ~AiAssistantView() override;

    void updateAiModelConfig();

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(Model *model) override;

private: // functions
    void handleProjectTreeChanges();

private: // variables
    Utils::UniqueObjectPtr<AiAssistantWidget> m_widget;
};

} // namespace QmlDesigner
