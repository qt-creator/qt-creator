// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fakeprocess.h"

FakeProcess::FakeProcess()
{
}

FakeProcess::~FakeProcess()
{
    if (m_isStarted && !m_isFinished)
        emit finished(0, QProcess::NormalExit);
}

void FakeProcess::finishUnsuccessfully()
{
    m_isFinished = true;
    emit finished(1, QProcess::NormalExit);
}

void FakeProcess::finishByCrash()
{
    m_isFinished = true;
    emit finished(0, QProcess::CrashExit);
}

void FakeProcess::finish()
{
    m_isFinished = true;
    emit finished(0, QProcess::NormalExit);
}

void FakeProcess::setArguments(const QStringList &arguments)
{
    m_arguments = arguments;
}

void FakeProcess::setProgram(const QString &program)
{
    m_applicationPath = program;
}

void FakeProcess::setProcessChannelMode(QProcess::ProcessChannelMode)
{
}

void FakeProcess::start()
{
    m_isStarted = true;
}

bool FakeProcess::isStarted() const
{
    return m_isStarted;
}

const QStringList &FakeProcess::arguments() const
{
    return m_arguments;
}

const QString &FakeProcess::applicationPath() const
{
    return m_applicationPath;
}
