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

// A match across lines (pattern containing \n) is reported on the line it ends on. All lines of
// the match are printed before that, as "<line>;<text>", without a list of hits. The hit has the
// length of the whole match including the line breaks, and the column of its start relative to the
// beginning of the first line of the output that did not start inside a match (counted in bytes).
// Searching for "ab\ncd":

:/home/jarek/dev/file.cpp
12;foo ab
13;4 5:cd
*/

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

// Parses "<line>;<hits>:<text>" (a line with matches), "<line>;<text>" (a line that is part of a
// match across lines) or "<line>:<text>" (a context line).
static bool parseOutputLine(QStringView line, OutputLine *outputLine)
{
    int delimiterPos = 0;
    while (delimiterPos < line.size() && line.at(delimiterPos) != ';' && line.at(delimiterPos) != ':')
        ++delimiterPos;
    if (delimiterPos == line.size())
        return false;

    if (!parseNumber(line.left(delimiterPos), &outputLine->lineNumber))
        return false;

    outputLine->isContext = line.at(delimiterPos) == ':';
    line = line.mid(delimiterPos + 1);
    if (!outputLine->isContext) {
        QStringView remaining = line;
        QList<QPair<int, int>> hits;
        if (parseLineHits(&remaining, &hits)) {
            outputLine->hits = hits;
            line = remaining;
        }
    }
    outputLine->bytes = line.toLatin1();
    outputLine->text = QString::fromUtf8(outputLine->bytes);
    return true;
}

// The column in the text of \a line that the column \a bytes in what ag printed corresponds to.
// The decoder replaces every byte that is not valid UTF-8 by a replacement character of its own,
// so a file in another encoding, such as Latin-1, keeps one character for each byte.
static int charactersForBytes(const OutputLine &line, int bytes)
{
    qsizetype characters = QString::fromUtf8(line.bytes.left(std::max(bytes, 0))).size();
    while (characters < line.text.size() && line.text.at(characters).isLowSurrogate())
        ++characters;
    return std::min(characters, line.text.size());
}

// Finds the range of the hit on line "lastLine" of the "run". The column of the hit is relative
// to the start of line "startLine". A line break is \a lineBreakSize bytes long.
static std::optional<Text::Range> resolveHit(const QList<OutputLine> &run, qsizetype startLine,
                                             qsizetype lastLine, const QPair<int, int> &hit,
                                             int lineBreakSize)
{
    qsizetype line = startLine;
    int column = hit.first;
    while (column > run.at(line).bytes.size()) {
        column -= run.at(line).bytes.size() + lineBreakSize;
        if (++line > lastLine)
            return {};
    }
    const qsizetype beginLine = line;
    const int beginColumn = column;

    int remaining = hit.second;
    while (remaining > run.at(line).bytes.size() - column) {
        remaining -= run.at(line).bytes.size() - column + lineBreakSize;
        column = 0;
        if (++line > lastLine)
            return {};
    }
    if (line != lastLine)
        return {};

    return Text::Range{{run.at(beginLine).lineNumber,
                        charactersForBytes(run.at(beginLine), beginColumn)},
                       {run.at(lastLine).lineNumber,
                        charactersForBytes(run.at(lastLine), column + remaining)}};
}

static QString matchedText(const QString &lineText, const Text::Range &range)
{
    const qsizetype begin = range.begin.column;
    qsizetype end = 0;
    for (int i = range.begin.line; i < range.end.line; ++i)
        end = lineText.indexOf('\n', end) + 1;
    end += range.end.column;
    return lineText.mid(begin, end - begin);
}

// Bounds the memory for a match across very many lines
enum { MaxRunLines = 10000 };

static void resetRun(ParserState *state)
{
    state->run.clear();
    state->starts.clear();
}

static void dropFirstLines(ParserState *state, qsizetype count)
{
    for (qsizetype i = 0; i < count; ++i) {
        const OutputLine &line = state->run.at(i);
        for (Start &start : state->starts) {
            if (start.lineNumber <= line.lineNumber)
                start.droppedBytes += line.bytes.size();
        }
    }
    state->run.remove(0, count);
    if (state->starts.size() > MaxRunLines)
        state->starts.remove(0, state->starts.size() - MaxRunLines);
}

namespace {

struct Candidate
{
    Text::Range range;
    QString lineText;
    QString matchText; // With the line breaks of the reading that was taken
    int startLineNumber = -1; // The line that the column counts from
};

} // namespace

// The text of a match across lines in a file with CRLF line breaks
static QString withCarriageReturns(const QString &text)
{
    QString result;
    result.reserve(text.size() + 8);
    for (qsizetype i = 0; i < text.size(); ++i) {
        if (text.at(i) == '\n' && (i == 0 || text.at(i - 1) != '\r'))
            result += '\r';
        result += text.at(i);
    }
    return result;
}

static QString withLineFeeds(QString text)
{
    text.replace("\r\n", "\n");
    return text;
}

static bool canConfirm(const ParserState &state, const std::optional<QRegularExpression> &regExp)
{
    return regExp || state.plainText;
}

// Whether the text of a candidate is a match of what was searched for.
static bool confirms(ParserState *state, const std::optional<QRegularExpression> &regExp,
                     const QString &text)
{
    if (regExp) {
        if (!state->wholeMatch || state->wholeMatchSource != *regExp) {
            state->wholeMatchSource = *regExp;
            state->wholeMatch = QRegularExpression(
                QRegularExpression::anchoredPattern(regExp->pattern()), regExp->patternOptions());
        }
        return state->wholeMatch->match(text).hasMatch();
    }
    // The line breaks of a plain text term are those of a document, which matches a file with
    // either kind of line breaks.
    return withLineFeeds(text).compare(withLineFeeds(*state->plainText),
                                       state->plainTextCaseSensitivity) == 0;
}

// A hit across lines counts the line breaks of the file, which are two bytes in a CRLF file even
// if Utils::Process turns them into one character. The output does not tell which it is, so both
// are tried and the one that results in a match of the search term is taken. If none does, or if
// the search term is not known, the first that fits the lines is taken. If the line itself is
// the only line that the column can count from, there is no other reading.
static std::optional<Candidate> resolve(ParserState *state,
                                        const std::optional<QRegularExpression> &regExp,
                                        const QPair<int, int> &hit)
{
    const qsizetype last = state->run.size() - 1;
    const int firstLineNumber = state->run.first().lineNumber;
    const bool onlyReading = state->starts.size() == 1
                             && state->starts.first().lineNumber == state->run.last().lineNumber;
    std::optional<Candidate> unconfirmed;
    for (int lineBreakSize = 1; lineBreakSize <= 2; ++lineBreakSize) {
        for (qsizetype s = state->starts.size() - 1; s >= 0; --s) {
            const Start &start = state->starts.at(s);
            const int droppedLines = std::max(0, firstLineNumber - start.lineNumber);
            const int column = hit.first - start.droppedBytes - droppedLines * lineBreakSize;
            if (column < 0)
                continue;
            qsizetype firstLine = std::max(0, start.lineNumber - firstLineNumber);
            const std::optional<Text::Range> range
                = resolveHit(state->run, firstLine, last, {column, hit.second}, lineBreakSize);
            if (!range)
                continue;

            // The first line of the range may be after the start line that was found.
            while (firstLine < last && state->run.at(firstLine).lineNumber < range->begin.line)
                ++firstLine;
            QStringList texts;
            for (qsizetype l = firstLine; l <= last; ++l)
                texts << state->run.at(l).text;

            Candidate candidate{*range, texts.join('\n'), {}, start.lineNumber};
            candidate.matchText = matchedText(candidate.lineText, candidate.range);
            if (lineBreakSize == 2)
                candidate.matchText = withCarriageReturns(candidate.matchText);
            if (onlyReading || !canConfirm(*state, regExp)
                || confirms(state, regExp, candidate.matchText)) {
                return candidate;
            }
            if (!unconfirmed)
                unconfirmed = candidate;
        }
    }
    return unconfirmed;
}

static void appendHits(ParserState *state, const std::optional<QRegularExpression> &regExp,
                       SearchResultItems *items)
{
    const OutputLine &lastLine = state->run.last();
    QList<int> resolvedStarts;

    for (const QPair<int, int> &hit : std::as_const(lastLine.hits)) {
        std::optional<Candidate> candidate = resolve(state, regExp, hit);
        if (candidate) {
            if (!resolvedStarts.contains(candidate->startLineNumber))
                resolvedStarts.append(candidate->startLineNumber);
        } else {
            // Not understood: keep the part of the hit that is on this line, and count the hits
            // after it from this line on.
            const Text::Range range{
                {lastLine.lineNumber, charactersForBytes(lastLine, hit.first)},
                {lastLine.lineNumber, charactersForBytes(lastLine, hit.first + hit.second)}};
            candidate = Candidate{range, lastLine.text, matchedText(lastLine.text, range)};
            if (!resolvedStarts.contains(lastLine.lineNumber))
                resolvedStarts.append(lastLine.lineNumber);
        }

        SearchResultItem item;
        item.setFilePath(state->filePath);
        item.setLineText(candidate->lineText);
        item.setUseTextEditorFont(true);
        item.setMainRange(candidate->range);
        // The captures go into the document, which has line feeds only.
        QVariant userData;
        if (regExp) {
            QStringList captured = regExp->match(candidate->matchText).capturedTexts();
            for (QString &text : captured)
                text.replace("\r\n", "\n");
            userData = captured;
        }
        item.setUserData(userData);
        items->append(item);
    }

    // All hits on a line count from the same line. A hit on a later line counts from that line too,
    // or from a line after this one. The lines in between start inside a match.
    if (!resolvedStarts.isEmpty()) {
        state->starts.removeIf([&resolvedStarts](const Start &start) {
            return !resolvedStarts.contains(start.lineNumber);
        });
    }
}

static void parseLine(QStringView line, const std::optional<QRegularExpression> &regExp,
                      ParserState *state, SearchResultItems *items)
{
    if (line.isEmpty()) {
        state->filePath = {}; // Clear the parser state
        resetRun(state);
        return;
    }

    if (line.startsWith(':')) {
        state->filePath = FilePath::fromPathPart(QString::fromUtf8(line.mid(1).toLatin1()));
        resetRun(state);
        return;
    }

    OutputLine outputLine;
    if (!parseOutputLine(line, &outputLine)) {
        resetRun(state);
        return;
    }

    // A match across lines does not span a gap.
    if (!state->run.isEmpty() && state->run.last().lineNumber + 1 != outputLine.lineNumber)
        resetRun(state);

    // Lines that do not start inside a match. The columns of a hit are relative to one of them.
    const bool startsInsideMatch = !state->run.isEmpty() && !state->run.last().isContext
                                   && state->run.last().hits.isEmpty();
    if (!startsInsideMatch)
        state->starts.append(Start{outputLine.lineNumber});
    state->run.append(outputLine);

    appendHits(state, regExp, items);

    // A hit is reported on the last line of its match, and a match begins after the end of the
    // match before it. So the lines before the last line with hits or the last context line are
    // not needed any more.
    qsizetype keep = state->run.size() - 1;
    while (keep > 0 && state->run.at(keep).hits.isEmpty() && !state->run.at(keep).isContext)
        --keep;
    dropFirstLines(state, std::max(keep, state->run.size() - MaxRunLines));
}

// The input is the output of ag decoded as Latin-1, so that each character is one byte of it.
// The columns and lengths of the hits count these bytes, and the text in them is decoded as UTF-8.
SearchResultItems parse(const QFuture<void> &future, const QString &input,
                        const std::optional<QRegularExpression> &regExp, ParserState *state)
{
    QTC_ASSERT(state, return {});
    SearchResultItems items;

    // The output does not end at a line break, the rest is the beginning of the next line.
    QString remaining = state->remainder + input;
    state->remainder.clear();
    QStringView remainingInput(remaining);
    while (true) {
        if (future.isCanceled())
            return {};

        const int newLinePos = remainingInput.indexOf('\n');
        if (newLinePos < 0) {
            state->remainder = remainingInput.toString();
            break;
        }
        parseLine(remainingInput.left(newLinePos), regExp, state, &items);
        remainingInput = remainingInput.mid(newLinePos + 1);
    }
    return items;
}

SearchResultItems parse(const QString &input, const std::optional<QRegularExpression> &regExp)
{
    QPromise<void> promise;
    promise.start();
    ParserState state;
    return SilverSearcher::parse(promise.future(), input.endsWith('\n') ? input : input + '\n',
                                 regExp, &state);
}

} // namespace SilverSearcher
