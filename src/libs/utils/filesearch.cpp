/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "filesearch.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QFutureInterface>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QRegExp>
#include <QtGui/QApplication>

#include <qtconcurrent/runextensions.h>

using namespace Core::Utils;

namespace {

void runFileSearch(QFutureInterface<FileSearchResult> &future,
                   QString searchTerm,
                   QStringList files,
                   QTextDocument::FindFlags flags)
{
    future.setProgressRange(0, files.size());
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

    foreach (QString s, files) {
        if (future.isPaused())
            future.waitForResume();
        if (future.isCanceled()) {
            future.setProgressValueAndText(numFilesSearched,
                                           qApp->translate("FileSearch", "%1: canceled. %2 occurrences found in %3 files.").
                                           arg(searchTerm).arg(numMatches).arg(numFilesSearched));
            break;
        }
        QFile file(s);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        int lineNr = 1;
        const char *startOfLastLine = NULL;

        bool firstChunk = true;
        while (!file.atEnd()) {
            if (!firstChunk)
                file.seek(file.pos()-sa.length()+1);

            const QByteArray chunk = file.read(chunkSize);
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
                        ( ((*beforeRegion >= '0' && *beforeRegion <= '9') || *beforeRegion >= 'A')
                        || ((*afterRegion >= '0' && *afterRegion <= '9') || *afterRegion >= 'A')))
                    {
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
                            future.reportResult(FileSearchResult(QDir::toNativeSeparators(s), lineNr, QString(res),
                                                          regionPtr - startOfLastLine, sa.length()));
                            ++numMatches;
                        }
                    }
                }
            }
            firstChunk = false;
        }
        ++numFilesSearched;
        future.setProgressValueAndText(numFilesSearched, qApp->translate("FileSearch", "%1: %2 occurrences found in %3 of %4 files.").
                                arg(searchTerm).arg(numMatches).arg(numFilesSearched).arg(files.size()));
    }
    if (!future.isCanceled())
        future.setProgressValueAndText(numFilesSearched, qApp->translate("FileSearch", "%1: %2 occurrences found in %3 files.").
                                arg(searchTerm).arg(numMatches).arg(numFilesSearched));
}

void runFileSearchRegExp(QFutureInterface<FileSearchResult> &future,
                   QString searchTerm,
                   QStringList files,
                   QTextDocument::FindFlags flags)
{
    future.setProgressRange(0, files.size());
    int numFilesSearched = 0;
    int numMatches = 0;
    if (flags & QTextDocument::FindWholeWords)
        searchTerm = QString("\\b%1\\b").arg(searchTerm);
    Qt::CaseSensitivity caseSensitivity = (flags & QTextDocument::FindCaseSensitively) ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QRegExp expression(searchTerm, caseSensitivity);

    foreach (QString s, files) {
        if (future.isPaused())
            future.waitForResume();
        if (future.isCanceled()) {
            future.setProgressValueAndText(numFilesSearched,
                                           qApp->translate("FileSearch", "%1: canceled. %2 occurrences found in %3 files.").
                                           arg(searchTerm).arg(numMatches).arg(numFilesSearched));
            break;
        }
        QFile file(s);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        QTextStream stream(&file);
        int lineNr = 1;
        QString line;
        while (!stream.atEnd()) {
            line = stream.readLine();
            int pos = 0;
            while ((pos = expression.indexIn(line, pos)) != -1) {
                future.reportResult(FileSearchResult(QDir::toNativeSeparators(s), lineNr, line,
                                              pos, expression.matchedLength()));
                pos += expression.matchedLength();
            }
            ++lineNr;
        }
        ++numFilesSearched;
        future.setProgressValueAndText(numFilesSearched, qApp->translate("FileSearch", "%1: %2 occurrences found in %3 of %4 files.").
                                arg(searchTerm).arg(numMatches).arg(numFilesSearched).arg(files.size()));
    }
    if (!future.isCanceled())
        future.setProgressValueAndText(numFilesSearched, qApp->translate("FileSearch", "%1: %2 occurrences found in %3 files.").
                                arg(searchTerm).arg(numMatches).arg(numFilesSearched));
}

} // namespace


QFuture<FileSearchResult> Core::Utils::findInFiles(const QString &searchTerm, const QStringList &files,
    QTextDocument::FindFlags flags)
{
    return QtConcurrent::run<FileSearchResult, QString, QStringList, QTextDocument::FindFlags>(runFileSearch, searchTerm, files, flags);
}

QFuture<FileSearchResult> Core::Utils::findInFilesRegExp(const QString &searchTerm, const QStringList &files,
    QTextDocument::FindFlags flags)
{
    return QtConcurrent::run<FileSearchResult, QString, QStringList, QTextDocument::FindFlags>(runFileSearchRegExp, searchTerm, files, flags);
}
