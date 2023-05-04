// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalprocessimpl.h"
#include "terminalwidget.h"

#include <QCoreApplication>
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
    {}

    expected_str<qint64> startStubProcess(const ProcessSetupData &setup) override
    {
        const Id id = Id::fromString(setup.m_commandLine.executable().toUserOutput());

        TerminalWidget *terminal = m_terminalPane->stoppedTerminalWithId(id);

        const OpenTerminalParameters openParameters{setup.m_commandLine,
                                                    std::nullopt,
                                                    std::nullopt,
                                                    ExitBehavior::Keep,
                                                    id};

        if (!terminal) {
            terminal = new TerminalWidget(nullptr, openParameters);

            terminal->setShellName(setup.m_commandLine.executable().fileName());
            m_terminalPane->addTerminal(terminal, "App");
        } else {
            terminal->restart(openParameters);
        }

        connect(terminal, &TerminalWidget::destroyed, m_process, [process = m_process] {
            if (process->inferiorProcessId())
                process->emitFinished(-1, QProcess::CrashExit);
        });

        return 0;
    }

    TerminalPane *m_terminalPane;
    TerminalProcessImpl *m_process;
};

TerminalProcessImpl::TerminalProcessImpl(TerminalPane *terminalPane)
    : TerminalInterface(false)
{
    auto creator = new ProcessStubCreator(this, terminalPane);
    creator->moveToThread(qApp->thread());
    setStubCreator(creator);
}

} // namespace Terminal
