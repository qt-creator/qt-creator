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

#include "runconfiguration.h"
#include "project.h"

#include <QtCore/QTimer>

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

#include <QtDebug>

using namespace ProjectExplorer;

// RunConfiguration
RunConfiguration::RunConfiguration(Project *project)
    : m_project(project)
{
}

RunConfiguration::~RunConfiguration()
{
}

Project *RunConfiguration::project() const
{
    return m_project.data();
}

QString RunConfiguration::name() const
{
    return m_name;
}

void RunConfiguration::setName(const QString &name)
{
    m_name = name;
    emit nameChanged();
}

void RunConfiguration::save(PersistentSettingsWriter &writer) const
{
    writer.saveValue("RunConfiguration.name", m_name);
}

void RunConfiguration::restore(const PersistentSettingsReader &reader)
{
    QVariant var = reader.restoreValue("RunConfiguration.name");
    if (var.isValid() && !var.toString().isEmpty())
        m_name = var.toString();
}


IRunConfigurationFactory::IRunConfigurationFactory()
{
}

IRunConfigurationFactory::~IRunConfigurationFactory()
{
}

IRunConfigurationRunner::IRunConfigurationRunner()
{
}

IRunConfigurationRunner::~IRunConfigurationRunner()
{
}

RunControl::RunControl(QSharedPointer<RunConfiguration> runConfiguration)
    : m_runConfiguration(runConfiguration)
{
}

QSharedPointer<RunConfiguration> RunControl::runConfiguration()
{
    return m_runConfiguration;
}

RunControl::~RunControl()
{
}

void RunControl::bringApplicationToForeground(qint64 pid)
{
#ifdef Q_OS_MAC
    m_internalPid = pid;
    m_foregroundCount = 0;
    bringApplicationToForegroundInternal();
#else
    Q_UNUSED(pid)
#endif
}

void RunControl::bringApplicationToForegroundInternal()
{
#ifdef Q_OS_MAC
    ProcessSerialNumber psn;
    GetProcessForPID(m_internalPid, &psn);
    if (SetFrontProcess(&psn) == procNotFound && m_foregroundCount < 15) {
        // somehow the mac/carbon api says
        // "-600 no eligible process with specified process id"
        // if we call SetFrontProcess too early
        ++m_foregroundCount;
        QTimer::singleShot(200, this, SLOT(bringApplicationToForegroundInternal()));
        return;
    }
#endif
}
