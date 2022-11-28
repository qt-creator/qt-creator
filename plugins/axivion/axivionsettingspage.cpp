/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Axivion Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "axivionsettingspage.h"

#include "axivionsettings.h"
#include "axiviontr.h"

#include <coreplugin/icore.h>
#include <utils/layoutbuilder.h>

#include <QLabel>
#include <QWidget>

namespace Axivion::Internal {

class AxivionSettingsWidget : public Core::IOptionsPageWidget
{
public:
    explicit AxivionSettingsWidget(AxivionSettings *settings);

    void apply() override;
private:
    AxivionSettings *m_settings;
};

AxivionSettingsWidget::AxivionSettingsWidget(AxivionSettings *settings)
    : m_settings(settings)
{
    using namespace Utils::Layouting;

    auto label = new QLabel(Tr::tr("...Placeholder..."));
    Row {
        Column { label, st }
    }.attachTo(this);
}

void AxivionSettingsWidget::apply()
{
    // set m_settings from widget
    m_settings->toSettings(Core::ICore::settings());
}

AxivionSettingsPage::AxivionSettingsPage(AxivionSettings *settings)
{
    setId("Axivion.Settings.General");
    setDisplayName(Tr::tr("General"));
    setCategory("XY.Axivion");
    setDisplayCategory(Tr::tr("Axivion"));
    setCategoryIconPath(":/axivion/images/axivion.png");
    setWidgetCreator([this] { return new AxivionSettingsWidget(m_settings); });
}

} // Axivion::Internal
