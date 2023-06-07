// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "silversearcherparser.h"

#include <QPromise>

using namespace Utils;

namespace SilverSearcher {

/*
// Example output (searching for "ab"):

:/home/jarek/dev/creator-master/src/libs/qmlpuppetcommunication/container/imagecontainer.cpp
149;60 2:    static const bool dontUseSharedMemory = qEnvironmentVariableIsSet("DESIGNER_DONT_USE_SHARED_MEMORY");
198;65 2:            qCInfo(imageContainerDebug()) << Q_FUNC_INFO << "Not able to create image:" << imageWidth << imageHeight << imageFormat;

:/home/jarek/dev/creator-master/src/libs/qmlpuppetcommunication/container/propertyabstractcontainer.cpp
4;18 2:#include "propertyabstractcontainer.h"
10;8 2,35 2:PropertyAbstractContainer::PropertyAbstractContainer()
*/

static QStringView nextLine(QStringView *remainingInput)
{
    const int newLinePos = remainingInput->indexOf('\n');
    if (newLinePos < 0) {
        QStringView ret = *remainingInput;
        *remainingInput = QStringView();
        return ret;
    }
    QStringView ret = remainingInput->left(newLinePos);
    *remainingInput = remainingInput->mid(newLinePos + 1);
    return ret;
}

static bool parseNumber(QStringView numberString, int *number)
{
    for (const auto &c : numberString) {
        if (!c.isDigit())
            return false;
    }
    bool ok = false;
    int parsedNumber = numberString.toInt(&ok);
    if (ok)
        *number = parsedNumber;
    return ok;
}

static bool parseLineNumber(QStringView *remainingInput, int *lineNumber)
{
    const int lineNumberDelimiterPos = remainingInput->indexOf(';');
    if (lineNumberDelimiterPos < 0)
        return false;

    if (!parseNumber(remainingInput->left(lineNumberDelimiterPos), lineNumber))
        return false;

    *remainingInput = remainingInput->mid(lineNumberDelimiterPos + 1);
    return true;
}

static bool parseLineHit(QStringView hitString, QPair<int, int> *hit)
{
    const int hitSpaceDelimiterPos = hitString.indexOf(' ');
    if (hitSpaceDelimiterPos < 0)
        return false;

    int hitStart = -1;
    if (!parseNumber(hitString.left(hitSpaceDelimiterPos), &hitStart))
        return false;

    int hitLength = -1;
    if (!parseNumber(hitString.mid(hitSpaceDelimiterPos + 1), &hitLength))
        return false;

    *hit = {hitStart, hitLength};
    return true;
}

static bool parseLineHits(QStringView *remainingInput, QList<QPair<int, int>> *hits)
{
    const int hitsDelimiterPos = remainingInput->indexOf(':');
    if (hitsDelimiterPos < 0)
        return false;

    const QStringView hitsString = remainingInput->left(hitsDelimiterPos);
    const QList<QStringView> hitStrings = hitsString.split(',', Qt::SkipEmptyParts);
    for (const auto hitString : hitStrings) {
        QPair<int, int> hit;
        if (!parseLineHit(hitString, &hit))
            return false;
        hits->append(hit);
    }
    *remainingInput = remainingInput->mid(hitsDelimiterPos + 1);
    return true;
}

SearchResultItems parse(const QFuture<void> &future, const QString &input,
                        const std::optional<QRegularExpression> &regExp, FilePath *lastFilePath)
{
    QTC_ASSERT(lastFilePath, return {});
    SearchResultItems items;

    QStringView remainingInput(input);
    while (true) {
        if (future.isCanceled())
            return {};

        if (remainingInput.isEmpty())
            break;

        const QStringView filePathLine = nextLine(&remainingInput);
        if (filePathLine.isEmpty()) {
            *lastFilePath = {}; // Clear the parser state
            continue;
        }

        if (filePathLine.startsWith(':'))
            *lastFilePath = FilePath::fromPathPart(filePathLine.mid(1));

        while (true) {
            QStringView hitLine = nextLine(&remainingInput);
            if (hitLine.isEmpty())
                break;
            int lineNumber = -1;
            if (!parseLineNumber(&hitLine, &lineNumber))
                break;

            // List of pairs <hit start pos, hit length>
            QList<QPair<int, int>> hits;
            if (!parseLineHits(&hitLine, &hits))
                break;

            SearchResultItem item;
            item.setFilePath(*lastFilePath);
            item.setDisplayText(hitLine.toString());
            item.setUseTextEditorFont(true);
            for (const QPair<int, int> &hit : hits) {
                item.setMainRange(lineNumber, hit.first, hit.second);
                item.setUserData(
                    regExp ? regExp->match(hitLine.mid(hit.first, hit.second)).capturedTexts()
                           : QVariant());
                items.append(item);
            }
        }
    }
    return items;
}

SearchResultItems parse(const QString &input, const std::optional<QRegularExpression> &regExp)
{
    QPromise<void> promise;
    promise.start();
    FilePath dummy;
    return SilverSearcher::parse(promise.future(), input, regExp, &dummy);
}

} // namespace SilverSearcher
