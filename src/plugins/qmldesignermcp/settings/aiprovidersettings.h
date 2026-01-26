// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aiproviderconfig.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

class StudioQuickWidget;

namespace QmlDesigner {
class AiProviderSettingsWidget;
class AiAssistantView;

class AiProviderSettings : public Core::IOptionsPage
{
public:
    AiProviderSettings();
    ~AiProviderSettings();

    static QList<AiModelInfo> allValidModels();

    void setView(AiAssistantView *view);

private:
    QPointer<AiAssistantView> m_view;
};

class AiProviderSettingsTab : public Core::IOptionsPageWidget
{
public:
    AiProviderSettingsTab(AiAssistantView *view);
    ~AiProviderSettingsTab();

protected:
    void apply() final;
    void cancel() final;

private:
    QList<AiProviderSettingsWidget *> m_providerWidgets;
    QPointer<AiAssistantView> m_view;
};

} // namespace QmlDesigner
