// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ripgrepparser.h"

#include <utils/filepath.h>
#include <utils/filesearch.h>

#include <QFuture>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Utils;

namespace Ripgrep {

// A text or a path is given as "text", or as "bytes" in base 64 if it is not valid UTF-8. The
// offsets of the matches are those of the bytes of the lines.
static std::optional<QByteArray> bytesOf(const QJsonValue &value)
{
    const QJsonObject object = value.toObject();
    if (object.contains("text"))
        return object.value("text").toString().toUtf8();
    if (object.contains("bytes"))
        return QByteArray::fromBase64(object.value("bytes").toString().toLatin1());
    return {};
}

// The path is not decoded again from UTF-8 with the local 8-bit encoding, which is not UTF-8 on
// Windows.
static std::optional<QString> pathOf(const QJsonValue &value)
{
    const QJsonObject object = value.toObject();
    if (object.contains("text"))
        return object.value("text").toString();
    if (object.contains("bytes"))
        return QString::fromLocal8Bit(QByteArray::fromBase64(object.value("bytes").toString().toLatin1()));
    return {};
}

// Like the built-in engine, a result shows no more than this of the text of a line
const int maxLineTextSize = 400;

static QString clippedText(const QString &text)
{
    if (text.size() > maxLineTextSize)
        return text.left(maxLineTextSize) + QChar(0x2026); // '...'
    return text;
}

// The line and the column (in characters) of byte offsets in a text that starts on a given
// line. The submatches of ripgrep come in ascending order, so only the bytes since the previous
// offset are looked at, which keeps many matches on a long line cheap.
class PositionFinder
{
public:
    PositionFinder(const QByteArray &text, int firstLine)
        : m_text(text)
        , m_firstLine(firstLine)
    {}

    Text::Position positionOf(qsizetype offset)
    {
        if (offset < m_offset) { // Not expected, but start over then
            m_offset = 0;
            m_line = 0;
            m_column = 0;
        }
        qsizetype segmentStart = m_offset;
        for (qsizetype i = m_text.indexOf('\n', m_offset); i >= 0 && i < offset;
             i = m_text.indexOf('\n', i + 1)) {
            ++m_line;
            m_column = 0;
            segmentStart = i + 1;
        }
        m_column += int(QString::fromUtf8(
                            QByteArrayView(m_text).sliced(segmentStart, offset - segmentStart))
                            .size());
        m_offset = offset;
        return {m_firstLine + m_line, m_column};
    }

private:
    const QByteArray &m_text;
    const int m_firstLine;
    qsizetype m_offset = 0;
    int m_line = 0;
    int m_column = 0;
};

// The match of a line is reported with the text of all the lines it covers, and the offsets of
// the matches in it. A match across lines is a single match covering several of them.
static void appendMatches(const QJsonObject &data, const std::optional<QRegularExpression> &regExp,
                          SearchResultItems *items)
{
    const std::optional<QString> path = pathOf(data.value("path"));
    const std::optional<QByteArray> lines = bytesOf(data.value("lines"));
    if (!path || !lines)
        return;
    const FilePath filePath = FilePath::fromUserInput(*path);
    const int firstLine = data.value("line_number").toInt();
    const QByteArray &bytes = *lines;
    QStringList lineTexts;
    for (const QByteArray &line : bytes.split('\n')) {
        QString lineText = QString::fromUtf8(line);
        if (lineText.endsWith('\r'))
            lineText.chop(1);
        lineTexts << clippedText(lineText);
    }

    // The captured texts are recomputed with the match in the text of the lines as a document
    // has it, as an expression can look before and after the match.
    QString documentText;
    QList<qsizetype> lineStarts;
    if (regExp) {
        documentText = QString::fromUtf8(bytes);
        documentText.replace("\r\n", "\n");
        lineStarts << 0;
        for (qsizetype i = documentText.indexOf('\n'); i >= 0;
             i = documentText.indexOf('\n', i + 1)) {
            lineStarts << i + 1;
        }
    }

    // A match that takes the CR of a CRLF ends before the line break of a document, and one that
    // starts or ends between the CR and the LF is at the end of the line of a document
    const auto documentOffset = [&bytes](qsizetype offset) {
        const bool afterCarriageReturn = offset > 0 && offset < bytes.size()
                                         && bytes.at(offset - 1) == '\r' && bytes.at(offset) == '\n';
        return afterCarriageReturn ? offset - 1 : offset;
    };

    PositionFinder finder(bytes, firstLine);
    // The matches on the same lines share their text
    int textBeginLine = -1;
    int textLastLine = -1;
    QString text;
    const QJsonArray submatches = data.value("submatches").toArray();
    for (const QJsonValue &submatch : submatches) {
        const QJsonObject object = submatch.toObject();
        const qsizetype start = object.value("start").toInt();
        const qsizetype end = object.value("end").toInt();
        if (start < 0 || end < start || end > bytes.size())
            continue;
        const Text::Position begin = finder.positionOf(documentOffset(start));
        const Text::Position last = finder.positionOf(documentOffset(end));

        // A match that ends with a line break ends in the first column of the next line, which
        // is not part of the text of the lines.
        if (begin.line != textBeginLine || last.line != textLastLine) {
            textBeginLine = begin.line;
            textLastLine = last.line;
            QStringList texts;
            for (int line = begin.line; line <= last.line; ++line) {
                const qsizetype index = line - firstLine;
                texts << (index < lineTexts.size() ? lineTexts.at(index) : QString());
            }
            text = texts.size() == 1 ? texts.constFirst() : texts.join('\n');
        }

        SearchResultItem item;
        item.setFilePath(filePath);
        item.setMainRange(Text::Range{begin, last});
        item.setLineText(text);
        item.setUseTextEditorFont(true);
        // Only a regular expression search is replaced with its captured texts, of the match as
        // a document has it, with a single LF as the line break. The text was made from UTF-8,
        // so it need not be checked again for each match.
        if (regExp) {
            const qsizetype index = begin.line - firstLine;
            const qsizetype lastIndex = last.line - firstLine;
            if (index < lineStarts.size() && lastIndex < lineStarts.size()) {
                const qsizetype offset = qMin(lineStarts.at(index) + begin.column,
                                              documentText.size());
                const qsizetype lineEnd = lastIndex + 1 < lineStarts.size()
                                              ? lineStarts.at(lastIndex + 1) - 1
                                              : documentText.size();
                const qsizetype endOffset
                    = qMax(offset, qMin(lineStarts.at(lastIndex) + last.column, lineEnd));
                const QRegularExpression::MatchOptions options
                    = QRegularExpression::AnchorAtOffsetMatchOption
                      | QRegularExpression::DontCheckSubjectStringMatchOption;
                QRegularExpressionMatch match = regExp->matchView(
                    documentText, offset, QRegularExpression::NormalMatch, options);
                // The expression can match more than ripgrep, which does not let a match go past
                // the end of a line in a search that is not across lines. The captured texts
                // must not go past the range that they replace.
                if (match.capturedEnd(0) != endOffset) {
                    const QRegularExpressionMatch withinRange = regExp->matchView(
                        QStringView(documentText).first(endOffset), offset,
                        QRegularExpression::NormalMatch, options);
                    if (withinRange.hasMatch())
                        match = withinRange;
                }
                item.setUserData(match.capturedTexts());
            }
        }
        items->append(item);
    }
}

SearchResultItems parse(const QFuture<void> &future, const QString &input,
                        const std::optional<QRegularExpression> &regExp, QString *remainder)
{
    // The captured texts are those of the text of a document, where a line break is a single LF
    std::optional<QRegularExpression> documentRegExp;
    if (regExp) {
        const bool caseInsensitive = regExp->patternOptions() & QRegularExpression::CaseInsensitiveOption;
        documentRegExp = findRegularExpression(
            regExp->pattern(),
            FindRegularExpression | (caseInsensitive ? FindFlags() : FindFlags(FindCaseSensitively)));
    }

    SearchResultItems items;
    const QString text = *remainder + input;
    qsizetype pos = 0;
    while (true) {
        if (future.isCanceled())
            return {};
        const qsizetype lineEnd = text.indexOf('\n', pos);
        if (lineEnd < 0) {
            *remainder = text.mid(pos);
            break;
        }
        const QString line = text.mid(pos, lineEnd - pos);
        pos = lineEnd + 1;
        if (line.isEmpty())
            continue;
        const QJsonObject message = QJsonDocument::fromJson(line.toUtf8()).object();
        if (message.value("type").toString() == "match")
            appendMatches(message.value("data").toObject(), documentRegExp, &items);
    }
    return items;
}

SearchResultItems parse(const QString &input, const std::optional<QRegularExpression> &regExp)
{
    QPromise<void> promise;
    promise.start();
    QString remainder;
    return Ripgrep::parse(promise.future(), input, regExp, &remainder);
}

} // namespace Ripgrep
