// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotsettings.h"
#include "copilottr.h"

#include <utils/algorithm.h>
#include <utils/environment.h>

using namespace Utils;

namespace Copilot {

CopilotSettings &CopilotSettings::instance()
{
    static CopilotSettings settings;
    return settings;
}

CopilotSettings::CopilotSettings()
{
    setAutoApply(false);

    const FilePath nodeFromPath = FilePath("node").searchInPath();

    const FilePaths searchDirs
        = {FilePath::fromUserInput("~/.vim/pack/github/start/copilot.vim/copilot/dist/agent.js"),
           FilePath::fromUserInput(
               "~/.config/nvim/pack/github/start/copilot.vim/copilot/dist/agent.js"),
           FilePath::fromUserInput(
               "~/vimfiles/pack/github/start/copilot.vim/copilot/dist/agent.js"),
           FilePath::fromUserInput(
               "~/AppData/Local/nvim/pack/github/start/copilot.vim/copilot/dist/agent.js")};

    const FilePath distFromVim = Utils::findOrDefault(searchDirs, [](const FilePath &fp) {
        return fp.exists();
    });

    registerAspect(&nodeJsPath);
    nodeJsPath.setExpectedKind(PathChooser::ExistingCommand);
    nodeJsPath.setDefaultFilePath(nodeFromPath);
    nodeJsPath.setSettingsKey("Copilot.NodeJsPath");
    nodeJsPath.setLabelText(Tr::tr("Node.js path:"));
    nodeJsPath.setHistoryCompleter("Copilot.NodePath.History");
    nodeJsPath.setDisplayName(Tr::tr("Node.js Path"));
    nodeJsPath.setToolTip(
        Tr::tr("Select path to node.js executable. See https://nodejs.org/de/download/"
               "for installation instructions."));

    registerAspect(&distPath);
    distPath.setExpectedKind(PathChooser::File);
    distPath.setDefaultFilePath(distFromVim);
    distPath.setSettingsKey("Copilot.DistPath");
    distPath.setLabelText(Tr::tr("Path to agent.js:"));
    distPath.setHistoryCompleter("Copilot.DistPath.History");
    distPath.setDisplayName(Tr::tr("Agent.js path"));
    distPath.setToolTip(Tr::tr(
        "Select path to agent.js in copilot neovim plugin. See "
        "https://github.com/github/copilot.vim#getting-started for installation instructions."));

    registerAspect(&autoComplete);
    autoComplete.setDisplayName(Tr::tr("Auto Complete"));
    autoComplete.setLabelText(Tr::tr("Request completions automatically"));
    autoComplete.setDefaultValue(true);
    autoComplete.setToolTip(Tr::tr("Automatically request suggestions for the current text cursor "
                                   "position after changes to the document"));
}

} // namespace Copilot
