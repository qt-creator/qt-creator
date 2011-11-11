/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "basefilefind.h"

#include <aggregation/aggregate.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/filemanager.h>
#include <find/textfindconstants.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/refactoringchanges.h>
#include <utils/stylehelper.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDirIterator>
#include <QtCore/QSettings>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtGui/QFileDialog>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QTextBlock>

using namespace Utils;
using namespace Find;
using namespace TextEditor;

BaseFileFind::BaseFileFind()
  : m_currentSearch(0),
    m_currentSearchCount(0),
    m_watcher(0),
    m_isSearching(false),
    m_resultLabel(0),
    m_filterCombo(0)
{
}

BaseFileFind::~BaseFileFind()
{
}

bool BaseFileFind::isEnabled() const
{
    return !m_isSearching;
}

void BaseFileFind::cancel()
{
    QTC_ASSERT(m_watcher, return);
    m_watcher->cancel();
}

QStringList BaseFileFind::fileNameFilters() const
{
    QStringList filters;
    if (m_filterCombo && !m_filterCombo->currentText().isEmpty()) {
        const QStringList parts = m_filterCombo->currentText().split(QLatin1Char(','));
        foreach (const QString &part, parts) {
            const QString filter = part.trimmed();
            if (!filter.isEmpty()) {
                filters << filter;
            }
        }
    }
    return filters;
}

void BaseFileFind::runNewSearch(const QString &txt, Find::FindFlags findFlags,
                                    SearchResultWindow::SearchMode searchMode)
{
    m_isSearching = true;
    m_currentFindSupport = 0;
    emit changed();
    if (m_filterCombo)
        updateComboEntries(m_filterCombo, true);
    delete m_watcher;
    m_watcher = new QFutureWatcher<FileSearchResultList>();
    m_watcher->setPendingResultsLimit(1);
    connect(m_watcher, SIGNAL(resultReadyAt(int)), this, SLOT(displayResult(int)));
    connect(m_watcher, SIGNAL(finished()), this, SLOT(searchFinished()));
    m_currentSearchCount = 0;
    m_currentSearch = Find::SearchResultWindow::instance()->startNewSearch(label(),
                           toolTip().arg(Find::IFindFilter::descriptionForFindFlags(findFlags)),
                           txt, searchMode, QString::fromLatin1("TextEditor"));
    m_currentSearch->setTextToReplace(txt);
    QVariantList searchParameters;
    searchParameters << qVariantFromValue(txt) << qVariantFromValue(findFlags);
    m_currentSearch->setUserData(searchParameters);
    connect(m_currentSearch, SIGNAL(activated(Find::SearchResultItem)), this, SLOT(openEditor(Find::SearchResultItem)));
    if (searchMode == SearchResultWindow::SearchAndReplace) {
        connect(m_currentSearch, SIGNAL(replaceButtonClicked(QString,QList<Find::SearchResultItem>)),
                this, SLOT(doReplace(QString,QList<Find::SearchResultItem>)));
    }
    connect(m_currentSearch, SIGNAL(visibilityChanged(bool)), this, SLOT(hideHighlightAll(bool)));
    Find::SearchResultWindow::instance()->popup(true);
    if (findFlags & Find::FindRegularExpression) {
        m_watcher->setFuture(Utils::findInFilesRegExp(txt, files(),
            textDocumentFlagsForFindFlags(findFlags), ITextEditor::openedTextEditorsContents()));
    } else {
        m_watcher->setFuture(Utils::findInFiles(txt, files(),
            textDocumentFlagsForFindFlags(findFlags), ITextEditor::openedTextEditorsContents()));
    }
    connect(m_currentSearch, SIGNAL(cancelled()), this, SLOT(cancel()));
    Core::FutureProgress *progress =
        Core::ICore::instance()->progressManager()->addTask(m_watcher->future(),
                                                                        tr("Search"),
                                                                        Constants::TASK_SEARCH);
    progress->setWidget(createProgressWidget());
    connect(progress, SIGNAL(clicked()), Find::SearchResultWindow::instance(), SLOT(popup()));
}

void BaseFileFind::findAll(const QString &txt, Find::FindFlags findFlags)
{
    runNewSearch(txt, findFlags, SearchResultWindow::SearchOnly);
}

void BaseFileFind::replaceAll(const QString &txt, Find::FindFlags findFlags)
{
    runNewSearch(txt, findFlags, SearchResultWindow::SearchAndReplace);
}

void BaseFileFind::doReplace(const QString &text,
                               const QList<Find::SearchResultItem> &items)
{
    QStringList files = replaceAll(text, items);
    Core::FileManager *fileManager = Core::ICore::instance()->fileManager();
    if (!files.isEmpty()) {
        fileManager->notifyFilesChangedInternally(files);
        Find::SearchResultWindow::instance()->hide();
    }
}

void BaseFileFind::displayResult(int index) {
    if (!m_currentSearch) {
        m_watcher->cancel();
        return;
    }
    Utils::FileSearchResultList results = m_watcher->resultAt(index);
    QList<Find::SearchResultItem> items;
    foreach (const Utils::FileSearchResult &result, results) {
        Find::SearchResultItem item;
        item.path = QStringList() << QDir::toNativeSeparators(result.fileName);
        item.lineNumber = result.lineNumber;
        item.text = result.matchingLine;
        item.textMarkLength = result.matchLength;
        item.textMarkPos = result.matchStart;
        item.useTextEditorFont = true;
        item.userData = result.regexpCapturedTexts;
        items << item;
    }
    m_currentSearch->addResults(items, Find::SearchResult::AddOrdered);
    m_currentSearchCount += items.count();
    if (m_resultLabel)
        m_resultLabel->setText(tr("%1 found").arg(m_currentSearchCount));
}

void BaseFileFind::searchFinished()
{
    if (m_currentSearch)
        m_currentSearch->finishSearch();
    m_currentSearch = 0;
    m_isSearching = false;
    m_resultLabel = 0;
    m_watcher->deleteLater();
    m_watcher = 0;
    emit changed();
}

QWidget *BaseFileFind::createProgressWidget()
{
    m_resultLabel = new QLabel;
    m_resultLabel->setAlignment(Qt::AlignCenter);
    // ### TODO this setup should be done by style
    QFont f = m_resultLabel->font();
    f.setBold(true);
    f.setPointSizeF(StyleHelper::sidebarFontSize());
    m_resultLabel->setFont(f);
    m_resultLabel->setPalette(StyleHelper::sidebarFontPalette(m_resultLabel->palette()));
    m_resultLabel->setText(tr("%1 found").arg(m_currentSearchCount));
    return m_resultLabel;
}

QWidget *BaseFileFind::createPatternWidget()
{
    QString filterToolTip = tr("List of comma separated wildcard filters");
    m_filterCombo = new QComboBox;
    m_filterCombo->setEditable(true);
    m_filterCombo->setModel(&m_filterStrings);
    m_filterCombo->setMaxCount(10);
    m_filterCombo->setMinimumContentsLength(10);
    m_filterCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_filterCombo->setInsertPolicy(QComboBox::InsertAtBottom);
    m_filterCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_filterCombo->setToolTip(filterToolTip);
    syncComboWithSettings(m_filterCombo, m_filterSetting);
    return m_filterCombo;
}

void BaseFileFind::writeCommonSettings(QSettings *settings)
{
    settings->setValue("filters", m_filterStrings.stringList());
    if (m_filterCombo)
        settings->setValue("currentFilter", m_filterCombo->currentText());
}

void BaseFileFind::readCommonSettings(QSettings *settings, const QString &defaultFilter)
{
    QStringList filters = settings->value("filters").toStringList();
    m_filterSetting = settings->value("currentFilter").toString();
    if (filters.isEmpty())
        filters << defaultFilter;
    if (m_filterSetting.isEmpty())
        m_filterSetting = filters.first();
    m_filterStrings.setStringList(filters);
    if (m_filterCombo)
        syncComboWithSettings(m_filterCombo, m_filterSetting);
}

void BaseFileFind::syncComboWithSettings(QComboBox *combo, const QString &setting)
{
    if (!combo)
        return;
    int index = combo->findText(setting);
    if (index < 0)
        combo->setEditText(setting);
    else
        combo->setCurrentIndex(index);
}

void BaseFileFind::updateComboEntries(QComboBox *combo, bool onTop)
{
    int index = combo->findText(combo->currentText());
    if (index < 0) {
        if (onTop) {
            combo->insertItem(0, combo->currentText());
        } else {
            combo->addItem(combo->currentText());
        }
        combo->setCurrentIndex(combo->findText(combo->currentText()));
    }
}

void BaseFileFind::openEditor(const Find::SearchResultItem &item)
{
    SearchResult *result = qobject_cast<SearchResult *>(sender());
    Core::IEditor *openedEditor = 0;
    if (item.path.size() > 0) {
        openedEditor = TextEditor::BaseTextEditorWidget::openEditorAt(QDir::fromNativeSeparators(item.path.first()),
                                                                      item.lineNumber,
                                                                      item.textMarkPos,
                                                                      Core::Id(),
                                                                      Core::EditorManager::ModeSwitch);
    } else {
        openedEditor = Core::EditorManager::instance()->openEditor(item.text, Core::Id(),
                                                                   Core::EditorManager::ModeSwitch);
    }
    if (m_currentFindSupport)
        m_currentFindSupport->clearResults();
    m_currentFindSupport = 0;
    if (!openedEditor)
        return;
    // highlight results
    if (IFindSupport *findSupport = Aggregation::query<IFindSupport>(openedEditor->widget())) {
        if (result) {
            QVariantList userData = result->userData().value<QVariantList>();
            QTC_ASSERT(userData.size() != 0, return);
            m_currentFindSupport = findSupport;
            m_currentFindSupport->highlightAll(userData.at(0).toString(), userData.at(1).value<FindFlags>());
        }
    }
}

void BaseFileFind::hideHighlightAll(bool visible)
{
    if (!visible && m_currentFindSupport)
        m_currentFindSupport->clearResults();
}

QStringList BaseFileFind::replaceAll(const QString &text,
                               const QList<Find::SearchResultItem> &items)
{
    if (items.isEmpty())
        return QStringList();

    RefactoringChanges refactoring;

    QHash<QString, QList<Find::SearchResultItem> > changes;
    foreach (const Find::SearchResultItem &item, items)
        changes[QDir::fromNativeSeparators(item.path.first())].append(item);

    QHashIterator<QString, QList<Find::SearchResultItem> > it(changes);
    while (it.hasNext()) {
        it.next();
        const QString fileName = it.key();
        const QList<Find::SearchResultItem> changeItems = it.value();

        ChangeSet changeSet;
        RefactoringFilePtr file = refactoring.file(fileName);
        QSet<QPair<int, int> > processed;
        foreach (const Find::SearchResultItem &item, changeItems) {
            const QPair<int, int> &p = qMakePair(item.lineNumber, item.textMarkPos);
            if (processed.contains(p))
                continue;
            processed.insert(p);

            QString replacement;
            if (item.userData.canConvert<QStringList>() && !item.userData.toStringList().isEmpty())
                replacement = Utils::expandRegExpReplacement(text, item.userData.toStringList());
            else
                replacement = text;

            const int start = file->position(item.lineNumber, item.textMarkPos + 1);
            const int end = file->position(item.lineNumber,
                                           item.textMarkPos + item.textMarkLength + 1);
            changeSet.replace(start, end, replacement);
        }
        file->setChangeSet(changeSet);
        file->apply();
    }

    return changes.keys();
}
