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

void SymbolsFindFilter::findAll(const QString &txt, FindFlags findFlags)
{
    SearchResultWindow *window = SearchResultWindow::instance();
    SearchResult *search = window->startNewSearch(label(), toolTip(findFlags), txt);
    search->setSearchAgainSupported(true);
    connect(search, &SearchResult::activated,
            this, &SymbolsFindFilter::openEditor);
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
    QSet<FilePath> projectFileNames;
    if (parameters.scope == SymbolSearcher::SearchProjectsOnly) {
        for (ProjectExplorer::Project *project : ProjectExplorer::ProjectManager::projects())
            projectFileNames += Utils::toSet(project->files(ProjectExplorer::Project::AllFiles));
    }

    const auto onSetup = [search, parameters, projectFileNames](Async<SearchResultItem> &task) {
        task.setConcurrentCallData(&SymbolSearcher::search, CppModelManager::snapshot(),
                                   parameters, projectFileNames);
        QObject::connect(&task, &AsyncBase::started, search, [taskPtr = &task, search] {
            FutureProgress *progress = ProgressManager::addTask(taskPtr->future(),
                                                                Tr::tr("Searching for Symbol"),
                                                                Core::Constants::TASK_SEARCH);
            QObject::connect(progress, &FutureProgress::clicked, search, &SearchResult::popup);
            QObject::connect(search, &SearchResult::paused, taskPtr, [taskPtr](bool paused) {
                auto future = taskPtr->future();
                if (!paused || future.isRunning()) // guard against pausing when the search is finished
                    future.setSuspended(paused);
            });
            QObject::connect(search, &SearchResult::canceled, taskPtr, [taskPtr] {
                taskPtr->future().cancel();
            });
        });
        QObject::connect(&task, &AsyncBase::resultsReadyAt, search,
                         [taskPtr = &task, search](int begin, int end) {
            SearchResultItems items;
            for (int i = begin; i < end; ++i)
                items << taskPtr->resultAt(i);
            search->addResults(items, SearchResult::AddSortedByContent);
        });
    };
    const auto onDone = [search](const Async<SearchResultItem> &task) {
        search->finishSearch(task.future().isCanceled());
    };
    m_taskTreeRunner.start({AsyncTask<SearchResultItem>(onSetup, onDone)});
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

Store SymbolsFindFilter::save() const
{
    Store s;
    if (m_symbolsToSearch != SearchSymbols::AllTypes)
        s.insert(SETTINGS_SYMBOLTYPES, int(m_symbolsToSearch));
    if (m_scope != SymbolSearcher::SearchProjectsOnly)
        s.insert(SETTINGS_SEARCHSCOPE, int(m_scope));
    return s;
}

void SymbolsFindFilter::restore(const Utils::Store &s)
{
    m_symbolsToSearch = static_cast<SearchSymbols::SymbolTypes>(
        s.value(SETTINGS_SYMBOLTYPES, int(SearchSymbols::AllTypes)).toInt());
    m_scope = static_cast<SearchScope>(
        s.value(SETTINGS_SEARCHSCOPE, int(SymbolSearcher::SearchProjectsOnly)).toInt());
    emit symbolsToSearchChanged();
}

QByteArray SymbolsFindFilter::settingsKey() const
{
    return SETTINGS_GROUP;
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
        .arg(
            searchScope() == SymbolSearcher::SearchGlobal ? Tr::tr("All", "Symbol search scope")
                                                          : Tr::tr("Projects"),
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
