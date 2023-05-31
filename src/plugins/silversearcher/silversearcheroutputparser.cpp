// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "silversearcheroutputparser.h"

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

static QStringView nextLine(QStringView *remainingOutput)
{
    const int newLinePos = remainingOutput->indexOf('\n');
    if (newLinePos < 0) {
        QStringView ret = *remainingOutput;
        *remainingOutput = QStringView();
        return ret;
    }
    QStringView ret = remainingOutput->left(newLinePos);
    *remainingOutput = remainingOutput->mid(newLinePos + 1);
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

static bool parseLineNumber(QStringView *remainingOutput, int *lineNumber)
{
    const int lineNumberDelimiterPos = remainingOutput->indexOf(';');
    if (lineNumberDelimiterPos < 0)
        return false;

    if (!parseNumber(remainingOutput->left(lineNumberDelimiterPos), lineNumber))
        return false;

    *remainingOutput = remainingOutput->mid(lineNumberDelimiterPos + 1);
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

static bool parseLineHits(QStringView *remainingOutput, QList<QPair<int, int>> *hits)
{
    const int hitsDelimiterPos = remainingOutput->indexOf(':');
    if (hitsDelimiterPos < 0)
        return false;

    const QStringView hitsString = remainingOutput->left(hitsDelimiterPos);
    const QList<QStringView> hitStrings = hitsString.split(',', Qt::SkipEmptyParts);
    for (const auto hitString : hitStrings) {
        QPair<int, int> hit;
        if (!parseLineHit(hitString, &hit))
            return false;
        hits->append(hit);
    }
    *remainingOutput = remainingOutput->mid(hitsDelimiterPos + 1);
    return true;
}

SearchResultItems parse(const QString &output, const std::optional<QRegularExpression> &regExp)
{
    SearchResultItems items;

    QStringView remainingOutput(output);
    while (true) {
        if (remainingOutput.isEmpty())
            break;

        const QStringView filePathLine = nextLine(&remainingOutput);
        if (filePathLine.isEmpty())
            continue;

        if (!filePathLine.startsWith(':'))
            continue;

        const FilePath filePath = FilePath::fromPathPart(filePathLine.mid(1));

        while (true) {
            QStringView hitLine = nextLine(&remainingOutput);
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
            item.setFilePath(filePath);
            item.setDisplayText(hitLine.toString());
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

} // namespace SilverSearcher
