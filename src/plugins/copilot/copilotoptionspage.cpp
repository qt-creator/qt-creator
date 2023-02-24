// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotoptionspage.h"

#include "authwidget.h"
#include "copilotsettings.h"
#include "copilottr.h"

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

        QLabel *helpLabel = new QLabel();
        helpLabel->setTextFormat(Qt::MarkdownText);
        helpLabel->setWordWrap(true);
        helpLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse
                                           | Qt::LinksAccessibleByKeyboard);
        helpLabel->setOpenExternalLinks(true);

        // clang-format off
        helpLabel->setText(Tr::tr(R"(
The Copilot plugin requires node.js and the Copilot neovim plugin.
If you install the neovim plugin as described in the
[README.md](https://github.com/github/copilot.vim),
the plugin will find the agent.js file automatically.

Otherwise you need to specify the path to the
[agent.js](https://github.com/github/copilot.vim/tree/release/copilot/dist)
file from the Copilot neovim plugin.
        )", "Markdown text for the copilot instruction label"));

        Column {
            authWidget, br,
            CopilotSettings::instance().nodeJsPath, br,
            CopilotSettings::instance().distPath, br,
            helpLabel, br,
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
    setCategoryIconPath(":/copilot/images/settingscategory_copilot.png");
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
