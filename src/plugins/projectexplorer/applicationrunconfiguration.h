/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef APPLICATIONRUNCONFIGURATION_H
#define APPLICATIONRUNCONFIGURATION_H

#include <projectexplorer/toolchain.h>

#include "runconfiguration.h"
#include "applicationlauncher.h"

namespace Utils {
class Environment;
}

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT LocalApplicationRunConfiguration : public RunConfiguration
{
    Q_OBJECT
public:
    enum RunMode {
        Console = ApplicationLauncher::Console,
        Gui
    };

    virtual ~LocalApplicationRunConfiguration();
    virtual QString executable() const = 0;
    virtual RunMode runMode() const = 0;
    virtual QString workingDirectory() const = 0;
    virtual QStringList commandLineArguments() const = 0;
    virtual Utils::Environment environment() const = 0;
    virtual QString dumperLibrary() const = 0;
    virtual QStringList dumperLibraryLocations() const = 0;
    virtual ProjectExplorer::ToolChain::ToolChainType toolChainType() const = 0;

protected:
    LocalApplicationRunConfiguration(Target *target, const QString &id);
    LocalApplicationRunConfiguration(Target *target, LocalApplicationRunConfiguration *rc);
};

namespace Internal {

class LocalApplicationRunControlFactory : public IRunControlFactory
{
    Q_OBJECT
public:
    LocalApplicationRunControlFactory ();
    virtual ~LocalApplicationRunControlFactory();
    virtual bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
    virtual QString displayName() const;
    virtual RunControl* create(RunConfiguration *runConfiguration, const QString &mode);
    virtual QWidget *createConfigurationWidget(RunConfiguration  *runConfiguration);
};

class LocalApplicationRunControl : public RunControl
{
    Q_OBJECT
public:
    LocalApplicationRunControl(LocalApplicationRunConfiguration *runConfiguration, QString mode);
    virtual ~LocalApplicationRunControl();
    virtual void start();
    virtual StopResult stop();
    virtual bool isRunning() const;
private slots:
    void processExited(int exitCode);
    void slotAddToOutputWindow(const QString &line, bool isError);
    void slotAppendMessage(const QString &err, bool isError);
private:
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
    QString m_executable;
    QStringList m_commandLineArguments;
    ProjectExplorer::ApplicationLauncher::Mode m_runMode;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // APPLICATIONRUNCONFIGURATION_H
