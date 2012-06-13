/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLPROJECTRUNCONTROL_H
#define QMLPROJECTRUNCONTROL_H

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/applicationlauncher.h>

namespace QmlProjectManager {

class QmlProjectRunConfiguration;

namespace Internal {

class QmlProjectRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    QmlProjectRunControl(QmlProjectRunConfiguration *runConfiguration,
                         ProjectExplorer::RunMode mode);
    virtual ~QmlProjectRunControl ();

    // RunControl
    virtual void start();
    virtual StopResult stop();
    virtual bool isRunning() const;
    virtual QIcon icon() const;

    QString mainQmlFile() const;

private slots:
    void processExited(int exitCode);
    void slotBringApplicationToForeground(qint64 pid);
    void slotAppendMessage(const QString &line, Utils::OutputFormat);

private:
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;

    QString m_executable;
    QString m_commandLineArguments;
    QString m_mainQmlFile;
};

class QmlProjectRunControlFactory : public ProjectExplorer::IRunControlFactory {
    Q_OBJECT
public:
    explicit QmlProjectRunControlFactory(QObject *parent = 0);
    virtual ~QmlProjectRunControlFactory();

    // IRunControlFactory
    virtual bool canRun(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode) const;
    virtual ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode);
    virtual QString displayName() const;

private:
    ProjectExplorer::RunControl *createDebugRunControl(QmlProjectRunConfiguration *runConfig);
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLPROJECTRUNCONTROL_H
