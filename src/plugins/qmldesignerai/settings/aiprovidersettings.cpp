// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiprovidersettings.h"

#include "aiassistantconstants.h"
#include "aiassistantview.h"
#include "aiproviderconfig.h"
#include "aiproviderdata.h"
#include "aiprovidersettingswidget.h"

#include <qmldesignerplugin.h>
#include <qmldesignertr.h>

#include <coreplugin/icore.h>
#include <utils/layoutbuilder.h>

namespace QmlDesigner {

static QList<AiProviderConfig> allProviderConfigs()
{
    Utils::QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::aiAssistantSettingsKey);
    settings->beginGroup(Constants::aiAssistantProviderConfigKey);
    const QStringList settingsKeys = settings->allKeys();
    settings->endGroup();
    settings->endGroup();

    QSet<QString> providerIdsSet;
    QStringList providerIds;
    for (const QString &key : settingsKeys) {
        const QString providerId = key.left(key.indexOf('/'));
        if (!providerId.isEmpty() && !providerIdsSet.contains(providerId)) {
            providerIdsSet.insert(providerId);
            providerIds.append(providerId);
        }
    }

    QList<AiProviderConfig> result;
    for (const QString &providerId : std::as_const(providerIds)) {
        result.append(
            AiProviderConfig::fromSettings(Constants::aiAssistantSettingsKey, settings, providerId));
    }
    return result;
}

AiProviderSettings::AiProviderSettings()
{
    setId(Constants::aiAssistantProviderSettingsPage);
    setDisplayName(Tr::tr("Ai Provider Settings"));
    setCategory(Constants::aiAssistantSettingsPageCategory);
    setWidgetCreator([this] { return new AiProviderSettingsTab(m_view); });
}

AiProviderSettings::~AiProviderSettings() = default;

QList<AiModelInfo> AiProviderSettings::allValidModels()
{
    QList<AiModelInfo> validModel;
    const QList<AiProviderConfig> allProviders = allProviderConfigs();
    for (const AiProviderConfig &provider : allProviders)
        validModel.append(provider.allValidModels());
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
    auto createSettingsWidget = [this](const QString &name) -> AiProviderSettingsWidget * {
        AiProviderSettingsWidget *widget = new AiProviderSettingsWidget(name, this);
        m_modelDataList.append(widget);
        return widget;
    };

    using namespace Layouting;
    Column providersCol;
    const QStringList providers
        = Utils::transform(AiProviderData::defaultProviders(), &AiProviderData::name);
    for (const QString &providerName : providers)
        providersCol.addItem(createSettingsWidget(providerName));
    providersCol.addItem(Stretch(1));
    providersCol.attachTo(this);
}

AiProviderSettingsTab::~AiProviderSettingsTab() = default;

void AiProviderSettingsTab::apply()
{
    bool changed = false;
    for (AiProviderSettingsWidget *settingsWidget : std::as_const(m_modelDataList))
        changed |= settingsWidget->saveIfChanged();

    if (changed && m_view)
        m_view->updateAiModelConfig();
}

void AiProviderSettingsTab::cancel()
{
    for (AiProviderSettingsWidget *settingsWidget : std::as_const(m_modelDataList))
        settingsWidget->load();
}

} // namespace QmlDesigner
