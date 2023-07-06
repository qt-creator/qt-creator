// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "terminalpane.h"
#include "terminalprocessimpl.h"
#include "terminalsettings.h"
#include "terminalwidget.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

namespace Terminal::Internal {

class TerminalPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Terminal.json")

public:
    TerminalPlugin() = default;

    ~TerminalPlugin() final
    {
        ExtensionSystem::PluginManager::removeObject(m_terminalPane);
        delete m_terminalPane;
        m_terminalPane = nullptr;
    }

    void extensionsInitialized() final
    {
        m_terminalPane = new TerminalPane;
        ExtensionSystem::PluginManager::addObject(m_terminalPane);

        TerminalWidget::initActions();

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
            if (isEnabled != settings().enableTerminal()) {
                isEnabled = settings().enableTerminal();
                if (isEnabled)
                    enable();
                else
                    disable();
            }
        };

        QObject::connect(&settings(),
                         &Utils::AspectContainer::applied,
                         this,
                         settingsChanged);

        settingsChanged();
    }

private:
    TerminalPane *m_terminalPane{nullptr};
};

} // namespace Terminal::Internal

#include "terminalplugin.moc"
