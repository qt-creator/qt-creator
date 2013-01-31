/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "localqmlprofilerrunner.h"
#include "qmlprofilerplugin.h"

using namespace QmlProfiler;
using namespace QmlProfiler::Internal;

LocalQmlProfilerRunner::LocalQmlProfilerRunner(const Configuration &configuration, QObject *parent) :
    AbstractQmlProfilerRunner(parent),
    m_configuration(configuration)
{
    connect(&m_launcher, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
            this, SIGNAL(appendMessage(QString,Utils::OutputFormat)));
}

void LocalQmlProfilerRunner::start()
{
    QString arguments = QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(m_configuration.port);

    if (!m_configuration.executableArguments.isEmpty())
        arguments += QLatin1Char(' ') + m_configuration.executableArguments;

    if (QmlProfilerPlugin::debugOutput)
        qWarning("QmlProfiler: Launching %s:%d", qPrintable(m_configuration.executable),
                 m_configuration.port);

    m_launcher.setWorkingDirectory(m_configuration.workingDirectory);
    m_launcher.setEnvironment(m_configuration.environment);
    connect(&m_launcher, SIGNAL(processExited(int)), this, SLOT(spontaneousStop(int)));
    m_launcher.start(ProjectExplorer::ApplicationLauncher::Gui, m_configuration.executable,
                     arguments);

    emit started();
}

void LocalQmlProfilerRunner::spontaneousStop(int exitCode)
{
    if (QmlProfilerPlugin::debugOutput)
        qWarning("QmlProfiler: Application exited (exit code %d).", exitCode);

    disconnect(&m_launcher, SIGNAL(processExited(int)), this, SLOT(spontaneousStop(int)));

    emit stopped();
}

void LocalQmlProfilerRunner::stop()
{
    if (QmlProfilerPlugin::debugOutput)
        qWarning("QmlProfiler: Stopping application ...");

    if (m_launcher.isRunning())
        m_launcher.stop();
}

quint16 LocalQmlProfilerRunner::debugPort() const
{
    return m_configuration.port;
}
