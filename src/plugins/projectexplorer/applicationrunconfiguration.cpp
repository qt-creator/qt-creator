/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "applicationrunconfiguration.h"
#include "persistentsettings.h"
#include "environment.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtGui/QLabel>
#include <QtGui/QTextDocument>
#include <QDebug>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

/// LocalApplicationRunConfiguration

LocalApplicationRunConfiguration::LocalApplicationRunConfiguration(Project *pro)
    : RunConfiguration(pro)
{
}

LocalApplicationRunConfiguration::~LocalApplicationRunConfiguration()
{
}

QString LocalApplicationRunConfiguration::type() const
{
    return "ProjectExplorer.LocalApplicationRunConfiguration";
}

void LocalApplicationRunConfiguration::save(PersistentSettingsWriter &writer) const
{
    RunConfiguration::save(writer);
}

void LocalApplicationRunConfiguration::restore(const PersistentSettingsReader &reader)
{
    RunConfiguration::restore(reader);
}

/// LocalApplicationRunControlFactory

LocalApplicationRunControlFactory::LocalApplicationRunControlFactory()
{
}

LocalApplicationRunControlFactory::~LocalApplicationRunControlFactory()
{
}

bool LocalApplicationRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode) const
{
    return (mode == ProjectExplorer::Constants::RUNMODE)
            && (qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration) != 0);
}

QString LocalApplicationRunControlFactory::displayName() const
{
    return tr("Run");
}

RunControl *LocalApplicationRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    return new LocalApplicationRunControl(qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration));
}

QWidget *LocalApplicationRunControlFactory::configurationWidget(RunConfiguration *runConfiguration)
{
    Q_UNUSED(runConfiguration)
    return new QLabel("TODO add Configuration widget");
}

// ApplicationRunControl

LocalApplicationRunControl::LocalApplicationRunControl(LocalApplicationRunConfiguration *runConfiguration)
    : RunControl(runConfiguration)
{
    m_applicationLauncher.setEnvironment(runConfiguration->environment().toStringList());
    m_applicationLauncher.setWorkingDirectory(runConfiguration->workingDirectory());

    m_executable = runConfiguration->executable();
    m_runMode = static_cast<ApplicationLauncher::Mode>(runConfiguration->runMode());
    m_commandLineArguments = runConfiguration->commandLineArguments();

    connect(&m_applicationLauncher, SIGNAL(applicationError(QString)),
            this, SLOT(slotError(QString)));
    connect(&m_applicationLauncher, SIGNAL(appendOutput(QString)),
            this, SLOT(slotAddToOutputWindow(QString)));
    connect(&m_applicationLauncher, SIGNAL(processExited(int)),
            this, SLOT(processExited(int)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(bringApplicationToForeground(qint64)));
}

LocalApplicationRunControl::~LocalApplicationRunControl()
{
}

void LocalApplicationRunControl::start()
{
    m_applicationLauncher.start(m_runMode, m_executable, m_commandLineArguments);
    emit started();

    emit addToOutputWindow(this, tr("Starting %1...").arg(QDir::toNativeSeparators(m_executable)));
}

void LocalApplicationRunControl::stop()
{
    m_applicationLauncher.stop();
}

bool LocalApplicationRunControl::isRunning() const
{
    return m_applicationLauncher.isRunning();
}

void LocalApplicationRunControl::slotError(const QString & err)
{
    emit error(this, err);
    emit finished();
}

void LocalApplicationRunControl::slotAddToOutputWindow(const QString &line)
{
    emit addToOutputWindowInline(this, line);
}

void LocalApplicationRunControl::processExited(int exitCode)
{
    emit addToOutputWindow(this, tr("%1 exited with code %2").arg(QDir::toNativeSeparators(m_executable)).arg(exitCode));
    emit finished();
}

