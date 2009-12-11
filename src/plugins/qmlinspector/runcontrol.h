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
#ifndef QMLINSPECTORRUNCONTROL_H
#define QMLINSPECTORRUNCONTROL_H

#include "qmlinspector.h"

#include <projectexplorer/runconfiguration.h>

#include <QtCore/qobject.h>

namespace ProjectExplorer {
    class ApplicationLauncher;
}

class QmlInspectorRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    explicit QmlInspectorRunControlFactory(QObject *parent);

    virtual bool canRun(
                ProjectExplorer::RunConfiguration *runConfiguration,
                const QString &mode) const;

    virtual ProjectExplorer::RunControl *create(
                ProjectExplorer::RunConfiguration *runConfiguration,
                const QString &mode);

    ProjectExplorer::RunControl *create(
                ProjectExplorer::RunConfiguration *runConfiguration,
                const QString &mode,
                const QmlInspector::StartParameters &sp);
                
    virtual QString displayName() const;

    virtual QWidget *configurationWidget(ProjectExplorer::RunConfiguration *runConfiguration);
};

class QmlInspectorRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    explicit QmlInspectorRunControl(ProjectExplorer::RunConfiguration *runConfiguration,
                                    const QmlInspector::StartParameters &sp = QmlInspector::StartParameters());
    ~QmlInspectorRunControl();
    
    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;

private slots:
    void appendOutput(const QString &s);
    void appStarted();
    void delayedStart();
    void viewerExited();
    void applicationError(const QString &error);

private:
    ProjectExplorer::RunConfiguration *m_configuration;
    bool m_running;
    ProjectExplorer::ApplicationLauncher *m_viewerLauncher;
    QmlInspector::StartParameters m_startParams;
};

#endif
