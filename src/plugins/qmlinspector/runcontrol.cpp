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
#include "runcontrol.h"

#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/qdebug.h>
#include <QtCore/qtimer.h>

using namespace ProjectExplorer;


QmlInspectorRunControlFactory::QmlInspectorRunControlFactory(QObject *parent)
    : ProjectExplorer::IRunControlFactory(parent)
{
}

bool QmlInspectorRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
    Q_UNUSED(runConfiguration);
    if (mode != ProjectExplorer::Constants::RUNMODE)
        return false;
    return true;
}

ProjectExplorer::RunControl *QmlInspectorRunControlFactory::create(RunConfiguration *runConfiguration, const QString &mode)
{
    Q_UNUSED(mode);
    return new QmlInspectorRunControl(runConfiguration);
}

ProjectExplorer::RunControl *QmlInspectorRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration,
const QString &mode, const QmlInspector::StartParameters &sp)
{
    Q_UNUSED(mode);
    return new QmlInspectorRunControl(runConfiguration, sp);
}
                
QString QmlInspectorRunControlFactory::displayName() const
{
    return tr("Qml Inspector");
}

QWidget *QmlInspectorRunControlFactory::configurationWidget(RunConfiguration *runConfiguration)
{
    Q_UNUSED(runConfiguration);
    return 0;
}



QmlInspectorRunControl::QmlInspectorRunControl(ProjectExplorer::RunConfiguration *runConfiguration,
const QmlInspector::StartParameters &sp)
    : ProjectExplorer::RunControl(runConfiguration),
      m_configuration(runConfiguration),
      m_running(false),
      m_viewerLauncher(0),
      m_startParams(sp)
{
}

QmlInspectorRunControl::~QmlInspectorRunControl()
{
}

void QmlInspectorRunControl::start()
{
    if (m_running || m_viewerLauncher)
        return;

    m_viewerLauncher = new ProjectExplorer::ApplicationLauncher(this);
    connect(m_viewerLauncher, SIGNAL(applicationError(QString)), SLOT(applicationError(QString)));
    connect(m_viewerLauncher, SIGNAL(processExited(int)), SLOT(viewerExited()));
    connect(m_viewerLauncher, SIGNAL(appendOutput(QString)), SLOT(appendOutput(QString)));
    connect(m_viewerLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(appStarted()));

    LocalApplicationRunConfiguration *rc = qobject_cast<LocalApplicationRunConfiguration *>(m_configuration);
    if (!rc)
        return;

    ProjectExplorer::Environment env = rc->environment();
    env.set("QML_DEBUG_SERVER_PORT", QString::number(m_startParams.port));

    QStringList arguments = rc->commandLineArguments();
    arguments << QLatin1String("-stayontop");

    m_viewerLauncher->setEnvironment(env.toStringList());
    m_viewerLauncher->setWorkingDirectory(rc->workingDirectory());

    m_running = true;

    m_viewerLauncher->start(static_cast<ApplicationLauncher::Mode>(rc->runMode()),
                            rc->executable(), arguments);
}

void QmlInspectorRunControl::stop()
{
    if (m_viewerLauncher->isRunning())
        m_viewerLauncher->stop();
}

bool QmlInspectorRunControl::isRunning() const
{
    return m_running;
}

void QmlInspectorRunControl::appStarted()
{
    QTimer::singleShot(500, this, SLOT(delayedStart()));
}

void QmlInspectorRunControl::appendOutput(const QString &s)
{
    emit addToOutputWindow(this, s);
}

void QmlInspectorRunControl::delayedStart()
{
    emit started();
}

void QmlInspectorRunControl::viewerExited()
{
    m_running = false;
    emit finished();
    
    deleteLater();
}

void QmlInspectorRunControl::applicationError(const QString &s)
{
    emit error(this, s);
}
