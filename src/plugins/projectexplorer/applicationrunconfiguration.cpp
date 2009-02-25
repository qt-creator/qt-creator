/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "applicationrunconfiguration.h"
#include "persistentsettings.h"
#include "environment.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QtGui/QLabel>
#include <QtGui/QTextDocument>
#include <QDebug>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

/// ApplicationRunConfiguration

ApplicationRunConfiguration::ApplicationRunConfiguration(Project *pro)
    : RunConfiguration(pro)
{
}

ApplicationRunConfiguration::~ApplicationRunConfiguration()
{
}

QString ApplicationRunConfiguration::type() const
{
    return "ProjectExplorer.ApplicationRunConfiguration";
}

void ApplicationRunConfiguration::save(PersistentSettingsWriter &writer) const
{
    RunConfiguration::save(writer);
}

void ApplicationRunConfiguration::restore(const PersistentSettingsReader &reader)
{
    RunConfiguration::restore(reader);
}

/// ApplicationRunConfigurationRunner

ApplicationRunConfigurationRunner::ApplicationRunConfigurationRunner()
{
}

ApplicationRunConfigurationRunner::~ApplicationRunConfigurationRunner()
{
}

bool ApplicationRunConfigurationRunner::canRun(QSharedPointer<RunConfiguration> runConfiguration, const QString &mode)
{
    return (mode == ProjectExplorer::Constants::RUNMODE)
            && (!qSharedPointerCast<ApplicationRunConfiguration>(runConfiguration).isNull());
}

QString ApplicationRunConfigurationRunner::displayName() const
{
    return QObject::tr("Run");
}

RunControl* ApplicationRunConfigurationRunner::run(QSharedPointer<RunConfiguration> runConfiguration, const QString &mode)
{
    QSharedPointer<ApplicationRunConfiguration> rc = qSharedPointerCast<ApplicationRunConfiguration>(runConfiguration);
    Q_ASSERT(rc);
    Q_ASSERT(mode == ProjectExplorer::Constants::RUNMODE);

    ApplicationRunControl *runControl = new ApplicationRunControl(rc);
    return runControl;
}

QWidget *ApplicationRunConfigurationRunner::configurationWidget(QSharedPointer<RunConfiguration> runConfiguration)
{
    Q_UNUSED(runConfiguration);
    return new QLabel("TODO add Configuration widget");
}

// ApplicationRunControl

ApplicationRunControl::ApplicationRunControl(QSharedPointer<ApplicationRunConfiguration> runConfiguration)
    : RunControl(runConfiguration), m_applicationLauncher()
{
    connect(&m_applicationLauncher, SIGNAL(applicationError(QString)),
            this, SLOT(slotError(QString)));
    connect(&m_applicationLauncher, SIGNAL(appendOutput(QString)),
            this, SLOT(slotAddToOutputWindow(QString)));
    connect(&m_applicationLauncher, SIGNAL(processExited(int)),
            this, SLOT(processExited(int)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(bringApplicationToForeground(qint64)));
}

ApplicationRunControl::~ApplicationRunControl()
{
}

void ApplicationRunControl::start()
{
    QSharedPointer<ApplicationRunConfiguration> rc = qSharedPointerCast<ApplicationRunConfiguration>(runConfiguration());
    Q_ASSERT(rc);

    m_applicationLauncher.setEnvironment(rc->environment().toStringList());
    m_applicationLauncher.setWorkingDirectory(rc->workingDirectory());

    m_executable = rc->executable();

    m_applicationLauncher.start(static_cast<Internal::ApplicationLauncher::Mode>(rc->runMode()),
                                m_executable, rc->commandLineArguments());
    emit started();

    emit addToOutputWindow(this, tr("Starting %1...").arg(m_executable));
}

void ApplicationRunControl::stop()
{
    m_applicationLauncher.stop();
}

bool ApplicationRunControl::isRunning() const
{
    return m_applicationLauncher.isRunning();
}

void ApplicationRunControl::slotError(const QString & err)
{
    emit error(this, err);
    emit finished();
}

void ApplicationRunControl::slotAddToOutputWindow(const QString &line)
{
    emit addToOutputWindowInline(this, line);
}

void ApplicationRunControl::processExited(int exitCode)
{
    emit addToOutputWindow(this, tr("%1 exited with code %2").arg(m_executable).arg(exitCode));
    emit finished();
}

