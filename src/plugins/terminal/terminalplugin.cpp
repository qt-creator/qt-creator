// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "consolehost.h"
#include "terminaleditor.h"
#include "terminalpane.h"
#include "terminalprocessimpl.h"
#include "terminalsettings.h"
#include "terminaltr.h"
#include "terminalwidget.h"

#ifdef WITH_TESTS
#include "shellprofilesaspect_test.h"
#include "terminaleditor_test.h"
#endif

#include <coreplugin/dialogs/ioptionspage.h>

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

    void initialize() final
    {
#ifdef WITH_TESTS
        addTestCreator(createShellProfilesAspectTest);
        addTestCreator([this] { return createTerminalEditorTest(m_terminalPane); });
#endif
    }

    void extensionsInitialized() final
    {
        m_terminalPane = new TerminalPane(this);
        setupTerminalEditor([this](TerminalWidget *terminal) {
            m_terminalPane->addTerminal(terminal, terminal->title());
        });

        ConsoleHost::apply();

        Core::IOptionsPage::registerCategory(
            "ZY.Terminal", Tr::tr("Terminal"), ":/terminal/images/settingscategory_terminal.png");

        TerminalWidget::initActions(this);

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

    ShutdownFlag aboutToShutdown() final
    {
        closeAllTerminalEditors();
        return SynchronousShutdown;
    }

private:
    TerminalPane *m_terminalPane{nullptr};
};

} // namespace Terminal::Internal

#include "terminalplugin.moc"
