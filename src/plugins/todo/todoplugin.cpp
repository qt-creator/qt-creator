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

#include "todoplugin.h"
#include <QtPlugin>
#include <QStringList>
#include <QtConcurrentRun>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <qtconcurrent/runextensions.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/itexteditor.h>
#include <coreplugin/ifile.h>
#include <extensionsystem/pluginmanager.h>
#include <QKeySequence>
#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QSettings>
#include <QMessageBox>
#include <QTextCodec>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>

TodoPlugin::TodoPlugin()
{
    qRegisterMetaTypeStreamOperators<Keyword>("Keyword");
    qRegisterMetaTypeStreamOperators<KeywordsList>("KeywordsList");
    currentProject = 0;
    inReading = false;
    readSettings();
}

TodoPlugin::~TodoPlugin()
{
// Do notning
}

void TodoPlugin::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("TODOPlugin");
    projectOptions = settings->value("project_options", 0).toInt();
    paneOptions = settings->value("pane_options", 0).toInt();
    KeywordsList defaultKeywords;
    defaultKeywords.append(Keyword("TODO", QIcon(":/warning"), QColor("#BFFFC8")));
    defaultKeywords.append(Keyword("NOTE", QIcon(":/info"), QColor("#E2DFFF")));
    defaultKeywords.append(Keyword("FIXME", QIcon(":/error"), QColor("#FFBFBF")));
    defaultKeywords.append(Keyword("BUG", QIcon(":/error"), QColor("#FFDFDF")));
    defaultKeywords.append(Keyword("HACK", QIcon(":/info"), QColor("#FFFFAA")));

    keywords = settings->value("keywords", qVariantFromValue(defaultKeywords)).value<KeywordsList>();
    settings->endGroup();
}

bool TodoPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);
    patternString = generatePatternString();
    settingsPage = new SettingsPage(keywords, projectOptions, paneOptions, this);
    if (paneOptions == 0)
    {
        outPane = new TodoOutputPane(this);
        addAutoReleasedObject(outPane);
        connect(outPane->getTodoList(), SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(gotoToRowInFile(QListWidgetItem*)));
        connect(outPane->getTodoList(), SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(gotoToRowInFile(QListWidgetItem*)));
    }
    else
    {
        ExtensionSystem::PluginManager* pluginManager = ExtensionSystem::PluginManager::instance();
        taskHub = pluginManager->getObject<ProjectExplorer::TaskHub>();
        if (!taskHub)
        {
            paneOptions = 1;
            outPane = new TodoOutputPane(this);
            addAutoReleasedObject(outPane);
            connect(outPane->getTodoList(), SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(gotoToRowInFile(QListWidgetItem*)));
            connect(outPane->getTodoList(), SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(gotoToRowInFile(QListWidgetItem*)));
            QMessageBox::warning((QWidget *)Core::ICore::instance()->mainWindow(), tr("TODO plugin"), tr("Task window object not found.\nWork in TODO Output pane."));
        }
        else
        {
            taskHub->addCategory("todoplugin", tr("TODOs comments"));
        }

    }
    addAutoReleasedObject(settingsPage);

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)), this, SLOT(currentEditorChanged(Core::IEditor*)));
    if (projectOptions != 0)
    {
        connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(currentProjectChanged(ProjectExplorer::Project*)), this, SLOT(projectChanged(ProjectExplorer::Project *)));
    }
    return true;
}

void TodoPlugin::extensionsInitialized()
{
// Do nothing
}

void TodoPlugin::shutdown()
{
// Do nothing
}

void TodoPlugin::showPane()
{
    if (paneOptions == 0)
    {
        outPane->visibilityChanged(true);
        outPane->setFocus();
    }
}

void TodoPlugin::gotoToRowInFile(QListWidgetItem *item)
{
    int row = item->data(Qt::UserRole + 2).toInt();
    QString file = item->data(Qt::UserRole + 1).toString();

    if (QFileInfo(file).exists())
    {
        Core::IEditor *editor = Core::EditorManager::instance()->openEditor(file);
        editor->gotoLine(row);
    }
}

void TodoPlugin::currentEditorChanged(Core::IEditor *editor)
{
    if (inReading)
        return;
    if (projectOptions == 0)
    {
        if (paneOptions == 0)
        {
            outPane->clearContents();
        }
        else
        {
            taskHub->clearTasks("todoplugin");
        }
    }

    if (!editor)
    {
        return;
    }
    connect(editor->file(), SIGNAL(changed()), this, SLOT(fileChanged()));
    QString fileName = editor->file()->fileName();
    if (projectOptions == 0)
        readFile(fileName);

}

void TodoPlugin::removeFromLocalTasks(QString filename)
{
    for (int i = 0; i < tasks.count(); ++i)
    {
        if (!tasks.at(i).file.compare(filename))
        {
            tasks.removeAt(i);
        }
    }
}

void TodoPlugin::addLocalTaskToTaskWindow()
{
    for (int i = 0; i < tasks.count(); ++i)
    {
        taskHub->addTask(tasks.at(i));
    }
}

void TodoPlugin::fileChanged()
{
    Core::IFile *file = (Core::IFile *)sender();
    if (file)
    {
        if (projectOptions == 0)
        {
            if (paneOptions == 0)
            {
                outPane->clearContents();
            }
            else
            {
                taskHub->clearTasks("todoplugin");
            }
        }
        else
        {
            if (paneOptions == 0)
            {
                outPane->clearContents(file->fileName());
            }
            else
            {
                taskHub->clearTasks("todoplugin");
                removeFromLocalTasks(file->fileName());
            }
        }
        readFile(file->fileName());
    }
}

Keyword TodoPlugin::prepareOutputString(QString &text)
{
    Keyword keyword;
    for(int i = 0; i < keywords.count(); ++i)
    {
        QRegExp keywordExp("//\\s*" + keywords.at(i).name + "(:|\\s)", Qt::CaseInsensitive);
        if (text.contains(keywordExp))
        {
            text = text.replace("\n", "");
            text = text.replace("\r", "");
            text = text.replace(keywordExp, keywords.at(i).name + ": ");
            text = text.trimmed();
            keyword = keywords.at(i);
            break;
        }
    }
    return keyword;
}

void TodoPlugin::readFile(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return;
    int i = 1;
    while (!file.atEnd())
    {
        QString currentLine = file.readLine();
        if (currentLine.contains(QRegExp(patternString, Qt::CaseInsensitive)))
        {
            Keyword resultKeyword = prepareOutputString(currentLine);
            QTextCodec *unicodeCodec = QTextCodec::codecForLocale();
            currentLine = unicodeCodec->toUnicode(currentLine.toAscii());
            if (paneOptions == 0)
            {
                outPane->addItem(currentLine, fileName, i, resultKeyword.icon, resultKeyword.warningColor);
                if (!inReading)
                    outPane->sort();
            }
            else
            {
                ProjectExplorer::Task task(ProjectExplorer::Task::Unknown, currentLine, fileName, i, "todoplugin");
                tasks.append(task);
            }
        }
        ++i;
    }

    if (paneOptions != 0 && !inReading)
    {
        qSort(tasks.begin(), tasks.end(), TodoPlugin::taskLessThan);
        addLocalTaskToTaskWindow();
    }
}

QString TodoPlugin::generatePatternString()
{
    QString result = "";

    if (keywords.count())
    {
        for (int i = 0; i < keywords.count() - 1; ++i)
        {
            result += "//\\s*" + keywords.at(i).name + "(:|\\s)|";
        }
        result += "//\\s*" + keywords.at(keywords.count() - 1).name + "(:|\\s)";
    }
    return result;
}

void TodoPlugin::projectChanged(ProjectExplorer::Project *project)
{
    if (!project)
        return;
    if (inReading)
        return;
    currentProject = project;
    if (paneOptions == 0)
    {
        outPane->clearContents();
    }
    else
    {
        taskHub->clearTasks("todoplugin");
    }
    inReading = true;

    QFuture<void> result = QtConcurrent::run(&TodoPlugin::readCurrentProject, this);
    Core::ICore::instance()->progressManager()->addTask(result, tr("Todoscan"), "Todo.Plugin.Scanning");

}

void TodoPlugin::readCurrentProject(QFutureInterface<void> &future, TodoPlugin *instance)
{
    QStringList filesList = instance->currentProject->files(ProjectExplorer::Project::ExcludeGeneratedFiles);
    future.setProgressRange(0, filesList.count()-1);
    for (int i = 0; i < filesList.count(); ++i)
    {
        instance->readFile(filesList.at(i));
        future.setProgressValue(i);
    }

    if (instance->paneOptions == 0)
    {
        instance->outPane->sort();
    }
    else
    {
        qSort(instance->tasks.begin(), instance->tasks.end(), TodoPlugin::taskLessThan);
        instance->addLocalTaskToTaskWindow();
    }

    instance->inReading = false;
    future.reportFinished();
}

bool TodoPlugin::taskLessThan(const ProjectExplorer::Task &t1, const ProjectExplorer::Task &t2)
{
    if (!t1.file.right(t1.file.size() - t1.file.lastIndexOf("/") - 1).compare(t2.file.right(t2.file.size() - t2.file.lastIndexOf("/") - 1)))
    {
        if (t1.line < t2.line)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return t1.file.right(t1.file.size() - t1.file.lastIndexOf("/") - 1).compare(t2.file.right(t2.file.size() - t2.file.lastIndexOf("/") - 1)) < 0;
    }
}



Q_EXPORT_PLUGIN(TodoPlugin)

 
