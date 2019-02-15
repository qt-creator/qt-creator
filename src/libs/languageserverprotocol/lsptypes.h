/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#pragma once

#include "jsonkeys.h"
#include "jsonobject.h"
#include "lsputils.h"

#include <utils/fileutils.h>
#include <utils/optional.h>
#include <utils/variant.h>
#include <utils/link.h>

#include <QTextCursor>
#include <QJsonObject>
#include <QUrl>
#include <QList>

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT DocumentUri : public QUrl
{
public:
    DocumentUri() = default;
    Utils::FileName toFileName() const;

    static DocumentUri fromProtocol(const QString &uri) { return DocumentUri(uri); }
    static DocumentUri fromFileName(const Utils::FileName &file) { return DocumentUri(file); }

    operator QJsonValue() const { return QJsonValue(toString()); }

private:
    DocumentUri(const QString &other);
    DocumentUri(const Utils::FileName &other);

    friend class LanguageClientValue<QString>;
};

class LANGUAGESERVERPROTOCOL_EXPORT Position : public JsonObject
{
public:
    Position() = default;
    Position(int line, int character);
    Position(const QTextCursor &cursor);
    using JsonObject::JsonObject;

    // Line position in a document (zero-based).
    int line() const { return typedValue<int>(lineKey); }
    void setLine(int line) { insert(lineKey, line); }

    /*
     * Character offset on a line in a document (zero-based). Assuming that the line is
     * represented as a string, the `character` value represents the gap between the
     * `character` and `character + 1`.
     *
     * If the character value is greater than the line length it defaults back to the
     * line length.
     */
    int character() const { return typedValue<int>(characterKey); }
    void setCharacter(int character) { insert(characterKey, character); }

    bool isValid(QStringList *error) const override
    { return check<int>(error, lineKey) && check<int>(error, characterKey); }

    int toPositionInDocument(QTextDocument *doc) const;
    QTextCursor toTextCursor(QTextDocument *doc) const;
};

static bool operator<=(const Position &first, const Position &second)
{
    return first.line() < second.line()
            || (first.line() == second.line() && first.character() <= second.character());
}

class LANGUAGESERVERPROTOCOL_EXPORT Range : public JsonObject
{
public:
    Range() = default;
    Range(const Position &start, const Position &end);
    explicit Range(const QTextCursor &cursor);
    using JsonObject::JsonObject;

    // The range's start position.
    Position start() const { return typedValue<Position>(startKey); }
    void setStart(const Position &start) { insert(startKey, start); }

    // The range's end position.
    Position end() const { return typedValue<Position>(endKey); }
    void setEnd(const Position &end) { insert(endKey, end); }

    bool contains(const Position &pos) const { return start() <= pos && pos <= end(); }
    bool overlaps(const Range &range) const;

    bool isValid(QStringList *error) const override
    { return check<Position>(error, startKey) && check<Position>(error, endKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT Location : public JsonObject
{
public:
    using JsonObject::JsonObject;
    using JsonObject::operator=;

    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    Utils::Link toLink() const;

    bool isValid(QStringList *error) const override
    { return check<QString>(error, uriKey) && check<Range>(error, rangeKey); }
};

enum class DiagnosticSeverity
{
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4

};

class LANGUAGESERVERPROTOCOL_EXPORT Diagnostic : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The range at which the message applies.
    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    // The diagnostic's severity. Can be omitted. If omitted it is up to the
    // client to interpret diagnostics as error, warning, info or hint.
    Utils::optional<DiagnosticSeverity> severity() const;
    void setSeverity(const DiagnosticSeverity &severity)
    { insert(severityKey, static_cast<int>(severity)); }
    void clearSeverity() { remove(severityKey); }

    // The diagnostic's code, which might appear in the user interface.
    using Code = Utils::variant<int, QString>;
    Utils::optional<Code> code() const;
    void setCode(const Code &code);
    void clearCode() { remove(codeKey); }

    // A human-readable string describing the source of this
    // diagnostic, e.g. 'typescript' or 'super lint'.
    Utils::optional<QString> source() const
    { return optionalValue<QString>(sourceKey); }
    void setSource(const QString &source) { insert(sourceKey, source); }
    void clearSource() { remove(sourceKey); }

    // The diagnostic's message.
    QString message() const
    { return typedValue<QString>(messageKey); }
    void setMessage(const QString &message) { insert(messageKey, message); }

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT Command : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // Title of the command, like `save`.
    QString title() const { return typedValue<QString>(titleKey); }
    void setTitle(const QString &title) { insert(titleKey, title); }
    void clearTitle() { remove(titleKey); }

    // The identifier of the actual command handler.
    QString command() const { return typedValue<QString>(commandKey); }
    void setCommand(const QString &command) { insert(commandKey, command); }
    void clearCommand() { remove(commandKey); }

    // Arguments that the command handler should be invoked with.
    Utils::optional<QJsonArray> arguments() const { return typedValue<QJsonArray>(argumentsKey); }
    void setArguments(const QJsonArray &arguments) { insert(argumentsKey, arguments); }
    void clearArguments() { remove(argumentsKey); }

    bool isValid(QStringList *error) const override
    { return check<QString>(error, titleKey)
                && check<QString>(error, commandKey)
                && checkOptional<QJsonArray>(error, argumentsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT TextEdit : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The range of the text document to be manipulated. To insert
    // text into a document create a range where start === end.
    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    // The string to be inserted. For delete operations use an empty string.
    QString newText() const { return typedValue<QString>(newTextKey); }
    void setNewText(const QString &text) { insert(newTextKey, text); }

    bool isValid(QStringList *error) const override
    { return check<Range>(error, rangeKey) && check<QString>(error, newTextKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentIdentifier : public JsonObject
{
public:
    TextDocumentIdentifier() : TextDocumentIdentifier(DocumentUri()) {}
    TextDocumentIdentifier(const DocumentUri &uri) { setUri(uri); }
    using JsonObject::JsonObject;

    // The text document's URI.
    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    bool isValid(QStringList *error) const override { return check<QString>(error, uriKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT VersionedTextDocumentIdentifier : public TextDocumentIdentifier
{
public:
    using TextDocumentIdentifier::TextDocumentIdentifier;

    /*
     * The version number of this document. If a versioned text document identifier
     * is sent from the server to the client and the file is not open in the editor
     * (the server has not received an open notification before) the server can send
     * `null` to indicate that the version is known and the content on disk is the
     * truth (as speced with document content ownership)
     */
    LanguageClientValue<int> version() const { return clientValue<int>(versionKey); }
    void setVersion(LanguageClientValue<int> version) { insert(versionKey, version); }

    bool isValid(QStringList *error) const override
    { return TextDocumentIdentifier::isValid(error) && check<int, std::nullptr_t>(error, versionKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentEdit : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The text document to change.
    VersionedTextDocumentIdentifier id() const
    { return  typedValue<VersionedTextDocumentIdentifier>(idKey); }
    void setId(const VersionedTextDocumentIdentifier &id) { insert(idKey, id); }

    // The edits to be applied.
    QList<TextEdit> edits() const { return array<TextEdit>(editsKey); }
    void setEdits(const QList<TextEdit> edits) { insertArray(editsKey, edits); }

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceEdit : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // Holds changes to existing resources.
    using Changes = QMap<DocumentUri, QList<TextEdit>>;
    Utils::optional<Changes> changes() const;
    void setChanges(const Changes &changes);

    /*
     * An array of `TextDocumentEdit`s to express changes to n different text documents
     * where each text document edit addresses a specific version of a text document.
     * Whether a client supports versioned document edits is expressed via
     * `WorkspaceClientCapabilities.workspaceEdit.documentChanges`.
     *
     * Note: If the client can handle versioned document edits and if documentChanges are present,
     * the latter are preferred over changes.
     */
    Utils::optional<QList<TextDocumentEdit>> documentChanges() const
    { return optionalArray<TextDocumentEdit>(documentChangesKey); }
    void setDocumentChanges(const QList<TextDocumentEdit> &changes)
    { insertArray(changesKey, changes); }

    bool isValid(QStringList *error) const override
    { return checkOptionalArray<TextDocumentEdit>(error, documentChangesKey); }
};

LANGUAGESERVERPROTOCOL_EXPORT QMap<QString, QString> languageIds();

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentItem : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The text document's URI.
    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    // The text document's language identifier.
    QString languageId() const { return typedValue<QString>(languageIdKey); }
    void setLanguageId(const QString &id) { insert(languageIdKey, id); }

    // The version number of this document (it will increase after each change, including undo/redo
    int version() const { return typedValue<int>(versionKey); }
    void setVersion(int version) { insert(versionKey, version); }

    // The content of the opened text document.
    QString text() const { return typedValue<QString>(textKey); }
    void setText(const QString &text) { insert(textKey, text); }

    bool isValid(QStringList *error) const override;

    static QString mimeTypeToLanguageId(const Utils::MimeType &mimeType);
    static QString mimeTypeToLanguageId(const QString &mimeTypeName);
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentPositionParams : public JsonObject
{
public:
    TextDocumentPositionParams();
    TextDocumentPositionParams(const TextDocumentIdentifier &document, const Position &position);
    using JsonObject::JsonObject;

    // The text document.
    TextDocumentIdentifier textDocument() const
    { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const TextDocumentIdentifier &id) { insert(textDocumentKey, id); }

    // The position inside the text document.
    Position position() const { return typedValue<Position>(positionKey); }
    void setPosition(const Position &position) { insert(positionKey, position); }

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentFilter : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // A language id, like `typescript`.
    Utils::optional<QString> language() const { return optionalValue<QString>(languageKey); }
    void setLanguage(const QString &language) { insert(languageKey, language); }
    void clearLanguage() { remove(languageKey); }

    // A Uri [scheme](#Uri.scheme), like `file` or `untitled`.
    Utils::optional<QString> scheme() const { return optionalValue<QString>(schemeKey); }
    void setScheme(const QString &scheme) { insert(schemeKey, scheme); }
    void clearScheme() { remove(schemeKey); }

    // A glob pattern, like `*.{ts,js}`.
    Utils::optional<QString> pattern() const { return optionalValue<QString>(patternKey); }
    void setPattern(const QString &pattern) { insert(patternKey, pattern); }
    void clearPattern() { remove(patternKey); }

    bool applies(const Utils::FileName &fileName,
                 const Utils::MimeType &mimeType = Utils::MimeType()) const;

    bool isValid(QStringList *error) const override;
};

enum class MarkupKind
{
    plaintext,
    markdown,
};

class LANGUAGESERVERPROTOCOL_EXPORT MarkupContent : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The type of the Markup
    MarkupKind kind() const { return static_cast<MarkupKind>(typedValue<int>(kindKey)); }
    void setKind(MarkupKind kind) { insert(kindKey, static_cast<int>(kind)); }

    // The content itself
    QString content() const { return typedValue<QString>(contentKey); }
    void setContent(const QString &content) { insert(contentKey, content); }

    bool isValid(QStringList *error) const override
    { return check<int>(error, kindKey) && check<QString>(error, contentKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT MarkupOrString : public Utils::variant<QString, MarkupContent>
{
public:
    MarkupOrString() = default;
    MarkupOrString(const Utils::variant<QString, MarkupContent> &val);
    explicit MarkupOrString(const QString &val);
    explicit MarkupOrString(const MarkupContent &val);
    MarkupOrString(const QJsonValue &val);

    bool isValid(QStringList *error) const;

    QJsonValue toJson() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkSpaceFolder : public JsonObject
{
public:
    WorkSpaceFolder() = default;
    WorkSpaceFolder(const QString &uri, const QString &name);
    using JsonObject::JsonObject;

    // The associated URI for this workspace folder.
    QString uri() const { return typedValue<QString>(uriKey); }
    void setUri(const QString &uri) { insert(uriKey, uri); }

    // The name of the workspace folder. Defaults to the uri's basename.
    QString name() const { return typedValue<QString>(nameKey); }
    void setName(const QString &name) { insert(nameKey, name); }

    bool isValid(QStringList *error) const override
    { return check<QString>(error, uriKey) && check<QString>(error, nameKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT SymbolInformation : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString name() const { return typedValue<QString>(nameKey); }
    void setName(const QString &name) { insert(nameKey, name); }

    int kind() const { return typedValue<int>(kindKey); }
    void setKind(int kind) { insert(kindKey, kind); }

    Utils::optional<bool> deprecated() const { return optionalValue<bool>(deprecatedKey); }
    void setDeprecated(bool deprecated) { insert(deprecatedKey, deprecated); }
    void clearDeprecated() { remove(deprecatedKey); }

    Location location() const { return typedValue<Location>(locationKey); }
    void setLocation(const Location &location) { insert(locationKey, location); }

    Utils::optional<QString> containerName() const
    { return optionalValue<QString>(containerNameKey); }
    void setContainerName(const QString &containerName) { insert(containerNameKey, containerName); }
    void clearContainerName() { remove(containerNameKey); }

    bool isValid(QStringList *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentSymbol : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString name() const { return typedValue<QString>(nameKey); }
    void setName(const QString &name) { insert(nameKey, name); }

    Utils::optional<QString> detail() const { return optionalValue<QString>(detailKey); }
    void setDetail(const QString &detail) { insert(detailKey, detail); }
    void clearDetail() { remove(detailKey); }

    int kind() const { return typedValue<int>(kindKey); }
    void setKind(int kind) { insert(kindKey, kind); }

    Utils::optional<bool> deprecated() const { return optionalValue<bool>(deprecatedKey); }
    void setDeprecated(bool deprecated) { insert(deprecatedKey, deprecated); }
    void clearDeprecated() { remove(deprecatedKey); }

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(Range range) { insert(rangeKey, range); }

    Range selectionRange() const { return typedValue<Range>(selectionRangeKey); }
    void setSelectionRange(Range selectionRange) { insert(selectionRangeKey, selectionRange); }

    Utils::optional<QList<DocumentSymbol>> children() const
    { return optionalArray<DocumentSymbol>(childrenKey); }
    void setChildren(QList<DocumentSymbol> children) { insertArray(childrenKey, children); }
    void clearChildren() { remove(childrenKey); }
};

enum class SymbolKind {
    File = 1,
    FirstSymbolKind = File,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    String = 15,
    Number = 16,
    Boolean = 17,
    Array = 18,
    Object = 19,
    Key = 20,
    Null = 21,
    EnumMember = 22,
    Struct = 23,
    Event = 24,
    Operator = 25,
    TypeParameter = 26,
    LastSymbolKind = TypeParameter,
};

namespace CompletionItemKind {
enum Kind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19,
    EnumMember = 20,
    Constant = 21,
    Struct = 22,
    Event = 23,
    Operator = 24,
    TypeParameter = 25
};
} // namespace CompletionItemKind

} // namespace LanguageClient
