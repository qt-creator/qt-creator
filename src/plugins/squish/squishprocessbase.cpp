// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishprocessbase.h"

namespace Squish::Internal {

SquishProcessBase::SquishProcessBase(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &Utils::Process::readyReadStandardError,
            this, &SquishProcessBase::onErrorOutput);
    connect(&m_process, &Utils::Process::done,
            this, &SquishProcessBase::onDone);
}

void SquishProcessBase::setState(SquishProcessState state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

void SquishProcessBase::start(const Utils::CommandLine &cmdline, const Utils::Environment &env)
{
    QTC_ASSERT(m_process.state() == QProcess::NotRunning, return);
    // avoid crashes on fast re-use
    m_process.close();

    m_process.setCommand(cmdline);
    m_process.setEnvironment(env);

    setState(Starting);
    m_process.start();
    if (!m_process.waitForStarted()) {
        setState(StartFailed);
        qWarning() << "squishprocess did not start within 30s";
    }
    setState(Started);
}

} // namespace Squish::Internal
