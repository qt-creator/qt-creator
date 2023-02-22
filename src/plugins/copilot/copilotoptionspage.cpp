// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotoptionspage.h"

#include "authwidget.h"
#include "copilotsettings.h"
#include "copilotsettings.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

using namespace Utils;
using namespace LanguageClient;

namespace Copilot {

class CopilotOptionsPageWidget : public QWidget
{
public:
    CopilotOptionsPageWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        using namespace Layouting;

        auto authWidget = new AuthWidget();

        // clang-format off
        Column {
            authWidget, br,
            CopilotSettings::instance().nodeJsPath, br,
            CopilotSettings::instance().distPath, br,
            st
        }.attachTo(this);
        // clang-format on
    }
};

CopilotOptionsPage::CopilotOptionsPage()
{
    setId("Copilot.General");
    setDisplayName("Copilot");
    setCategory("ZY.Copilot");
    setDisplayCategory("Copilot");

    setCategoryIconPath(":/languageclient/images/settingscategory_languageclient.png");
}

CopilotOptionsPage::~CopilotOptionsPage() {}

void CopilotOptionsPage::init() {}

QWidget *CopilotOptionsPage::widget()
{
    return new CopilotOptionsPageWidget();
}

void CopilotOptionsPage::apply()
{
    CopilotSettings::instance().apply();
    CopilotSettings::instance().writeSettings(Core::ICore::settings());
}

void CopilotOptionsPage::finish() {}

CopilotOptionsPage &CopilotOptionsPage::instance()
{
    static CopilotOptionsPage settingsPage;
    return settingsPage;
}

} // namespace Copilot
