// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiprovidersettings.h"

#include "aiassistantconstants.h"
#include "aiassistantview.h"
#include "aiproviderdata.h"
#include "aiprovidersettingswidget.h"

#include <qmldesignerplugin.h>
#include <qmldesignertr.h>

#include <coreplugin/icore.h>
#include <utils/layoutbuilder.h>

namespace QmlDesigner {

static QList<AiProviderConfig> allProviderConfigs()
{
    const QStringList providerNames = AiProviderData::defaultProvidersNames();

    QList<AiProviderConfig> result;
    for (const QString &providerName : providerNames)
        result.append(AiProviderConfig{providerName});

    return result;
}

AiProviderSettings::AiProviderSettings()
{
    setId(Constants::aiAssistantSettingsPageId);
    setDisplayName(Tr::tr("AI Provider Settings"));
    setCategory(Constants::aiAssistantSettingsPageCategory);
    setWidgetCreator([this] { return new AiProviderSettingsTab(m_view); });
}

AiProviderSettings::~AiProviderSettings() = default;

QList<AiModelInfo> AiProviderSettings::allValidModels()
{
    QList<AiModelInfo> validModel;
    const QList<AiProviderConfig> allProviders = allProviderConfigs();
    for (const AiProviderConfig &provider : allProviders) {
        if (provider.isChecked())
            validModel.append(provider.allValidModels());
    }
    return validModel;
}

void AiProviderSettings::setView(AiAssistantView *view)
{
    m_view = view;
}

AiProviderSettingsTab::AiProviderSettingsTab(AiAssistantView *view)
    : Core::IOptionsPageWidget()
    , m_view(view)
{
    auto createProviderWidget = [this](const QString &name) -> AiProviderSettingsWidget * {
        auto widget = new AiProviderSettingsWidget(name, this);
        m_providerWidgets.append(widget);
        return widget;
    };

    using namespace Layouting;
    Column providersCol;
    const QStringList providers = AiProviderData::defaultProvidersNames();
    for (const QString &providerName : providers)
        providersCol.addItem(createProviderWidget(providerName));

    providersCol.addItem(Stretch(1));
    providersCol.attachTo(this);
}

AiProviderSettingsTab::~AiProviderSettingsTab() = default;

void AiProviderSettingsTab::apply()
{
    QTC_ASSERT(m_view, return);

    bool changed = false;
    for (AiProviderSettingsWidget *w : std::as_const(m_providerWidgets))
        changed |= w->save();

    if (changed)
        m_view->updateAiModelConfig();
}

void AiProviderSettingsTab::cancel()
{
    for (AiProviderSettingsWidget *w : std::as_const(m_providerWidgets))
        w->load();
}

} // namespace QmlDesigner
