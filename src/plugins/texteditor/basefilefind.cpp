// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basefilefind.h"

#include "refactoringchanges.h"
#include "textdocument.h"
#include "texteditortr.h"

#include <aggregation/aggregate.h>

#include <coreplugin/dialogs/readonlyfilesdialog.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/ifindsupport.h>
#include <coreplugin/find/textfindconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <utils/algorithm.h>
#include <utils/fadingindicator.h>
#include <utils/futuresynchronizer.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QFutureWatcher>
#include <QHash>
#include <QLabel>
#include <QPair>
#include <QPointer>
#include <QPromise>
#include <QStringListModel>

using namespace Utils;
using namespace Core;

namespace TextEditor {

static std::optional<QRegularExpression> regExpFromParameters(const FileFindParameters &parameters)
{
    if (!(parameters.flags & FindRegularExpression))
        return {};

    const QRegularExpression::PatternOptions options = parameters.flags & FindCaseSensitively
        ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption;
    QRegularExpression regExp;
    regExp.setPattern(parameters.text);
    regExp.setPatternOptions(options);
    return regExp;
}

void searchInProcessOutput(QPromise<SearchResultItems> &promise,
                           const FileFindParameters &parameters,
                           const ProcessSetupHandler &processSetupHandler,
                           const ProcessOutputParser &processOutputParser)
{
    if (promise.isCanceled())
        return;

    QEventLoop loop;

    Process process;
    processSetupHandler(process);

    const std::optional<QRegularExpression> regExp = regExpFromParameters(parameters);
    QStringList outputBuffer;
    // The states transition exactly in this order:
    enum State { BelowLimit, AboveLimit, Paused, Resumed };
    State state = BelowLimit;
    int reportedItemsCount = 0;
    QFuture<void> future(promise.future());
    process.setStdOutCallback([&](const QString &output) {
        if (promise.isCanceled()) {
            process.close();
            loop.quit();
            return;
        }
        // The SearchResultWidget is going to pause the search anyway, so start buffering
        // the output.
        if (state == AboveLimit || state == Paused) {
            outputBuffer.append(output);
        } else {
            const SearchResultItems items = processOutputParser(future, output, regExp);
            if (!items.isEmpty())
                promise.addResult(items);
            reportedItemsCount += items.size();
            if (state == BelowLimit && reportedItemsCount > 200000)
                state = AboveLimit;
        }
    });
    QObject::connect(&process, &Process::done, &loop, [&loop, &promise, &state] {
        if (state == BelowLimit || state == Resumed || promise.isCanceled())
            loop.quit();
    });

    if (promise.isCanceled())
        return;

    process.start();
    if (process.state() == QProcess::NotRunning)
        return;

    QFutureWatcher<void> watcher;
    QObject::connect(&watcher, &QFutureWatcherBase::canceled, &loop, [&process, &loop] {
        process.close();
        loop.quit();
    });
    QObject::connect(&watcher, &QFutureWatcherBase::paused, &loop, [&state] { state = Paused; });
    QObject::connect(&watcher, &QFutureWatcherBase::resumed, &loop, [&] {
        state = Resumed;
        for (const QString &output : outputBuffer) {
            if (promise.isCanceled()) {
                process.close();
                loop.quit();
            }
            const SearchResultItems items = processOutputParser(future, output, regExp);
            if (!items.isEmpty())
                promise.addResult(items);
        }
        outputBuffer.clear();
        if (process.state() == QProcess::NotRunning)
            loop.quit();
    });
    watcher.setFuture(future);
    if (promise.isCanceled())
        return;
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

namespace Internal {

class InternalEngine : public TextEditor::SearchEngine
{
public:
    InternalEngine() : m_widget(new QWidget) {}
    ~InternalEngine() override { delete m_widget;}
    QString title() const override { return Tr::tr("Internal"); }
    QString toolTip() const override { return {}; }
    QWidget *widget() const override { return m_widget; }
    void readSettings(QtcSettings * /*settings*/) override {}
    void writeSettings(QtcSettings * /*settings*/) const override {}
    SearchExecutor searchExecutor() const override
    {
        return [](const FileFindParameters &parameters) {
            return Utils::findInFiles(parameters.text, parameters.fileContainerProvider(),
                                      parameters.flags, TextDocument::openedTextDocumentContents());
        };
    }

private:
    QWidget *m_widget;
};

class SearchEnginePrivate
{
public:
    bool isEnabled = true;
};

class CountingLabel : public QLabel
{
public:
    CountingLabel();
    void updateCount(int count);
};

class BaseFileFindPrivate
{
public:
    QPointer<IFindSupport> m_currentFindSupport;

    Utils::FutureSynchronizer m_futureSynchronizer;
    QLabel *m_resultLabel = nullptr;
    // models in native path format
    QStringListModel m_filterStrings;
    QStringListModel m_exclusionStrings;
    // current filter in portable path format
    QString m_filterSetting;
    QString m_exclusionSetting;
    QPointer<QComboBox> m_filterCombo;
    QPointer<QComboBox> m_exclusionCombo;
    QVector<SearchEngine *> m_searchEngines;
    InternalEngine m_internalSearchEngine;
    int m_currentSearchEngineIndex = -1;
    FilePath m_searchDir;
};

} // namespace Internal

static void syncComboWithSettings(QComboBox *combo, const QString &setting)
{
    if (!combo)
        return;
    const QString &nativeSettings = QDir::toNativeSeparators(setting);
    int index = combo->findText(nativeSettings);
    if (index < 0)
        combo->setEditText(nativeSettings);
    else
        combo->setCurrentIndex(index);
}

static void updateComboEntries(QComboBox *combo, bool onTop)
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

using namespace Internal;

SearchEngine::SearchEngine(QObject *parent)
    : QObject(parent), d(new SearchEnginePrivate)
{
}

SearchEngine::~SearchEngine()
{
    delete d;
}

bool SearchEngine::isEnabled() const
{
    return d->isEnabled;
}

void SearchEngine::setEnabled(bool enabled)
{
    if (enabled == d->isEnabled)
        return;
    d->isEnabled = enabled;
    emit enabledChanged(d->isEnabled);
}

BaseFileFind::BaseFileFind() : d(new BaseFileFindPrivate)
{
    addSearchEngine(&d->m_internalSearchEngine);
}

BaseFileFind::~BaseFileFind()
{
    delete d;
}

bool BaseFileFind::isEnabled() const
{
    return true;
}

QStringList BaseFileFind::fileNameFilters() const
{
    if (d->m_filterCombo)
        return splitFilterUiText(d->m_filterCombo->currentText());
    return {};
}

QStringList BaseFileFind::fileExclusionFilters() const
{
    if (d->m_exclusionCombo)
        return splitFilterUiText(d->m_exclusionCombo->currentText());
    return {};
}

SearchEngine *BaseFileFind::currentSearchEngine() const
{
    if (d->m_searchEngines.isEmpty() || d->m_currentSearchEngineIndex == -1)
        return nullptr;
    return d->m_searchEngines[d->m_currentSearchEngineIndex];
}

QVector<SearchEngine *> BaseFileFind::searchEngines() const
{
    return d->m_searchEngines;
}

void BaseFileFind::setCurrentSearchEngine(int index)
{
    if (d->m_currentSearchEngineIndex == index)
        return;
    d->m_currentSearchEngineIndex = index;
    emit currentSearchEngineChanged();
}

void BaseFileFind::runNewSearch(const QString &txt, FindFlags findFlags,
                                    SearchResultWindow::SearchMode searchMode)
{
    d->m_currentFindSupport = nullptr;
    if (d->m_filterCombo)
        updateComboEntries(d->m_filterCombo, true);
    if (d->m_exclusionCombo)
        updateComboEntries(d->m_exclusionCombo, true);
    const QString tooltip = toolTip();

    SearchResult *search = SearchResultWindow::instance()->startNewSearch(
                label(),
                tooltip.arg(IFindFilter::descriptionForFindFlags(findFlags)),
                txt, searchMode, SearchResultWindow::PreserveCaseEnabled,
                QString::fromLatin1("TextEditor"));
    setupSearch(search);
    search->setTextToReplace(txt);
    search->setSearchAgainSupported(true);
    SearchEngine *searchEngine = currentSearchEngine();
    FileFindParameters parameters;
    parameters.text = txt;
    parameters.flags = findFlags;
    parameters.nameFilters = fileNameFilters();
    parameters.exclusionFilters = fileExclusionFilters();
    parameters.searchDir = searchDir();
    parameters.fileContainerProvider = fileContainerProvider();
    parameters.editorOpener = searchEngine->editorOpener();
    parameters.searchExecutor = searchEngine->searchExecutor();

    search->setUserData(QVariant::fromValue(parameters));
    connect(search, &SearchResult::activated, this, [this, search](const SearchResultItem &item) {
        openEditor(search, item);
    });
    if (searchMode == SearchResultWindow::SearchAndReplace)
        connect(search, &SearchResult::replaceButtonClicked, this, &BaseFileFind::doReplace);
    connect(search, &SearchResult::visibilityChanged, this, &BaseFileFind::hideHighlightAll);
    connect(search, &SearchResult::searchAgainRequested, this, [this, search] {
        searchAgain(search);
    });

    runSearch(search);
}

void BaseFileFind::runSearch(SearchResult *search)
{
    const FileFindParameters parameters = search->userData().value<FileFindParameters>();
    SearchResultWindow::instance()->popup(IOutputPane::Flags(IOutputPane::ModeSwitch|IOutputPane::WithFocus));
    auto watcher = new QFutureWatcher<SearchResultItems>;
    watcher->setPendingResultsLimit(1);
    // search is deleted if it is removed from search panel
    connect(search, &QObject::destroyed, watcher, &QFutureWatcherBase::cancel);
    connect(search, &SearchResult::canceled, watcher, &QFutureWatcherBase::cancel);
    connect(search, &SearchResult::paused, watcher, [watcher](bool paused) {
        if (!paused || watcher->isRunning()) // guard against pausing when the search is finished
            watcher->setPaused(paused);
    });
    connect(watcher, &QFutureWatcherBase::resultReadyAt, search, [watcher, search](int index) {
        search->addResults(watcher->resultAt(index), SearchResult::AddOrdered);
    });
    // auto-delete:
    connect(watcher, &QFutureWatcherBase::finished, watcher, &QObject::deleteLater);
    connect(watcher, &QFutureWatcherBase::finished, search, [watcher, search]() {
        search->finishSearch(watcher->isCanceled());
    });
    QFuture<SearchResultItems> future = parameters.searchExecutor(parameters);
    watcher->setFuture(future);
    d->m_futureSynchronizer.addFuture(future);
    FutureProgress *progress = ProgressManager::addTask(future,
                                                        Tr::tr("Searching"),
                                                        Core::Constants::TASK_SEARCH);
    connect(search, &SearchResult::countChanged, progress, [progress](int c) {
        progress->setSubtitle(Tr::tr("%n found.", nullptr, c));
    });
    progress->setSubtitleVisibleInStatusBar(true);
    connect(progress, &FutureProgress::clicked, search, &SearchResult::popup);
}

void BaseFileFind::findAll(const QString &txt, FindFlags findFlags)
{
    runNewSearch(txt, findFlags, SearchResultWindow::SearchOnly);
}

void BaseFileFind::replaceAll(const QString &txt, FindFlags findFlags)
{
    runNewSearch(txt, findFlags, SearchResultWindow::SearchAndReplace);
}

void BaseFileFind::addSearchEngine(SearchEngine *searchEngine)
{
    d->m_searchEngines.push_back(searchEngine);
    if (d->m_searchEngines.size() == 1) // empty before, make sure we have a current engine
        setCurrentSearchEngine(0);
}

void BaseFileFind::doReplace(const QString &text, const SearchResultItems &items,
                             bool preserveCase)
{
    const FilePaths files = replaceAll(text, items, preserveCase);
    if (!files.isEmpty()) {
        FadingIndicator::showText(ICore::dialogParent(),
            Tr::tr("%n occurrences replaced.", nullptr, items.size()), FadingIndicator::SmallText);
        SearchResultWindow::instance()->hide();
    }
}

static QComboBox *createCombo(QAbstractItemModel *model)
{
    auto combo = new QComboBox;
    combo->setEditable(true);
    combo->setModel(model);
    combo->setMaxCount(10);
    combo->setMinimumContentsLength(10);
    combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    combo->setInsertPolicy(QComboBox::InsertAtBottom);
    combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return combo;
}

static QLabel *createLabel(const QString &text)
{
    auto filePatternLabel = new QLabel(text);
    filePatternLabel->setMinimumWidth(80);
    filePatternLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    filePatternLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return filePatternLabel;
}

QList<QPair<QWidget *, QWidget *>> BaseFileFind::createPatternWidgets()
{
    QLabel *filterLabel = createLabel(msgFilePatternLabel());
    d->m_filterCombo = createCombo(&d->m_filterStrings);
    d->m_filterCombo->setToolTip(msgFilePatternToolTip());
    filterLabel->setBuddy(d->m_filterCombo);
    syncComboWithSettings(d->m_filterCombo, d->m_filterSetting);
    QLabel *exclusionLabel = createLabel(msgExclusionPatternLabel());
    d->m_exclusionCombo = createCombo(&d->m_exclusionStrings);
    d->m_exclusionCombo->setToolTip(msgFilePatternToolTip(InclusionType::Excluded));
    exclusionLabel->setBuddy(d->m_exclusionCombo);
    syncComboWithSettings(d->m_exclusionCombo, d->m_exclusionSetting);
    return {{filterLabel, d->m_filterCombo}, {exclusionLabel, d->m_exclusionCombo}};
}

void BaseFileFind::setSearchDir(const FilePath &dir)
{
    if (dir == d->m_searchDir)
        return;
    d->m_searchDir = dir;
    emit searchDirChanged(d->m_searchDir);
}

FilePath BaseFileFind::searchDir() const
{
    return d->m_searchDir;
}

void BaseFileFind::writeCommonSettings(QtcSettings *settings)
{
    const auto fromNativeSeparators = [](const QStringList &files) -> QStringList {
        return Utils::transform(files, &QDir::fromNativeSeparators);
    };

    settings->setValue("filters", fromNativeSeparators(d->m_filterStrings.stringList()));
    if (d->m_filterCombo)
        settings->setValue("currentFilter",
                           QDir::fromNativeSeparators(d->m_filterCombo->currentText()));
    settings->setValue("exclusionFilters", fromNativeSeparators(d->m_exclusionStrings.stringList()));
    if (d->m_exclusionCombo)
        settings->setValue("currentExclusionFilter",
                           QDir::fromNativeSeparators(d->m_exclusionCombo->currentText()));

    for (const SearchEngine *searchEngine : std::as_const(d->m_searchEngines))
        searchEngine->writeSettings(settings);
    settings->setValue("currentSearchEngineIndex", d->m_currentSearchEngineIndex);
}

void BaseFileFind::readCommonSettings(QtcSettings *settings, const QString &defaultFilter,
                                      const QString &defaultExclusionFilter)
{
    const auto toNativeSeparators = [](const QStringList &files) -> QStringList {
        return Utils::transform(files, &QDir::toNativeSeparators);
    };

    const QStringList filterSetting = settings->value("filters").toStringList();
    const QStringList filters = filterSetting.isEmpty() ? QStringList(defaultFilter)
                                                        : filterSetting;
    const QVariant currentFilter = settings->value("currentFilter");
    d->m_filterSetting = currentFilter.isValid() ? currentFilter.toString()
                                                 : filters.first();
    d->m_filterStrings.setStringList(toNativeSeparators(filters));
    if (d->m_filterCombo)
        syncComboWithSettings(d->m_filterCombo, d->m_filterSetting);

    QStringList exclusionFilters = settings->value("exclusionFilters").toStringList();
    if (!exclusionFilters.contains(defaultExclusionFilter))
        exclusionFilters << defaultExclusionFilter;
    const QVariant currentExclusionFilter = settings->value("currentExclusionFilter");
    d->m_exclusionSetting = currentExclusionFilter.isValid() ? currentExclusionFilter.toString()
                                                          : exclusionFilters.first();
    d->m_exclusionStrings.setStringList(toNativeSeparators(exclusionFilters));
    if (d->m_exclusionCombo)
        syncComboWithSettings(d->m_exclusionCombo, d->m_exclusionSetting);

    for (SearchEngine* searchEngine : std::as_const(d->m_searchEngines))
        searchEngine->readSettings(settings);
    const int currentSearchEngineIndex = settings->value("currentSearchEngineIndex", 0).toInt();
    syncSearchEngineCombo(currentSearchEngineIndex);
}

void BaseFileFind::openEditor(SearchResult *result, const SearchResultItem &item)
{
    const FileFindParameters parameters = result->userData().value<FileFindParameters>();
    IEditor *openedEditor = parameters.editorOpener ? parameters.editorOpener(item, parameters)
                                                    : nullptr;
    if (!openedEditor)
        EditorManager::openEditorAtSearchResult(item, Id(), EditorManager::DoNotSwitchToDesignMode);
    if (d->m_currentFindSupport)
        d->m_currentFindSupport->clearHighlights();
    d->m_currentFindSupport = nullptr;
    if (!openedEditor)
        return;
    // highlight results
    if (auto findSupport = Aggregation::query<IFindSupport>(openedEditor->widget())) {
        d->m_currentFindSupport = findSupport;
        d->m_currentFindSupport->highlightAll(parameters.text, parameters.flags);
    }
}

void BaseFileFind::hideHighlightAll(bool visible)
{
    if (!visible && d->m_currentFindSupport)
        d->m_currentFindSupport->clearHighlights();
}

void BaseFileFind::searchAgain(SearchResult *search)
{
    search->restart();
    runSearch(search);
}

void BaseFileFind::setupSearch(SearchResult *search)
{
    connect(this, &IFindFilter::enabledChanged, search, [this, search] {
        search->setSearchAgainEnabled(isEnabled());
    });
}

FilePaths BaseFileFind::replaceAll(const QString &text, const SearchResultItems &items,
                                   bool preserveCase)
{
    if (items.isEmpty())
        return {};

    RefactoringChanges refactoring;

    QHash<FilePath, SearchResultItems> changes;
    for (const SearchResultItem &item : items)
        changes[FilePath::fromUserInput(item.path().constFirst())].append(item);

    // Checking for files without write permissions
    QSet<FilePath> roFiles;
    for (auto it = changes.cbegin(), end = changes.cend(); it != end; ++it) {
        if (!it.key().isWritableFile())
            roFiles.insert(it.key());
    }

    // Query the user for permissions
    if (!roFiles.isEmpty()) {
        ReadOnlyFilesDialog roDialog(Utils::toList(roFiles), ICore::dialogParent());
        roDialog.setShowFailWarning(true, Tr::tr("Aborting replace."));
        if (roDialog.exec() == ReadOnlyFilesDialog::RO_Cancel)
            return {};
    }

    for (auto it = changes.cbegin(), end = changes.cend(); it != end; ++it) {
        const FilePath filePath = it.key();
        const SearchResultItems changeItems = it.value();

        ChangeSet changeSet;
        RefactoringFilePtr file = refactoring.file(filePath);
        QSet<QPair<int, int>> processed;
        for (const SearchResultItem &item : changeItems) {
            const QPair<int, int> p{item.mainRange().begin.line, item.mainRange().begin.column};
            if (!Utils::insert(processed, p))
                continue;

            QString replacement;
            if (item.userData().canConvert<QStringList>() && !item.userData().toStringList().isEmpty()) {
                replacement = Utils::expandRegExpReplacement(text, item.userData().toStringList());
            } else if (preserveCase) {
                Text::Range range = item.mainRange();
                range.end.line -= range.begin.line - 1;
                range.begin.line = 1;
                QString originalText = item.lineText();
                const int rangeLength = range.length(item.lineText());
                if (rangeLength > 0)
                    originalText = originalText.mid(range.begin.column, rangeLength);
                replacement = Utils::matchCaseReplacement(originalText, text);
            } else {
                replacement = text;
            }

            const int start = file->position(item.mainRange().begin.line,
                                             item.mainRange().begin.column + 1);
            const int end = file->position(item.mainRange().end.line,
                                           item.mainRange().end.column + 1);
            changeSet.replace(start, end, replacement);
        }
        file->setChangeSet(changeSet);
        file->apply();
    }

    return changes.keys();
}

namespace Internal {

} // namespace Internal
} // namespace TextEditor
