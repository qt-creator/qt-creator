/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "filesearch.h"
#include "mapreduce.h"

#include <QCoreApplication>
#include <QMutex>
#include <QRegularExpression>
#include <QTextCodec>

#include <cctype>

using namespace Utils;

static inline QString msgCanceled(const QString &searchTerm, int numMatches, int numFilesSearched)
{
    return QCoreApplication::translate("Utils::FileSearch",
                                       "%1: canceled. %n occurrences found in %2 files.",
                                       0, numMatches).arg(searchTerm).arg(numFilesSearched);
}

static inline QString msgFound(const QString &searchTerm, int numMatches, int numFilesSearched)
{
    return QCoreApplication::translate("Utils::FileSearch",
                                       "%1: %n occurrences found in %2 files.",
                                       0, numMatches).arg(searchTerm).arg(numFilesSearched);
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
bool openStream(const QString &filePath, QTextCodec *encoding, QTextStream *stream, QFile *file,
                QString *tempString,
                const QMap<QString, QString> &fileToContentsMap)
{
    if (fileToContentsMap.contains(filePath)) {
        *tempString = fileToContentsMap.value(filePath);
        stream->setString(tempString);
    } else {
        file->setFileName(filePath);
        if (!file->open(QIODevice::ReadOnly))
            return false;
        stream->setDevice(file);
        stream->setCodec(encoding);
    }
    return true;
}

class FileSearch
{
public:
    FileSearch(const QString &searchTerm, QTextDocument::FindFlags flags,
               QMap<QString, QString> fileToContentsMap);
    void operator()(QFutureInterface<FileSearchResultList> &futureInterface,
                    const FileIterator::Item &item) const;

private:
    QMap<QString, QString> fileToContentsMap;
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
    FileSearchRegExp(const QString &searchTerm, QTextDocument::FindFlags flags,
                     QMap<QString, QString> fileToContentsMap);
    FileSearchRegExp(const FileSearchRegExp &other);
    void operator()(QFutureInterface<FileSearchResultList> &futureInterface,
                    const FileIterator::Item &item) const;

private:
    QRegularExpressionMatch doGuardedMatch(const QString &line, int offset) const;

    QMap<QString, QString> fileToContentsMap;
    QRegularExpression expression;
    mutable QMutex mutex;
};

FileSearch::FileSearch(const QString &searchTerm, QTextDocument::FindFlags flags,
                       QMap<QString, QString> fileToContentsMap)
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

void FileSearch::operator()(QFutureInterface<FileSearchResultList> &futureInterface,
                            const FileIterator::Item &item) const
{
    if (futureInterface.isCanceled())
        return;
    futureInterface.setProgressRange(0, 1);
    futureInterface.setProgressValue(0);
    FileSearchResultList results;
    QFile file;
    QTextStream stream;
    QString tempString;
    if (!openStream(item.filePath, item.encoding, &stream, &file, &tempString, fileToContentsMap)) {
        futureInterface.cancel(); // failure
        return;
    }
    int lineNr = 0;

    while (!stream.atEnd()) {
        ++lineNr;
        const QString chunk = stream.readLine();
        const QString resultItemText = clippedText(chunk, MAX_LINE_SIZE);
        int chunkLength = chunk.length();
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
                        }
                    }
                }
                if (equal) {
                    results << FileSearchResult(item.filePath, lineNr, resultItemText,
                                                regionPtr - chunkPtr, termMaxIndex + 1,
                                                QStringList());
                    regionPtr += termMaxIndex; // another +1 done by for-loop
                }
            }
        }
        if (futureInterface.isPaused())
            futureInterface.waitForResume();
        if (futureInterface.isCanceled())
            break;
    }
    if (file.isOpen())
        file.close();
    if (!futureInterface.isCanceled()) {
        futureInterface.reportResult(results);
        futureInterface.setProgressValue(1);
    }
}

FileSearchRegExp::FileSearchRegExp(const QString &searchTerm, QTextDocument::FindFlags flags,
                                   QMap<QString, QString> fileToContentsMap)
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

void FileSearchRegExp::operator()(QFutureInterface<FileSearchResultList> &futureInterface,
                                  const FileIterator::Item &item) const
{
    if (futureInterface.isCanceled())
        return;
    futureInterface.setProgressRange(0, 1);
    futureInterface.setProgressValue(0);
    FileSearchResultList results;
    QFile file;
    QTextStream stream;
    QString tempString;
    if (!openStream(item.filePath, item.encoding, &stream, &file, &tempString, fileToContentsMap)) {
        futureInterface.cancel(); // failure
        return;
    }
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
            results << FileSearchResult(item.filePath, lineNr, resultItemText,
                                          pos, match.capturedLength(),
                                          match.capturedTexts());
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
    if (file.isOpen())
        file.close();
    if (!futureInterface.isCanceled()) {
        futureInterface.reportResult(results);
        futureInterface.setProgressValue(1);
    }
}

struct SearchState
{
    SearchState(const QString &term, FileIterator *iterator) : searchTerm(term), files(iterator) {}
    QString searchTerm;
    FileIterator *files = 0;
    FileSearchResultList cachedResults;
    int numFilesSearched = 0;
    int numMatches = 0;
};

SearchState initFileSearch(QFutureInterface<FileSearchResultList> &futureInterface,
                           const QString &searchTerm, FileIterator *files)
{
    futureInterface.setProgressRange(0, files->maxProgress());
    futureInterface.setProgressValueAndText(files->currentProgress(), msgFound(searchTerm, 0, 0));
    return SearchState(searchTerm, files);
}

void collectSearchResults(QFutureInterface<FileSearchResultList> &futureInterface,
                          SearchState &state,
                          const FileSearchResultList &results)
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

void cleanUpFileSearch(QFutureInterface<FileSearchResultList> &futureInterface,
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

QFuture<FileSearchResultList> Utils::findInFiles(const QString &searchTerm, FileIterator *files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap)
{
    return mapReduce(files->begin(), files->end(),
                     [searchTerm, files](QFutureInterface<FileSearchResultList> &futureInterface) {
                         return initFileSearch(futureInterface, searchTerm, files);
                     },
                     FileSearch(searchTerm, flags, fileToContentsMap),
                     &collectSearchResults,
                     &cleanUpFileSearch);
}

QFuture<FileSearchResultList> Utils::findInFilesRegExp(const QString &searchTerm, FileIterator *files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap)
{
    return mapReduce(files->begin(), files->end(),
                     [searchTerm, files](QFutureInterface<FileSearchResultList> &futureInterface) {
                         return initFileSearch(futureInterface, searchTerm, files);
                     },
                     FileSearchRegExp(searchTerm, flags, fileToContentsMap),
                     &collectSearchResults,
                     &cleanUpFileSearch);
}

QString Utils::expandRegExpReplacement(const QString &replaceText, const QStringList &capturedTexts)
{
    // handles \1 \\ \& & \t \n
    QString result;
    const int numCaptures = capturedTexts.size() - 1;
    for (int i = 0; i < replaceText.length(); ++i) {
        QChar c = replaceText.at(i);
        if (c == QLatin1Char('\\') && i < replaceText.length() - 1) {
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
                } else {
                    result += QLatin1Char('\\');
                    result += c;
                }
            } else {
                result += QLatin1Char('\\');
                result += c;
            }
        } else if (c == QLatin1Char('&')) {
            result += capturedTexts.at(0);
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
            return replaceText; // mixed
    }

    if (restIsLowerCase) {
        QString res = replaceText.toLower();
        if (firstIsUpperCase)
            res.replace(0, 1, res.at(0).toUpper());
        return res;
    }

    if (restIsUpperCase) {
        QString res = replaceText.toUpper();
        if (firstIsLowerCase)
            res.replace(0, 1, res.at(0).toLower());
        return res;
    }

    return replaceText;         // mixed
}
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

QTextCodec *encodingAt(const QList<QTextCodec *> encodings, int index)
{
    if (index >= 0 && index < encodings.size())
        return encodings.at(index);
    return QTextCodec::codecForLocale();
}

FileListIterator::FileListIterator(const QStringList &fileList,
                                   const QList<QTextCodec *> encodings)
    : m_maxIndex(-1)
{
    m_items.reserve(fileList.size());
    for (int i = 0; i < fileList.size(); ++i)
        m_items.append(Item(fileList.at(i), encodingAt(encodings, i)));
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

SubDirFileIterator::SubDirFileIterator(const QStringList &directories, const QStringList &filters,
                                       QTextCodec *encoding)
    : m_filters(filters), m_progress(0)
{
    m_encoding = (encoding == 0 ? QTextCodec::codecForLocale() : encoding);
    qreal maxPer = qreal(MAX_PROGRESS) / directories.count();
    foreach (const QString &directoryEntry, directories) {
        if (!directoryEntry.isEmpty()) {
            m_dirs.push(QDir(directoryEntry));
            m_progressValues.push(maxPer);
            m_processedValues.push(false);
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
        QDir dir = m_dirs.pop();
        const qreal dirProgressMax = m_progressValues.pop();
        const bool processed = m_processedValues.pop();
        if (dir.exists()) {
            QStringList subDirs;
            if (!processed)
                subDirs = dir.entryList(QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot);
            if (subDirs.isEmpty()) {
                QStringList fileEntries = dir.entryList(m_filters,
                    QDir::Files|QDir::Hidden);
                QStringListIterator it(fileEntries);
                it.toBack();
                m_items.reserve(m_items.size() + fileEntries.size());
                while (it.hasPrevious()) {
                    const QString &file = it.previous();
                    m_items.append(new Item(dir.path()+ QLatin1Char('/') + file, m_encoding));
                }
                m_progress += dirProgressMax;
            } else {
                qreal subProgress = dirProgressMax/(subDirs.size()+1);
                m_dirs.push(dir);
                m_progressValues.push(subProgress);
                m_processedValues.push(true);
                QStringListIterator it(subDirs);
                it.toBack();
                while (it.hasPrevious()) {
                    const QString &directory = it.previous();
                    m_dirs.push(QDir(dir.path()+ QLatin1Char('/') + directory));
                    m_progressValues.push(subProgress);
                    m_processedValues.push(false);
                }
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
