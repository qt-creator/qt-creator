/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
