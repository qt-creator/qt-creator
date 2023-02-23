// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "terminalplugin.h"

#include "terminalpane.h"
#include "terminalprocessinterface.h"
#include "terminalsettings.h"
#include "terminalsettingspage.h"

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

void TerminalPlugin::extensionsInitialized()
{
    TerminalSettingsPage::instance().init();
    TerminalSettings::instance().readSettings(Core::ICore::settings());

    m_terminalPane = new TerminalPane();
    ExtensionSystem::PluginManager::instance()->addObject(m_terminalPane);

    Utils::Terminal::Hooks::instance().openTerminalHook().set(
        [this](const Utils::Terminal::OpenTerminalParameters &p) {
            m_terminalPane->openTerminal(p);
        });

    /*Utils::Terminal::Hooks::instance().createTerminalProcessInterfaceHook().set(
        [this]() -> Utils::ProcessInterface * {
            return new TerminalProcessInterface(m_terminalPane);
        });*/
}

} // namespace Internal
} // namespace Terminal
