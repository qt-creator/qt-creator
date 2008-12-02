/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "applicationrunconfiguration.h"
#include "persistentsettings.h"
#include "environment.h"
#include <projectexplorer/projectexplorerconstants.h>

#include <QtGui/QLabel>
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
    connect(&m_applicationLauncher, SIGNAL(applicationError(const QString &)),
            this, SLOT(slotError(const QString &)));
    connect(&m_applicationLauncher, SIGNAL(appendOutput(const QString &)),
            this, SLOT(slotAddToOutputWindow(const QString &)));
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

    emit addToOutputWindow(this, tr("Starting %1").arg(m_executable));
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
    emit addToOutputWindow(this, line);
}

void ApplicationRunControl::processExited(int exitCode)
{
    emit addToOutputWindow(this, tr("%1 exited with code %2").arg(m_executable).arg(exitCode));
    emit finished();
}

