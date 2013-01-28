/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
    virtual ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                                                ProjectExplorer::RunMode mode, QString *errorMessage);
    virtual QString displayName() const;

private:
    ProjectExplorer::RunControl *createDebugRunControl(QmlProjectRunConfiguration *runConfig, QString *errorMessage);
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLPROJECTRUNCONTROL_H
