// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalprocessimpl.h"
#include "terminalwidget.h"

#include <utils/externalterminalprocessimpl.h>

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QTemporaryFile>
#include <QTimer>

Q_LOGGING_CATEGORY(terminalProcessLog, "qtc.terminal.stubprocess", QtDebugMsg)

using namespace Utils;
using namespace Utils::Terminal;

namespace Terminal {

class ProcessStubCreator : public StubCreator
{
public:
    ProcessStubCreator(TerminalProcessImpl *interface, TerminalPane *terminalPane)
        : m_terminalPane(terminalPane)
        , m_process(interface)
        , m_interface(interface)
    {}

    expected_str<qint64> startStubProcess(const ProcessSetupData &setup) override
    {
        if (QApplication::activeModalWidget()) {
            m_fallbackStubCreator = std::make_unique<Utils::ProcessStubCreator>(m_interface);
            return m_fallbackStubCreator->startStubProcess(setup);
        }

        const Id id = Id::fromString(setup.m_commandLine.executable().toUserOutput());

        TerminalWidget *terminal = m_terminalPane->stoppedTerminalWithId(id);

        OpenTerminalParameters openParameters{setup.m_commandLine};
        openParameters.m_exitBehavior = ExitBehavior::Keep;
        openParameters.identifier = id;

        if (!terminal) {
            terminal = new TerminalWidget(nullptr, openParameters);

            terminal->setShellName(
                setup.m_extraData
                    .value(TERMINAL_SHELL_NAME, setup.m_commandLine.executable().fileName())
                    .toString());

            m_terminalPane->addTerminal(terminal, "App");
        } else {
            terminal->restart(openParameters);
        }

        m_terminalPane->ensureVisible(terminal);

        connect(terminal, &TerminalWidget::destroyed, m_process, [process = m_process] {
            if (process->inferiorProcessId())
                process->emitFinished(-1, QProcess::CrashExit);
        });

        return 0;
    }

    TerminalPane *m_terminalPane;
    TerminalProcessImpl *m_process;
    TerminalInterface *m_interface;
    std::unique_ptr<Utils::ProcessStubCreator> m_fallbackStubCreator;
};

TerminalProcessImpl::TerminalProcessImpl(TerminalPane *terminalPane)
    : TerminalInterface(false)
{
    auto creator = new ProcessStubCreator(this, terminalPane);
    creator->moveToThread(qApp->thread());
    setStubCreator(creator);
}

} // namespace Terminal
