// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesearch.h"

#include "algorithm.h"
#include "filepath.h"
#include "mapreduce.h"
#include "qtcassert.h"
#include "searchresultitem.h"
#include "stringutils.h"
#include "utilstr.h"
#include "utiltypes.h"

#include <QLoggingCategory>
#include <QMutex>
#include <QRegularExpression>
#include <QTextCodec>

#include <cctype>

Q_LOGGING_CATEGORY(searchLog, "qtc.utils.filesearch", QtWarningMsg)

using namespace Utils;

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

namespace {

const int MAX_LINE_SIZE = 400;

QString clippedText(const QString &text, int maxLength)
{
    if (text.length() > maxLength)
        return text.left(maxLength) + QChar(0x2026); // '...'
    return text;
}

// returns success
static bool getFileContent(const FilePath &filePath,
                           QTextCodec *encoding,
                           QString *tempString,
                           const QMap<FilePath, QString> &fileToContentsMap)
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

class FileSearch
{
public:
    FileSearch(const QString &searchTerm,
               QTextDocument::FindFlags flags,
               const QMap<FilePath, QString> &fileToContentsMap);
    void operator()(QFutureInterface<SearchResultItems> &futureInterface,
                    const FileIterator::Item &item) const;

private:
    QMap<FilePath, QString> fileToContentsMap;
    QString searchTermLower;
    QString searchTermUpper;
    int termMaxIndex;
    const QChar *termData;
    const QChar *termDataLower;
    const QChar *termDataUpper;
    bool caseSensitive;
    bool wholeWord;
};

class FileSearchRegExp
{
public:
    FileSearchRegExp(const QString &searchTerm,
                     QTextDocument::FindFlags flags,
                     const QMap<FilePath, QString> &fileToContentsMap);
    FileSearchRegExp(const FileSearchRegExp &other);
    void operator()(QFutureInterface<SearchResultItems> &futureInterface,
                    const FileIterator::Item &item) const;

private:
    QRegularExpressionMatch doGuardedMatch(const QString &line, int offset) const;

    QMap<FilePath, QString> fileToContentsMap;
    QRegularExpression expression;
    mutable QMutex mutex;
};

FileSearch::FileSearch(const QString &searchTerm,
                       QTextDocument::FindFlags flags,
                       const QMap<FilePath, QString> &fileToContentsMap)
{
    this->fileToContentsMap = fileToContentsMap;
    caseSensitive = (flags & QTextDocument::FindCaseSensitively);
    wholeWord = (flags & QTextDocument::FindWholeWords);
    searchTermLower = searchTerm.toLower();
    searchTermUpper = searchTerm.toUpper();
    termMaxIndex = searchTerm.length() - 1;
    termData = searchTerm.constData();
    termDataLower = searchTermLower.constData();
    termDataUpper = searchTermUpper.constData();
}

void FileSearch::operator()(QFutureInterface<SearchResultItems> &futureInterface,
                            const FileIterator::Item &item) const
{
    if (futureInterface.isCanceled())
        return;
    qCDebug(searchLog) << "Searching in" << item.filePath;
    futureInterface.setProgressRange(0, 1);
    futureInterface.setProgressValue(0);
    SearchResultItems results;
    QString tempString;
    if (!getFileContent(item.filePath, item.encoding, &tempString, fileToContentsMap)) {
        qCDebug(searchLog) << "- failed to get content for" << item.filePath;
        futureInterface.cancel(); // failure
        return;
    }
    QTextStream stream(&tempString);
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
                    result.setFilePath(item.filePath);
                    result.setMainRange(lineNr, regionPtr - chunkPtr, termMaxIndex + 1);
                    result.setDisplayText(clippedText(chunk, MAX_LINE_SIZE));
                    result.setUserData(QStringList());
                    result.setUseTextEditorFont(true);
                    results << result;
                    regionPtr += termMaxIndex; // another +1 done by for-loop
                }
            }
        }
        if (futureInterface.isPaused())
            futureInterface.waitForResume();
        if (futureInterface.isCanceled())
            break;
    }
    if (!futureInterface.isCanceled()) {
        futureInterface.reportResult(results);
        futureInterface.setProgressValue(1);
    }
    qCDebug(searchLog) << "- finished searching in" << item.filePath;
}

FileSearchRegExp::FileSearchRegExp(const QString &searchTerm,
                                   QTextDocument::FindFlags flags,
                                   const QMap<FilePath, QString> &fileToContentsMap)
{
    this->fileToContentsMap = fileToContentsMap;
    QString term = searchTerm;
    if (flags & QTextDocument::FindWholeWords)
        term = QString::fromLatin1("\\b%1\\b").arg(term);
    const QRegularExpression::PatternOptions patternOptions = (flags & QTextDocument::FindCaseSensitively)
            ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption;
    expression = QRegularExpression(term, patternOptions);
}

FileSearchRegExp::FileSearchRegExp(const FileSearchRegExp &other)
    : fileToContentsMap(other.fileToContentsMap),
      expression(other.expression)
{
}

QRegularExpressionMatch FileSearchRegExp::doGuardedMatch(const QString &line, int offset) const
{
    QMutexLocker lock(&mutex);
    return expression.match(line, offset);
}

void FileSearchRegExp::operator()(QFutureInterface<SearchResultItems> &futureInterface,
                                  const FileIterator::Item &item) const
{
    if (!expression.isValid()) {
        futureInterface.cancel();
        return;
    }
    if (futureInterface.isCanceled())
        return;
    qCDebug(searchLog) << "Searching in" << item.filePath;
    futureInterface.setProgressRange(0, 1);
    futureInterface.setProgressValue(0);
    SearchResultItems results;
    QString tempString;
    if (!getFileContent(item.filePath, item.encoding, &tempString, fileToContentsMap)) {
        qCDebug(searchLog) << "- failed to get content for" << item.filePath;
        futureInterface.cancel(); // failure
        return;
    }
    QTextStream stream(&tempString);
    int lineNr = 0;

    QString line;
    QRegularExpressionMatch match;
    while (!stream.atEnd()) {
        ++lineNr;
        line = stream.readLine();
        const QString resultItemText = clippedText(line, MAX_LINE_SIZE);
        int lengthOfLine = line.size();
        int pos = 0;
        while ((match = doGuardedMatch(line, pos)).hasMatch()) {
            pos = match.capturedStart();
            SearchResultItem result;
            result.setFilePath(item.filePath);
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
        if (futureInterface.isPaused())
            futureInterface.waitForResume();
        if (futureInterface.isCanceled())
            break;
    }
    if (!futureInterface.isCanceled()) {
        futureInterface.reportResult(results);
        futureInterface.setProgressValue(1);
    }
    qCDebug(searchLog) << "- finished searching in" << item.filePath;
}

struct SearchState
{
    SearchState(const QString &term, FileIterator *iterator) : searchTerm(term), files(iterator) {}
    QString searchTerm;
    FileIterator *files = nullptr;
    SearchResultItems cachedResults;
    int numFilesSearched = 0;
    int numMatches = 0;
};

SearchState initFileSearch(QFutureInterface<SearchResultItems> &futureInterface,
                           const QString &searchTerm, FileIterator *files)
{
    futureInterface.setProgressRange(0, files->maxProgress());
    futureInterface.setProgressValueAndText(files->currentProgress(), msgFound(searchTerm, 0, 0));
    return SearchState(searchTerm, files);
}

void collectSearchResults(QFutureInterface<SearchResultItems> &futureInterface,
                          SearchState &state,
                          const SearchResultItems &results)
{
    state.numMatches += results.size();
    state.cachedResults << results;
    state.numFilesSearched += 1;
    if (futureInterface.isProgressUpdateNeeded()
            || futureInterface.progressValue() == 0 /*workaround for regression in Qt*/) {
        if (!state.cachedResults.isEmpty()) {
            futureInterface.reportResult(state.cachedResults);
            state.cachedResults.clear();
        }
        futureInterface.setProgressRange(0, state.files->maxProgress());
        futureInterface.setProgressValueAndText(state.files->currentProgress(),
                                                 msgFound(state.searchTerm,
                                                          state.numMatches,
                                                          state.numFilesSearched));
    }
}

void cleanUpFileSearch(QFutureInterface<SearchResultItems> &futureInterface,
                       SearchState &state)
{
    if (!state.cachedResults.isEmpty()) {
        futureInterface.reportResult(state.cachedResults);
        state.cachedResults.clear();
    }
    if (futureInterface.isCanceled()) {
        futureInterface.setProgressValueAndText(state.files->currentProgress(),
                                                msgCanceled(state.searchTerm,
                                                            state.numMatches,
                                                            state.numFilesSearched));
    } else {
        futureInterface.setProgressValueAndText(state.files->currentProgress(),
                                                msgFound(state.searchTerm,
                                                         state.numMatches,
                                                         state.numFilesSearched));
    }
    delete state.files;
}

} // namespace

QFuture<SearchResultItems> Utils::findInFiles(const QString &searchTerm,
                                              FileIterator *files,
                                              QTextDocument::FindFlags flags,
                                              const QMap<FilePath, QString> &fileToContentsMap)
{
    return mapReduce(files->begin(), files->end(),
                     [searchTerm, files](QFutureInterface<SearchResultItems> &futureInterface) {
                         return initFileSearch(futureInterface, searchTerm, files);
                     },
                     FileSearch(searchTerm, flags, fileToContentsMap),
                     &collectSearchResults,
                     &cleanUpFileSearch);
}

QFuture<SearchResultItems> Utils::findInFilesRegExp(
    const QString &searchTerm,
    FileIterator *files,
    QTextDocument::FindFlags flags,
    const QMap<FilePath, QString> &fileToContentsMap)
{
    return mapReduce(files->begin(), files->end(),
                     [searchTerm, files](QFutureInterface<SearchResultItems> &futureInterface) {
                         return initFileSearch(futureInterface, searchTerm, files);
                     },
                     FileSearchRegExp(searchTerm, flags, fileToContentsMap),
                     &collectSearchResults,
                     &cleanUpFileSearch);
}

QString Utils::expandRegExpReplacement(const QString &replaceText, const QStringList &capturedTexts)
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

namespace Utils {
namespace Internal {
QString matchCaseReplacement(const QString &originalText, const QString &replaceText)
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
} // namespace

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

FilePathPredicate filterFileFunction(const QStringList &filters, const QStringList &exclusionFilters)
{
    const QList<QRegularExpression> filterRegs = filtersToRegExps(filters);
    const QList<QRegularExpression> exclusionRegs = filtersToRegExps(exclusionFilters);
    return [filterRegs, exclusionRegs](const FilePath &filePath) {
        return isFileIncluded(filterRegs, exclusionRegs, filePath);
    };
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

QString msgFilePatternToolTip()
{
    return Tr::tr("List of comma separated wildcard filters. "
                  "Files with file name or full file path matching any filter are included.");
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
            + Internal::matchCaseReplacement(originalText.mid(prefixLen, originalTextLen - prefixLen - suffixLen),
                                             replaceText.mid(prefixLen, replaceTextLen - prefixLen - suffixLen))
            + originalText.right(suffixLen);

}

// #pragma mark -- FileIterator

void FileIterator::advance(FileIterator::const_iterator *it) const
{
    if (it->m_index < 0) // == end
        return;
    ++it->m_index;
    const_cast<FileIterator *>(this)->update(it->m_index);
    if (it->m_index >= currentFileCount())
        it->m_index = -1; // == end
}

FileIterator::const_iterator FileIterator::begin() const
{
    const_cast<FileIterator *>(this)->update(0);
    if (currentFileCount() == 0)
        return end();
    return FileIterator::const_iterator(this, 0/*index*/);
}

FileIterator::const_iterator FileIterator::end() const
{
    return FileIterator::const_iterator(this, -1/*end*/);
}

// #pragma mark -- FileListIterator

QList<FileIterator::Item> constructItems(const FilePaths &fileList,
                                         const QList<QTextCodec *> &encodings)
{
    QList<FileIterator::Item> items;
    items.reserve(fileList.size());
    QTextCodec *defaultEncoding = QTextCodec::codecForLocale();
    for (int i = 0; i < fileList.size(); ++i)
        items.append(FileIterator::Item(fileList.at(i), encodings.value(i, defaultEncoding)));
    return items;
}

FileListIterator::FileListIterator(const FilePaths &fileList, const QList<QTextCodec *> &encodings)
    : m_items(constructItems(fileList, encodings))
{
}

void FileListIterator::update(int requestedIndex)
{
    if (requestedIndex > m_maxIndex)
        m_maxIndex = requestedIndex;
}

int FileListIterator::currentFileCount() const
{
    return m_items.size();
}

const FileIterator::Item &FileListIterator::itemAt(int index) const
{
    return m_items.at(index);
}

int FileListIterator::maxProgress() const
{
    return m_items.size();
}

int FileListIterator::currentProgress() const
{
    return m_maxIndex + 1;
}

// #pragma mark -- SubDirFileIterator

namespace {
    const int MAX_PROGRESS = 1000;
}

SubDirFileIterator::SubDirFileIterator(const FilePaths &directories,
                                       const QStringList &filters,
                                       const QStringList &exclusionFilters,
                                       QTextCodec *encoding)
    : m_filterFiles(filterFilesFunction(filters, exclusionFilters))
    , m_progress(0)
{
    m_encoding = (encoding == nullptr ? QTextCodec::codecForLocale() : encoding);
    qreal maxPer = qreal(MAX_PROGRESS) / directories.count();
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

SubDirFileIterator::~SubDirFileIterator()
{
    qDeleteAll(m_items);
}

void SubDirFileIterator::update(int index)
{
    if (index < m_items.size())
        return;
    // collect files from the directories until we have enough for the given index
    while (!m_dirs.isEmpty() && index >= m_items.size()) {
        FilePath dir = m_dirs.pop();
        const qreal dirProgressMax = m_progressValues.pop();
        const bool processed = m_processedValues.pop();
        if (dir.exists()) {
            using Dir = FilePath;
            using CanonicalDir = FilePath;
            std::vector<std::pair<Dir, CanonicalDir>> subDirs;
            if (!processed) {
                for (const FilePath &entry :
                     dir.dirEntries(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot)) {
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
                    m_items.append(new Item(file, m_encoding));
                });
                m_progress += dirProgressMax;
            } else {
                qreal subProgress = dirProgressMax/(subDirs.size()+1);
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
    if (index >= m_items.size())
        m_progress = MAX_PROGRESS;
}

int SubDirFileIterator::currentFileCount() const
{
    return m_items.size();
}

const FileIterator::Item &SubDirFileIterator::itemAt(int index) const
{
    return *m_items.at(index);
}

int SubDirFileIterator::maxProgress() const
{
    return MAX_PROGRESS;
}

int SubDirFileIterator::currentProgress() const
{
    return qMin(qRound(m_progress), MAX_PROGRESS);
}

}
