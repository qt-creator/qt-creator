/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "basefilefind.h"
#include "basefilefind_p.h"
#include "textdocument.h"

#include <aggregation/aggregate.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/dialogs/readonlyfilesdialog.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/ifindsupport.h>
#include <texteditor/texteditor.h>
#include <texteditor/refactoringchanges.h>
#include <utils/fadingindicator.h>
#include <utils/filesearch.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QDebug>
#include <QSettings>
#include <QHash>
#include <QPair>
#include <QStringListModel>
#include <QFutureWatcher>
#include <QPointer>
#include <QComboBox>
#include <QLabel>
#include <QLabel>

using namespace Utils;
using namespace Core;

namespace TextEditor {
namespace Internal {

class BaseFileFindPrivate
{
public:
    BaseFileFindPrivate() : m_resultLabel(0), m_filterCombo(0) {}

    QMap<QFutureWatcher<FileSearchResultList> *, QPointer<SearchResult> > m_watchers;
    QPointer<IFindSupport> m_currentFindSupport;

    QLabel *m_resultLabel;
    QStringListModel m_filterStrings;
    QString m_filterSetting;
    QPointer<QComboBox> m_filterCombo;
};

} // namespace Internal

using namespace Internal;

BaseFileFind::BaseFileFind() : d(new BaseFileFindPrivate)
{
}

BaseFileFind::~BaseFileFind()
{
    delete d;
}

bool BaseFileFind::isEnabled() const
{
    return true;
}

void BaseFileFind::cancel()
{
    SearchResult *search = qobject_cast<SearchResult *>(sender());
    QTC_ASSERT(search, return);
    QFutureWatcher<FileSearchResultList> *watcher = d->m_watchers.key(search);
    QTC_ASSERT(watcher, return);
    watcher->cancel();
}

void BaseFileFind::setPaused(bool paused)
{
    SearchResult *search = qobject_cast<SearchResult *>(sender());
    QTC_ASSERT(search, return);
    QFutureWatcher<FileSearchResultList> *watcher = d->m_watchers.key(search);
    QTC_ASSERT(watcher, return);
    if (!paused || watcher->isRunning()) // guard against pausing when the search is finished
        watcher->setPaused(paused);
}

QStringList BaseFileFind::fileNameFilters() const
{
    QStringList filters;
    if (d->m_filterCombo && !d->m_filterCombo->currentText().isEmpty()) {
        const QStringList parts = d->m_filterCombo->currentText().split(QLatin1Char(','));
        foreach (const QString &part, parts) {
            const QString filter = part.trimmed();
            if (!filter.isEmpty())
                filters << filter;
        }
    }
    return filters;
}

void BaseFileFind::runNewSearch(const QString &txt, FindFlags findFlags,
                                    SearchResultWindow::SearchMode searchMode)
{
    d->m_currentFindSupport = 0;
    if (d->m_filterCombo)
        updateComboEntries(d->m_filterCombo, true);
    SearchResult *search = SearchResultWindow::instance()->startNewSearch(label(),
                           toolTip().arg(IFindFilter::descriptionForFindFlags(findFlags)),
                           txt, searchMode, SearchResultWindow::PreserveCaseEnabled,
                           QString::fromLatin1("TextEditor"));
    search->setTextToReplace(txt);
    search->setSearchAgainSupported(true);
    FileFindParameters parameters;
    parameters.text = txt;
    parameters.flags = findFlags;
    parameters.nameFilters = fileNameFilters();
    parameters.additionalParameters = additionalParameters();
    search->setUserData(qVariantFromValue(parameters));
    connect(search, SIGNAL(activated(Core::SearchResultItem)), this, SLOT(openEditor(Core::SearchResultItem)));
    if (searchMode == SearchResultWindow::SearchAndReplace) {
        connect(search, SIGNAL(replaceButtonClicked(QString,QList<Core::SearchResultItem>,bool)),
                this, SLOT(doReplace(QString,QList<Core::SearchResultItem>,bool)));
    }
    connect(search, SIGNAL(visibilityChanged(bool)), this, SLOT(hideHighlightAll(bool)));
    connect(search, SIGNAL(cancelled()), this, SLOT(cancel()));
    connect(search, SIGNAL(paused(bool)), this, SLOT(setPaused(bool)));
    connect(search, SIGNAL(searchAgainRequested()), this, SLOT(searchAgain()));
    connect(this, SIGNAL(enabledChanged(bool)), search, SIGNAL(requestEnabledCheck()));
    connect(search, SIGNAL(requestEnabledCheck()), this, SLOT(recheckEnabled()));

    runSearch(search);
}

void BaseFileFind::runSearch(SearchResult *search)
{
    FileFindParameters parameters = search->userData().value<FileFindParameters>();
    CountingLabel *label = new CountingLabel;
    connect(search, SIGNAL(countChanged(int)), label, SLOT(updateCount(int)));
    CountingLabel *statusLabel = new CountingLabel;
    connect(search, SIGNAL(countChanged(int)), statusLabel, SLOT(updateCount(int)));
    SearchResultWindow::instance()->popup(IOutputPane::Flags(IOutputPane::ModeSwitch|IOutputPane::WithFocus));
    QFutureWatcher<FileSearchResultList> *watcher = new QFutureWatcher<FileSearchResultList>();
    d->m_watchers.insert(watcher, search);
    watcher->setPendingResultsLimit(1);
    connect(watcher, SIGNAL(resultReadyAt(int)), this, SLOT(displayResult(int)));
    connect(watcher, SIGNAL(finished()), this, SLOT(searchFinished()));
    if (parameters.flags & FindRegularExpression) {
        watcher->setFuture(Utils::findInFilesRegExp(parameters.text,
            files(parameters.nameFilters, parameters.additionalParameters),
            textDocumentFlagsForFindFlags(parameters.flags),
            TextDocument::openedTextDocumentContents()));
    } else {
        watcher->setFuture(Utils::findInFiles(parameters.text,
            files(parameters.nameFilters, parameters.additionalParameters),
            textDocumentFlagsForFindFlags(parameters.flags),
            TextDocument::openedTextDocumentContents()));
    }
    FutureProgress *progress =
        ProgressManager::addTask(watcher->future(), tr("Searching"), Constants::TASK_SEARCH);
    progress->setWidget(label);
    progress->setStatusBarWidget(statusLabel);
    connect(progress, SIGNAL(clicked()), search, SLOT(popup()));
}

void BaseFileFind::findAll(const QString &txt, FindFlags findFlags)
{
    runNewSearch(txt, findFlags, SearchResultWindow::SearchOnly);
}

void BaseFileFind::replaceAll(const QString &txt, FindFlags findFlags)
{
    runNewSearch(txt, findFlags, SearchResultWindow::SearchAndReplace);
}

void BaseFileFind::doReplace(const QString &text,
                             const QList<SearchResultItem> &items,
                             bool preserveCase)
{
    QStringList files = replaceAll(text, items, preserveCase);
    if (!files.isEmpty()) {
        Utils::FadingIndicator::showText(ICore::mainWindow(),
            tr("%n occurrences replaced.", 0, items.size()),
            Utils::FadingIndicator::SmallText);
        DocumentManager::notifyFilesChangedInternally(files);
        SearchResultWindow::instance()->hide();
    }
}

void BaseFileFind::displayResult(int index) {
    QFutureWatcher<FileSearchResultList> *watcher =
            static_cast<QFutureWatcher<FileSearchResultList> *>(sender());
    SearchResult *search = d->m_watchers.value(watcher);
    if (!search) {
        // search was removed from search history while the search is running
        watcher->cancel();
        return;
    }
    FileSearchResultList results = watcher->resultAt(index);
    QList<SearchResultItem> items;
    foreach (const FileSearchResult &result, results) {
        SearchResultItem item;
        item.path = QStringList() << QDir::toNativeSeparators(result.fileName);
        item.lineNumber = result.lineNumber;
        item.text = result.matchingLine;
        item.textMarkLength = result.matchLength;
        item.textMarkPos = result.matchStart;
        item.useTextEditorFont = true;
        item.userData = result.regexpCapturedTexts;
        items << item;
    }
    search->addResults(items, SearchResult::AddOrdered);
}

void BaseFileFind::searchFinished()
{
    QFutureWatcher<FileSearchResultList> *watcher =
            static_cast<QFutureWatcher<FileSearchResultList> *>(sender());
    SearchResult *search = d->m_watchers.value(watcher);
    if (search)
        search->finishSearch(watcher->isCanceled());
    d->m_watchers.remove(watcher);
    watcher->deleteLater();
}

QWidget *BaseFileFind::createPatternWidget()
{
    QString filterToolTip = tr("List of comma separated wildcard filters");
    d->m_filterCombo = new QComboBox;
    d->m_filterCombo->setEditable(true);
    d->m_filterCombo->setModel(&d->m_filterStrings);
    d->m_filterCombo->setMaxCount(10);
    d->m_filterCombo->setMinimumContentsLength(10);
    d->m_filterCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    d->m_filterCombo->setInsertPolicy(QComboBox::InsertAtBottom);
    d->m_filterCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d->m_filterCombo->setToolTip(filterToolTip);
    syncComboWithSettings(d->m_filterCombo, d->m_filterSetting);
    return d->m_filterCombo;
}

void BaseFileFind::writeCommonSettings(QSettings *settings)
{
    settings->setValue(QLatin1String("filters"), d->m_filterStrings.stringList());
    if (d->m_filterCombo)
        settings->setValue(QLatin1String("currentFilter"), d->m_filterCombo->currentText());
}

void BaseFileFind::readCommonSettings(QSettings *settings, const QString &defaultFilter)
{
    QStringList filters = settings->value(QLatin1String("filters")).toStringList();
    d->m_filterSetting = settings->value(QLatin1String("currentFilter")).toString();
    if (filters.isEmpty())
        filters << defaultFilter;
    if (d->m_filterSetting.isEmpty())
        d->m_filterSetting = filters.first();
    d->m_filterStrings.setStringList(filters);
    if (d->m_filterCombo)
        syncComboWithSettings(d->m_filterCombo, d->m_filterSetting);
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
        if (onTop)
            combo->insertItem(0, combo->currentText());
        else
            combo->addItem(combo->currentText());
        combo->setCurrentIndex(combo->findText(combo->currentText()));
    }
}

void BaseFileFind::openEditor(const SearchResultItem &item)
{
    SearchResult *result = qobject_cast<SearchResult *>(sender());
    IEditor *openedEditor = 0;
    if (item.path.size() > 0) {
        openedEditor = EditorManager::openEditorAt(QDir::fromNativeSeparators(item.path.first()),
                                                   item.lineNumber,
                                                   item.textMarkPos, Id(),
                                                   EditorManager::DoNotSwitchToDesignMode);
    } else {
        openedEditor = EditorManager::openEditor(QDir::fromNativeSeparators(item.text));
    }
    if (d->m_currentFindSupport)
        d->m_currentFindSupport->clearHighlights();
    d->m_currentFindSupport = 0;
    if (!openedEditor)
        return;
    // highlight results
    if (IFindSupport *findSupport = Aggregation::query<IFindSupport>(openedEditor->widget())) {
        if (result) {
            FileFindParameters parameters = result->userData().value<FileFindParameters>();
            d->m_currentFindSupport = findSupport;
            d->m_currentFindSupport->highlightAll(parameters.text, parameters.flags);
        }
    }
}

void BaseFileFind::hideHighlightAll(bool visible)
{
    if (!visible && d->m_currentFindSupport)
        d->m_currentFindSupport->clearHighlights();
}

void BaseFileFind::searchAgain()
{
    SearchResult *search = qobject_cast<SearchResult *>(sender());
    search->restart();
    runSearch(search);
}

void BaseFileFind::recheckEnabled()
{
    SearchResult *search = qobject_cast<SearchResult *>(sender());
    if (!search)
        return;
    search->setSearchAgainEnabled(isEnabled());
}

QStringList BaseFileFind::replaceAll(const QString &text,
                                     const QList<SearchResultItem> &items,
                                     bool preserveCase)
{
    if (items.isEmpty())
        return QStringList();

    RefactoringChanges refactoring;

    QHash<QString, QList<SearchResultItem> > changes;
    foreach (const SearchResultItem &item, items)
        changes[QDir::fromNativeSeparators(item.path.first())].append(item);

    // Checking for files without write permissions
    QHashIterator<QString, QList<SearchResultItem> > it(changes);
    QSet<QString> roFiles;
    while (it.hasNext()) {
        it.next();
        const QFileInfo fileInfo(it.key());
        if (!fileInfo.isWritable())
            roFiles.insert(it.key());
    }

    // Query the user for permissions
    if (!roFiles.isEmpty()) {
        ReadOnlyFilesDialog roDialog(roFiles.toList(), ICore::mainWindow());
        roDialog.setShowFailWarning(true, tr("Aborting replace."));
        if (roDialog.exec() == ReadOnlyFilesDialog::RO_Cancel)
            return QStringList();
    }

    it.toFront();
    while (it.hasNext()) {
        it.next();
        const QString fileName = it.key();
        const QList<SearchResultItem> changeItems = it.value();

        ChangeSet changeSet;
        RefactoringFilePtr file = refactoring.file(fileName);
        QSet<QPair<int, int> > processed;
        foreach (const SearchResultItem &item, changeItems) {
            const QPair<int, int> &p = qMakePair(item.lineNumber, item.textMarkPos);
            if (processed.contains(p))
                continue;
            processed.insert(p);

            QString replacement;
            if (item.userData.canConvert<QStringList>() && !item.userData.toStringList().isEmpty()) {
                replacement = Utils::expandRegExpReplacement(text, item.userData.toStringList());
            } else if (preserveCase) {
                const QString originalText = (item.textMarkLength == 0) ? item.text
                                                                        : item.text.mid(item.textMarkPos, item.textMarkLength);
                replacement = Utils::matchCaseReplacement(originalText, text);
            } else {
                replacement = text;
            }

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

QVariant BaseFileFind::getAdditionalParameters(SearchResult *search)
{
    return search->userData().value<FileFindParameters>().additionalParameters;
}

CountingLabel::CountingLabel()
{
    setAlignment(Qt::AlignCenter);
    // ### TODO this setup should be done by style
    QFont f = font();
    f.setBold(true);
    f.setPointSizeF(StyleHelper::sidebarFontSize());
    setFont(f);
    setPalette(StyleHelper::sidebarFontPalette(palette()));
    updateCount(0);
}

void CountingLabel::updateCount(int count)
{
    setText(tr("%1 found").arg(count));
}

} // namespace TextEditor
