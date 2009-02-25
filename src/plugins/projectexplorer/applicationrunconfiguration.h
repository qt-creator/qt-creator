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

#ifndef APPLICATIONRUNCONFIGURATION_H
#define APPLICATIONRUNCONFIGURATION_H

#include "runconfiguration.h"
#include "applicationlauncher.h"

namespace ProjectExplorer {

class Environment;

class PROJECTEXPLORER_EXPORT ApplicationRunConfiguration : public RunConfiguration
{
    Q_OBJECT
public:
    enum RunMode {
        Console = Internal::ApplicationLauncher::Console,
        Gui
    };

    ApplicationRunConfiguration(Project *pro);
    virtual ~ApplicationRunConfiguration();
    virtual QString type() const;
    virtual QString executable() const = 0;
    virtual RunMode runMode() const = 0;
    virtual QString workingDirectory() const = 0;
    virtual QStringList commandLineArguments() const = 0;
    virtual Environment environment() const = 0;

    virtual void save(PersistentSettingsWriter &writer) const;
    virtual void restore(const PersistentSettingsReader &reader);
};

namespace Internal {

class ApplicationRunConfigurationRunner : public IRunConfigurationRunner
{
    Q_OBJECT
public:
    ApplicationRunConfigurationRunner();
    virtual ~ApplicationRunConfigurationRunner();
    virtual bool canRun(QSharedPointer<RunConfiguration> runConfiguration, const QString &mode);
    virtual QString displayName() const;
    virtual RunControl* run(QSharedPointer<RunConfiguration> runConfiguration, const QString &mode);
    virtual QWidget *configurationWidget(QSharedPointer<RunConfiguration> runConfiguration);
};

class ApplicationRunControl : public RunControl
{
    Q_OBJECT
public:
    ApplicationRunControl(QSharedPointer<ApplicationRunConfiguration> runConfiguration);
    virtual ~ApplicationRunControl();
    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;
private slots:
    void processExited(int exitCode);
    void slotAddToOutputWindow(const QString &line);
    void slotError(const QString & error);
private:
    ApplicationLauncher m_applicationLauncher;
    QString m_executable;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // APPLICATIONRUNCONFIGURATION_H
