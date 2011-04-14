/***************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** This file is part of the documentation of Qt Creator.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "headerfilterprogress.h"
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/iprojectmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetexteditor.h>
#include <find/searchresultwindow.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <find/textfindconstants.h>
#include <utils/stylehelper.h>
#include <utils/filesearch.h>
#include <coreplugin/icore.h>
#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QFutureWatcher>
#include <QtGui/QStringListModel>
#include <QtGui/QLabel>
#include <QtGui/QFont>
#include <QtGui/QMessageBox>
#include <QtGui/QGridLayout>

using namespace Core;
using namespace Utils;
using namespace TextEditor;


struct HeaderFilterProgressData
 {
    HeaderFilterProgressData() : projectPlugin(0), m_searchResultWindow(0){}
    QFutureWatcher<FileSearchResult> watcher;
    QLabel* resultLabel;


    ProjectExplorer::ProjectExplorerPlugin* projectExplorer()
     {
         if(projectPlugin)
             return projectPlugin;

         ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();
         projectPlugin = pm->getObject<ProjectExplorer::ProjectExplorerPlugin>();
         return projectPlugin;
     }

    // Method to search and return the search window

    Find::SearchResultWindow* searchResultWindow()
    {
        if(m_searchResultWindow)
            return m_searchResultWindow;

        ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();
        m_searchResultWindow = pm->getObject<Find::SearchResultWindow>();
        return m_searchResultWindow;
    }


 private:
     ProjectExplorer::ProjectExplorerPlugin* projectPlugin;
     Find::SearchResultWindow* m_searchResultWindow;

};

HeaderFilterProgress::HeaderFilterProgress()
{
    d = new HeaderFilterProgressData;
    d->watcher.setPendingResultsLimit(1);
    d->resultLabel = 0 ;


    // displayResult(int) is called when every a new
    // search result is generated
    connect(&d->watcher, SIGNAL(resultReadyAt(int)),this, SLOT(displayResult(int)));
}

HeaderFilterProgress::~HeaderFilterProgress()
{
    delete d;
}

QString HeaderFilterProgress::id() const
{
    return "HeaderFilter";
}

QString HeaderFilterProgress::name() const
{
    return tr("Header Filter");
}

bool HeaderFilterProgress::isEnabled() const
{
    QList<ProjectExplorer::Project*> projects = d->projectExplorer()->session()->projects();
    if(projects.count())
        return true;

    return false;
}

QKeySequence HeaderFilterProgress::defaultShortcut() const
{
    return QKeySequence();
}



void HeaderFilterProgress::findAll(const QString &text,QTextDocument::FindFlags findFlags)
 {

    // Fetch a list of all open projects
    QList<ProjectExplorer::Project*> projects = d->projectExplorer()->session()->projects();

    // Make a list of files in each project
    QStringList files;
    Q_FOREACH(ProjectExplorer::Project* project, projects)
            files += project->files(ProjectExplorer::Project::AllFiles);

    // Remove duplicates
    files.removeDuplicates();

    //------------------------------------------------------------
    // Begin searching
    QString includeline = "#include <" + text + '>';
    Find::SearchResult* result = d->searchResultWindow()->startNewSearch();

    d->watcher.setFuture(QFuture<FileSearchResult>());

   //When result gets activated it invokes the openEditor function
    connect(result, SIGNAL(activated(Find::SearchResultItem)),
            this, SLOT(openEditor(Find::SearchResultItem)));

    d->searchResultWindow()->popup(true);

    // Let the watcher monitor the search results
    d->watcher.setFuture(Utils::findInFiles(includeline, files, findFlags));

    //Creates the Progres bar
    Core::FutureProgress* progress =
            Core::ICore::instance()->progressManager()->addTask(d->watcher.future(),
                                                                            "MySearch",
                                                                            Find::Constants::TASK_SEARCH,
                                                                            Core::ProgressManager::KeepOnFinish
                                                                            );
    progress->setWidget(createProgressWidget());
    connect(progress, SIGNAL(clicked()), d->searchResultWindow(), SLOT(popup()));
}


QWidget *HeaderFilterProgress::createProgressWidget()
{
    d->resultLabel = new QLabel;
    d->resultLabel->setAlignment(Qt::AlignCenter);
    QFont f = d->resultLabel->font();
    f.setBold(true);
    f.setPointSizeF(StyleHelper::sidebarFontSize());
    d->resultLabel->setFont(f);
    d->resultLabel->setPalette(StyleHelper::sidebarFontPalette(d->resultLabel->palette()));
    d->resultLabel->setText(tr("%1 found").arg(d->searchResultWindow()->numberOfResults()));
    return d->resultLabel;
}

QWidget *HeaderFilterProgress::createConfigWidget()
{
    return (new QLabel("This is a header filter"));
}

void HeaderFilterProgress::displayResult(int index)
{
    FileSearchResult result = d->watcher.future().resultAt(index);

    d->searchResultWindow()->addResult(result.fileName,
                                       result.lineNumber,
                                       result.matchingLine,
                                       result.matchStart,
                                       result.matchLength);
    if (d->resultLabel)
        d->resultLabel->setText(tr("%1 found").arg(d->searchResultWindow()->numberOfResults()));
}



void HeaderFilterProgress::openEditor(const Find::SearchResultItem &item)
{
    TextEditor::BaseTextEditor::openEditorAt(item.fileName, item.lineNumber, item.index);
}


