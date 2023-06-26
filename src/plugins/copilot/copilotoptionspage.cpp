// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotoptionspage.h"

#include "authwidget.h"
#include "copilotconstants.h"
#include "copilotsettings.h"
#include "copilottr.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QToolTip>

using namespace Utils;
using namespace LanguageClient;

namespace Copilot {

class CopilotOptionsPageWidget : public Core::IOptionsPageWidget
{
public:
    CopilotOptionsPageWidget()
    {
        using namespace Layouting;

        auto warningLabel = new QLabel;
        warningLabel->setWordWrap(true);
        warningLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse
                                              | Qt::LinksAccessibleByKeyboard
                                              | Qt::TextSelectableByMouse);
        warningLabel->setText(Tr::tr(
            "Enabling %1 is subject to your agreement and abidance with your applicable "
            "%1 terms. It is your responsibility to know and accept the requirements and "
            "parameters of using tools like %1. This may include, but is not limited to, "
            "ensuring you have the rights to allow %1 access to your code, as well as "
            "understanding any implications of your use of %1 and suggestions produced "
            "(like copyright, accuracy, etc.)." ).arg("Copilot"));

        auto authWidget = new AuthWidget();

        auto helpLabel = new QLabel();
        helpLabel->setTextFormat(Qt::MarkdownText);
        helpLabel->setWordWrap(true);
        helpLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse
                                           | Qt::LinksAccessibleByKeyboard
                                           | Qt::TextSelectableByMouse);
        helpLabel->setOpenExternalLinks(true);
        connect(helpLabel, &QLabel::linkHovered, [](const QString &link) {
            QToolTip::showText(QCursor::pos(), link);
        });

        // clang-format off
        helpLabel->setText(Tr::tr(
            "The Copilot plugin requires node.js and the Copilot neovim plugin. "
            "If you install the neovim plugin as described in %1, "
            "the plugin will find the agent.js file automatically.\n\n"
            "Otherwise you need to specify the path to the %2 "
            "file from the Copilot neovim plugin.",
            "Markdown text for the copilot instruction label")
                           .arg("[README.md](https://github.com/github/copilot.vim)")
                           .arg("[agent.js](https://github.com/github/copilot.vim/tree/release/copilot/dist)"));

        Column {
            QString("<b>" + Tr::tr("Note:") + "</b>"), br,
            warningLabel, br,
            CopilotSettings::instance().enableCopilot, br,
            authWidget, br,
            CopilotSettings::instance().nodeJsPath, br,
            CopilotSettings::instance().distPath, br,
            CopilotSettings::instance().autoComplete, br,
            helpLabel, br,
            st
        }.attachTo(this);
        // clang-format on

        auto updateAuthWidget = [authWidget]() {
            authWidget->updateClient(
                FilePath::fromUserInput(CopilotSettings::instance().nodeJsPath.volatileValue()),
                FilePath::fromUserInput(CopilotSettings::instance().distPath.volatileValue()));
        };

        connect(CopilotSettings::instance().nodeJsPath.pathChooser(),
                &PathChooser::textChanged,
                authWidget,
                updateAuthWidget);
        connect(CopilotSettings::instance().distPath.pathChooser(),
                &PathChooser::textChanged,
                authWidget,
                updateAuthWidget);
        updateAuthWidget();

        setOnApply([] {
            CopilotSettings::instance().apply();
            CopilotSettings::instance().writeSettings(Core::ICore::settings());
        });
    }
};

CopilotOptionsPage::CopilotOptionsPage()
{
    setId(Constants::COPILOT_GENERAL_OPTIONS_ID);
    setDisplayName("Copilot");
    setCategory(Constants::COPILOT_GENERAL_OPTIONS_CATEGORY);
    setDisplayCategory(Constants::COPILOT_GENERAL_OPTIONS_DISPLAY_CATEGORY);
    setCategoryIconPath(":/copilot/images/settingscategory_copilot.png");
    setWidgetCreator([] { return new CopilotOptionsPageWidget; });
}

CopilotOptionsPage &CopilotOptionsPage::instance()
{
    static CopilotOptionsPage settingsPage;
    return settingsPage;
}

} // namespace Copilot
