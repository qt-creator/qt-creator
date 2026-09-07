// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lsputils.h"

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>

#include <QRegularExpression>
#include <QUrl>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

namespace LanguageServerProtocol {

QString uriFromPath(const Utils::FilePath &path)
{
    return QUrl::fromLocalFile(path.path()).toString();
}

Utils::FilePath pathFromUri(const QString &uri)
{
    // QUrl decodes the percent-encoding of the path, and only of the path: a
    // uri decoded as a whole beforehand loses a literal '%' in a file name to
    // a second round of decoding, and ends at the first '#'.
    const QUrl url(uri);
    if (!url.isLocalFile())
        return {};
    return Utils::FilePath::fromUserInput(url.toLocalFile());
}

Position positionOf(const QTextCursor &cursor)
{
    return Position().line(cursor.blockNumber()).character(cursor.positionInBlock());
}

int positionInDocument(const Position &position, const QTextDocument *document)
{
    const QTextBlock block = document->findBlockByNumber(position.line());
    if (!block.isValid())
        return -1;
    if (block.length() <= position.character())
        return block.position() + block.length() - 1;
    return block.position() + position.character();
}

QTextCursor toTextCursor(const Position &position, QTextDocument *document)
{
    QTextCursor cursor(document);
    cursor.setPosition(positionInDocument(position, document));
    return cursor;
}

Position withOffset(const Position &position, int offset, const QTextDocument *document)
{
    int line = 0;
    int character = 0;
    Utils::Text::convertPosition(document, positionInDocument(position, document) + offset,
                                 &line, &character);
    return Position().line(line - 1).character(character);
}

QString plainText(const std::variant<QString, MarkupContent> &value)
{
    if (const auto text = std::get_if<QString>(&value))
        return *text;
    return std::get<MarkupContent>(value).value();
}

static QHash<Utils::MimeType, QString> mimeTypeLanguageIdMap()
{
    static QHash<Utils::MimeType, QString> hash;
    if (!hash.isEmpty())
        return hash;
    const QPair<QString, QString> languageIdsForMimeTypeNames[] = {
        {"text/x-python", "python"},
        {"text/x-bibtex", "bibtex"},
        {"application/vnd.coffeescript", "coffeescript"},
        {"text/x-chdr", "c"},
        {"text/x-csrc", "c"},
        {"text/x-c++hdr", "cpp"},
        {"text/x-c++src", "cpp"},
        {"text/x-moc", "cpp"},
        {"text/x-csharp", "csharp"},
        {"text/vnd.qtcreator.git.commit", "git-commit"},
        {"text/vnd.qtcreator.git.rebase", "git-rebase"},
        {"text/x-go", "go"},
        {"text/html", "html"},
        {"text/x-java", "java"},
        {"application/javascript", "javascript"},
        {"application/json", "json"},
        {"text/x-tex", "tex"},
        {"text/x-lua", "lua"},
        {"text/x-makefile", "makefile"},
        {"text/markdown", "markdown"},
        {"text/x-objcsrc", "objective-c"},
        {"text/x-objc++src", "objective-cpp"},
        {"application/x-perl", "perl"},
        {"application/x-php", "php"},
        {"application/x-ruby", "ruby"},
        {"text/rust", "rust"},
        {"text/x-sass", "sass"},
        {"text/x-scss", "scss"},
        {"application/x-shellscript", "shellscript"},
        {"application/xml", "xml"},
        {"application/xslt+xml", "xsl"},
        {"application/x-yaml", "yaml"},
        {"text/x-swift", "swift"},
    };
    for (const QPair<QString, QString> &languageIdForMimeTypeName : languageIdsForMimeTypeNames) {
        const Utils::MimeType mimeType = Utils::mimeTypeForName(languageIdForMimeTypeName.first);
        if (mimeType.isValid())
            hash[mimeType] = languageIdForMimeTypeName.second;
    }
    return hash;
}

QString mimeTypeToLanguageId(const Utils::MimeType &mimeType)
{
    return mimeTypeLanguageIdMap().value(mimeType);
}

QString mimeTypeToLanguageId(const QString &mimeTypeName)
{
    return mimeTypeToLanguageId(Utils::mimeTypeForName(mimeTypeName));
}

static QString expressionForGlob(QString globPattern)
{
    const QString anySubDir("qtc_anysubdir_id");
    globPattern.replace("**/", anySubDir);
    QString regexp = QRegularExpression::wildcardToRegularExpression(globPattern);
    regexp.replace(anySubDir, "(.*[/\\\\])*");
    regexp.replace("\\{", "(");
    regexp.replace("\\}", ")");
    regexp.replace(",", "|");
    return regexp;
}

static bool patternApplies(const GlobPattern &pattern, const Utils::FilePath &filePath)
{
    const Pattern *plain = std::get_if<Pattern>(&pattern);
    const QString glob = plain ? *plain : std::get<RelativePattern>(pattern).pattern();
    const QRegularExpression regexp(expressionForGlob(glob),
                                    QRegularExpression::CaseInsensitiveOption);
    return regexp.isValid() && regexp.match(filePath.path()).hasMatch();
}

static bool languageApplies(const QString &language, const Utils::FilePath &filePath,
                            const Utils::MimeType &mimeType)
{
    const auto matches = [&language](const Utils::MimeType &type) {
        return language == mimeTypeToLanguageId(type);
    };
    if (mimeType.isValid() && matches(mimeType))
        return true;
    return Utils::anyOf(Utils::mimeTypesForFileName(filePath.toFSPathString()), matches);
}

bool applies(const DocumentFilter &filter, const Utils::FilePath &filePath,
             const Utils::MimeType &mimeType)
{
    const auto textFilter = std::get_if<TextDocumentFilter>(&filter);
    if (!textFilter)
        return false;
    std::optional<QString> language;
    std::optional<GlobPattern> pattern;
    if (const auto languageFilter = std::get_if<TextDocumentFilterLanguage>(textFilter)) {
        language = languageFilter->language();
        pattern = languageFilter->pattern();
    } else if (const auto schemeFilter = std::get_if<TextDocumentFilterScheme>(textFilter)) {
        language = schemeFilter->language();
        pattern = schemeFilter->pattern();
    } else {
        const auto &patternFilter = std::get<TextDocumentFilterPattern>(*textFilter);
        language = patternFilter.language();
        pattern = patternFilter.pattern();
    }
    if (pattern && patternApplies(*pattern, filePath))
        return true;
    if (language)
        return languageApplies(*language, filePath, mimeType);
    return !pattern;
}

bool applies(const std::optional<DocumentSelector> &selector, const Utils::FilePath &filePath,
             const Utils::MimeType &mimeType)
{
    if (!selector)
        return true;
    return Utils::anyOf(*selector, [&](const DocumentFilter &filter) {
        return applies(filter, filePath, mimeType);
    });
}

Range rangeOf(const QTextCursor &cursor)
{
    Range range;
    int line = 0;
    int character = 0;
    Utils::Text::convertPosition(cursor.document(), cursor.selectionStart(), &line, &character);
    if (line <= 0 || character < 0)
        return range;
    range.start(Position().line(line - 1).character(character));
    Utils::Text::convertPosition(cursor.document(), cursor.selectionEnd(), &line, &character);
    if (line <= 0 || character < 0)
        return range;
    range.end(Position().line(line - 1).character(character));
    return range;
}

QTextCursor toSelection(const Range &range, QTextDocument *document)
{
    QTC_ASSERT(document, return {});
    QTextCursor cursor = toTextCursor(range.start(), document);
    cursor.setPosition(positionInDocument(range.end(), document), QTextCursor::KeepAnchor);
    return cursor;
}

bool contains(const Range &range, const Range &other)
{
    return range.start() <= other.start() && other.end() <= range.end();
}

bool isLeftOf(const Range &range, const Range &other)
{
    if (isEmpty(range) || isEmpty(other))
        return range.end() < other.start();
    return range.end() <= other.start();
}

bool overlaps(const Range &range, const Range &other)
{
    return !isLeftOf(range, other) && !isLeftOf(other, range);
}

} // namespace LanguageServerProtocol
