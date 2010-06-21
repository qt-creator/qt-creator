#include "headerfilter.h"
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/filesearch.h>
#include<QFutureWatcher>
#include<QLabel>
#include <find/searchresultwindow.h>
#include <texteditor/basetexteditor.h>



using namespace Core;
using namespace Utils;


struct HeaderFilterData
 {
    HeaderFilterData() : m_projectPlugin(0), m_searchResultWindow(0){}
    QFutureWatcher<FileSearchResult> watcher;

    ProjectExplorer::ProjectExplorerPlugin* projectExplorer()
     {
         if(m_projectPlugin)
             return m_projectPlugin;

         ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();
         m_projectPlugin = pm->getObject<ProjectExplorer::ProjectExplorerPlugin>();
         return m_projectPlugin;
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
     ProjectExplorer::ProjectExplorerPlugin* m_projectPlugin;
     Find::SearchResultWindow *m_searchResultWindow;

};

HeaderFilter::HeaderFilter()
{
    d = new HeaderFilterData;
    d->watcher.setPendingResultsLimit(1);

    // displayResult(int) is called when every a new
    // search result is generated
    connect(&d->watcher, SIGNAL(resultReadyAt(int)),this, SLOT(displayResult(int)));
}

HeaderFilter::~HeaderFilter()
{
    delete d;
}

QString HeaderFilter::id() const
{
    return "HeaderFilter";
}

QString HeaderFilter::name() const
{
    return tr("Header Filter");
}

bool HeaderFilter::isEnabled() const
{
    QList<ProjectExplorer::Project*> projects = d->projectExplorer()->session()->projects();
    if(projects.count())
        return true;

    return false;
}

QKeySequence HeaderFilter::defaultShortcut() const
{
    return QKeySequence();
}

QWidget *HeaderFilter::createConfigWidget()
{
    return (new QLabel("This is a header filter"));
}


void HeaderFilter::findAll(const QString &text,QTextDocument::FindFlags findFlags)
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
    QString includeline = "#include <" + text + ">";
    Find::SearchResult *result = d->searchResultWindow()->startNewSearch();

    d->watcher.setFuture(QFuture<FileSearchResult>());

   //When result gets activated it invokes the openEditor function
    connect(result, SIGNAL(activated(Find::SearchResultItem)),
            this, SLOT(openEditor(Find::SearchResultItem)));

    d->searchResultWindow()->popup(true);

    // Let the watcher monitor the search results
    d->watcher.setFuture(Utils::findInFiles(includeline, files, findFlags));
}

void HeaderFilter::displayResult(int index)
{
    FileSearchResult result = d->watcher.future().resultAt(index);

    d->searchResultWindow()->addResult(result.fileName,
                                       result.lineNumber,
                                       result.matchingLine,
                                       result.matchStart,
                                       result.matchLength);
}

void HeaderFilter::openEditor(const Find::SearchResultItem &item)
{
    TextEditor::BaseTextEditor::openEditorAt(item.fileName, item.lineNumber, item.index);
}

