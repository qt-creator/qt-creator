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

#include "outputformatter.h"

#include "algorithm.h"
#include "ansiescapecodehandler.h"
#include "fileinprojectfinder.h"
#include "qtcassert.h"
#include "synchronousprocess.h"
#include "theme/theme.h"

#include <QDir>
#include <QFileInfo>
#include <QPair>
#include <QPlainTextEdit>
#include <QPointer>
#include <QRegularExpressionMatch>
#include <QTextCursor>

#include <numeric>

namespace Utils {

class OutputLineParser::Private
{
public:
    FilePaths searchDirs;
    QPointer<const OutputLineParser> redirectionDetector;
    bool skipFileExistsCheck = false;
    bool demoteErrorsToWarnings = false;
    FileInProjectFinder *fileFinder = nullptr;
};

OutputLineParser::OutputLineParser() : d(new Private) { }

OutputLineParser::~OutputLineParser() { delete d; }

Q_GLOBAL_STATIC_WITH_ARGS(QString, linkPrefix, {"olpfile://"})
Q_GLOBAL_STATIC_WITH_ARGS(QString, linkSep, {"::"})

QString OutputLineParser::createLinkTarget(const FilePath &filePath, int line = -1, int column = -1)
{
    return *linkPrefix() + filePath.toString() + *linkSep() + QString::number(line)
            + *linkSep() + QString::number(column);
}

bool OutputLineParser::isLinkTarget(const QString &target)
{
    return target.startsWith(*linkPrefix());
}

void OutputLineParser::parseLinkTarget(const QString &target, FilePath &filePath, int &line,
                                       int &column)
{
    const QStringList parts = target.mid(linkPrefix()->length()).split(*linkSep());
    if (parts.isEmpty())
        return;
    filePath = FilePath::fromString(parts.first());
    line = parts.length() > 1 ? parts.at(1).toInt() : 0;
    column = parts.length() > 2 ? parts.at(2).toInt() : 0;
}

// The redirection mechanism is needed for broken build tools (e.g. xcodebuild) that get invoked
// indirectly as part of the build process and redirect their child processes' stderr output
// to stdout. A parser might be able to detect this condition and inform interested
// other parsers that they need to interpret stdout data as stderr.
void OutputLineParser::setRedirectionDetector(const OutputLineParser *detector)
{
    d->redirectionDetector = detector;
}

bool OutputLineParser::needsRedirection() const
{
    return d->redirectionDetector && (d->redirectionDetector->hasDetectedRedirection()
                                      || d->redirectionDetector->needsRedirection());
}

void OutputLineParser::addSearchDir(const FilePath &dir)
{
    d->searchDirs << dir;
}

void OutputLineParser::dropSearchDir(const FilePath &dir)
{
    const int idx = d->searchDirs.lastIndexOf(dir);

    // TODO: This apparently triggers. Find out why and either remove the assertion (if it's legit)
    //       or fix the culprit.
    QTC_ASSERT(idx != -1, return);

    d->searchDirs.removeAt(idx);
}

const FilePaths OutputLineParser::searchDirectories() const
{
    return d->searchDirs;
}

void OutputLineParser::setFileFinder(FileInProjectFinder *finder)
{
    d->fileFinder = finder;
}

void OutputLineParser::setDemoteErrorsToWarnings(bool demote)
{
    d->demoteErrorsToWarnings = demote;
}

bool OutputLineParser::demoteErrorsToWarnings() const
{
    return d->demoteErrorsToWarnings;
}

FilePath OutputLineParser::absoluteFilePath(const FilePath &filePath)
{
    if (filePath.isEmpty() || filePath.toFileInfo().isAbsolute())
        return filePath;
    FilePaths candidates;
    for (const FilePath &dir : searchDirectories()) {
        const FilePath candidate = dir.pathAppended(filePath.toString());
        if (candidate.exists() || d->skipFileExistsCheck)
            candidates << candidate;
    }
    if (candidates.count() == 1)
        return FilePath::fromString(QDir::cleanPath(candidates.first().toString()));

    QString fp = filePath.toString();
    while (fp.startsWith("../"))
        fp.remove(0, 3);
    bool found = false;
    candidates = d->fileFinder->findFile(QUrl::fromLocalFile(fp), &found);
    if (found && candidates.size() == 1)
        return candidates.first();

    return filePath;
}

void OutputLineParser::addLinkSpecForAbsoluteFilePath(OutputLineParser::LinkSpecs &linkSpecs,
        const FilePath &filePath, int lineNo, int pos, int len)
{
    if (filePath.toFileInfo().isAbsolute())
        linkSpecs.append({pos, len, createLinkTarget(filePath, lineNo)});
}

void OutputLineParser::addLinkSpecForAbsoluteFilePath(OutputLineParser::LinkSpecs &linkSpecs,
        const FilePath &filePath, int lineNo, const QRegularExpressionMatch &match,
        int capIndex)
{
    addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineNo, match.capturedStart(capIndex),
                                   match.capturedLength(capIndex));
}
void OutputLineParser::addLinkSpecForAbsoluteFilePath(OutputLineParser::LinkSpecs &linkSpecs,
        const FilePath &filePath, int lineNo, const QRegularExpressionMatch &match,
                                                      const QString &capName)
{
    addLinkSpecForAbsoluteFilePath(linkSpecs, filePath, lineNo, match.capturedStart(capName),
                                   match.capturedLength(capName));
}

QString OutputLineParser::rightTrimmed(const QString &in)
{
    int pos = in.length();
    for (; pos > 0; --pos) {
        if (!in.at(pos - 1).isSpace())
            break;
    }
    return in.mid(0, pos);
}

#ifdef WITH_TESTS
void OutputLineParser::skipFileExistsCheck()
{
    d->skipFileExistsCheck = true;
}
#endif


class OutputFormatter::Private
{
public:
    QPlainTextEdit *plainTextEdit = nullptr;
    QTextCharFormat formats[NumberOfFormats];
    QTextCursor cursor;
    AnsiEscapeCodeHandler escapeCodeHandler;
    QPair<QString, OutputFormat> incompleteLine;
    optional<QTextCharFormat> formatOverride;
    QList<OutputLineParser *> lineParsers;
    OutputLineParser *nextParser = nullptr;
    FileInProjectFinder fileFinder;
    PostPrintAction postPrintAction;
    bool boldFontEnabled = true;
    bool prependCarriageReturn = false;
};

OutputFormatter::OutputFormatter() : d(new Private) { }

OutputFormatter::~OutputFormatter()
{
    qDeleteAll(d->lineParsers);
    delete d;
}

QPlainTextEdit *OutputFormatter::plainTextEdit() const
{
    return d->plainTextEdit;
}

void OutputFormatter::setPlainTextEdit(QPlainTextEdit *plainText)
{
    d->plainTextEdit = plainText;
    d->cursor = plainText ? plainText->textCursor() : QTextCursor();
    d->cursor.movePosition(QTextCursor::End);
    initFormats();
}

void OutputFormatter::setLineParsers(const QList<OutputLineParser *> &parsers)
{
    flush();
    qDeleteAll(d->lineParsers);
    d->lineParsers.clear();
    d->nextParser = nullptr;
    addLineParsers(parsers);
}

void OutputFormatter::addLineParsers(const QList<OutputLineParser *> &parsers)
{
    for (OutputLineParser * const p : qAsConst(parsers))
        addLineParser(p);
}

void OutputFormatter::addLineParser(OutputLineParser *parser)
{
    setupLineParser(parser);
    d->lineParsers << parser;
}

void OutputFormatter::setupLineParser(OutputLineParser *parser)
{
    parser->setFileFinder(&d->fileFinder);
    connect(parser, &OutputLineParser::newSearchDir, this, &OutputFormatter::addSearchDir);
    connect(parser, &OutputLineParser::searchDirExpired, this, &OutputFormatter::dropSearchDir);
}

void OutputFormatter::setFileFinder(const FileInProjectFinder &finder)
{
    d->fileFinder = finder;
}

void OutputFormatter::setDemoteErrorsToWarnings(bool demote)
{
    for (OutputLineParser * const p : qAsConst(d->lineParsers))
        p->setDemoteErrorsToWarnings(demote);
}

void OutputFormatter::overridePostPrintAction(const PostPrintAction &postPrintAction)
{
    d->postPrintAction = postPrintAction;
}

void OutputFormatter::doAppendMessage(const QString &text, OutputFormat format)
{
    const QTextCharFormat charFmt = charFormat(format);
    const QList<FormattedText> formattedText = parseAnsi(text, charFmt);
    const QString cleanLine = std::accumulate(formattedText.begin(), formattedText.end(), QString(),
            [](const FormattedText &t1, const FormattedText &t2) { return t1.text + t2.text; });
    QList<OutputLineParser *> involvedParsers;
    const OutputLineParser::Result res = handleMessage(cleanLine, format, involvedParsers);
    if (res.newContent) {
        append(res.newContent.value(), charFmt);
        return;
    }
    for (const FormattedText &output : linkifiedText(formattedText, res.linkSpecs))
        append(output.text, output.format);
    for (OutputLineParser * const p : qAsConst(involvedParsers)) {
        if (d->postPrintAction)
            d->postPrintAction(p);
        else
            p->runPostPrintActions();
    }
}

OutputLineParser::Result OutputFormatter::handleMessage(const QString &text, OutputFormat format,
                                                        QList<OutputLineParser *> &involvedParsers)
{
    // We only invoke the line parsers for stdout and stderr
    if (format != StdOutFormat && format != StdErrFormat)
        return OutputLineParser::Status::NotHandled;
    const OutputLineParser * const oldNextParser = d->nextParser;
    if (d->nextParser) {
        involvedParsers << d->nextParser;
        const OutputLineParser::Result res
                = d->nextParser->handleLine(text, outputTypeForParser(d->nextParser, format));
        switch (res.status) {
        case OutputLineParser::Status::Done:
            d->nextParser = nullptr;
            return res;
        case OutputLineParser::Status::InProgress:
            return res;
        case OutputLineParser::Status::NotHandled:
            d->nextParser = nullptr;
            break;
        }
    }
    QTC_CHECK(!d->nextParser);
    for (OutputLineParser * const parser : qAsConst(d->lineParsers)) {
        if (parser == oldNextParser) // We tried that one already.
            continue;
        const OutputLineParser::Result res
                = parser->handleLine(text, outputTypeForParser(parser, format));
        switch (res.status) {
        case OutputLineParser::Status::Done:
            involvedParsers << parser;
            return res;
        case OutputLineParser::Status::InProgress:
            involvedParsers << parser;
            d->nextParser = parser;
            return res;
        case OutputLineParser::Status::NotHandled:
            break;
        }
    }
    return OutputLineParser::Status::NotHandled;
}

QTextCharFormat OutputFormatter::charFormat(OutputFormat format) const
{
    return d->formatOverride ? d->formatOverride.value() : d->formats[format];
}

QList<FormattedText> OutputFormatter::parseAnsi(const QString &text, const QTextCharFormat &format)
{
    return d->escapeCodeHandler.parseText(FormattedText(text, format));
}

const QList<FormattedText> OutputFormatter::linkifiedText(
        const QList<FormattedText> &text, const OutputLineParser::LinkSpecs &linkSpecs)
{
    if (linkSpecs.isEmpty())
        return text;

    QList<FormattedText> linkified;
    int totalTextLengthSoFar = 0;
    int nextLinkSpecIndex = 0;

    for (const FormattedText &t : text) {
        const int totalPreviousTextLength = totalTextLengthSoFar;

        // There is no more linkification work to be done. Just copy the text as-is.
        if (nextLinkSpecIndex >= linkSpecs.size()) {
            linkified << t;
            continue;
        }

        for (int nextLocalTextPos = 0; nextLocalTextPos < t.text.size(); ) {

            // There are no more links in this part, so copy the rest of the text as-is.
            if (nextLinkSpecIndex >= linkSpecs.size()) {
                linkified << FormattedText(t.text.mid(nextLocalTextPos), t.format);
                totalTextLengthSoFar += t.text.length() - nextLocalTextPos;
                break;
            }

            const OutputLineParser::LinkSpec &linkSpec = linkSpecs.at(nextLinkSpecIndex);
            const int localLinkStartPos = linkSpec.startPos - totalPreviousTextLength;
            ++nextLinkSpecIndex;

            // We ignore links that would cross format boundaries.
            if (localLinkStartPos < nextLocalTextPos
                    || localLinkStartPos + linkSpec.length > t.text.length()) {
                linkified << FormattedText(t.text.mid(nextLocalTextPos), t.format);
                totalTextLengthSoFar += t.text.length() - nextLocalTextPos;
                break;
            }

            // Now we know we have a link that is fully inside this part of the text.
            // Split the text so that the link part gets the appropriate format.
            const int prefixLength = localLinkStartPos - nextLocalTextPos;
            const QString textBeforeLink = t.text.mid(nextLocalTextPos, prefixLength);
            linkified << FormattedText(textBeforeLink, t.format);
            const QString linkedText = t.text.mid(localLinkStartPos, linkSpec.length);
            linkified << FormattedText(linkedText, linkFormat(t.format, linkSpec.target));
            nextLocalTextPos = localLinkStartPos + linkSpec.length;
            totalTextLengthSoFar += prefixLength + linkSpec.length;
        }
    }
    return linkified;
}

void OutputFormatter::append(const QString &text, const QTextCharFormat &format)
{
    if (!plainTextEdit())
        return;
    int startPos = 0;
    int crPos = -1;
    while ((crPos = text.indexOf('\r', startPos)) >= 0)  {
        d->cursor.insertText(text.mid(startPos, crPos - startPos), format);
        d->cursor.clearSelection();
        d->cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        startPos = crPos + 1;
    }
    if (startPos < text.count())
        d->cursor.insertText(text.mid(startPos), format);
}

QTextCharFormat OutputFormatter::linkFormat(const QTextCharFormat &inputFormat, const QString &href)
{
    QTextCharFormat result = inputFormat;
    result.setForeground(creatorTheme()->color(Theme::TextColorLink));
    result.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    result.setAnchor(true);
    result.setAnchorHref(href);
    return result;
}

#ifdef WITH_TESTS
void OutputFormatter::overrideTextCharFormat(const QTextCharFormat &fmt)
{
    d->formatOverride = fmt;
}

QList<OutputLineParser *> OutputFormatter::lineParsers() const
{
    return d->lineParsers;
}
#endif // WITH_TESTS

void OutputFormatter::clearLastLine()
{
    // Note that this approach will fail if the text edit is not read-only and users
    // have messed with the last line between programmatic inputs.
    // We live with this risk, as all the alternatives are worse.
    if (!d->cursor.atEnd())
        d->cursor.movePosition(QTextCursor::End);
    d->cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    d->cursor.removeSelectedText();
}

void OutputFormatter::initFormats()
{
    if (!plainTextEdit())
        return;

    Theme *theme = creatorTheme();
    d->formats[NormalMessageFormat].setForeground(theme->color(Theme::OutputPanes_NormalMessageTextColor));
    d->formats[ErrorMessageFormat].setForeground(theme->color(Theme::OutputPanes_ErrorMessageTextColor));
    d->formats[LogMessageFormat].setForeground(theme->color(Theme::OutputPanes_WarningMessageTextColor));
    d->formats[StdOutFormat].setForeground(theme->color(Theme::OutputPanes_StdOutTextColor));
    d->formats[StdErrFormat].setForeground(theme->color(Theme::OutputPanes_StdErrTextColor));
    d->formats[DebugFormat].setForeground(theme->color(Theme::OutputPanes_DebugTextColor));
    setBoldFontEnabled(d->boldFontEnabled);
}

void OutputFormatter::flushIncompleteLine()
{
    clearLastLine();
    doAppendMessage(d->incompleteLine.first, d->incompleteLine.second);
    d->incompleteLine.first.clear();
}

void OutputFormatter::dumpIncompleteLine(const QString &line, OutputFormat format)
{
    if (line.isEmpty())
        return;
    append(line, charFormat(format));
    d->incompleteLine.first.append(line);
    d->incompleteLine.second = format;
}

bool OutputFormatter::handleFileLink(const QString &href)
{
    if (!OutputLineParser::isLinkTarget(href))
        return false;
    FilePath filePath;
    int line;
    int column;
    OutputLineParser::parseLinkTarget(href, filePath, line, column);
    QTC_ASSERT(!filePath.isEmpty(), return false);
    emit openInEditorRequested(filePath, line, column);
    return true;
}

void OutputFormatter::handleLink(const QString &href)
{
    QTC_ASSERT(!href.isEmpty(), return);
    // We can handle absolute file paths ourselves. Other types of references are forwarded
    // to the line parsers.
    if (handleFileLink(href))
        return;
    for (OutputLineParser * const f : qAsConst(d->lineParsers)) {
        if (f->handleLink(href))
            return;
    }
}

void OutputFormatter::clear()
{
    if (plainTextEdit())
        plainTextEdit()->clear();
}

void OutputFormatter::reset()
{
    d->prependCarriageReturn = false;
    d->incompleteLine.first.clear();
    d->nextParser = nullptr;
    qDeleteAll(d->lineParsers);
    d->lineParsers.clear();
    d->fileFinder = FileInProjectFinder();
    d->formatOverride.reset();
    d->escapeCodeHandler = AnsiEscapeCodeHandler();
}

void OutputFormatter::setBoldFontEnabled(bool enabled)
{
    d->boldFontEnabled = enabled;
    const QFont::Weight fontWeight = enabled ? QFont::Bold : QFont::Normal;
    d->formats[NormalMessageFormat].setFontWeight(fontWeight);
    d->formats[ErrorMessageFormat].setFontWeight(fontWeight);
}

void OutputFormatter::flush()
{
    if (!d->incompleteLine.first.isEmpty())
        flushIncompleteLine();
    d->escapeCodeHandler.endFormatScope();
    for (OutputLineParser * const p : qAsConst(d->lineParsers))
        p->flush();
    if (d->nextParser)
        d->nextParser->runPostPrintActions();
}

bool OutputFormatter::hasFatalErrors() const
{
    return anyOf(d->lineParsers, [](const OutputLineParser *p) {
        return p->hasFatalErrors();
    });
}

void OutputFormatter::addSearchDir(const FilePath &dir)
{
    for (OutputLineParser * const p : qAsConst(d->lineParsers))
        p->addSearchDir(dir);
}

void OutputFormatter::dropSearchDir(const FilePath &dir)
{
    for (OutputLineParser * const p : qAsConst(d->lineParsers))
        p->dropSearchDir(dir);
}

OutputFormat OutputFormatter::outputTypeForParser(const OutputLineParser *parser,
                                                  OutputFormat type) const
{
    if (type == StdOutFormat && parser->needsRedirection())
        return StdErrFormat;
    return type;
}

void OutputFormatter::appendMessage(const QString &text, OutputFormat format)
{
    if (text.isEmpty())
        return;

    // If we have an existing incomplete line and its format is different from this one,
    // then we consider the two messages unrelated. We re-insert the previous incomplete line,
    // possibly formatted now, and start from scratch with the new input.
    if (!d->incompleteLine.first.isEmpty() && d->incompleteLine.second != format)
        flushIncompleteLine();

    QString out = text;
    if (d->prependCarriageReturn) {
        d->prependCarriageReturn = false;
        out.prepend('\r');
    }
    out = SynchronousProcess::normalizeNewlines(out);
    if (out.endsWith('\r')) {
        d->prependCarriageReturn = true;
        out.chop(1);
    }

    // If the input is a single incomplete line, we do not forward it to the specialized
    // formatting code, but simply dump it as-is. Once it becomes complete or it needs to
    // be flushed for other reasons, we remove the unformatted part and re-insert it, this
    // time with proper formatting.
    if (!out.contains('\n')) {
        dumpIncompleteLine(out, format);
        return;
    }

    // We have at least one complete line, so let's remove the previously dumped
    // incomplete line and prepend it to the first line of our new input.
    if (!d->incompleteLine.first.isEmpty()) {
        clearLastLine();
        out.prepend(d->incompleteLine.first);
        d->incompleteLine.first.clear();
    }

    // Forward all complete lines to the specialized formatting code, and handle a
    // potential trailing incomplete line the same way as above.
    for (int startPos = 0; ;) {
        const int eolPos = out.indexOf('\n', startPos);
        if (eolPos == -1) {
            dumpIncompleteLine(out.mid(startPos), format);
            break;
        }
        doAppendMessage(out.mid(startPos, eolPos - startPos + 1), format);
        startPos = eolPos + 1;
    }
}

} // namespace Utils
