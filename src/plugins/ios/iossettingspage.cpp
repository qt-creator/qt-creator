// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iossettingspage.h"

#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iostr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>

using namespace std::placeholders;

namespace Ios::Internal {

class IosSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    IosSettingsWidget();
    ~IosSettingsWidget() final;

private:
    void apply() final;

    void saveSettings();

private:
    QCheckBox *m_deviceAskCheckBox;
};

IosSettingsWidget::IosSettingsWidget()
{
    setWindowTitle(Tr::tr("iOS Configuration"));

    m_deviceAskCheckBox = new QCheckBox(Tr::tr("Ask about devices not in developer mode"));
    m_deviceAskCheckBox->setChecked(!IosConfigurations::ignoreAllDevices());

    auto xcodeLabel = new QLabel(
        Tr::tr("Configure available simulator devices in <a href=\"%1\">Xcode</a>.")
            .arg("https://developer.apple.com/documentation/xcode/"
                 "running-your-app-in-simulator-or-on-a-device/"
                 "#Configure-the-list-of-simulated-devices"));
    xcodeLabel->setOpenExternalLinks(true);

    // clang-format off
    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Devices")),
            Row { m_deviceAskCheckBox }
        },
        Group {
            title(Tr::tr("Simulator")),
            Row { xcodeLabel }
        },
        st
    }.attachTo(this);
    // clang-format on
}

IosSettingsWidget::~IosSettingsWidget() = default;

void IosSettingsWidget::apply()
{
    saveSettings();
    IosConfigurations::updateAutomaticKitList();
}

void IosSettingsWidget::saveSettings()
{
    IosConfigurations::setIgnoreAllDevices(!m_deviceAskCheckBox->isChecked());
}

// IosSettingsPage

class IosSettingsPage final : public Core::IOptionsPage
{
public:
    IosSettingsPage()
    {
        setId(Constants::IOS_SETTINGS_ID);
        setDisplayName(Tr::tr("iOS"));
        setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new IosSettingsWidget; });
    }
};

void setupIosSettingsPage()
{
    static IosSettingsPage theIosSettingsPage;
}

} // Ios::Internal

