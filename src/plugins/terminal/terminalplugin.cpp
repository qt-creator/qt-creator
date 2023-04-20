// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "terminalplugin.h"

#include "terminalpane.h"
#include "terminalprocessimpl.h"
#include "terminalsettings.h"
#include "terminalsettingspage.h"
#include "terminalcommands.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

namespace Terminal {
namespace Internal {

TerminalPlugin::TerminalPlugin() {}

TerminalPlugin::~TerminalPlugin()
{
    ExtensionSystem::PluginManager::instance()->removeObject(m_terminalPane);
    delete m_terminalPane;
    m_terminalPane = nullptr;
}

bool TerminalPlugin::delayedInitialize()
{
    TerminalCommands::instance().lazyInitCommands();
    return true;
}

void TerminalPlugin::extensionsInitialized()
{
    (void) TerminalSettingsPage::instance();
    TerminalSettings::instance().readSettings(Core::ICore::settings());

    m_terminalPane = new TerminalPane();
    ExtensionSystem::PluginManager::instance()->addObject(m_terminalPane);

    auto enable = [this] {
        Utils::Terminal::Hooks::instance()
            .addCallbackSet("Internal",
                            {[this](const Utils::Terminal::OpenTerminalParameters &p) {
                                 m_terminalPane->openTerminal(p);
                             },
                             [this] { return new TerminalProcessImpl(m_terminalPane); }});
    };

    auto disable = [] { Utils::Terminal::Hooks::instance().removeCallbackSet("Internal"); };

    static bool isEnabled = false;
    auto settingsChanged = [enable, disable] {
        if (isEnabled != TerminalSettings::instance().enableTerminal.value()) {
            isEnabled = TerminalSettings::instance().enableTerminal.value();
            if (isEnabled)
                enable();
            else
                disable();
        }
    };

    QObject::connect(&TerminalSettings::instance(),
                     &Utils::AspectContainer::applied,
                     this,
                     settingsChanged);

    settingsChanged();
}

} // namespace Internal
} // namespace Terminal
