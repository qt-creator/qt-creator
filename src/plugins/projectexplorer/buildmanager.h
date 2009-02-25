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

#ifndef BUILDMANAGER_H
#define BUILDMANAGER_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QFutureWatcher>
#include <qtconcurrent/QtConcurrentTools>

namespace ProjectExplorer {

namespace Internal {
    class CompileOutputWindow;
    class TaskWindow;
    class BuildProgressFuture;
}

class BuildStep;
class Project;
class ProjectExplorerPlugin;

class PROJECTEXPLORER_EXPORT BuildManager
  : public QObject
{
    Q_OBJECT

    //NBS TODO this class has to many different variables which hold state:
    // m_buildQueue, m_running, m_canceled, m_progress, m_maxProgress, m_activeBuildSteps and ...
    // I might need to reduce that

public:
    BuildManager(ProjectExplorerPlugin *parent);
    ~BuildManager();

    bool isBuilding() const;

    bool tasksAvailable() const;
    //shows with focus
    void gotoTaskWindow();

    void buildProject(Project *p, const QString &configuration);
    void buildProjects(const QList<Project *> &projects, const QList<QString> &configurations);
    void cleanProject(Project *p, const QString &configuration);
    void cleanProjects(const QList<Project *> &projects, const QList<QString> &configurations);
    bool isBuilding(Project *p);

    // Append any build step to the list of build steps (currently only used to add the QMakeStep)
    void appendStep(BuildStep *step, const QString& configuration);

public slots:
    void cancel();
    // Shows without focus
    void showTaskWindow();
    void toggleTaskWindow();
    void toggleOutputWindow();

signals:
    void buildStateChanged(ProjectExplorer::Project *pro);
    void buildQueueFinished(bool success);
    void tasksChanged();

private slots:
    void addToTaskWindow(const QString &file, int type, int line, const QString &description);
    void addToOutputWindow(const QString &string);

    void nextBuildQueue();
    void emitCancelMessage();
    void showBuildResults();

private:
    void startBuildQueue();
    void nextStep();
    void clearBuildQueue();
    void buildQueueAppend(BuildStep * bs, const QString &configuration);
    void incrementActiveBuildSteps(Project *pro);
    void decrementActiveBuildSteps(Project *pro);

    Internal::CompileOutputWindow *m_outputWindow;
    Internal::TaskWindow *m_taskWindow;

    QList<BuildStep *> m_buildQueue;
    QStringList m_configurations; // the corresponding configuration to the m_buildQueue
    ProjectExplorerPlugin *m_projectExplorerPlugin;
    bool m_running;
    QFutureWatcher<bool> m_watcher;
    BuildStep *m_currentBuildStep;
    QString m_currentConfiguration;
    // used to decide if we are building a project to decide when to emit buildStateChanged(Project *)
    QHash<Project *, int>  m_activeBuildSteps;
    Project *m_previousBuildStepProject;
    // is set to true while canceling, so that nextBuildStep knows that the BuildStep finished because of canceling
    bool m_canceling;

    // Progress reporting to the progress manager
    int m_progress;
    int m_maxProgress;
    QFutureInterface<void> *m_progressFutureInterface;
    QFutureWatcher<void> m_progressWatcher;
};

} // namespace ProjectExplorer

#endif // BUILDMANAGER_H
