// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalprocessinterface.h"
#include "terminalwidget.h"

namespace Terminal {

TerminalProcessInterface::TerminalProcessInterface(TerminalPane *terminalPane)
    : m_terminalPane(terminalPane)
{}

// It's being called only in Starting state. Just before this method is being called,
// the process transitions from NotRunning into Starting state.
void TerminalProcessInterface::start()
{
    QTC_ASSERT(!m_setup.m_commandLine.executable().needsDevice(), return);

    TerminalWidget *terminal = new TerminalWidget(nullptr,
                                                  {m_setup.m_commandLine,
                                                   m_setup.m_workingDirectory,
                                                   m_setup.m_environment,
                                                   Utils::Terminal::ExitBehavior::Keep});

    connect(terminal, &TerminalWidget::started, this, [this](qint64 pid) { emit started(pid); });

    connect(terminal, &QObject::destroyed, this, [this]() {
        emit done(Utils::ProcessResultData{});
    });

    m_terminalPane->addTerminal(terminal, "App");
}

// It's being called only in Running state.
qint64 TerminalProcessInterface::write(const QByteArray &data)
{
    Q_UNUSED(data);
    return 0;
}

// It's being called in Starting or Running state.
void TerminalProcessInterface::sendControlSignal(Utils::ControlSignal controlSignal)
{
    Q_UNUSED(controlSignal);
}

} // namespace Terminal
