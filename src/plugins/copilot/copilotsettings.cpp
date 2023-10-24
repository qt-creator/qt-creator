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

    // clang-format off

    // From: https://github.com/github/copilot.vim/blob/release/README.md#getting-started
    const FilePaths searchDirs = {
        // Vim, Linux/macOS:
        FilePath::fromUserInput("~/.vim/pack/github/start/copilot.vim/dist/agent.js"),
        FilePath::fromUserInput("~/.vim/pack/github/start/copilot.vim/copilot/dist/agent.js"),

        // Neovim, Linux/macOS:
        FilePath::fromUserInput("~/.config/nvim/pack/github/start/copilot.vim/dist/agent.js"),
        FilePath::fromUserInput("~/.config/nvim/pack/github/start/copilot.vim/copilot/dist/agent.js"),

        // Vim, Windows (PowerShell command):
        FilePath::fromUserInput("~/vimfiles/pack/github/start/copilot.vim/dist/agent.js"),
        FilePath::fromUserInput("~/vimfiles/pack/github/start/copilot.vim/copilot/dist/agent.js"),

        // Neovim, Windows (PowerShell command):
        FilePath::fromUserInput("~/AppData/Local/nvim/pack/github/start/copilot.vim/dist/agent.js"),
        FilePath::fromUserInput("~/AppData/Local/nvim/pack/github/start/copilot.vim/copilot/dist/agent.js")
    };
    // clang-format on

    const FilePath distFromVim = findOrDefault(searchDirs, &FilePath::exists);

    nodeJsPath.setExpectedKind(PathChooser::ExistingCommand);
    nodeJsPath.setDefaultValue(nodeFromPath.toUserOutput());
    nodeJsPath.setSettingsKey("Copilot.NodeJsPath");
    nodeJsPath.setLabelText(Tr::tr("Node.js path:"));
    nodeJsPath.setHistoryCompleter("Copilot.NodePath.History");
    nodeJsPath.setDisplayName(Tr::tr("Node.js Path"));
    //: %1 is the URL to nodejs
    nodeJsPath.setToolTip(Tr::tr("Select path to node.js executable. See %1 "
                                 "for installation instructions.")
                              .arg("https://nodejs.org/en/download/"));

    distPath.setExpectedKind(PathChooser::File);
    distPath.setDefaultValue(distFromVim.toUserOutput());
    distPath.setSettingsKey("Copilot.DistPath");
    distPath.setLabelText(Tr::tr("Path to agent.js:"));
    distPath.setHistoryCompleter("Copilot.DistPath.History");
    distPath.setDisplayName(Tr::tr("Agent.js path"));
    //: %1 is the URL to copilot.vim getting started
    distPath.setToolTip(Tr::tr("Select path to agent.js in Copilot Neovim plugin. See "
                               "%1 for installation instructions.")
                            .arg("https://github.com/github/copilot.vim#getting-started"));

    autoComplete.setDisplayName(Tr::tr("Auto Request"));
    autoComplete.setSettingsKey("Copilot.Autocomplete");
    autoComplete.setLabelText(Tr::tr("Auto request"));
    autoComplete.setDefaultValue(true);
    autoComplete.setToolTip(Tr::tr("Automatically request suggestions for the current text cursor "
                                   "position after changes to the document."));

    useProxy.setDisplayName(Tr::tr("Use Proxy"));
    useProxy.setSettingsKey("Copilot.UseProxy");
    useProxy.setLabelText(Tr::tr("Use proxy"));
    useProxy.setDefaultValue(false);
    useProxy.setToolTip(Tr::tr("Use a proxy to connect to the Copilot servers."));

    proxyHost.setDisplayName(Tr::tr("Proxy Host"));
    proxyHost.setDisplayStyle(StringAspect::LineEditDisplay);
    proxyHost.setSettingsKey("Copilot.ProxyHost");
    proxyHost.setLabelText(Tr::tr("Proxy host:"));
    proxyHost.setDefaultValue("");
    proxyHost.setToolTip(Tr::tr("The host name of the proxy server."));
    proxyHost.setHistoryCompleter("Copilot.ProxyHost.History");

    proxyPort.setDisplayName(Tr::tr("Proxy Port"));
    proxyPort.setSettingsKey("Copilot.ProxyPort");
    proxyPort.setLabelText(Tr::tr("Proxy port:"));
    proxyPort.setDefaultValue(3128);
    proxyPort.setToolTip(Tr::tr("The port of the proxy server."));
    proxyPort.setRange(1, 65535);

    proxyUser.setDisplayName(Tr::tr("Proxy User"));
    proxyUser.setDisplayStyle(StringAspect::LineEditDisplay);
    proxyUser.setSettingsKey("Copilot.ProxyUser");
    proxyUser.setLabelText(Tr::tr("Proxy user:"));
    proxyUser.setDefaultValue("");
    proxyUser.setToolTip(Tr::tr("The user name to access the proxy server."));
    proxyUser.setHistoryCompleter("Copilot.ProxyUser.History");

    saveProxyPassword.setDisplayName(Tr::tr("Save Proxy Password"));
    saveProxyPassword.setSettingsKey("Copilot.SaveProxyPassword");
    saveProxyPassword.setLabelText(Tr::tr("Save proxy password"));
    saveProxyPassword.setDefaultValue(false);
    saveProxyPassword.setToolTip(
        Tr::tr("Save the password to access the proxy server. The password is stored insecurely."));

    proxyPassword.setDisplayName(Tr::tr("Proxy Password"));
    proxyPassword.setDisplayStyle(StringAspect::PasswordLineEditDisplay);
    proxyPassword.setSettingsKey("Copilot.ProxyPassword");
    proxyPassword.setLabelText(Tr::tr("Proxy password:"));
    proxyPassword.setDefaultValue("");
    proxyPassword.setToolTip(Tr::tr("The password for the proxy server."));

    proxyRejectUnauthorized.setDisplayName(Tr::tr("Reject Unauthorized"));
    proxyRejectUnauthorized.setSettingsKey("Copilot.ProxyRejectUnauthorized");
    proxyRejectUnauthorized.setLabelText(Tr::tr("Reject unauthorized"));
    proxyRejectUnauthorized.setDefaultValue(true);
    proxyRejectUnauthorized.setToolTip(Tr::tr("Reject unauthorized certificates from the proxy "
                                              "server. Turning this off is a security risk."));

    initEnableAspect(enableCopilot);

    readSettings();

    // TODO: As a workaround we set the enabler after reading the settings, as that does not signal
    // a change.
    nodeJsPath.setEnabler(&enableCopilot);
    distPath.setEnabler(&enableCopilot);
    autoComplete.setEnabler(&enableCopilot);
    useProxy.setEnabler(&enableCopilot);

    proxyHost.setEnabler(&useProxy);
    proxyPort.setEnabler(&useProxy);
    proxyUser.setEnabler(&useProxy);
    saveProxyPassword.setEnabler(&useProxy);
    proxyRejectUnauthorized.setEnabler(&useProxy);

    proxyPassword.setEnabler(&saveProxyPassword);

    setLayouter([this] {
        using namespace Layouting;

        auto warningLabel = new QLabel;
        warningLabel->setWordWrap(true);
        warningLabel->setTextInteractionFlags(
            Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard | Qt::TextSelectableByMouse);
        warningLabel->setText(
            Tr::tr("Enabling %1 is subject to your agreement and abidance with your applicable "
                   "%1 terms. It is your responsibility to know and accept the requirements and "
                   "parameters of using tools like %1. This may include, but is not limited to, "
                   "ensuring you have the rights to allow %1 access to your code, as well as "
                   "understanding any implications of your use of %1 and suggestions produced "
                   "(like copyright, accuracy, etc.).")
                .arg("Copilot"));

        auto authWidget = new AuthWidget();

        auto helpLabel = new QLabel();
        helpLabel->setTextFormat(Qt::MarkdownText);
        helpLabel->setWordWrap(true);
        helpLabel->setTextInteractionFlags(
            Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard | Qt::TextSelectableByMouse);
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
                           .arg("[agent.js](https://github.com/github/copilot.vim/tree/release/dist)"));

        return Column {
            Group {
                title(Tr::tr("Note")),
                Column {
                    warningLabel, br,
                    helpLabel, br,
                }
            },
            Form {
                authWidget, br,
                enableCopilot, br,
                nodeJsPath, br,
                distPath, br,
                autoComplete, br,
                hr, br,
                useProxy, br,
                proxyHost, br,
                proxyPort, br,
                proxyRejectUnauthorized, br,
                proxyUser, br,
                saveProxyPassword, br,
                proxyPassword, br,
            },
            st
        };
        // clang-format on
    });
}

CopilotProjectSettings::CopilotProjectSettings(ProjectExplorer::Project *project)
{
    setAutoApply(true);

    useGlobalSettings.setSettingsKey(Constants::COPILOT_USE_GLOBAL_SETTINGS);
    useGlobalSettings.setDefaultValue(true);

    initEnableAspect(enableCopilot);

    Store map = storeFromVariant(project->namedSettings(Constants::COPILOT_PROJECT_SETTINGS_ID));
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
    Store map;
    toMap(map);
    project->setNamedSettings(Constants::COPILOT_PROJECT_SETTINGS_ID, variantFromStore(map));

    // This triggers a restart of the Copilot language server.
    settings().apply();
}

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
        setSettingsProvider([] { return &settings(); });
    }
};

const CopilotSettingsPage settingsPage;

} // namespace Copilot
