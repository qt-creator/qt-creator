// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotsettings.h"

#include "authwidget.h"
#include "copilotconstants.h"
#include "copilottr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/project.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QToolTip>

using namespace Utils;

namespace Copilot {

static void initEnableAspect(BoolAspect &enableCopilot)
{
    enableCopilot.setSettingsKey(Constants::ENABLE_COPILOT);
    enableCopilot.setDisplayName(Tr::tr("Enable Copilot"));
    enableCopilot.setLabelText(Tr::tr("Enable Copilot"));
    enableCopilot.setToolTip(Tr::tr("Enables the Copilot integration."));
    enableCopilot.setDefaultValue(false);
}

CopilotSettings &settings()
{
    static CopilotSettings settings;
    return settings;
}

CopilotSettings::CopilotSettings()
{
    setAutoApply(false);

    const FilePath nodeFromPath = FilePath("node").searchInPath();

    const FilePaths searchDirs

        = {FilePath::fromUserInput("~/.vim/pack/github/start/copilot.vim/dist/agent.js"),
           FilePath::fromUserInput("~/.vim/pack/github/start/copilot.vim/copilot/dist/agent.js"),
           FilePath::fromUserInput(
               "~/.config/nvim/pack/github/start/copilot.vim/copilot/dist/agent.js"),
           FilePath::fromUserInput(
               "~/vimfiles/pack/github/start/copilot.vim/copilot/dist/agent.js"),
           FilePath::fromUserInput(
               "~/AppData/Local/nvim/pack/github/start/copilot.vim/copilot/dist/agent.js")};

    const FilePath distFromVim = findOrDefault(searchDirs, &FilePath::exists);

    nodeJsPath.setExpectedKind(PathChooser::ExistingCommand);
    nodeJsPath.setDefaultValue(nodeFromPath);
    nodeJsPath.setSettingsKey("Copilot.NodeJsPath");
    nodeJsPath.setLabelText(Tr::tr("Node.js path:"));
    nodeJsPath.setHistoryCompleter("Copilot.NodePath.History");
    nodeJsPath.setDisplayName(Tr::tr("Node.js Path"));
    nodeJsPath.setEnabler(&enableCopilot);
    nodeJsPath.setToolTip(
        Tr::tr("Select path to node.js executable. See https://nodejs.org/en/download/"
               "for installation instructions."));

    distPath.setExpectedKind(PathChooser::File);
    distPath.setDefaultValue(distFromVim);
    distPath.setSettingsKey("Copilot.DistPath");
    distPath.setLabelText(Tr::tr("Path to agent.js:"));
    distPath.setHistoryCompleter("Copilot.DistPath.History");
    distPath.setDisplayName(Tr::tr("Agent.js path"));
    distPath.setEnabler(&enableCopilot);
    distPath.setToolTip(Tr::tr(
        "Select path to agent.js in Copilot Neovim plugin. See "
        "https://github.com/github/copilot.vim#getting-started for installation instructions."));

    autoComplete.setDisplayName(Tr::tr("Auto Complete"));
    autoComplete.setSettingsKey("Copilot.Autocomplete");
    autoComplete.setLabelText(Tr::tr("Request completions automatically"));
    autoComplete.setDefaultValue(true);
    autoComplete.setEnabler(&enableCopilot);
    autoComplete.setToolTip(Tr::tr("Automatically request suggestions for the current text cursor "
                                   "position after changes to the document."));

    initEnableAspect(enableCopilot);

    readSettings();
}

CopilotProjectSettings::CopilotProjectSettings(ProjectExplorer::Project *project, QObject *parent)
    : AspectContainer(parent)
{
    setAutoApply(true);

    useGlobalSettings.setSettingsKey(Constants::COPILOT_USE_GLOBAL_SETTINGS);
    useGlobalSettings.setDefaultValue(true);

    initEnableAspect(enableCopilot);

    QVariantMap map = project->namedSettings(Constants::COPILOT_PROJECT_SETTINGS_ID).toMap();
    fromMap(map);

    connect(&enableCopilot, &BaseAspect::changed, this, [this, project] { save(project); });
    connect(&useGlobalSettings, &BaseAspect::changed, this, [this, project] { save(project); });
}

void CopilotProjectSettings::setUseGlobalSettings(bool useGlobal)
{
    useGlobalSettings.setValue(useGlobal);
}

bool CopilotProjectSettings::isEnabled() const
{
    if (useGlobalSettings())
        return settings().enableCopilot();
    return enableCopilot();
}

void CopilotProjectSettings::save(ProjectExplorer::Project *project)
{
    QVariantMap map;
    toMap(map);
    project->setNamedSettings(Constants::COPILOT_PROJECT_SETTINGS_ID, map);

    // This triggers a restart of the Copilot language server.
    settings().apply();
}

// CopilotOptionsPageWidget

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
            settings().enableCopilot, br,
            authWidget, br,
            settings().nodeJsPath, br,
            settings().distPath, br,
            settings().autoComplete, br,
            helpLabel, br,
            st
        }.attachTo(this);
        // clang-format on

        auto updateAuthWidget = [authWidget]() {
            authWidget->updateClient(
                FilePath::fromUserInput(settings().nodeJsPath.volatileValue()),
                FilePath::fromUserInput(settings().distPath.volatileValue()));
        };

        connect(settings().nodeJsPath.pathChooser(),
                &PathChooser::textChanged,
                authWidget,
                updateAuthWidget);
        connect(settings().distPath.pathChooser(),
                &PathChooser::textChanged,
                authWidget,
                updateAuthWidget);
        updateAuthWidget();

        setOnApply([] {
            settings().apply();
            settings().writeSettings();
        });
    }
};

class CopilotSettingsPage : public Core::IOptionsPage
{
public:
    CopilotSettingsPage()
    {
        setId(Constants::COPILOT_GENERAL_OPTIONS_ID);
        setDisplayName("Copilot");
        setCategory(Constants::COPILOT_GENERAL_OPTIONS_CATEGORY);
        setDisplayCategory(Constants::COPILOT_GENERAL_OPTIONS_DISPLAY_CATEGORY);
        setCategoryIconPath(":/copilot/images/settingscategory_copilot.png");
        setWidgetCreator([] { return new CopilotOptionsPageWidget; });
    }
};

const CopilotSettingsPage settingsPage;

} // Copilot
