/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLPROJECTRUNCONTROL_H
#define QMLPROJECTRUNCONTROL_H

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/applicationlauncher.h>

namespace QmlProjectManager {

class QmlProjectRunConfiguration;

namespace Internal {

class QmlRunControl : public ProjectExplorer::RunControl {
    Q_OBJECT
public:
    explicit QmlRunControl(QmlProjectRunConfiguration *runConfiguration, bool debugMode);
    virtual ~QmlRunControl ();

    // RunControl
    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;

private slots:
    void processExited(int exitCode);
    void slotBringApplicationToForeground(qint64 pid);
    void slotAddToOutputWindow(const QString &line);
    void slotError(const QString & error);

private:
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;

    QString m_executable;
    QStringList m_commandLineArguments;
    bool m_debugMode;
};

class QmlRunControlFactory : public ProjectExplorer::IRunControlFactory {
    Q_OBJECT
public:
    explicit QmlRunControlFactory(QObject *parent = 0);
    virtual ~QmlRunControlFactory();

    // IRunControlFactory
    virtual bool canRun(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode) const;
    virtual ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode);
    virtual QString displayName() const;
    virtual QWidget *configurationWidget(ProjectExplorer::RunConfiguration *runConfiguration);
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLPROJECTRUNCONTROL_H
