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
