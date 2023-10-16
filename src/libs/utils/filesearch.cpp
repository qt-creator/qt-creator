// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesearch.h"

#include "algorithm.h"
#include "async.h"
#include "qtcassert.h"
#include "stringutils.h"
#include "utilstr.h"

#include <QLoggingCategory>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QTextCodec>

Q_LOGGING_CATEGORY(searchLog, "qtc.utils.filesearch", QtWarningMsg)

namespace Utils {

const int MAX_LINE_SIZE = 400;

static QString clippedText(const QString &text, int maxLength)
{
    if (text.length() > maxLength)
        return text.left(maxLength) + QChar(0x2026); // '...'
    return text;
}

QTextDocument::FindFlags textDocumentFlagsForFindFlags(FindFlags flags)
{
    QTextDocument::FindFlags textDocFlags;
    if (flags & FindBackward)
        textDocFlags |= QTextDocument::FindBackward;
    if (flags & FindCaseSensitively)
        textDocFlags |= QTextDocument::FindCaseSensitively;
    if (flags & FindWholeWords)
        textDocFlags |= QTextDocument::FindWholeWords;
    return textDocFlags;
}

static SearchResultItems searchWithoutRegExp(const QFuture<void> &future, const QString &searchTerm,
                                             FindFlags flags, const FilePath &filePath,
                                             const QString &contents)
{
    const bool caseSensitive = (flags & QTextDocument::FindCaseSensitively);
    const bool wholeWord = (flags & QTextDocument::FindWholeWords);
    const QString searchTermLower = searchTerm.toLower();
    const QString searchTermUpper = searchTerm.toUpper();
    const int termMaxIndex = searchTerm.length() - 1;
    const QChar *termData = searchTerm.constData();
    const QChar *termDataLower = searchTermLower.constData();
    const QChar *termDataUpper = searchTermUpper.constData();

    SearchResultItems results;
    QString copy = contents;
    QTextStream stream(&copy);
    int lineNr = 0;

    while (!stream.atEnd()) {
        ++lineNr;
        const QString chunk = stream.readLine();
        const int chunkLength = chunk.length();
        const QChar *chunkPtr = chunk.constData();
        const QChar *chunkEnd = chunkPtr + chunkLength - 1;
        for (const QChar *regionPtr = chunkPtr; regionPtr + termMaxIndex <= chunkEnd; ++regionPtr) {
            const QChar *regionEnd = regionPtr + termMaxIndex;
            if ( /* optimization check for start and end of region */
                // case sensitive
                (caseSensitive && *regionPtr == termData[0]
                 && *regionEnd == termData[termMaxIndex])
                ||
                // case insensitive
                (!caseSensitive && (*regionPtr == termDataLower[0]
                                    || *regionPtr == termDataUpper[0])
                 && (*regionEnd == termDataLower[termMaxIndex]
                     || *regionEnd == termDataUpper[termMaxIndex]))
                ) {
                bool equal = true;

                // whole word check
                const QChar *beforeRegion = regionPtr - 1;
                const QChar *afterRegion = regionEnd + 1;
                if (wholeWord
                    && (((beforeRegion >= chunkPtr)
                         && (beforeRegion->isLetterOrNumber()
                             || ((*beforeRegion) == QLatin1Char('_'))))
                        ||
                        ((afterRegion <= chunkEnd)
                         && (afterRegion->isLetterOrNumber()
                             || ((*afterRegion) == QLatin1Char('_'))))
                        )) {
                    equal = false;
                } else {
                    // check all chars
                    int regionIndex = 1;
                    for (const QChar *regionCursor = regionPtr + 1;
                         regionCursor < regionEnd;
                         ++regionCursor, ++regionIndex) {
                        if (  // case sensitive
                            (caseSensitive
                             && *regionCursor != termData[regionIndex])
                            ||
                            // case insensitive
                            (!caseSensitive
                             && *regionCursor != termDataLower[regionIndex]
                             && *regionCursor != termDataUpper[regionIndex])
                            ) {
                            equal = false;
                            break;
                        }
                    }
                }
                if (equal) {
                    SearchResultItem result;
                    result.setFilePath(filePath);
                    result.setMainRange(lineNr, regionPtr - chunkPtr, termMaxIndex + 1);
                    result.setDisplayText(clippedText(chunk, MAX_LINE_SIZE));
                    result.setUserData(QStringList());
                    result.setUseTextEditorFont(true);
                    results << result;
                    regionPtr += termMaxIndex; // another +1 done by for-loop
                }
            }
        }
        if (future.isCanceled())
            return {};
    }
    if (future.isCanceled())
        return {};
    return results;
}

static SearchResultItems searchWithRegExp(const QFuture<void> &future, const QString &searchTerm,
                                          FindFlags flags, const FilePath &filePath,
                                          const QString &contents)
{
    const QString term = flags & QTextDocument::FindWholeWords
        ? QString::fromLatin1("\\b%1\\b").arg(searchTerm) : searchTerm;
    const QRegularExpression::PatternOptions patternOptions = (flags & FindCaseSensitively)
        ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption;
    const QRegularExpression expression = QRegularExpression(term, patternOptions);
    if (!expression.isValid()) {
        QFuture<void> nonConstFuture = future;
        nonConstFuture.cancel();
        return {};
    }

    SearchResultItems results;
    QString copy = contents;
    QTextStream stream(&copy);
    int lineNr = 0;

    QRegularExpressionMatch match;
    while (!stream.atEnd()) {
        ++lineNr;
        const QString line = stream.readLine();
        const QString resultItemText = clippedText(line, MAX_LINE_SIZE);
        int lengthOfLine = line.size();
        int pos = 0;
        while ((match = expression.match(line, pos)).hasMatch()) {
            pos = match.capturedStart();
            SearchResultItem result;
            result.setFilePath(filePath);
            result.setMainRange(lineNr, pos, match.capturedLength());
            result.setDisplayText(resultItemText);
            result.setUserData(match.capturedTexts());
            result.setUseTextEditorFont(true);
            results << result;
            if (match.capturedLength() == 0)
                break;
            pos += match.capturedLength();
            if (pos >= lengthOfLine)
                break;
        }
        if (future.isCanceled())
            return {};
    }
    if (future.isCanceled())
        return {};
    return results;
}

static SearchResultItems searchInContents(const QFuture<void> &future, const QString &searchTerm,
                                          FindFlags flags, const FilePath &filePath,
                                          const QString &contents)
{
    if (flags & FindRegularExpression)
        return searchWithRegExp(future, searchTerm, flags, filePath, contents);
    return searchWithoutRegExp(future, searchTerm, flags, filePath, contents);
}

void searchInContents(QPromise<SearchResultItems> &promise, const QString &searchTerm,
                      FindFlags flags, const FilePath &filePath, const QString &contents)
{
    const QFuture<void> future(promise.future());
    const SearchResultItems results = searchInContents(future, searchTerm, flags, filePath,
                                                       contents);
    if (!promise.isCanceled())
        promise.addResult(results);
}

static inline QString msgCanceled(const QString &searchTerm, int numMatches, int numFilesSearched)
{
    return Tr::tr("%1: canceled. %n occurrences found in %2 files.",
                  nullptr, numMatches).arg(searchTerm).arg(numFilesSearched);
}

static inline QString msgFound(const QString &searchTerm, int numMatches, int numFilesSearched)
{
    return Tr::tr("%1: %n occurrences found in %2 files.",
                  nullptr, numMatches).arg(searchTerm).arg(numFilesSearched);
}

static bool getFileContent(const FilePath &filePath, QTextCodec *encoding,
                           QString *tempString, const QMap<FilePath, QString> &fileToContentsMap)
{
    if (fileToContentsMap.contains(filePath)) {
        *tempString = fileToContentsMap.value(filePath);
    } else {
        const expected_str<QByteArray> content = filePath.fileContents();
        if (!content)
            return false;
        *tempString = QTC_GUARD(encoding) ? encoding->toUnicode(*content)
                                          : QTextCodec::codecForLocale()->toUnicode(*content);
    }
    return true;
}

static void fileSearch(QPromise<SearchResultItems> &promise,
                       const FileContainerIterator::Item &item, const QString &searchTerm,
                       FindFlags flags, const QMap<FilePath, QString> &fileToContentsMap)
{
    if (promise.isCanceled())
        return;
    qCDebug(searchLog) << "Searching in" << item.filePath;
    promise.setProgressRange(0, 1);
    promise.setProgressValue(0);
    QString contents;
    if (!getFileContent(item.filePath, item.encoding, &contents, fileToContentsMap)) {
        qCDebug(searchLog) << "- failed to get content for" << item.filePath;
        promise.future().cancel(); // failure
        return;
    }

    const QFuture<void> future(promise.future());
    const SearchResultItems results = searchInContents(future, searchTerm, flags, item.filePath,
                                                       contents);
    if (!promise.isCanceled()) {
        promise.addResult(results);
        promise.setProgressValue(1);
    }
    qCDebug(searchLog) << "- finished searching in" << item.filePath;
}

static void findInFilesImpl(QPromise<SearchResultItems> &promise, const QString &searchTerm,
                            const FileContainer &container, FindFlags flags,
                            const QMap<FilePath, QString> &fileToContentsMap)
{
    QEventLoop loop;
    // The states transition exactly in this order:
    enum State { BelowLimit, AboveLimit, Resumed };
    State state = BelowLimit;
    int reportedItemsCount = 0;
    int searchedFilesCount = 0;

    const int progressMaximum = container.progressMaximum();
    promise.setProgressRange(0, progressMaximum);
    promise.setProgressValueAndText(0, msgFound(searchTerm, 0, 0));
    const int threadsCount = qMax(1, QThread::idealThreadCount() / 2);
    QSet<QFutureWatcher<SearchResultItems> *> watchers;
    FutureSynchronizer futureSynchronizer;

    const auto cleanup = qScopeGuard([&] {
        qDeleteAll(watchers);
        const QString message = promise.isCanceled()
                              ? msgCanceled(searchTerm, reportedItemsCount, searchedFilesCount)
                              : msgFound(searchTerm, reportedItemsCount, searchedFilesCount);
        promise.setProgressValueAndText(progressMaximum, message);
    });

    FileContainerIterator it = container.begin();
    const FileContainerIterator itEnd = container.end();
    if (it == itEnd)
        return;

    std::function<void()> scheduleNext;
    scheduleNext = [&] {
        if (promise.isCanceled() || (it == itEnd && watchers.isEmpty())) {
            loop.quit();
            return;
        }
        if (it == itEnd)
            return;

        if (state == AboveLimit)
            return;

        if (watchers.size() == threadsCount)
            return;

        const FileContainerIterator::Item item = *it;
        const int progress = it.progressValue();
        ++it;
        const QFuture<SearchResultItems> future
            = Utils::asyncRun(fileSearch, item, searchTerm, flags, fileToContentsMap);
        QFutureWatcher<SearchResultItems> *watcher = new QFutureWatcher<SearchResultItems>;
        QObject::connect(watcher, &QFutureWatcherBase::finished, &loop,
                [watcher, progress, &searchTerm, &reportedItemsCount, &searchedFilesCount, &state,
                 &watchers, &promise, &scheduleNext] {
            const QFuture<SearchResultItems> future = watcher->future();
            if (future.resultCount()) {
                const SearchResultItems items = future.result();
                reportedItemsCount += items.size();
                if (state == BelowLimit && reportedItemsCount > 200000)
                    state = AboveLimit;
                if (!items.isEmpty())
                    promise.addResult(items);
            }
            ++searchedFilesCount;
            promise.setProgressValueAndText(progress, msgFound(searchTerm, reportedItemsCount,
                                                               searchedFilesCount));
            watcher->deleteLater();
            watchers.remove(watcher);
            scheduleNext();
        });
        watcher->setFuture(future);
        futureSynchronizer.addFuture(future);
        watchers.insert(watcher);
        scheduleNext();
    };

    QFutureWatcher<void> watcher;
    QObject::connect(&watcher, &QFutureWatcherBase::canceled, &loop, &QEventLoop::quit);
    QObject::connect(&watcher, &QFutureWatcherBase::resumed, &loop, [&state, &scheduleNext] {
        state = Resumed;
        scheduleNext();
    });

    watcher.setFuture(QFuture<void>(promise.future()));

    if (promise.isCanceled())
        return;

    QTimer::singleShot(0, &loop, scheduleNext);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

QFuture<SearchResultItems> findInFiles(const QString &searchTerm, const FileContainer &container,
                                       FindFlags flags,
                                       const QMap<FilePath, QString> &fileToContentsMap)
{
    return Utils::asyncRun(findInFilesImpl, searchTerm, container, flags, fileToContentsMap);
}

QString expandRegExpReplacement(const QString &replaceText, const QStringList &capturedTexts)
{
    // handles \1 \\ \& \t \n $1 $$ $&
    QString result;
    const int numCaptures = capturedTexts.size() - 1;
    const int replaceLength = replaceText.length();
    for (int i = 0; i < replaceLength; ++i) {
        QChar c = replaceText.at(i);
        if (c == QLatin1Char('\\') && i < replaceLength - 1) {
            c = replaceText.at(++i);
            if (c == QLatin1Char('\\')) {
                result += QLatin1Char('\\');
            } else if (c == QLatin1Char('&')) {
                result += QLatin1Char('&');
            } else if (c == QLatin1Char('t')) {
                result += QLatin1Char('\t');
            } else if (c == QLatin1Char('n')) {
                result += QLatin1Char('\n');
            } else if (c.isDigit()) {
                int index = c.unicode()-'1';
                if (index < numCaptures) {
                    result += capturedTexts.at(index+1);
                } // else add nothing
            } else {
                result += QLatin1Char('\\');
                result += c;
            }
        } else if (c== '$' && i < replaceLength - 1) {
            c = replaceText.at(++i);
            if (c == '$') {
                result += '$';
            } else if (c == '&') {
                result += capturedTexts.at(0);
            } else if (c.isDigit()) {
                int index = c.unicode()-'1';
                if (index < numCaptures) {
                    result += capturedTexts.at(index+1);
                } // else add nothing
            } else {
                result += '$';
                result += c;
            }
        } else {
            result += c;
        }
    }
    return result;
}

static QString matchCaseReplacementHelper(const QString &originalText, const QString &replaceText)
{
    if (originalText.isEmpty() || replaceText.isEmpty())
       return replaceText;

    //Now proceed with actual case matching
    bool firstIsUpperCase = originalText.at(0).isUpper();
    bool firstIsLowerCase = originalText.at(0).isLower();
    bool restIsLowerCase = true; // to be verified
    bool restIsUpperCase = true; // to be verified

    for (int i = 1; i < originalText.length(); ++i) {
        if (originalText.at(i).isUpper())
            restIsLowerCase = false;
        else if (originalText.at(i).isLower())
            restIsUpperCase = false;

        if (!restIsLowerCase && !restIsUpperCase)
            break;
    }

    if (restIsLowerCase) {
        QString res = replaceText.toLower();
        if (firstIsUpperCase)
            res.replace(0, 1, res.at(0).toUpper());
        return res;
    } else if (restIsUpperCase) {
        QString res = replaceText.toUpper();
        if (firstIsLowerCase)
            res.replace(0, 1, res.at(0).toLower());
        return res;
    } else {
        return replaceText; // mixed
    }
}

static QList<QRegularExpression> filtersToRegExps(const QStringList &filters)
{
    return Utils::transform(filters, [](const QString &filter) {
        return QRegularExpression(Utils::wildcardToRegularExpression(filter),
                                  QRegularExpression::CaseInsensitiveOption);
    });
}

static bool matches(const QList<QRegularExpression> &exprList, const FilePath &filePath)
{
    return Utils::anyOf(exprList, [&filePath](const QRegularExpression &reg) {
        return (reg.match(filePath.toString()).hasMatch()
                || reg.match(filePath.fileName()).hasMatch());
    });
}

static bool isFileIncluded(const QList<QRegularExpression> &filterRegs,
                           const QList<QRegularExpression> &exclusionRegs,
                           const FilePath &filePath)
{
    const bool isIncluded = filterRegs.isEmpty() || matches(filterRegs, filePath);
    return isIncluded && (exclusionRegs.isEmpty() || !matches(exclusionRegs, filePath));
}

std::function<FilePaths(const FilePaths &)> filterFilesFunction(const QStringList &filters,
                                                                const QStringList &exclusionFilters)
{
    const QList<QRegularExpression> filterRegs = filtersToRegExps(filters);
    const QList<QRegularExpression> exclusionRegs = filtersToRegExps(exclusionFilters);
    return [filterRegs, exclusionRegs](const FilePaths &filePaths) {
        return Utils::filtered(filePaths, [&filterRegs, &exclusionRegs](const FilePath &filePath) {
            return isFileIncluded(filterRegs, exclusionRegs, filePath);
        });
    };
}

QStringList splitFilterUiText(const QString &text)
{
    const QStringList parts = text.split(',');
    const QStringList trimmedPortableParts = Utils::transform(parts, [](const QString &s) {
        return QDir::fromNativeSeparators(s.trimmed());
    });
    return Utils::filtered(trimmedPortableParts, [](const QString &s) { return !s.isEmpty(); });
}

QString msgFilePatternLabel()
{
    return Tr::tr("Fi&le pattern:");
}

QString msgExclusionPatternLabel()
{
    return Tr::tr("Excl&usion pattern:");
}

QString msgFilePatternToolTip(InclusionType inclusionType)
{
    return Tr::tr("List of comma separated wildcard filters.") + " "
        + (inclusionType == InclusionType::Included
            ? Tr::tr("Files with file name or full file path matching any filter are included.")
            : Tr::tr("Files with file name or full file path matching any filter are excluded."));
}

QString matchCaseReplacement(const QString &originalText, const QString &replaceText)
{
    if (originalText.isEmpty())
        return replaceText;

    //Find common prefix & suffix: these will be unaffected
    const int replaceTextLen = replaceText.length();
    const int originalTextLen = originalText.length();

    int prefixLen = 0;
    for (; prefixLen < replaceTextLen && prefixLen < originalTextLen; ++prefixLen)
        if (replaceText.at(prefixLen).toLower() != originalText.at(prefixLen).toLower())
            break;

    int suffixLen = 0;
    for (; suffixLen < replaceTextLen - prefixLen && suffixLen < originalTextLen - prefixLen; ++suffixLen)
        if (replaceText.at(replaceTextLen - 1 - suffixLen).toLower() != originalText.at(originalTextLen- 1 - suffixLen).toLower())
            break;

    //keep prefix and suffix, and do actual replacement on the 'middle' of the string
    return originalText.left(prefixLen)
            + matchCaseReplacementHelper(originalText.mid(prefixLen, originalTextLen - prefixLen - suffixLen),
                                         replaceText.mid(prefixLen, replaceTextLen - prefixLen - suffixLen))
            + originalText.right(suffixLen);
}

void FileContainerIterator::operator++()
{
    QTC_ASSERT(m_data.m_container, return);
    QTC_ASSERT(m_data.m_index >= 0, return);
    QTC_ASSERT(m_data.m_advancer, return);
    m_data.m_advancer(&m_data);
}

int FileContainerIterator::progressMaximum() const
{
    return m_data.m_container ? m_data.m_container->progressMaximum() : 0;
}

static QList<FileContainerIterator::Item> toFileListCache(const FilePaths &fileList,
                                                          const QList<QTextCodec *> &encodings)
{
    QList<FileContainerIterator::Item> items;
    items.reserve(fileList.size());
    QTextCodec *defaultEncoding = QTextCodec::codecForLocale();
    for (int i = 0; i < fileList.size(); ++i)
        items.append({fileList.at(i), encodings.value(i, defaultEncoding)});
    return items;
}

static FileContainerIterator::Advancer fileListAdvancer(
    const QList<FileContainerIterator::Item> &items)
{
    return [items](FileContainerIterator::Data *iterator) {
        ++iterator->m_index;
        if (iterator->m_index >= items.size() || iterator->m_index < 0) {
            iterator->m_value = {};
            iterator->m_index = -1;
            iterator->m_progressValue = items.size();
            return;
        }
        iterator->m_value = items.at(iterator->m_index);
        iterator->m_progressValue = iterator->m_index;
    };
}

static FileContainer::AdvancerProvider fileListAdvancerProvider(const FilePaths &fileList,
    const QList<QTextCodec *> &encodings)
{
    const auto initialCache = toFileListCache(fileList, encodings);
    return [=] { return fileListAdvancer(initialCache); };
}

FileListContainer::FileListContainer(const FilePaths &fileList,
                                     const QList<QTextCodec *> &encodings)
    : FileContainer(fileListAdvancerProvider(fileList, encodings), fileList.size()) {}

const int s_progressMaximum = 1000;

struct SubDirCache
{
    SubDirCache(const FilePaths &directories, const QStringList &filters,
                const QStringList &exclusionFilters, QTextCodec *encoding);

    std::optional<FileContainerIterator::Item> updateCache(int advanceIntoIndex,
                                                           const SubDirCache &initialCache);

    std::function<FilePaths(const FilePaths &)> m_filterFiles;
    QTextCodec *m_encoding = nullptr;
    QStack<FilePath> m_dirs;
    QSet<FilePath> m_knownDirs;
    QStack<qreal> m_progressValues;
    QStack<bool> m_processedValues;
    qreal m_progress = 0;
    QList<FileContainerIterator::Item> m_items;
    // When forward iterating, we construct some results for the future iterations
    // and keep them in m_items cache. Later, when we iterated over all from the cache,
    // we don't want to keep the cache anymore, so we are clearing it.
    // In order to match the iterator's index with the position inside m_items cache,
    // we need to remember how many items were removed from the cache and subtract
    // this value from the iterator's index when a new advance comes.
    int m_removedItemsCount = 0;
};

SubDirCache::SubDirCache(const FilePaths &directories, const QStringList &filters,
                         const QStringList &exclusionFilters, QTextCodec *encoding)
    : m_filterFiles(filterFilesFunction(filters, exclusionFilters))
    , m_encoding(encoding == nullptr ? QTextCodec::codecForLocale() : encoding)
{
    const qreal maxPer = qreal(s_progressMaximum) / directories.count();
    for (const FilePath &directoryEntry : directories) {
        if (!directoryEntry.isEmpty()) {
            const FilePath canonicalPath = directoryEntry.canonicalPath();
            if (!canonicalPath.isEmpty() && directoryEntry.exists()) {
                m_dirs.push(directoryEntry);
                m_knownDirs.insert(canonicalPath);
                m_progressValues.push(maxPer);
                m_processedValues.push(false);
            }
        }
    }
}

std::optional<FileContainerIterator::Item> SubDirCache::updateCache(int advanceIntoIndex,
    const SubDirCache &initialCache)
{
    QTC_ASSERT(advanceIntoIndex >= 0, return {});
    if (advanceIntoIndex < m_removedItemsCount)
        *this = initialCache; // Regenerate the cache from scratch
    const int currentIndex = advanceIntoIndex - m_removedItemsCount;
    if (currentIndex < m_items.size())
        return m_items.at(currentIndex);

    m_removedItemsCount += m_items.size();
    m_items.clear();
    const int newCurrentIndex = advanceIntoIndex - m_removedItemsCount;

    while (!m_dirs.isEmpty() && newCurrentIndex >= m_items.size()) {
        const FilePath dir = m_dirs.pop();
        const qreal dirProgressMax = m_progressValues.pop();
        const bool processed = m_processedValues.pop();
        if (dir.exists()) {
            using Dir = FilePath;
            using CanonicalDir = FilePath;
            std::vector<std::pair<Dir, CanonicalDir>> subDirs;
            if (!processed) {
                const FilePaths entries = dir.dirEntries(QDir::Dirs | QDir::Hidden
                                                         | QDir::NoDotAndDotDot);
                for (const FilePath &entry : entries) {
                    const FilePath canonicalDir = entry.canonicalPath();
                    if (!m_knownDirs.contains(canonicalDir))
                        subDirs.emplace_back(entry, canonicalDir);
                }
            }
            if (subDirs.empty()) {
                const FilePaths allFilePaths = dir.dirEntries(QDir::Files | QDir::Hidden);
                const FilePaths filePaths = m_filterFiles(allFilePaths);
                m_items.reserve(m_items.size() + filePaths.size());
                Utils::reverseForeach(filePaths, [this](const FilePath &file) {
                    m_items.append({file, m_encoding});
                });
                m_progress += dirProgressMax;
            } else {
                const qreal subProgress = dirProgressMax / (subDirs.size() + 1);
                m_dirs.push(dir);
                m_progressValues.push(subProgress);
                m_processedValues.push(true);
                Utils::reverseForeach(subDirs,
                                      [this, subProgress](const std::pair<Dir, CanonicalDir> &dir) {
                                          m_dirs.push(dir.first);
                                          m_knownDirs.insert(dir.second);
                                          m_progressValues.push(subProgress);
                                          m_processedValues.push(false);
                                      });
            }
        } else {
            m_progress += dirProgressMax;
        }
    }
    if (newCurrentIndex < m_items.size())
        return m_items.at(newCurrentIndex);

    m_progress = s_progressMaximum;
    return {};
}

static FileContainerIterator::Advancer subDirAdvancer(const SubDirCache &initialCache)
{
    const std::shared_ptr<SubDirCache> sharedCache(new SubDirCache(initialCache));
    return [=](FileContainerIterator::Data *iterator) {
        ++iterator->m_index;
        const std::optional<FileContainerIterator::Item> item
            = sharedCache->updateCache(iterator->m_index, initialCache);
        if (!item) {
            iterator->m_value = {};
            iterator->m_index = -1;
            iterator->m_progressValue = s_progressMaximum;
            return;
        }
        iterator->m_value = *item;
        iterator->m_progressValue = qMin(qRound(sharedCache->m_progress), s_progressMaximum);
    };
}

static FileContainer::AdvancerProvider subDirAdvancerProvider(const FilePaths &directories,
    const QStringList &filters, const QStringList &exclusionFilters, QTextCodec *encoding)
{
    const SubDirCache initialCache(directories, filters, exclusionFilters, encoding);
    return [=] { return subDirAdvancer(initialCache); };
}

SubDirFileContainer::SubDirFileContainer(const FilePaths &directories, const QStringList &filters,
                                         const QStringList &exclusionFilters, QTextCodec *encoding)
    : FileContainer(subDirAdvancerProvider(directories, filters, exclusionFilters, encoding),
                    s_progressMaximum) {}

} // namespace Utils
