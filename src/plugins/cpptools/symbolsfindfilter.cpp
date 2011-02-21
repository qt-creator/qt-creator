/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "symbolsfindfilter.h"

#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/icore.h>
#include <find/textfindconstants.h>
#include <qtconcurrent/runextensions.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>

#include <QtCore/QSet>
#include <QtCore/QRegExp>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QButtonGroup>

using namespace CppTools;
using namespace CppTools::Internal;

namespace {
    const char * const SETTINGS_GROUP = "CppSymbols";
    const char * const SETTINGS_SYMBOLTYPES = "SymbolsToSearchFor";
    const char * const SETTINGS_SEARCHSCOPE = "SearchScope";

    void runSearch(QFutureInterface<Find::SearchResultItem> &future,
              QString txt, Find::FindFlags findFlags, CPlusPlus::Snapshot snapshot,
              SearchSymbols *search, QSet<QString> fileNames)
    {
        future.setProgressRange(0, snapshot.size());
        future.setProgressValue(0);
        int progress = 0;

        CPlusPlus::Snapshot::const_iterator it = snapshot.begin();

        QString findString = (findFlags & Find::FindRegularExpression ? txt : QRegExp::escape(txt));
        if (findFlags & Find::FindWholeWords)
            findString = QString::fromLatin1("\\b%1\\b").arg(findString);
        QRegExp matcher(findString, (findFlags & Find::FindCaseSensitively ? Qt::CaseSensitive : Qt::CaseInsensitive));
        while (it != snapshot.end() && !future.isCanceled()) {
            if (fileNames.isEmpty() || fileNames.contains(it.value()->fileName())) {
                QVector<Find::SearchResultItem> resultItems;
                QList<ModelItemInfo> modelInfos = (*search)(it.value());
                foreach (const ModelItemInfo &info, modelInfos) {
                    int index = matcher.indexIn(info.symbolName);
                    if (index != -1) {
                        QStringList path = info.fullyQualifiedName.mid(0, info.fullyQualifiedName.size() - 1);
                        Find::SearchResultItem item;
                        item.path = path;
                        item.text = info.symbolName;
                        item.textMarkPos = -1;
                        item.textMarkLength = 0;
                        item.icon = info.icon;
                        item.lineNumber = -1;
                        item.userData = qVariantFromValue(info);
                        resultItems << item;
                    }
                }
                if (!resultItems.isEmpty())
                    future.reportResults(resultItems);
            }
            ++it;
            ++progress;
            future.setProgressValue(progress);
        }
    }
} //namespace

SymbolsFindFilter::SymbolsFindFilter(CppModelManager *manager)
    : m_manager(manager),
    m_isRunning(false),
    m_enabled(true),
    m_symbolsToSearch(SearchSymbols::AllTypes),
    m_scope(SearchProjectsOnly)
{
    // for disabling while parser is running
    connect(Core::ICore::instance()->progressManager(), SIGNAL(taskStarted(QString)),
            this, SLOT(onTaskStarted(QString)));
    connect(Core::ICore::instance()->progressManager(), SIGNAL(allTasksFinished(QString)),
            this, SLOT(onAllTasksFinished(QString)));

    connect(&m_watcher, SIGNAL(finished()),
            this, SLOT(finish()));
    connect(&m_watcher, SIGNAL(resultsReadyAt(int,int)),
            this, SLOT(addResults(int, int)));
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
    return !m_isRunning && m_enabled;
}

bool SymbolsFindFilter::canCancel() const
{
    return m_isRunning;
}

void SymbolsFindFilter::cancel()
{
    m_watcher.cancel();
}

Find::FindFlags SymbolsFindFilter::supportedFindFlags() const
{
    return Find::FindCaseSensitively | Find::FindRegularExpression | Find::FindWholeWords;
}

void SymbolsFindFilter::findAll(const QString &txt, Find::FindFlags findFlags)
{
    m_isRunning = true;
    emit changed();
    Find::SearchResultWindow *window = Find::SearchResultWindow::instance();
    Find::SearchResult *result = window->startNewSearch();
    connect(result, SIGNAL(activated(Find::SearchResultItem)), this, SLOT(openEditor(Find::SearchResultItem)));
    window->popup(true);

    m_search.setSymbolsToSearchFor(m_symbolsToSearch);
    m_search.setSeparateScope(true);
    QSet<QString> projectFileNames;
    if (m_scope == SymbolsFindFilter::SearchProjectsOnly) {
        foreach (ProjectExplorer::Project *project,
                 ProjectExplorer::ProjectExplorerPlugin::instance()->session()->projects()) {
            projectFileNames += project->files(ProjectExplorer::Project::AllFiles).toSet();
        }
    }

    m_watcher.setFuture(QtConcurrent::run<Find::SearchResultItem, QString,
        Find::FindFlags, CPlusPlus::Snapshot,
        SearchSymbols *, QSet<QString> >(runSearch, txt, findFlags, m_manager->snapshot(),
                                    &m_search, projectFileNames));
    Core::ICore::instance()->progressManager()->addTask(m_watcher.future(),
                                                        tr("Searching"),
                                                        Find::Constants::TASK_SEARCH);
}

void SymbolsFindFilter::addResults(int begin, int end)
{
    Find::SearchResultWindow *window = Find::SearchResultWindow::instance();
    QList<Find::SearchResultItem> items;
    for (int i = begin; i < end; ++i)
        items << m_watcher.resultAt(i);
    window->addResults(items, Find::SearchResultWindow::AddSorted);
}

void SymbolsFindFilter::finish()
{
    Find::SearchResultWindow *window = Find::SearchResultWindow::instance();
    window->finishSearch();
    m_isRunning = false;
    emit changed();
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
    settings->setValue(SETTINGS_SYMBOLTYPES, (int)m_symbolsToSearch);
    settings->setValue(SETTINGS_SEARCHSCOPE, (int)m_scope);
    settings->endGroup();
}

void SymbolsFindFilter::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(SETTINGS_GROUP));
    m_symbolsToSearch = (SearchSymbols::SymbolTypes)settings->value(SETTINGS_SYMBOLTYPES,
                                        (int)SearchSymbols::AllTypes).toInt();
    m_scope = (SearchScope)settings->value(SETTINGS_SEARCHSCOPE,
                                           (int)SearchProjectsOnly).toInt();
    settings->endGroup();
    emit symbolsToSearchChanged();
}

void SymbolsFindFilter::onTaskStarted(const QString &type)
{
    if (type == CppTools::Constants::TASK_INDEX) {
        m_enabled = false;
        emit changed();
    }
}

void SymbolsFindFilter::onAllTasksFinished(const QString &type)
{
    if (type == CppTools::Constants::TASK_INDEX) {
        m_enabled = true;
        emit changed();
    }
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
    m_typeClasses->setChecked(symbols & SearchSymbols::Classes);
    m_typeMethods->setChecked(symbols & SearchSymbols::Functions);
    m_typeEnums->setChecked(symbols & SearchSymbols::Enums);
    m_typeDeclarations->setChecked(symbols & SearchSymbols::Declarations);

    SymbolsFindFilter::SearchScope scope = m_filter->searchScope();
    m_searchProjectsOnly->setChecked(scope == SymbolsFindFilter::SearchProjectsOnly);
    m_searchGlobal->setChecked(scope == SymbolsFindFilter::SearchGlobal);
}

void SymbolsFindFilterConfigWidget::setState() const
{
    SearchSymbols::SymbolTypes symbols;
    if (m_typeClasses->isChecked())
        symbols |= SearchSymbols::Classes;
    if (m_typeMethods->isChecked())
        symbols |= SearchSymbols::Functions;
    if (m_typeEnums->isChecked())
        symbols |= SearchSymbols::Enums;
    if (m_typeDeclarations->isChecked())
        symbols |= SearchSymbols::Declarations;
    m_filter->setSymbolsToSearch(symbols);

    if (m_searchProjectsOnly->isChecked())
        m_filter->setSearchScope(SymbolsFindFilter::SearchProjectsOnly);
    else
        m_filter->setSearchScope(SymbolsFindFilter::SearchGlobal);
}
