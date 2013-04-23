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

#include "symbolsfindfilter.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <utils/runextensions.h>
#include <utils/qtcassert.h>

#include <QSet>
#include <QGridLayout>
#include <QLabel>
#include <QButtonGroup>

using namespace CppTools;
using namespace CppTools::Internal;

namespace {
    const char * const SETTINGS_GROUP = "CppSymbols";
    const char * const SETTINGS_SYMBOLTYPES = "SymbolsToSearchFor";
    const char * const SETTINGS_SEARCHSCOPE = "SearchScope";
} // anonymous namespace

SymbolsFindFilter::SymbolsFindFilter(CppModelManager *manager)
    : m_manager(manager),
      m_enabled(true),
      m_symbolsToSearch(SearchSymbols::AllTypes),
      m_scope(SymbolSearcher::SearchProjectsOnly)
{
    // for disabling while parser is running
    connect(Core::ICore::progressManager(), SIGNAL(taskStarted(QString)),
            this, SLOT(onTaskStarted(QString)));
    connect(Core::ICore::progressManager(), SIGNAL(allTasksFinished(QString)),
            this, SLOT(onAllTasksFinished(QString)));
}

QString SymbolsFindFilter::id() const
{
    return QLatin1String("CppSymbols");
}

QString SymbolsFindFilter::displayName() const
{
    return tr("C++ Symbols");
}

bool SymbolsFindFilter::isEnabled() const
{
    return m_enabled;
}

void SymbolsFindFilter::cancel()
{
    Find::SearchResult *search = qobject_cast<Find::SearchResult *>(sender());
    QTC_ASSERT(search, return);
    QFutureWatcher<Find::SearchResultItem> *watcher = m_watchers.key(search);
    QTC_ASSERT(watcher, return);
    watcher->cancel();
}

void SymbolsFindFilter::setPaused(bool paused)
{
    Find::SearchResult *search = qobject_cast<Find::SearchResult *>(sender());
    QTC_ASSERT(search, return);
    QFutureWatcher<Find::SearchResultItem> *watcher = m_watchers.key(search);
    QTC_ASSERT(watcher, return);
    if (!paused || watcher->isRunning()) // guard against pausing when the search is finished
        watcher->setPaused(paused);
}

Find::FindFlags SymbolsFindFilter::supportedFindFlags() const
{
    return Find::FindCaseSensitively | Find::FindRegularExpression | Find::FindWholeWords;
}

void SymbolsFindFilter::findAll(const QString &txt, Find::FindFlags findFlags)
{
    Find::SearchResultWindow *window = Find::SearchResultWindow::instance();
    Find::SearchResult *search = window->startNewSearch(label(), toolTip(findFlags), txt);
    search->setSearchAgainSupported(true);
    connect(search, SIGNAL(activated(Find::SearchResultItem)),
            this, SLOT(openEditor(Find::SearchResultItem)));
    connect(search, SIGNAL(cancelled()), this, SLOT(cancel()));
    connect(search, SIGNAL(paused(bool)), this, SLOT(setPaused(bool)));
    connect(search, SIGNAL(searchAgainRequested()), this, SLOT(searchAgain()));
    connect(this, SIGNAL(enabledChanged(bool)), search, SLOT(setSearchAgainEnabled(bool)));
    window->popup(Core::IOutputPane::ModeSwitch | Core::IOutputPane::WithFocus);

    SymbolSearcher::Parameters parameters;
    parameters.text = txt;
    parameters.flags = findFlags;
    parameters.types = m_symbolsToSearch;
    parameters.scope = m_scope;
    search->setUserData(qVariantFromValue(parameters));
    startSearch(search);
}

void SymbolsFindFilter::startSearch(Find::SearchResult *search)
{
    SymbolSearcher::Parameters parameters = search->userData().value<SymbolSearcher::Parameters>();
    QSet<QString> projectFileNames;
    if (parameters.scope == SymbolSearcher::SearchProjectsOnly) {
        foreach (ProjectExplorer::Project *project,
                 ProjectExplorer::ProjectExplorerPlugin::instance()->session()->projects()) {
            projectFileNames += project->files(ProjectExplorer::Project::AllFiles).toSet();
        }
    }

    QFutureWatcher<Find::SearchResultItem> *watcher = new QFutureWatcher<Find::SearchResultItem>();
    m_watchers.insert(watcher, search);
    connect(watcher, SIGNAL(finished()),
            this, SLOT(finish()));
    connect(watcher, SIGNAL(resultsReadyAt(int,int)),
            this, SLOT(addResults(int,int)));
    SymbolSearcher *symbolSearcher = m_manager->indexingSupport()->createSymbolSearcher(parameters, projectFileNames);
    connect(watcher, SIGNAL(finished()),
            symbolSearcher, SLOT(deleteLater()));
    watcher->setFuture(QtConcurrent::run(&SymbolSearcher::runSearch, symbolSearcher));
    Core::FutureProgress *progress = Core::ICore::progressManager()->addTask(watcher->future(),
                                                        tr("Searching"),
                                                        QLatin1String(Find::Constants::TASK_SEARCH));
    connect(progress, SIGNAL(clicked()), search, SLOT(popup()));
}

void SymbolsFindFilter::addResults(int begin, int end)
{
    QFutureWatcher<Find::SearchResultItem> *watcher =
            static_cast<QFutureWatcher<Find::SearchResultItem> *>(sender());
    Find::SearchResult *search = m_watchers.value(watcher);
    if (!search) {
        // search was removed from search history while the search is running
        watcher->cancel();
        return;
    }
    QList<Find::SearchResultItem> items;
    for (int i = begin; i < end; ++i)
        items << watcher->resultAt(i);
    search->addResults(items, Find::SearchResult::AddSorted);
}

void SymbolsFindFilter::finish()
{
    QFutureWatcher<Find::SearchResultItem> *watcher =
            static_cast<QFutureWatcher<Find::SearchResultItem> *>(sender());
    Find::SearchResult *search = m_watchers.value(watcher);
    if (search)
        search->finishSearch(watcher->isCanceled());
    m_watchers.remove(watcher);
    watcher->deleteLater();
}

void SymbolsFindFilter::openEditor(const Find::SearchResultItem &item)
{
    if (!item.userData.canConvert<ModelItemInfo>())
        return;
    ModelItemInfo info = item.userData.value<ModelItemInfo>();
    TextEditor::BaseTextEditorWidget::openEditorAt(info.fileName,
                                             info.line,
                                             info.column);
}

QWidget *SymbolsFindFilter::createConfigWidget()
{
    return new SymbolsFindFilterConfigWidget(this);
}

void SymbolsFindFilter::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(SETTINGS_GROUP));
    settings->setValue(QLatin1String(SETTINGS_SYMBOLTYPES), (int)m_symbolsToSearch);
    settings->setValue(QLatin1String(SETTINGS_SEARCHSCOPE), (int)m_scope);
    settings->endGroup();
}

void SymbolsFindFilter::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(SETTINGS_GROUP));
    m_symbolsToSearch = (SearchSymbols::SymbolTypes)settings->value(QLatin1String(SETTINGS_SYMBOLTYPES),
                                        (int)SearchSymbols::AllTypes).toInt();
    m_scope = (SearchScope)settings->value(QLatin1String(SETTINGS_SEARCHSCOPE),
                                           (int)SymbolSearcher::SearchProjectsOnly).toInt();
    settings->endGroup();
    emit symbolsToSearchChanged();
}

void SymbolsFindFilter::onTaskStarted(const QString &type)
{
    if (type == QLatin1String(CppTools::Constants::TASK_INDEX)) {
        m_enabled = false;
        emit enabledChanged(m_enabled);
    }
}

void SymbolsFindFilter::onAllTasksFinished(const QString &type)
{
    if (type == QLatin1String(CppTools::Constants::TASK_INDEX)) {
        m_enabled = true;
        emit enabledChanged(m_enabled);
    }
}

void SymbolsFindFilter::searchAgain()
{
    Find::SearchResult *search = qobject_cast<Find::SearchResult *>(sender());
    QTC_ASSERT(search, return);
    search->restart();
    startSearch(search);
}

QString SymbolsFindFilter::label() const
{
    return tr("C++ Symbols:");
}

QString SymbolsFindFilter::toolTip(Find::FindFlags findFlags) const
{
    QStringList types;
    if (m_symbolsToSearch & SymbolSearcher::Classes)
        types.append(tr("Classes"));
    if (m_symbolsToSearch & SymbolSearcher::Functions)
        types.append(tr("Methods"));
    if (m_symbolsToSearch & SymbolSearcher::Enums)
        types.append(tr("Enums"));
    if (m_symbolsToSearch & SymbolSearcher::Declarations)
        types.append(tr("Declarations"));
    return tr("Scope: %1\nTypes: %2\nFlags: %3")
            .arg(searchScope() == SymbolSearcher::SearchGlobal ? tr("All") : tr("Projects"))
            .arg(types.join(tr(", ")))
            .arg(Find::IFindFilter::descriptionForFindFlags(findFlags));
}

// #pragma mark -- SymbolsFindFilterConfigWidget

SymbolsFindFilterConfigWidget::SymbolsFindFilterConfigWidget(SymbolsFindFilter *filter)
    : m_filter(filter)
{
    connect(m_filter, SIGNAL(symbolsToSearchChanged()), this, SLOT(getState()));

    QGridLayout *layout = new QGridLayout(this);
    setLayout(layout);
    layout->setMargin(0);

    QLabel *typeLabel = new QLabel(tr("Types:"));
    layout->addWidget(typeLabel, 0, 0);

    m_typeClasses = new QCheckBox(tr("Classes"));
    layout->addWidget(m_typeClasses, 0, 1);

    m_typeMethods = new QCheckBox(tr("Methods"));
    layout->addWidget(m_typeMethods, 0, 2);

    m_typeEnums = new QCheckBox(tr("Enums"));
    layout->addWidget(m_typeEnums, 1, 1);

    m_typeDeclarations = new QCheckBox(tr("Declarations"));
    layout->addWidget(m_typeDeclarations, 1, 2);

    // hacks to fix layouting:
    typeLabel->setMinimumWidth(80);
    typeLabel->setAlignment(Qt::AlignRight);
    m_typeClasses->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_typeMethods->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(m_typeClasses, SIGNAL(clicked(bool)), this, SLOT(setState()));
    connect(m_typeMethods, SIGNAL(clicked(bool)), this, SLOT(setState()));
    connect(m_typeEnums, SIGNAL(clicked(bool)), this, SLOT(setState()));
    connect(m_typeDeclarations, SIGNAL(clicked(bool)), this, SLOT(setState()));

    m_searchProjectsOnly = new QRadioButton(tr("Projects only"));
    layout->addWidget(m_searchProjectsOnly, 2, 1);

    m_searchGlobal = new QRadioButton(tr("All files"));
    layout->addWidget(m_searchGlobal, 2, 2);

    m_searchGroup = new QButtonGroup(this);
    m_searchGroup->addButton(m_searchProjectsOnly);
    m_searchGroup->addButton(m_searchGlobal);

    connect(m_searchProjectsOnly, SIGNAL(clicked(bool)),
            this, SLOT(setState()));
    connect(m_searchGlobal, SIGNAL(clicked(bool)),
            this, SLOT(setState()));
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
