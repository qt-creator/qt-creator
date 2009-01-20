/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef DEBUGGERRUNNER_H
#define DEBUGGERRUNNER_H

#include <projectexplorer/runconfiguration.h>

namespace ProjectExplorer {
class ApplicationRunConfiguration;
}

namespace Debugger {
namespace Internal {

class DebuggerManager;
class StartData;

typedef QSharedPointer<ProjectExplorer::RunConfiguration>
    RunConfigurationPtr;

typedef QSharedPointer<ProjectExplorer::ApplicationRunConfiguration>
    ApplicationRunConfigurationPtr;

class DebuggerRunner : public ProjectExplorer::IRunConfigurationRunner
{
    Q_OBJECT

public:
    explicit DebuggerRunner(DebuggerManager *manager);

    virtual bool canRun(RunConfigurationPtr runConfiguration, const QString &mode);
    virtual QString displayName() const;
    virtual ProjectExplorer::RunControl *run(RunConfigurationPtr runConfiguration,
        const QString &mode);
    virtual QWidget *configurationWidget(RunConfigurationPtr runConfiguration);

private:
    DebuggerManager *m_manager;
};


class DebuggerRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    DebuggerRunControl(DebuggerManager *manager,
        ApplicationRunConfigurationPtr runConfiguration);

    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;

private slots:
    void debuggingFinished();
    void slotAddToOutputWindowInline(const QString &output);

private:
    DebuggerManager *m_manager;
    bool m_running;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERRUNNER_H
