/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "filesearch.h"
#include <cctype>

#include <QtCore/QIODevice>
#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QFutureInterface>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QRegExp>
#include <QtCore/QCoreApplication>

#include <qtconcurrent/runextensions.h>

using namespace Utils;

static inline QString msgCanceled(const QString &searchTerm, int numMatches, int numFilesSearched)
{
    return QCoreApplication::translate("Utils::FileSearch",
                                       "%1: canceled. %n occurrences found in %2 files.",
                                       0, QCoreApplication::CodecForTr, numMatches).
                                       arg(searchTerm).arg(numFilesSearched);
}

static inline QString msgFound(const QString &searchTerm, int numMatches, int numFilesSearched)
{
    return QCoreApplication::translate("Utils::FileSearch",
                                       "%1: %n occurrences found in %2 files.",
                                       0, QCoreApplication::CodecForTr, numMatches).
                                       arg(searchTerm).arg(numFilesSearched);
}

static inline QString msgFound(const QString &searchTerm, int numMatches, int numFilesSearched, int filesSize)
{
    return QCoreApplication::translate("Utils::FileSearch",
                                       "%1: %n occurrences found in %2 of %3 files.",
                                       0, QCoreApplication::CodecForTr, numMatches).
                                       arg(searchTerm).arg(numFilesSearched).arg(filesSize);
}

namespace {

void runFileSearch(QFutureInterface<FileSearchResult> &future,
                   QString searchTerm,
                   FileIterator *files,
                   QTextDocument::FindFlags flags,
                   QMap<QString, QString> fileToContentsMap)
{
    future.setProgressRange(0, files->maxProgress());
    int numFilesSearched = 0;
    int numMatches = 0;

    bool caseInsensitive = !(flags & QTextDocument::FindCaseSensitively);
    bool wholeWord = (flags & QTextDocument::FindWholeWords);

    QByteArray sa = searchTerm.toUtf8();
    int scMaxIndex = sa.length()-1;
    const char *sc = sa.constData();

    QByteArray sal = searchTerm.toLower().toUtf8();
    const char *scl = sal.constData();

    QByteArray sau = searchTerm.toUpper().toUtf8();
    const char *scu = sau.constData();

    int chunkSize = qMax(100000, sa.length());

    QFile file;
    QBuffer buffer;
    while (files->hasNext()) {
        const QString &s = files->next();
        future.setProgressRange(0, files->maxProgress());
        if (future.isPaused())
            future.waitForResume();
        if (future.isCanceled()) {
            future.setProgressValueAndText(files->currentProgress(), msgCanceled(searchTerm, numMatches, numFilesSearched));
            break;
        }
        QIODevice *device;
        if (fileToContentsMap.contains(s)) {
            buffer.setData(fileToContentsMap.value(s).toLocal8Bit());
            device = &buffer;
        } else {
            file.setFileName(s);
            device = &file;
        }
        if (!device->open(QIODevice::ReadOnly))
            continue;
        int lineNr = 1;
        const char *startOfLastLine = NULL;

        bool firstChunk = true;
        while (!device->atEnd()) {
            if (!firstChunk)
                device->seek(device->pos()-sa.length()+1);

            const QByteArray chunk = device->read(chunkSize);
            const char *chunkPtr = chunk.constData();
            startOfLastLine = chunkPtr;
            for (const char *regionPtr = chunkPtr; regionPtr < chunkPtr + chunk.length()-scMaxIndex; ++regionPtr) {
                const char *regionEnd = regionPtr + scMaxIndex;

                if (*regionPtr == '\n') {
                    startOfLastLine = regionPtr + 1;
                    ++lineNr;
                }
                else if (
                        // case sensitive
                        (!caseInsensitive && *regionPtr == sc[0] && *regionEnd == sc[scMaxIndex])
                        ||
                        // case insensitive
                        (caseInsensitive && (*regionPtr == scl[0] || *regionPtr == scu[0])
                        && (*regionEnd == scl[scMaxIndex] || *regionEnd == scu[scMaxIndex]))
                         ) {
                    const char *afterRegion = regionEnd + 1;
                    const char *beforeRegion = regionPtr - 1;
                    bool equal = true;
                    if (wholeWord &&
                            (  isalnum((unsigned char)*beforeRegion)
                            || (*beforeRegion == '_')
                            || isalnum((unsigned char)*afterRegion)
                            || (*afterRegion == '_'))) {
                        equal = false;
                    }

                    int regionIndex = 1;
                    for (const char *regionCursor = regionPtr + 1; regionCursor < regionEnd; ++regionCursor, ++regionIndex) {
                        if (  // case sensitive
                              (!caseInsensitive && equal && *regionCursor != sc[regionIndex])
                              ||
                              // case insensitive
                              (caseInsensitive && equal && *regionCursor != sc[regionIndex] && *regionCursor != scl[regionIndex] && *regionCursor != scu[regionIndex])
                               ) {
                         equal = false;
                        }
                    }
                    if (equal) {
                        int textLength = chunk.length() - (startOfLastLine - chunkPtr);
                        if (textLength > 0) {
                            QByteArray res;
                            res.reserve(256);
                            int i = 0;
                            int n = 0;
                            while (startOfLastLine[i] != '\n' && startOfLastLine[i] != '\r' && i < textLength && n++ < 256)
                                res.append(startOfLastLine[i++]);
                            future.reportResult(FileSearchResult(s, lineNr, QString(res),
                                                          regionPtr - startOfLastLine, sa.length(),
                                                          QStringList()));
                            ++numMatches;
                        }
                    }
                }
            }
            firstChunk = false;
        }
        ++numFilesSearched;
        if (future.isProgressUpdateNeeded())
            future.setProgressValueAndText(files->currentProgress(), msgFound(searchTerm, numMatches, numFilesSearched));
        device->close();
    }
    if (!future.isCanceled())
        future.setProgressValueAndText(files->currentProgress(), msgFound(searchTerm, numMatches, numFilesSearched));
    delete files;
}

void runFileSearchRegExp(QFutureInterface<FileSearchResult> &future,
                   QString searchTerm,
                   FileIterator *files,
                   QTextDocument::FindFlags flags,
                   QMap<QString, QString> fileToContentsMap)
{
    future.setProgressRange(0, files->maxProgress());
    int numFilesSearched = 0;
    int numMatches = 0;
    if (flags & QTextDocument::FindWholeWords)
        searchTerm = QString::fromLatin1("\\b%1\\b").arg(searchTerm);
    const Qt::CaseSensitivity caseSensitivity = (flags & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    const QRegExp expression(searchTerm, caseSensitivity);

    QFile file;
    QString str;
    QTextStream stream;
    while (files->hasNext()) {
        const QString &s = files->next();
        future.setProgressRange(0, files->maxProgress());
        if (future.isPaused())
            future.waitForResume();
        if (future.isCanceled()) {
            future.setProgressValueAndText(files->currentProgress(), msgCanceled(searchTerm, numMatches, numFilesSearched));
            break;
        }

        bool needsToCloseFile = false;
        if (fileToContentsMap.contains(s)) {
            str = fileToContentsMap.value(s);
            stream.setString(&str);
        } else {
            file.setFileName(s);
            if (!file.open(QIODevice::ReadOnly))
                continue;
            needsToCloseFile = true;
            stream.setDevice(&file);
        }
        int lineNr = 1;
        QString line;
        while (!stream.atEnd()) {
            line = stream.readLine();
            int pos = 0;
            while ((pos = expression.indexIn(line, pos)) != -1) {
                future.reportResult(FileSearchResult(s, lineNr, line,
                                              pos, expression.matchedLength(),
                                              expression.capturedTexts()));
                pos += expression.matchedLength();
            }
            ++lineNr;
        }
        ++numFilesSearched;
        if (future.isProgressUpdateNeeded())
            future.setProgressValueAndText(files->currentProgress(), msgFound(searchTerm, numMatches, numFilesSearched));
        if (needsToCloseFile)
            file.close();
    }
    if (!future.isCanceled())
        future.setProgressValueAndText(files->currentProgress(), msgFound(searchTerm, numMatches, numFilesSearched));
    delete files;
}

} // namespace


QFuture<FileSearchResult> Utils::findInFiles(const QString &searchTerm, FileIterator *files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap)
{
    return QtConcurrent::run<FileSearchResult, QString, FileIterator *, QTextDocument::FindFlags, QMap<QString, QString> >
            (runFileSearch, searchTerm, files, flags, fileToContentsMap);
}

QFuture<FileSearchResult> Utils::findInFilesRegExp(const QString &searchTerm, FileIterator *files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap)
{
    return QtConcurrent::run<FileSearchResult, QString, FileIterator *, QTextDocument::FindFlags, QMap<QString, QString> >
            (runFileSearchRegExp, searchTerm, files, flags, fileToContentsMap);
}

QString Utils::expandRegExpReplacement(const QString &replaceText, const QStringList &capturedTexts)
{
    QString result;
    int numCaptures = capturedTexts.size() - 1;
    for (int i = 0; i < replaceText.length(); ++i) {
        QChar c = replaceText.at(i);
        if (c == QLatin1Char('\\') && i < replaceText.length() - 1) {
            c = replaceText.at(++i);
            if (c == QLatin1Char('\\')) {
                result += QLatin1Char('\\');
            } else if (c == QLatin1Char('&')) {
                result += QLatin1Char('&');
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

// #pragma mark -- FileIterator

FileIterator::FileIterator()
    : m_list(QStringList()),
    m_iterator(0),
    m_index(0)
{
}

FileIterator::FileIterator(const QStringList &fileList)
    : m_list(fileList),
    m_iterator(new QStringListIterator(m_list)),
    m_index(0)
{
}

FileIterator::~FileIterator()
{
    if (m_iterator)
        delete m_iterator;
}

bool FileIterator::hasNext() const
{
    Q_ASSERT(m_iterator);
    return m_iterator->hasNext();
}

QString FileIterator::next()
{
    Q_ASSERT(m_iterator);
    ++m_index;
    return m_iterator->next();
}

int FileIterator::maxProgress() const
{
    return m_list.size();
}

int FileIterator::currentProgress() const
{
    return m_index;
}

// #pragma mark -- SubDirFileIterator

namespace {
    const int MAX_PROGRESS = 360;
}

SubDirFileIterator::SubDirFileIterator(const QStringList &directories, const QStringList &filters)
    : m_filters(filters), m_progress(0)
{
    int maxPer = MAX_PROGRESS/directories.count();
    foreach (const QString &directoryEntry, directories) {
        if (!directoryEntry.isEmpty()) {
            m_dirs.push(QDir(directoryEntry));
            m_progressValues.push(maxPer);
            m_processedValues.push(false);
        }
    }
}

bool SubDirFileIterator::hasNext() const
{
    if (!m_currentFiles.isEmpty())
        return true;
    while(!m_dirs.isEmpty() && m_currentFiles.isEmpty()) {
        QDir dir = m_dirs.pop();
        int dirProgressMax = m_progressValues.pop();
        bool processed = m_processedValues.pop();
        if (dir.exists()) {
            QStringList subDirs;
            if (!processed) {
                subDirs = dir.entryList(QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot);
            }
            if (subDirs.isEmpty()) {
                QStringList fileEntries = dir.entryList(m_filters,
                    QDir::Files|QDir::Hidden);
                QStringListIterator it(fileEntries);
                it.toBack();
                while (it.hasPrevious()) {
                    const QString &file = it.previous();
                    m_currentFiles.append(dir.path()+ QLatin1Char('/') +file);
                }
                m_progress += dirProgressMax;
            } else {
                int subProgress = dirProgressMax/(subDirs.size()+1);
                int selfProgress = subProgress + dirProgressMax%(subDirs.size()+1);
                m_dirs.push(dir);
                m_progressValues.push(selfProgress);
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
    if (m_currentFiles.isEmpty()) {
        m_progress = MAX_PROGRESS;
        return false;
    }

    return true;
}

QString SubDirFileIterator::next()
{
    Q_ASSERT(!m_currentFiles.isEmpty());
    return m_currentFiles.takeFirst();
}

int SubDirFileIterator::maxProgress() const
{
    return MAX_PROGRESS;
}

int SubDirFileIterator::currentProgress() const
{
    return m_progress;
}
