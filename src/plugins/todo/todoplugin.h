/*
 *
 *  TODO plugin - Add pane with list all TODO, FIXME, etc. comments.
 *
 *  Copyright (C) 2010  VasiliySorokin
 *
 *  Authors: Vasiliy Sorokin <sorokin.vasiliy@gmail.com>
 *
 *  This file is part of TODO plugin for QtCreator.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * * Neither the name of the vsorokin nor the names of its contributors may be used to endorse or
 * promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#ifndef TODOPLUGIN_H
#define TODOPLUGIN_H
#include <QFuture>
#include <QList>
#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include "todooutputpane.h"
#include "settingspage.h"
#include "keyword.h"


class TodoPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
public:
    TodoPlugin();
    ~TodoPlugin();
    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString * errorString);
    void shutdown();

public slots:
    void showPane();
    void gotoToRowInFile(QListWidgetItem *);
    void currentEditorChanged(Core::IEditor *editor);
    void fileChanged();
    void projectChanged(ProjectExplorer::Project *);

signals:
    void updateFutureValue(int value);
    void setFutureRange(int, int);

private:
    void readFile(QString);
    static void readCurrentProject(QFutureInterface<void> &future, TodoPlugin* instance);
    void removeFromLocalTasks(QString filename);
    void addLocalTaskToTaskWindow();

    static bool taskLessThan(const ProjectExplorer::Task &t1, const ProjectExplorer::Task &t2);

    Keyword prepareOutputString(QString &text);
    QString generatePatternString();
    void readSettings();
    TodoOutputPane *outPane;
    ProjectExplorer::TaskHub *taskHub;
    SettingsPage *settingsPage;
    QString patternString;
    KeywordsList keywords;
    int projectOptions;
    int paneOptions;
    ProjectExplorer::Project *currentProject;
    QList<ProjectExplorer::Task> tasks;
    bool inReading;
    QFutureInterface<void> *progressObject;
};
#endif // TODOPLUGIN_H
 
