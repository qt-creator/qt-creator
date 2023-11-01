// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "symbolsfindfilter.h"

#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppmodelmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>
#include <coreplugin/find/textfindconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/qtcassert.h>

#include <QButtonGroup>
#include <QGridLayout>
#include <QLabel>
#include <QSet>

using namespace Core;
using namespace Utils;

namespace CppEditor::Internal {

const char SETTINGS_GROUP[] = "CppSymbols";
const char SETTINGS_SYMBOLTYPES[] = "SymbolsToSearchFor";
const char SETTINGS_SEARCHSCOPE[] = "SearchScope";

SymbolsFindFilter::SymbolsFindFilter()
    : m_enabled(true),
      m_symbolsToSearch(SearchSymbols::AllTypes),
      m_scope(SymbolSearcher::SearchProjectsOnly)
{
    // for disabling while parser is running
    connect(ProgressManager::instance(), &ProgressManager::taskStarted,
            this, &SymbolsFindFilter::onTaskStarted);
    connect(ProgressManager::instance(), &ProgressManager::allTasksFinished,
            this, &SymbolsFindFilter::onAllTasksFinished);
}

QString SymbolsFindFilter::id() const
{
    return QLatin1String(Constants::SYMBOLS_FIND_FILTER_ID);
}

QString SymbolsFindFilter::displayName() const
{
    return Tr::tr(Constants::SYMBOLS_FIND_FILTER_DISPLAY_NAME);
}

bool SymbolsFindFilter::isEnabled() const
{
    return m_enabled;
}

void SymbolsFindFilter::cancel(SearchResult *search)
{
    QFutureWatcher<SearchResultItem> *watcher = m_watchers.key(search);
    QTC_ASSERT(watcher, return);
    watcher->cancel();
}

void SymbolsFindFilter::setPaused(SearchResult *search, bool paused)
{
    QFutureWatcher<SearchResultItem> *watcher = m_watchers.key(search);
    QTC_ASSERT(watcher, return);
    if (!paused || watcher->isRunning()) // guard against pausing when the search is finished
        watcher->setPaused(paused);
}

void SymbolsFindFilter::findAll(const QString &txt, FindFlags findFlags)
{
    SearchResultWindow *window = SearchResultWindow::instance();
    SearchResult *search = window->startNewSearch(label(), toolTip(findFlags), txt);
    search->setSearchAgainSupported(true);
    connect(search, &SearchResult::activated,
            this, &SymbolsFindFilter::openEditor);
    connect(search, &SearchResult::canceled, this, [this, search] { cancel(search); });
    connect(search, &SearchResult::paused,
            this, [this, search](bool paused) { setPaused(search, paused); });
    connect(search, &SearchResult::searchAgainRequested, this, [this, search] {
        search->restart();
        startSearch(search);
    });
    connect(this, &IFindFilter::enabledChanged, search, &SearchResult::setSearchAgainEnabled);
    window->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);

    SymbolSearcher::Parameters parameters;
    parameters.text = txt;
    parameters.flags = findFlags;
    parameters.types = m_symbolsToSearch;
    parameters.scope = m_scope;
    search->setUserData(QVariant::fromValue(parameters));
    startSearch(search);
}

void SymbolsFindFilter::startSearch(SearchResult *search)
{
    SymbolSearcher::Parameters parameters = search->userData().value<SymbolSearcher::Parameters>();
    QSet<QString> projectFileNames;
    if (parameters.scope == SymbolSearcher::SearchProjectsOnly) {
        for (ProjectExplorer::Project *project : ProjectExplorer::ProjectManager::projects())
            projectFileNames += Utils::transform<QSet>(project->files(ProjectExplorer::Project::AllFiles), &Utils::FilePath::toString);
    }

    auto watcher = new QFutureWatcher<SearchResultItem>;
    m_watchers.insert(watcher, search);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] { finish(watcher); });
    connect(watcher, &QFutureWatcherBase::resultsReadyAt, this, [this, watcher]
            (int begin, int end) { addResults(watcher, begin, end); });
    SymbolSearcher *symbolSearcher = new SymbolSearcher(parameters, projectFileNames);
    connect(watcher, &QFutureWatcherBase::finished,
            symbolSearcher, &QObject::deleteLater);
    watcher->setFuture(Utils::asyncRun(CppModelManager::sharedThreadPool(),
                                       &SymbolSearcher::runSearch, symbolSearcher));
    FutureProgress *progress = ProgressManager::addTask(watcher->future(), Tr::tr("Searching for Symbol"),
                                                        Core::Constants::TASK_SEARCH);
    connect(progress, &FutureProgress::clicked, search, &SearchResult::popup);
}

void SymbolsFindFilter::addResults(QFutureWatcher<SearchResultItem> *watcher, int begin, int end)
{
    SearchResult *search = m_watchers.value(watcher);
    if (!search) {
        // search was removed from search history while the search is running
        watcher->cancel();
        return;
    }
    SearchResultItems items;
    for (int i = begin; i < end; ++i)
        items << watcher->resultAt(i);
    search->addResults(items, SearchResult::AddSortedByContent);
}

void SymbolsFindFilter::finish(QFutureWatcher<SearchResultItem> *watcher)
{
    SearchResult *search = m_watchers.value(watcher);
    if (search)
        search->finishSearch(watcher->isCanceled());
    m_watchers.remove(watcher);
    watcher->deleteLater();
}

void SymbolsFindFilter::openEditor(const SearchResultItem &item)
{
    if (!item.userData().canConvert<IndexItem::Ptr>())
        return;
    IndexItem::Ptr info = item.userData().value<IndexItem::Ptr>();
    EditorManager::openEditorAt({info->filePath(), info->line(), info->column()},
                                {},
                                Core::EditorManager::AllowExternalEditor);
}

QWidget *SymbolsFindFilter::createConfigWidget()
{
    return new SymbolsFindFilterConfigWidget(this);
}

void SymbolsFindFilter::writeSettings(QtcSettings *settings)
{
    settings->beginGroup(SETTINGS_GROUP);
    settings->setValue(SETTINGS_SYMBOLTYPES, int(m_symbolsToSearch));
    settings->setValue(SETTINGS_SEARCHSCOPE, int(m_scope));
    settings->endGroup();
}

void SymbolsFindFilter::readSettings(QtcSettings *settings)
{
    settings->beginGroup(SETTINGS_GROUP);
    m_symbolsToSearch = static_cast<SearchSymbols::SymbolTypes>(
                settings->value(SETTINGS_SYMBOLTYPES, int(SearchSymbols::AllTypes)).toInt());
    m_scope = static_cast<SearchScope>(
                settings->value(SETTINGS_SEARCHSCOPE,
                                int(SymbolSearcher::SearchProjectsOnly)).toInt());
    settings->endGroup();
    emit symbolsToSearchChanged();
}

void SymbolsFindFilter::onTaskStarted(Id type)
{
    if (type == Constants::TASK_INDEX) {
        m_enabled = false;
        emit enabledChanged(m_enabled);
    }
}

void SymbolsFindFilter::onAllTasksFinished(Id type)
{
    if (type == Constants::TASK_INDEX) {
        m_enabled = true;
        emit enabledChanged(m_enabled);
    }
}

QString SymbolsFindFilter::label() const
{
    return Tr::tr("C++ Symbols:");
}

QString SymbolsFindFilter::toolTip(FindFlags findFlags) const
{
    QStringList types;
    if (m_symbolsToSearch & SymbolSearcher::Classes)
        types.append(Tr::tr("Classes"));
    if (m_symbolsToSearch & SymbolSearcher::Functions)
        types.append(Tr::tr("Functions"));
    if (m_symbolsToSearch & SymbolSearcher::Enums)
        types.append(Tr::tr("Enums"));
    if (m_symbolsToSearch & SymbolSearcher::Declarations)
        types.append(Tr::tr("Declarations"));
    return Tr::tr("Scope: %1\nTypes: %2\nFlags: %3")
        .arg(searchScope() == SymbolSearcher::SearchGlobal ? Tr::tr("All") : Tr::tr("Projects"),
             types.join(", "),
             IFindFilter::descriptionForFindFlags(findFlags));
}

// #pragma mark -- SymbolsFindFilterConfigWidget

SymbolsFindFilterConfigWidget::SymbolsFindFilterConfigWidget(SymbolsFindFilter *filter)
    : m_filter(filter)
{
    connect(m_filter, &SymbolsFindFilter::symbolsToSearchChanged,
            this, &SymbolsFindFilterConfigWidget::getState);

    auto layout = new QGridLayout(this);
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);

    auto typeLabel = new QLabel(Tr::tr("Types:"));
    layout->addWidget(typeLabel, 0, 0);

    m_typeClasses = new QCheckBox(Tr::tr("Classes"));
    layout->addWidget(m_typeClasses, 0, 1);

    m_typeMethods = new QCheckBox(Tr::tr("Functions"));
    layout->addWidget(m_typeMethods, 0, 2);

    m_typeEnums = new QCheckBox(Tr::tr("Enums"));
    layout->addWidget(m_typeEnums, 1, 1);

    m_typeDeclarations = new QCheckBox(Tr::tr("Declarations"));
    layout->addWidget(m_typeDeclarations, 1, 2);

    // hacks to fix layouting:
    typeLabel->setMinimumWidth(80);
    typeLabel->setAlignment(Qt::AlignRight);
    m_typeClasses->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_typeMethods->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(m_typeClasses, &QAbstractButton::clicked,
            this, &SymbolsFindFilterConfigWidget::setState);
    connect(m_typeMethods, &QAbstractButton::clicked,
            this, &SymbolsFindFilterConfigWidget::setState);
    connect(m_typeEnums, &QAbstractButton::clicked,
            this, &SymbolsFindFilterConfigWidget::setState);
    connect(m_typeDeclarations, &QAbstractButton::clicked,
            this, &SymbolsFindFilterConfigWidget::setState);

    m_searchProjectsOnly = new QRadioButton(Tr::tr("Projects only"));
    layout->addWidget(m_searchProjectsOnly, 2, 1);

    m_searchGlobal = new QRadioButton(Tr::tr("All files"));
    layout->addWidget(m_searchGlobal, 2, 2);

    m_searchGroup = new QButtonGroup(this);
    m_searchGroup->addButton(m_searchProjectsOnly);
    m_searchGroup->addButton(m_searchGlobal);

    connect(m_searchProjectsOnly, &QAbstractButton::clicked,
            this, &SymbolsFindFilterConfigWidget::setState);
    connect(m_searchGlobal, &QAbstractButton::clicked,
            this, &SymbolsFindFilterConfigWidget::setState);
}

void SymbolsFindFilterConfigWidget::getState()
{
    SearchSymbols::SymbolTypes symbols = m_filter->symbolsToSearch();
    m_typeClasses->setChecked(symbols & SymbolSearcher::Classes);
    m_typeMethods->setChecked(symbols & SymbolSearcher::Functions);
    m_typeEnums->setChecked(symbols & SymbolSearcher::Enums);
    m_typeDeclarations->setChecked(symbols & SymbolSearcher::Declarations);

    SymbolsFindFilter::SearchScope scope = m_filter->searchScope();
    m_searchProjectsOnly->setChecked(scope == SymbolSearcher::SearchProjectsOnly);
    m_searchGlobal->setChecked(scope == SymbolSearcher::SearchGlobal);
}

void SymbolsFindFilterConfigWidget::setState() const
{
    SearchSymbols::SymbolTypes symbols;
    if (m_typeClasses->isChecked())
        symbols |= SymbolSearcher::Classes;
    if (m_typeMethods->isChecked())
        symbols |= SymbolSearcher::Functions;
    if (m_typeEnums->isChecked())
        symbols |= SymbolSearcher::Enums;
    if (m_typeDeclarations->isChecked())
        symbols |= SymbolSearcher::Declarations;
    m_filter->setSymbolsToSearch(symbols);

    if (m_searchProjectsOnly->isChecked())
        m_filter->setSearchScope(SymbolSearcher::SearchProjectsOnly);
    else
        m_filter->setSearchScope(SymbolSearcher::SearchGlobal);
}

} // namespace CppEditor::Internal
