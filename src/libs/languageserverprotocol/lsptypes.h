// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonkeys.h"
#include "jsonobject.h"
#include "lsputils.h"

#include <utils/filepath.h>
#include <utils/link.h>
#include <utils/mimeutils.h>
#include <utils/textutils.h>

#include <QTextCursor>
#include <QJsonObject>
#include <QUrl>
#include <QList>

#include <functional>
#include <optional>
#include <variant>

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT DocumentUri : public QUrl
{
public:
    DocumentUri() = default;
    using PathMapper = std::function<Utils::FilePath(const Utils::FilePath &)>;
    Utils::FilePath toFilePath(const PathMapper &mapToHostPath) const;

    static DocumentUri fromProtocol(const QString &uri);
    static DocumentUri fromFilePath(const Utils::FilePath &file, const PathMapper &mapToServerPath);

    operator QJsonValue() const { return QJsonValue(toString()); }

private:
    DocumentUri(const QString &other);
    DocumentUri(const Utils::FilePath &other, const PathMapper &mapToServerPath);

    friend class LanguageClientValue<QString>;
};

class LANGUAGESERVERPROTOCOL_EXPORT Position : public JsonObject
{
public:
    Position() = default;
    Position(int line, int character);
    explicit Position(const QTextCursor &cursor);
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

    bool isValid() const override
    { return contains(lineKey) && contains(characterKey); }

    int toPositionInDocument(const QTextDocument *doc) const;
    QTextCursor toTextCursor(QTextDocument *doc) const;
    Position withOffset(int offset, const QTextDocument *doc) const;
};

inline bool operator<(const Position &first, const Position &second)
{
    return first.line() < second.line()
            || (first.line() == second.line() && first.character() < second.character());
}

inline bool operator>(const Position &first, const Position &second)
{
    return second < first;
}

inline bool operator>=(const Position &first, const Position &second)
{
    return !(first < second);
}

inline bool operator<=(const Position &first, const Position &second)
{
    return !(first > second);
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

    bool isEmpty() const { return start() == end(); }
    bool contains(const Position &pos) const { return start() <= pos && pos <= end(); }
    bool contains(const Range &other) const;
    bool overlaps(const Range &range) const;
    bool isLeftOf(const Range &other) const
    { return isEmpty() || other.isEmpty() ? end() < other.start() : end() <= other.start(); }

    QTextCursor toSelection(QTextDocument *doc) const;

    bool isValid() const override
    { return JsonObject::contains(startKey) && JsonObject::contains(endKey); }
};

inline bool operator==(const Range &r1, const Range &r2)
{
    return r1.contains(r2) && r2.contains(r1);
}
inline bool operator!=(const Range &r1, const Range &r2) { return !(r1 == r2); }

class LANGUAGESERVERPROTOCOL_EXPORT Location : public JsonObject
{
public:
    using JsonObject::JsonObject;

    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(const Range &range) { insert(rangeKey, range); }

    Utils::Link toLink(const DocumentUri::PathMapper &mapToHostPath) const;

    bool isValid() const override { return contains(uriKey) && contains(rangeKey); }
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
    std::optional<DiagnosticSeverity> severity() const;
    void setSeverity(const DiagnosticSeverity &severity)
    { insert(severityKey, static_cast<int>(severity)); }
    void clearSeverity() { remove(severityKey); }

    // The diagnostic's code, which might appear in the user interface.
    using Code = std::variant<int, QString>;
    std::optional<Code> code() const;
    void setCode(const Code &code);
    void clearCode() { remove(codeKey); }

    // A human-readable string describing the source of this
    // diagnostic, e.g. 'typescript' or 'super lint'.
    std::optional<QString> source() const
    { return optionalValue<QString>(sourceKey); }
    void setSource(const QString &source) { insert(sourceKey, source); }
    void clearSource() { remove(sourceKey); }

    // The diagnostic's message.
    QString message() const
    { return typedValue<QString>(messageKey); }
    void setMessage(const QString &message) { insert(messageKey, message); }

    bool isValid() const override { return contains(rangeKey) && contains(messageKey); }
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
    std::optional<QJsonArray> arguments() const { return typedValue<QJsonArray>(argumentsKey); }
    void setArguments(const QJsonArray &arguments) { insert(argumentsKey, arguments); }
    void clearArguments() { remove(argumentsKey); }

    bool isValid() const override { return contains(titleKey) && contains(commandKey); }
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

    bool isValid() const override
    { return contains(rangeKey) && contains(newTextKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentIdentifier : public JsonObject
{
public:
    TextDocumentIdentifier() : TextDocumentIdentifier(DocumentUri()) {}
    explicit TextDocumentIdentifier(const DocumentUri &uri) { setUri(uri); }
    using JsonObject::JsonObject;

    // The text document's URI.
    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    bool isValid() const override { return contains(uriKey); }
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

    bool isValid() const override
    {
        return TextDocumentIdentifier::isValid() && contains(versionKey);
    }
};

class LANGUAGESERVERPROTOCOL_EXPORT TextDocumentEdit : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The text document to change.
    VersionedTextDocumentIdentifier textDocument() const
    { return  typedValue<VersionedTextDocumentIdentifier>(textDocumentKey); }
    void setTextDocument(const VersionedTextDocumentIdentifier &textDocument)
    { insert(textDocumentKey, textDocument); }

    // The edits to be applied.
    QList<TextEdit> edits() const { return array<TextEdit>(editsKey); }
    void setEdits(const QList<TextEdit> edits) { insertArray(editsKey, edits); }

    bool isValid() const override { return contains(textDocumentKey) && contains(editsKey); }
};

class CreateFileOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    std::optional<bool> overwrite() const { return optionalValue<bool>(overwriteKey); }
    void setOverwrite(bool overwrite) { insert(overwriteKey, overwrite); }
    void clearOverwrite() { remove(overwriteKey); }

    std::optional<bool> ignoreIfExists() const { return optionalValue<bool>(ignoreIfExistsKey); }
    void setIgnoreIfExists(bool ignoreIfExists) { insert(ignoreIfExistsKey, ignoreIfExists); }
    void clearIgnoreIfExists() { remove(ignoreIfExistsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT CreateFileOperation : public JsonObject
{
public:
    using JsonObject::JsonObject;
    CreateFileOperation();

    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    std::optional<CreateFileOptions> options() const
    { return optionalValue<CreateFileOptions>(optionsKey); }
    void setOptions(const CreateFileOptions &options) { insert(optionsKey, options); }
    void clearOptions() { remove(optionsKey); }

    QString message(const DocumentUri::PathMapper &mapToHostPath) const;

    bool isValid() const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT RenameFileOperation : public JsonObject
{
public:
    using JsonObject::JsonObject;
    RenameFileOperation();

    DocumentUri oldUri() const { return DocumentUri::fromProtocol(typedValue<QString>(oldUriKey)); }
    void setOldUri(const DocumentUri &oldUri) { insert(oldUriKey, oldUri); }

    DocumentUri newUri() const { return DocumentUri::fromProtocol(typedValue<QString>(newUriKey)); }
    void setNewUri(const DocumentUri &newUri) { insert(newUriKey, newUri); }

    std::optional<CreateFileOptions> options() const
    { return optionalValue<CreateFileOptions>(optionsKey); }
    void setOptions(const CreateFileOptions &options) { insert(optionsKey, options); }
    void clearOptions() { remove(optionsKey); }

    QString message(const DocumentUri::PathMapper &mapToHostPath) const;

    bool isValid() const override;
};

class DeleteFileOptions : public JsonObject
{
public:
    using JsonObject::JsonObject;

    std::optional<bool> recursive() const { return optionalValue<bool>(recursiveKey); }
    void setRecursive(bool recursive) { insert(recursiveKey, recursive); }
    void clearRecursive() { remove(recursiveKey); }

    std::optional<bool> ignoreIfNotExists() const { return optionalValue<bool>(ignoreIfNotExistsKey); }
    void setIgnoreIfNotExists(bool ignoreIfNotExists) { insert(ignoreIfNotExistsKey, ignoreIfNotExists); }
    void clearIgnoreIfNotExists() { remove(ignoreIfNotExistsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DeleteFileOperation : public JsonObject
{
public:
    using JsonObject::JsonObject;
    DeleteFileOperation();

    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    std::optional<DeleteFileOptions> options() const
    { return optionalValue<DeleteFileOptions>(optionsKey); }
    void setOptions(const DeleteFileOptions &options) { insert(optionsKey, options); }
    void clearOptions() { remove(optionsKey); }

    QString message(const DocumentUri::PathMapper &mapToHostPath) const;

    bool isValid() const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentChange
    : public std::variant<TextDocumentEdit, CreateFileOperation, RenameFileOperation, DeleteFileOperation>
{
public:
    using variant::variant;
    DocumentChange(const QJsonValue &value);

    bool isValid() const;

    operator const QJsonValue() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceEdit : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // Holds changes to existing resources.
    using Changes = QMap<DocumentUri, QList<TextEdit>>;
    std::optional<Changes> changes() const;
    void setChanges(const Changes &changes);

    /*
     * Depending on the client capability
     * `workspace.workspaceEdit.resourceOperations` document changes are either
     * an array of `TextDocumentEdit`s to express changes to n different text
     * documents where each text document edit addresses a specific version of
     * a text document. Or it can contain above `TextDocumentEdit`s mixed with
     * create, rename and delete file / folder operations.
     *
     * Whether a client supports versioned document edits is expressed via
     * `workspace.workspaceEdit.documentChanges` client capability.
     *
     * If a client neither supports `documentChanges` nor
     * `workspace.workspaceEdit.resourceOperations` then only plain `TextEdit`s
     * using the `changes` property are supported.
     */
    std::optional<QList<DocumentChange>> documentChanges() const
    { return optionalArray<DocumentChange>(documentChangesKey); }
    void setDocumentChanges(const QList<DocumentChange> &changes)
    { insertArray(documentChangesKey, changes); }
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

    bool isValid() const override;

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

    bool isValid() const override { return contains(textDocumentKey) && contains(positionKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentFilter : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // A language id, like `typescript`.
    std::optional<QString> language() const { return optionalValue<QString>(languageKey); }
    void setLanguage(const QString &language) { insert(languageKey, language); }
    void clearLanguage() { remove(languageKey); }

    // A Uri [scheme](#Uri.scheme), like `file` or `untitled`.
    std::optional<QString> scheme() const { return optionalValue<QString>(schemeKey); }
    void setScheme(const QString &scheme) { insert(schemeKey, scheme); }
    void clearScheme() { remove(schemeKey); }

    /**
     * A glob pattern, like `*.{ts,js}`.
     *
     * Glob patterns can have the following syntax:
     * - `*` to match one or more characters in a path segment
     * - `?` to match on one character in a path segment
     * - `**` to match any number of path segments, including none
     * - `{}` to group sub patterns into an OR expression. (e.g. `*.{ts,js}`
     *   matches all TypeScript and JavaScript files)
     * - `[]` to declare a range of characters to match in a path segment
     *   (e.g., `example.[0-9]` to match on `example.0`, `example.1`, …)
     * - `[!...]` to negate a range of characters to match in a path segment
     *   (e.g., `example.[!0-9]` to match on `example.a`, `example.b`, but
     *   not `example.0`)
     */
    std::optional<QString> pattern() const { return optionalValue<QString>(patternKey); }
    void setPattern(const QString &pattern) { insert(patternKey, pattern); }
    void clearPattern() { remove(patternKey); }

    bool applies(const Utils::FilePath &fileName,
                 const Utils::MimeType &mimeType = Utils::MimeType()) const;
};

class LANGUAGESERVERPROTOCOL_EXPORT MarkupKind
{
public:
    enum Value { plaintext, markdown };
    MarkupKind() = default;
    MarkupKind(const Value value)
        : m_value(value)
    {}
    explicit MarkupKind(const QJsonValue &value);

    operator QJsonValue() const;
    Value value() const { return m_value; }

    bool operator==(const Value &value) const { return m_value == value; }

    bool isValid() const { return true; }
private:
    Value m_value = plaintext;
};

class LANGUAGESERVERPROTOCOL_EXPORT MarkupContent : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // The type of the Markup
    MarkupKind kind() const { return MarkupKind(value(kindKey)); }
    void setKind(MarkupKind kind) { insert(kindKey, kind); }
    Qt::TextFormat textFormat() const
    { return kind() == MarkupKind::markdown ? Qt::MarkdownText : Qt::PlainText; }

    // The content itself
    QString content() const { return typedValue<QString>(contentKey); }
    void setContent(const QString &content) { insert(contentKey, content); }

    bool isValid() const override
    { return contains(kindKey) && contains(contentKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT MarkupOrString : public std::variant<QString, MarkupContent>
{
public:
    MarkupOrString() = default;
    explicit MarkupOrString(const std::variant<QString, MarkupContent> &val);
    explicit MarkupOrString(const QString &val);
    explicit MarkupOrString(const MarkupContent &val);
    MarkupOrString(const QJsonValue &val);

    bool isValid() const { return true; }

    QJsonValue toJson() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkSpaceFolder : public JsonObject
{
public:
    WorkSpaceFolder() = default;
    WorkSpaceFolder(const DocumentUri &uri, const QString &name);
    using JsonObject::JsonObject;

    // The associated URI for this workspace folder.
    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    // The name of the workspace folder. Defaults to the uri's basename.
    QString name() const { return typedValue<QString>(nameKey); }
    void setName(const QString &name) { insert(nameKey, name); }

    bool isValid() const override
    { return contains(uriKey) && contains(nameKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT SymbolInformation : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString name() const { return typedValue<QString>(nameKey); }
    void setName(const QString &name) { insert(nameKey, name); }

    int kind() const { return typedValue<int>(kindKey); }
    void setKind(int kind) { insert(kindKey, kind); }

    std::optional<bool> deprecated() const { return optionalValue<bool>(deprecatedKey); }
    void setDeprecated(bool deprecated) { insert(deprecatedKey, deprecated); }
    void clearDeprecated() { remove(deprecatedKey); }

    Location location() const { return typedValue<Location>(locationKey); }
    void setLocation(const Location &location) { insert(locationKey, location); }

    std::optional<QString> containerName() const
    { return optionalValue<QString>(containerNameKey); }
    void setContainerName(const QString &containerName) { insert(containerNameKey, containerName); }
    void clearContainerName() { remove(containerNameKey); }

    bool isValid() const override
    { return contains(nameKey) && contains(kindKey) && contains(locationKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DocumentSymbol : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString name() const { return typedValue<QString>(nameKey); }
    void setName(const QString &name) { insert(nameKey, name); }

    std::optional<QString> detail() const { return optionalValue<QString>(detailKey); }
    void setDetail(const QString &detail) { insert(detailKey, detail); }
    void clearDetail() { remove(detailKey); }

    int kind() const { return typedValue<int>(kindKey); }
    void setKind(int kind) { insert(kindKey, kind); }

    std::optional<bool> deprecated() const { return optionalValue<bool>(deprecatedKey); }
    void setDeprecated(bool deprecated) { insert(deprecatedKey, deprecated); }
    void clearDeprecated() { remove(deprecatedKey); }

    Range range() const { return typedValue<Range>(rangeKey); }
    void setRange(Range range) { insert(rangeKey, range); }

    Range selectionRange() const { return typedValue<Range>(selectionRangeKey); }
    void setSelectionRange(Range selectionRange) { insert(selectionRangeKey, selectionRange); }

    std::optional<QList<DocumentSymbol>> children() const
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
