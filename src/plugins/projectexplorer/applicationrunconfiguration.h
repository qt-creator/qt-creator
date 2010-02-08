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

#ifndef APPLICATIONRUNCONFIGURATION_H
#define APPLICATIONRUNCONFIGURATION_H

#include <projectexplorer/toolchain.h>

#include "runconfiguration.h"
#include "applicationlauncher.h"

namespace ProjectExplorer {

class Environment;

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
    virtual Environment environment() const = 0;
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
    virtual QWidget *configurationWidget(RunConfiguration  *runConfiguration);
};

class LocalApplicationRunControl : public RunControl
{
    Q_OBJECT
public:
    LocalApplicationRunControl(LocalApplicationRunConfiguration *runConfiguration);
    virtual ~LocalApplicationRunControl();
    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;
private slots:
    void processExited(int exitCode);
    void slotAddToOutputWindow(const QString &line);
    void slotError(const QString & error);
private:
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
    QString m_executable;
    QStringList m_commandLineArguments;
    ProjectExplorer::ApplicationLauncher::Mode m_runMode;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // APPLICATIONRUNCONFIGURATION_H
