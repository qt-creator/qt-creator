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

#include "filesearch.h"
#include "runextensions.h"

#include <QCoreApplication>
#include <QMutex>
#include <QRegularExpression>
#include <QTextCodec>
#include <QtConcurrentMap>

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
               QMap<QString, QString> fileToContentsMap,
               QFutureInterface<FileSearchResultList> *futureInterface);
    const FileSearchResultList operator()(const FileIterator::Item &item) const;

private:
    QMap<QString, QString> fileToContentsMap;
    QFutureInterface<FileSearchResultList> *future;
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
                     QMap<QString, QString> fileToContentsMap,
                     QFutureInterface<FileSearchResultList> *futureInterface);
    const FileSearchResultList operator()(const FileIterator::Item &item) const;

private:
    QRegularExpressionMatch doGuardedMatch(const QString &line, int offset) const;

    QMap<QString, QString> fileToContentsMap;
    QFutureInterface<FileSearchResultList> *future;
    QRegularExpression expression;
    mutable QMutex mutex;
};

FileSearch::FileSearch(const QString &searchTerm, QTextDocument::FindFlags flags,
                       QMap<QString, QString> fileToContentsMap,
                       QFutureInterface<FileSearchResultList> *futureInterface)
{
    this->fileToContentsMap = fileToContentsMap;
    caseSensitive = (flags & QTextDocument::FindCaseSensitively);
    wholeWord = (flags & QTextDocument::FindWholeWords);
    future = futureInterface;
    searchTermLower = searchTerm.toLower();
    searchTermUpper = searchTerm.toUpper();
    termMaxIndex = searchTerm.length() - 1;
    termData = searchTerm.constData();
    termDataLower = searchTermLower.constData();
    termDataUpper = searchTermUpper.constData();
}

const FileSearchResultList FileSearch::operator()(const FileIterator::Item &item) const
{
    FileSearchResultList results;
    if (future->isCanceled())
        return results;
    QFile file;
    QTextStream stream;
    QString tempString;
    if (!openStream(item.filePath, item.encoding, &stream, &file, &tempString, fileToContentsMap))
        return results;
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
        if (future->isPaused())
            future->waitForResume();
        if (future->isCanceled())
            break;
    }
    if (file.isOpen())
        file.close();
    return results;
}

FileSearchRegExp::FileSearchRegExp(const QString &searchTerm, QTextDocument::FindFlags flags,
                                   QMap<QString, QString> fileToContentsMap,
                                   QFutureInterface<FileSearchResultList> *futureInterface)
{
    this->fileToContentsMap = fileToContentsMap;
    future = futureInterface;
    QString term = searchTerm;
    if (flags & QTextDocument::FindWholeWords)
        term = QString::fromLatin1("\\b%1\\b").arg(term);
    const QRegularExpression::PatternOptions patternOptions = (flags & QTextDocument::FindCaseSensitively)
            ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption;
    expression = QRegularExpression(term, patternOptions);
}

QRegularExpressionMatch FileSearchRegExp::doGuardedMatch(const QString &line, int offset) const
{
    QMutexLocker lock(&mutex);
    return expression.match(line, offset);
}

const FileSearchResultList FileSearchRegExp::operator()(const FileIterator::Item &item) const
{
    FileSearchResultList results;
    if (future->isCanceled())
        return results;
    QFile file;
    QTextStream stream;
    QString tempString;
    if (!openStream(item.filePath, item.encoding, &stream, &file, &tempString, fileToContentsMap))
        return results;
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
        if (future->isPaused())
            future->waitForResume();
        if (future->isCanceled())
            break;
    }
    if (file.isOpen())
        file.close();
    return results;
}

class RunFileSearch
{
public:
    RunFileSearch(QFutureInterface<FileSearchResultList> &future,
                  const QString &searchTerm,
                  FileIterator *files,
                  const std::function<FileSearchResultList(FileIterator::Item)> &searchFunction);

    void run();
    void collect(const FileSearchResultList &results);

private:
    QFutureInterface<FileSearchResultList> &m_future;
    QString m_searchTerm;
    FileIterator *m_files;
    std::function<FileSearchResultList(FileIterator::Item)> m_searchFunction;

    int m_numFilesSearched;
    int m_numMatches;
    FileSearchResultList m_results;
    bool m_canceled;
};

RunFileSearch::RunFileSearch(QFutureInterface<FileSearchResultList> &future,
                             const QString &searchTerm, FileIterator *files,
                             const std::function<FileSearchResultList (FileIterator::Item)> &searchFunction)
    : m_future(future),
      m_searchTerm(searchTerm),
      m_files(files),
      m_searchFunction(searchFunction),
      m_numFilesSearched(0),
      m_numMatches(0),
      m_canceled(false)
{
    m_future.setProgressRange(0, m_files->maxProgress());
    m_future.setProgressValueAndText(m_files->currentProgress(), msgFound(m_searchTerm,
                                                                          m_numMatches,
                                                                          m_numFilesSearched));
}

void RunFileSearch::run()
{
    // This thread waits for blockingMappedReduced to finish, so reduce the pool's used thread count
    // so the blockingMappedReduced can use one more thread, and increase it again afterwards.
    QThreadPool::globalInstance()->releaseThread();
    QtConcurrent::blockingMappedReduced<FileSearchResultList>(m_files->begin(), m_files->end(),
                                                  m_searchFunction,
                                                  [this](FileSearchResultList &, const FileSearchResultList &results) {
                                                      collect(results);
                                                  },
                                                  QtConcurrent::OrderedReduce | QtConcurrent::SequentialReduce);
    QThreadPool::globalInstance()->reserveThread();
    if (!m_results.isEmpty()) {
        m_future.reportResult(m_results);
        m_results.clear();
    }
    if (!m_future.isCanceled())
        m_future.setProgressValueAndText(m_files->currentProgress(), msgFound(m_searchTerm,
                                                                              m_numMatches,
                                                                              m_numFilesSearched));
    delete m_files;
    if (m_future.isPaused())
        m_future.waitForResume();
}

void RunFileSearch::collect(const FileSearchResultList &results)
{
    if (m_future.isCanceled()) {
        if (!m_canceled) {
            m_future.setProgressValueAndText(m_files->currentProgress(),
                                             msgCanceled(m_searchTerm,
                                                         m_numMatches,
                                                         m_numFilesSearched));
            m_canceled = true;
        }
        return;
    }
    m_numMatches += results.size();
    m_results << results;
    ++m_numFilesSearched;
    if (m_future.isProgressUpdateNeeded()
            || m_future.progressValue() == 0 /*workaround for regression in Qt*/) {
        if (!m_results.isEmpty()) {
            m_future.reportResult(m_results);
            m_results.clear();
        }
        m_future.setProgressRange(0, m_files->maxProgress());
        m_future.setProgressValueAndText(m_files->currentProgress(), msgFound(m_searchTerm,
                                                                              m_numMatches,
                                                                              m_numFilesSearched));
    }
}

void runFileSearch(QFutureInterface<FileSearchResultList> &future,
                   QString searchTerm,
                   FileIterator *files,
                   QTextDocument::FindFlags flags,
                   QMap<QString, QString> fileToContentsMap)
{
    FileSearch searchFunction(searchTerm, flags, fileToContentsMap, &future);
    RunFileSearch search(future, searchTerm, files, std::bind(&FileSearch::operator(),
                                                              &searchFunction,
                                                              std::placeholders::_1));
    search.run();
}

void runFileSearchRegExp(QFutureInterface<FileSearchResultList> &future,
                   QString searchTerm,
                   FileIterator *files,
                   QTextDocument::FindFlags flags,
                   QMap<QString, QString> fileToContentsMap)
{
    FileSearchRegExp searchFunction(searchTerm, flags, fileToContentsMap, &future);
    RunFileSearch search(future, searchTerm, files, std::bind(&FileSearchRegExp::operator(),
                                                              &searchFunction,
                                                              std::placeholders::_1));
    search.run();
}

} // namespace


QFuture<FileSearchResultList> Utils::findInFiles(const QString &searchTerm, FileIterator *files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap)
{
    return QtConcurrent::run<FileSearchResultList, QString, FileIterator *, QTextDocument::FindFlags, QMap<QString, QString> >
            (runFileSearch, searchTerm, files, flags, fileToContentsMap);
}

QFuture<FileSearchResultList> Utils::findInFilesRegExp(const QString &searchTerm, FileIterator *files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap)
{
    return QtConcurrent::run<FileSearchResultList, QString, FileIterator *, QTextDocument::FindFlags, QMap<QString, QString> >
            (runFileSearchRegExp, searchTerm, files, flags, fileToContentsMap);
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

void FileIterator::next(FileIterator::const_iterator *it)
{
    if (it->m_index < 0) // == end
        return;
    ++it->m_index;
    update(it->m_index);
    if (it->m_index < currentFileCount()) {
        it->m_item.filePath = fileAt(it->m_index);
        it->m_item.encoding = codecAt(it->m_index);
    } else {
        it->m_index = -1; // == end
        it->m_item.filePath.clear();
        it->m_item.encoding = 0;
    }
}

FileIterator::const_iterator FileIterator::begin()
{
    update(0);
    if (currentFileCount() == 0)
        return end();
    return FileIterator::const_iterator(this,
                                        FileIterator::Item(fileAt(0), codecAt(0)),
                                        0/*index*/);
}

FileIterator::const_iterator FileIterator::end()
{
    return FileIterator::const_iterator(this, FileIterator::Item(QString(), 0),
                                        -1/*end*/);
}

// #pragma mark -- FileListIterator

FileListIterator::FileListIterator(const QStringList &fileList,
                                   const QList<QTextCodec *> encodings)
    : m_files(fileList),
      m_encodings(encodings),
      m_maxIndex(-1)
{
}

void FileListIterator::update(int requestedIndex)
{
    if (requestedIndex > m_maxIndex)
        m_maxIndex = requestedIndex;
}

int FileListIterator::currentFileCount() const
{
    return m_files.size();
}

QString FileListIterator::fileAt(int index) const
{
    return m_files.at(index);
}

QTextCodec *FileListIterator::codecAt(int index) const
{
    return m_encodings.at(index);
}

int FileListIterator::maxProgress() const
{
    return m_files.size();
}

int FileListIterator::currentProgress() const
{
    return m_maxIndex + 1;
}

QTextCodec *FileListIterator::encodingAt(int index) const
{
    if (index >= 0 && index < m_encodings.size())
        return m_encodings.at(index);
    return QTextCodec::codecForLocale();
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

void SubDirFileIterator::update(int index)
{
    if (index < m_files.size())
        return;
    // collect files from the directories until we have enough for the given index
    while (!m_dirs.isEmpty() && index >= m_files.size()) {
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
                while (it.hasPrevious()) {
                    const QString &file = it.previous();
                    m_files.append(dir.path()+ QLatin1Char('/') +file);
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
    if (index >= m_files.size())
        m_progress = MAX_PROGRESS;
}

int SubDirFileIterator::currentFileCount() const
{
    return m_files.size();
}

QString SubDirFileIterator::fileAt(int index) const
{
    return m_files.at(index);
}

QTextCodec *SubDirFileIterator::codecAt(int index) const
{
    Q_UNUSED(index)
    return m_encoding;
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
