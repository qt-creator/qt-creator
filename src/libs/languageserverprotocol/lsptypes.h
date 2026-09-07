/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 python3 \
  scripts/generate_cpp_from_schema.py \
  src/libs/languageserverprotocol/lsp.schema.json src/libs/languageserverprotocol/lsptypes.h --namespace LanguageServerProtocol --cpp-output src/libs/languageserverprotocol/lsptypes.cpp --export-macro LANGUAGESERVERPROTOCOL_EXPORT --export-header languageserverprotocol_global.h --no-cxx20
*/
#pragma once

#include "languageserverprotocol_global.h"

#include <utils/result.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariant>

#include <cmath>
#include <limits>
#include <memory>
#include <variant>

namespace LanguageServerProtocol {

/**
 * Optional value stored indirectly, for types that contain themselves.
 */
template<typename T>
class Recursive
{
public:
    Recursive() = default;
    Recursive(const T &value) : m_value(std::make_unique<T>(value)) {}
    Recursive(const Recursive &other) { *this = other; }
    Recursive(Recursive &&other) = default;

    Recursive &operator=(const Recursive &other)
    {
        if (this != &other)
            m_value = other.m_value ? std::make_unique<T>(*other.m_value) : nullptr;
        return *this;
    }
    Recursive &operator=(Recursive &&other) = default;
    Recursive &operator=(const T &value)
    {
        m_value = std::make_unique<T>(value);
        return *this;
    }

    bool operator==(const Recursive &other) const
    {
        if (!m_value || !other.m_value)
            return !m_value && !other.m_value;
        return *m_value == *other.m_value;
    }

    bool has_value() const { return m_value != nullptr; }
    explicit operator bool() const { return has_value(); }
    const T &operator*() const { return *m_value; }
    const T *operator->() const { return m_value.get(); }

private:
    std::unique_ptr<T> m_value;
};

template<typename T> Utils::Result<T> fromJson(const QJsonValue &val) = delete;

// Defs that carry no constraints beyond "an object" alias to QJsonObject; these
// let such aliases take part in the generated conversions unchanged.
template<> inline Utils::Result<QJsonObject> fromJson<QJsonObject>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError(QString("Expected JSON object"));
    return val.toObject();
}

inline QJsonObject toJson(const QJsonObject &data) { return data; }

template<typename T>
Utils::Result<T> fromJson(const QString &field, const QJsonValue &val)
{
    const Utils::Result<T> result = fromJson<T>(val);
    if (result)
        return result;
    return Utils::ResultError(field + ": " + result.error());
}

template<>
inline Utils::Result<int> fromJson<int>(const QJsonValue &val)
{
    if (!val.isDouble())
        return Utils::ResultError(QString("Expected a number"));
    return val.toInt();
}

inline QJsonValue toJsonValue(int value) { return value; }

/**
 * Position in a text document expressed as zero-based line and character
 * offset. Prior to 3.17 the offsets were always based on a UTF-16 string
 * representation. So a string of the form `aU+10400b` the character offset of the
 * character `a` is 0, the character offset of `U+10400` is 1 and the character
 * offset of b is 3 since `U+10400` is represented using two code units in UTF-16.
 * Since 3.17 clients and servers can agree on a different string encoding
 * representation (e.g. UTF-8). The client announces it's supported encoding
 * via the client capability [`general.positionEncodings`](https://microsoft.github.io/language-server-protocol/specifications/specification-current/#clientCapabilities).
 * The value is an array of position encodings the client supports, with
 * decreasing preference (e.g. the encoding at index `0` is the most preferred
 * one). To stay backwards compatible the only mandatory encoding is UTF-16
 * represented via the string `utf-16`. The server can pick one of the
 * encodings offered by the client and signals that encoding back to the
 * client via the initialize result's property
 * [`capabilities.positionEncoding`](https://microsoft.github.io/language-server-protocol/specifications/specification-current/#serverCapabilities). If the string value
 * `utf-16` is missing from the client's capability `general.positionEncodings`
 * servers can safely assume that the client supports UTF-16. If the server
 * omits the position encoding in its initialize result the encoding defaults
 * to the string value `utf-16`. Implementation considerations: since the
 * conversion from one encoding into another requires the content of the
 * file / line the conversion is best done where the file is read which is
 * usually on the server side.
 *
 * Positions are line end character agnostic. So you can not specify a position
 * that denotes `\r|\n` or `\n|` where `|` represents the character offset.
 *
 * @since 3.17.0 - support for negotiated position encoding.
 */
struct Position {
    int _line{};  //!< Line position in a document (zero-based).
    /**
     * Character offset on a line in a document (zero-based).
     *
     * The meaning of this offset is determined by the negotiated
     * `PositionEncodingKind`.
     */
    int _character{};

    Position& line(int v) { _line = v; return *this; }
    Position& character(int v) { _character = v; return *this; }

    const int& line() const { return _line; }
    const int& character() const { return _character; }

    bool operator==(const Position &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Position> fromJson<Position>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const Position &data);

using ProgressToken = std::variant<int, QString>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ProgressToken> fromJson<ProgressToken>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ProgressToken &val);

/** A literal to identify a text document in the client. */
struct TextDocumentIdentifier {
    QString _uri{};  //!< The text document's uri.

    TextDocumentIdentifier& uri(const QString & v) { _uri = v; return *this; }

    const QString& uri() const { return _uri; }

    bool operator==(const TextDocumentIdentifier &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentIdentifier> fromJson<TextDocumentIdentifier>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentIdentifier &data);

struct ImplementationParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};

    ImplementationParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    ImplementationParams& position(const Position & v) { _position = v; return *this; }
    ImplementationParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    ImplementationParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }

    bool operator==(const ImplementationParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ImplementationParams> fromJson<ImplementationParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ImplementationParams &data);

/**
 * A range in a text document expressed as (zero-based) start and end positions.
 *
 * If you want to specify a range that contains a line including the line ending
 * character(s) then use an end position denoting the start of the next line.
 * For example:
 * ```ts
 * {
 * start: { line: 5, character: 23 }
 * end : { line 6, character : 0 }
 * }
 * ```
 */
struct Range {
    Position _start{};  //!< The range's start position.
    Position _end{};  //!< The range's end position.

    Range& start(const Position & v) { _start = v; return *this; }
    Range& end(const Position & v) { _end = v; return *this; }

    const Position& start() const { return _start; }
    const Position& end() const { return _end; }

    bool operator==(const Range &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Range> fromJson<Range>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const Range &data);

/**
 * Represents a location inside a resource, such as a line
 * inside a text file.
 */
struct Location {
    QString _uri{};
    Range _range{};

    Location& uri(const QString & v) { _uri = v; return *this; }
    Location& range(const Range & v) { _range = v; return *this; }

    const QString& uri() const { return _uri; }
    const Range& range() const { return _range; }

    bool operator==(const Location &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Location> fromJson<Location>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const Location &data);

using Pattern = QString;
template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Pattern> fromJson<Pattern>(const QJsonValue &val);

/** A workspace folder inside a client. */
struct WorkspaceFolder {
    QString _uri{};  //!< The associated URI for this workspace folder.
    /**
     * The name of the workspace folder. Used to refer to this
     * workspace folder in the user interface.
     */
    QString _name{};

    WorkspaceFolder& uri(const QString & v) { _uri = v; return *this; }
    WorkspaceFolder& name(const QString & v) { _name = v; return *this; }

    const QString& uri() const { return _uri; }
    const QString& name() const { return _name; }

    bool operator==(const WorkspaceFolder &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceFolder> fromJson<WorkspaceFolder>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceFolder &data);

using RelativePatternBaseUri = std::variant<WorkspaceFolder, QString>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RelativePatternBaseUri> fromJson<RelativePatternBaseUri>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const RelativePatternBaseUri &val);

/**
 * A relative pattern is a helper to construct glob patterns that are matched
 * relatively to a base URI. The common value for a `baseUri` is a workspace
 * folder root, but it can be another absolute URI as well.
 *
 * @since 3.17.0
 */
struct RelativePattern {
    /**
     * A workspace folder or a base URI to which this pattern will be matched
     * against relatively.
     */
    RelativePatternBaseUri _baseUri{};
    Pattern _pattern{};  //!< The actual glob pattern;

    RelativePattern& baseUri(const RelativePatternBaseUri & v) { _baseUri = v; return *this; }
    RelativePattern& pattern(const Pattern & v) { _pattern = v; return *this; }

    const RelativePatternBaseUri& baseUri() const { return _baseUri; }
    const Pattern& pattern() const { return _pattern; }

    bool operator==(const RelativePattern &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RelativePattern> fromJson<RelativePattern>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RelativePattern &data);

/**
 * The glob pattern. Either a string pattern or a relative pattern.
 *
 * @since 3.17.0
 */
using GlobPattern = std::variant<Pattern, RelativePattern>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<GlobPattern> fromJson<GlobPattern>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const GlobPattern &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const GlobPattern &val);

/**
 * A notebook document filter where `notebookType` is required field.
 *
 * @since 3.18.0
 */
struct NotebookDocumentFilterNotebookType {
    QString _notebookType{};  //!< The type of the enclosing notebook.
    std::optional<QString> _scheme{};  //!< A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
    std::optional<GlobPattern> _pattern{};  //!< A glob pattern.

    NotebookDocumentFilterNotebookType& notebookType(const QString & v) { _notebookType = v; return *this; }
    NotebookDocumentFilterNotebookType& scheme(const std::optional<QString> & v) { _scheme = v; return *this; }
    NotebookDocumentFilterNotebookType& pattern(const std::optional<GlobPattern> & v) { _pattern = v; return *this; }

    const QString& notebookType() const { return _notebookType; }
    const std::optional<QString>& scheme() const { return _scheme; }
    const std::optional<GlobPattern>& pattern() const { return _pattern; }

    bool operator==(const NotebookDocumentFilterNotebookType &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentFilterNotebookType> fromJson<NotebookDocumentFilterNotebookType>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentFilterNotebookType &data);

/**
 * A notebook document filter where `pattern` is required field.
 *
 * @since 3.18.0
 */
struct NotebookDocumentFilterPattern {
    std::optional<QString> _notebookType{};  //!< The type of the enclosing notebook.
    std::optional<QString> _scheme{};  //!< A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
    GlobPattern _pattern{};  //!< A glob pattern.

    NotebookDocumentFilterPattern& notebookType(const std::optional<QString> & v) { _notebookType = v; return *this; }
    NotebookDocumentFilterPattern& scheme(const std::optional<QString> & v) { _scheme = v; return *this; }
    NotebookDocumentFilterPattern& pattern(const GlobPattern & v) { _pattern = v; return *this; }

    const std::optional<QString>& notebookType() const { return _notebookType; }
    const std::optional<QString>& scheme() const { return _scheme; }
    const GlobPattern& pattern() const { return _pattern; }

    bool operator==(const NotebookDocumentFilterPattern &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentFilterPattern> fromJson<NotebookDocumentFilterPattern>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentFilterPattern &data);

/**
 * A notebook document filter where `scheme` is required field.
 *
 * @since 3.18.0
 */
struct NotebookDocumentFilterScheme {
    std::optional<QString> _notebookType{};  //!< The type of the enclosing notebook.
    QString _scheme{};  //!< A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
    std::optional<GlobPattern> _pattern{};  //!< A glob pattern.

    NotebookDocumentFilterScheme& notebookType(const std::optional<QString> & v) { _notebookType = v; return *this; }
    NotebookDocumentFilterScheme& scheme(const QString & v) { _scheme = v; return *this; }
    NotebookDocumentFilterScheme& pattern(const std::optional<GlobPattern> & v) { _pattern = v; return *this; }

    const std::optional<QString>& notebookType() const { return _notebookType; }
    const QString& scheme() const { return _scheme; }
    const std::optional<GlobPattern>& pattern() const { return _pattern; }

    bool operator==(const NotebookDocumentFilterScheme &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentFilterScheme> fromJson<NotebookDocumentFilterScheme>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentFilterScheme &data);

/**
 * A notebook document filter denotes a notebook document by
 * different properties. The properties will be match
 * against the notebook's URI (same as with documents)
 *
 * @since 3.17.0
 */
using NotebookDocumentFilter = std::variant<NotebookDocumentFilterNotebookType, NotebookDocumentFilterScheme, NotebookDocumentFilterPattern>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentFilter> fromJson<NotebookDocumentFilter>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentFilter &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const NotebookDocumentFilter &val);

using NotebookCellTextDocumentFilterNotebook = std::variant<QString, NotebookDocumentFilter>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookCellTextDocumentFilterNotebook> fromJson<NotebookCellTextDocumentFilterNotebook>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const NotebookCellTextDocumentFilterNotebook &val);

/**
 * A notebook cell text document filter denotes a cell text
 * document by different properties.
 *
 * @since 3.17.0
 */
struct NotebookCellTextDocumentFilter {
    /**
     * A filter that matches against the notebook
     * containing the notebook cell. If a string
     * value is provided it matches against the
     * notebook type. '*' matches every notebook.
     */
    NotebookCellTextDocumentFilterNotebook _notebook{};
    /**
     * A language id like `python`.
     *
     * Will be matched against the language id of the
     * notebook cell document. '*' matches every language.
     */
    std::optional<QString> _language{};

    NotebookCellTextDocumentFilter& notebook(const NotebookCellTextDocumentFilterNotebook & v) { _notebook = v; return *this; }
    NotebookCellTextDocumentFilter& language(const std::optional<QString> & v) { _language = v; return *this; }

    const NotebookCellTextDocumentFilterNotebook& notebook() const { return _notebook; }
    const std::optional<QString>& language() const { return _language; }

    bool operator==(const NotebookCellTextDocumentFilter &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookCellTextDocumentFilter> fromJson<NotebookCellTextDocumentFilter>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookCellTextDocumentFilter &data);

/**
 * A document filter where `language` is required field.
 *
 * @since 3.18.0
 */
struct TextDocumentFilterLanguage {
    QString _language{};  //!< A language id, like `typescript`.
    std::optional<QString> _scheme{};  //!< A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
    /**
     * A glob pattern, like **\/\*.{ts,js}. See TextDocumentFilter for examples.
     *
     * @since 3.18.0 - support for relative patterns. Whether clients support
     * relative patterns depends on the client capability
     * `textDocuments.filters.relativePatternSupport`.
     */
    std::optional<GlobPattern> _pattern{};

    TextDocumentFilterLanguage& language(const QString & v) { _language = v; return *this; }
    TextDocumentFilterLanguage& scheme(const std::optional<QString> & v) { _scheme = v; return *this; }
    TextDocumentFilterLanguage& pattern(const std::optional<GlobPattern> & v) { _pattern = v; return *this; }

    const QString& language() const { return _language; }
    const std::optional<QString>& scheme() const { return _scheme; }
    const std::optional<GlobPattern>& pattern() const { return _pattern; }

    bool operator==(const TextDocumentFilterLanguage &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentFilterLanguage> fromJson<TextDocumentFilterLanguage>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentFilterLanguage &data);

/**
 * A document filter where `pattern` is required field.
 *
 * @since 3.18.0
 */
struct TextDocumentFilterPattern {
    std::optional<QString> _language{};  //!< A language id, like `typescript`.
    std::optional<QString> _scheme{};  //!< A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
    /**
     * A glob pattern, like **\/\*.{ts,js}. See TextDocumentFilter for examples.
     *
     * @since 3.18.0 - support for relative patterns. Whether clients support
     * relative patterns depends on the client capability
     * `textDocuments.filters.relativePatternSupport`.
     */
    GlobPattern _pattern{};

    TextDocumentFilterPattern& language(const std::optional<QString> & v) { _language = v; return *this; }
    TextDocumentFilterPattern& scheme(const std::optional<QString> & v) { _scheme = v; return *this; }
    TextDocumentFilterPattern& pattern(const GlobPattern & v) { _pattern = v; return *this; }

    const std::optional<QString>& language() const { return _language; }
    const std::optional<QString>& scheme() const { return _scheme; }
    const GlobPattern& pattern() const { return _pattern; }

    bool operator==(const TextDocumentFilterPattern &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentFilterPattern> fromJson<TextDocumentFilterPattern>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentFilterPattern &data);

/**
 * A document filter where `scheme` is required field.
 *
 * @since 3.18.0
 */
struct TextDocumentFilterScheme {
    std::optional<QString> _language{};  //!< A language id, like `typescript`.
    QString _scheme{};  //!< A Uri {@link Uri.scheme scheme}, like `file` or `untitled`.
    /**
     * A glob pattern, like **\/\*.{ts,js}. See TextDocumentFilter for examples.
     *
     * @since 3.18.0 - support for relative patterns. Whether clients support
     * relative patterns depends on the client capability
     * `textDocuments.filters.relativePatternSupport`.
     */
    std::optional<GlobPattern> _pattern{};

    TextDocumentFilterScheme& language(const std::optional<QString> & v) { _language = v; return *this; }
    TextDocumentFilterScheme& scheme(const QString & v) { _scheme = v; return *this; }
    TextDocumentFilterScheme& pattern(const std::optional<GlobPattern> & v) { _pattern = v; return *this; }

    const std::optional<QString>& language() const { return _language; }
    const QString& scheme() const { return _scheme; }
    const std::optional<GlobPattern>& pattern() const { return _pattern; }

    bool operator==(const TextDocumentFilterScheme &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentFilterScheme> fromJson<TextDocumentFilterScheme>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentFilterScheme &data);

/**
 * A document filter denotes a document by different properties like
 * the {@link TextDocument.languageId language}, the {@link Uri.scheme scheme} of
 * its resource, or a glob-pattern that is applied to the {@link TextDocument.fileName path}.
 *
 * Glob patterns can have the following syntax:
 * - `*` to match zero or more characters in a path segment
 * - `?` to match on one character in a path segment
 * - `**` to match any number of path segments, including none
 * - `{}` to group sub patterns into an OR expression. (e.g. `**\/\*.{ts,js}` matches all TypeScript and JavaScript files)
 * - `[]` to declare a range of characters to match in a path segment (e.g., `example.[0-9]` to match on `example.0`, `example.1`, ...)
 * - `[!...]` to negate a range of characters to match in a path segment (e.g., `example.[!0-9]` to match on `example.a`, `example.b`, but not `example.0`)
 *
 * @sample A language filter that applies to typescript files on disk: `{ language: 'typescript', scheme: 'file' }`
 * @sample A language filter that applies to all package.json paths: `{ language: 'json', pattern: '**package.json' }`
 *
 * @since 3.17.0
 */
using TextDocumentFilter = std::variant<TextDocumentFilterLanguage, TextDocumentFilterScheme, TextDocumentFilterPattern>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentFilter> fromJson<TextDocumentFilter>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentFilter &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const TextDocumentFilter &val);

/**
 * A document filter describes a top level text document or
 * a notebook cell document.
 *
 * @since 3.17.0 - support for NotebookCellTextDocumentFilter.
 */
using DocumentFilter = std::variant<TextDocumentFilter, NotebookCellTextDocumentFilter>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentFilter> fromJson<DocumentFilter>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentFilter &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const DocumentFilter &val);

/**
 * A document selector is the combination of one or many document filters.
 *
 * @sample `let sel:DocumentSelector = [{ language: 'typescript' }, { language: 'json', pattern: '**\/tsconfig.json' }]`;
 *
 * The use of a string as a document filter is deprecated @since 3.16.0.
 */
using DocumentSelector = QList<DocumentFilter>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentSelector> fromJson<DocumentSelector>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const DocumentSelector &data);

struct ImplementationRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    ImplementationRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    ImplementationRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    ImplementationRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const ImplementationRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ImplementationRegistrationOptions> fromJson<ImplementationRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ImplementationRegistrationOptions &data);

struct TypeDefinitionParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};

    TypeDefinitionParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    TypeDefinitionParams& position(const Position & v) { _position = v; return *this; }
    TypeDefinitionParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    TypeDefinitionParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }

    bool operator==(const TypeDefinitionParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeDefinitionParams> fromJson<TypeDefinitionParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeDefinitionParams &data);

struct TypeDefinitionRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    TypeDefinitionRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    TypeDefinitionRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    TypeDefinitionRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const TypeDefinitionRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeDefinitionRegistrationOptions> fromJson<TypeDefinitionRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeDefinitionRegistrationOptions &data);

/** The workspace folder change event. */
struct WorkspaceFoldersChangeEvent {
    QList<WorkspaceFolder> _added{};  //!< The array of added workspace folders
    QList<WorkspaceFolder> _removed{};  //!< The array of the removed workspace folders

    WorkspaceFoldersChangeEvent& added(const QList<WorkspaceFolder> & v) { _added = v; return *this; }
    WorkspaceFoldersChangeEvent& addAdded(const WorkspaceFolder & v) { _added.append(v); return *this; }
    WorkspaceFoldersChangeEvent& removed(const QList<WorkspaceFolder> & v) { _removed = v; return *this; }
    WorkspaceFoldersChangeEvent& addRemoved(const WorkspaceFolder & v) { _removed.append(v); return *this; }

    const QList<WorkspaceFolder>& added() const { return _added; }
    const QList<WorkspaceFolder>& removed() const { return _removed; }

    bool operator==(const WorkspaceFoldersChangeEvent &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceFoldersChangeEvent> fromJson<WorkspaceFoldersChangeEvent>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceFoldersChangeEvent &data);

/** The parameters of a `workspace/didChangeWorkspaceFolders` notification. */
struct DidChangeWorkspaceFoldersParams {
    WorkspaceFoldersChangeEvent _event{};  //!< The actual workspace folder change event.

    DidChangeWorkspaceFoldersParams& event(const WorkspaceFoldersChangeEvent & v) { _event = v; return *this; }

    const WorkspaceFoldersChangeEvent& event() const { return _event; }

    bool operator==(const DidChangeWorkspaceFoldersParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidChangeWorkspaceFoldersParams> fromJson<DidChangeWorkspaceFoldersParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidChangeWorkspaceFoldersParams &data);

struct ConfigurationItem {
    std::optional<QString> _scopeUri{};  //!< The scope to get the configuration section for.
    std::optional<QString> _section{};  //!< The configuration section asked for.

    ConfigurationItem& scopeUri(const std::optional<QString> & v) { _scopeUri = v; return *this; }
    ConfigurationItem& section(const std::optional<QString> & v) { _section = v; return *this; }

    const std::optional<QString>& scopeUri() const { return _scopeUri; }
    const std::optional<QString>& section() const { return _section; }

    bool operator==(const ConfigurationItem &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ConfigurationItem> fromJson<ConfigurationItem>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ConfigurationItem &data);

/** The parameters of a configuration request. */
struct ConfigurationParams {
    QList<ConfigurationItem> _items{};

    ConfigurationParams& items(const QList<ConfigurationItem> & v) { _items = v; return *this; }
    ConfigurationParams& addItem(const ConfigurationItem & v) { _items.append(v); return *this; }

    const QList<ConfigurationItem>& items() const { return _items; }

    bool operator==(const ConfigurationParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ConfigurationParams> fromJson<ConfigurationParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ConfigurationParams &data);

/** Parameters for a {@link DocumentColorRequest}. */
struct DocumentColorParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The text document.

    DocumentColorParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    DocumentColorParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    DocumentColorParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }

    bool operator==(const DocumentColorParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentColorParams> fromJson<DocumentColorParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentColorParams &data);

/** Represents a color in RGBA space. */
struct Color {
    double _red{};  //!< The red component of this color in the range [0-1].
    double _green{};  //!< The green component of this color in the range [0-1].
    double _blue{};  //!< The blue component of this color in the range [0-1].
    double _alpha{};  //!< The alpha component of this color in the range [0-1].

    Color& red(double v) { _red = v; return *this; }
    Color& green(double v) { _green = v; return *this; }
    Color& blue(double v) { _blue = v; return *this; }
    Color& alpha(double v) { _alpha = v; return *this; }

    const double& red() const { return _red; }
    const double& green() const { return _green; }
    const double& blue() const { return _blue; }
    const double& alpha() const { return _alpha; }

    bool operator==(const Color &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Color> fromJson<Color>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const Color &data);

/** Represents a color range from a document. */
struct ColorInformation {
    Range _range{};  //!< The range in the document where this color appears.
    Color _color{};  //!< The actual color value for this color range.

    ColorInformation& range(const Range & v) { _range = v; return *this; }
    ColorInformation& color(const Color & v) { _color = v; return *this; }

    const Range& range() const { return _range; }
    const Color& color() const { return _color; }

    bool operator==(const ColorInformation &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ColorInformation> fromJson<ColorInformation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ColorInformation &data);

struct DocumentColorRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    DocumentColorRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    DocumentColorRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    DocumentColorRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const DocumentColorRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentColorRegistrationOptions> fromJson<DocumentColorRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentColorRegistrationOptions &data);

/** Parameters for a {@link ColorPresentationRequest}. */
struct ColorPresentationParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Color _color{};  //!< The color to request presentations for.
    Range _range{};  //!< The range where the color would be inserted. Serves as a context.

    ColorPresentationParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    ColorPresentationParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    ColorPresentationParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    ColorPresentationParams& color(const Color & v) { _color = v; return *this; }
    ColorPresentationParams& range(const Range & v) { _range = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Color& color() const { return _color; }
    const Range& range() const { return _range; }

    bool operator==(const ColorPresentationParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ColorPresentationParams> fromJson<ColorPresentationParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ColorPresentationParams &data);

/** A text edit applicable to a text document. */
struct TextEdit {
    /**
     * The range of the text document to be manipulated. To insert
     * text into a document create a range where start === end.
     */
    Range _range{};
    /**
     * The string to be inserted. For delete operations use an
     * empty string.
     */
    QString _newText{};

    TextEdit& range(const Range & v) { _range = v; return *this; }
    TextEdit& newText(const QString & v) { _newText = v; return *this; }

    const Range& range() const { return _range; }
    const QString& newText() const { return _newText; }

    bool operator==(const TextEdit &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextEdit> fromJson<TextEdit>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextEdit &data);

struct ColorPresentation {
    /**
     * The label of this color presentation. It will be shown on the color
     * picker header. By default this is also the text that is inserted when selecting
     * this color presentation.
     */
    QString _label{};
    /**
     * An {@link TextEdit edit} which is applied to a document when selecting
     * this presentation for the color.  When `falsy` the {@link ColorPresentation.label label}
     * is used.
     */
    std::optional<TextEdit> _textEdit{};
    /**
     * An optional array of additional {@link TextEdit text edits} that are applied when
     * selecting this color presentation. Edits must not overlap with the main {@link ColorPresentation.textEdit edit} nor with themselves.
     */
    std::optional<QList<TextEdit>> _additionalTextEdits{};

    ColorPresentation& label(const QString & v) { _label = v; return *this; }
    ColorPresentation& textEdit(const std::optional<TextEdit> & v) { _textEdit = v; return *this; }
    ColorPresentation& additionalTextEdits(const std::optional<QList<TextEdit>> & v) { _additionalTextEdits = v; return *this; }
    ColorPresentation& addAdditionalTextEdit(const TextEdit & v) { if (!_additionalTextEdits) _additionalTextEdits = QList<TextEdit>{}; (*_additionalTextEdits).append(v); return *this; }

    const QString& label() const { return _label; }
    const std::optional<TextEdit>& textEdit() const { return _textEdit; }
    const std::optional<QList<TextEdit>>& additionalTextEdits() const { return _additionalTextEdits; }

    bool operator==(const ColorPresentation &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ColorPresentation> fromJson<ColorPresentation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ColorPresentation &data);

/** Parameters for a {@link FoldingRangeRequest}. */
struct FoldingRangeParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The text document.

    FoldingRangeParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    FoldingRangeParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    FoldingRangeParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }

    bool operator==(const FoldingRangeParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FoldingRangeParams> fromJson<FoldingRangeParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FoldingRangeParams &data);

/** A set of predefined range kinds. */
using FoldingRangeKind = QString;

namespace FoldingRangeKinds {
    constexpr char Comment[] = "comment";
    constexpr char Imports[] = "imports";
    constexpr char Region[] = "region";
} // namespace FoldingRangeKinds
/**
 * Represents a folding range. To be valid, start and end line must be bigger than zero and smaller
 * than the number of lines in the document. Clients are free to ignore invalid ranges.
 */
struct FoldingRange {
    /**
     * The zero-based start line of the range to fold. The folded area starts after the line's last character.
     * To be valid, the end must be zero or larger and smaller than the number of lines in the document.
     */
    int _startLine{};
    std::optional<int> _startCharacter{};  //!< The zero-based character offset from where the folded range starts. If not defined, defaults to the length of the start line.
    /**
     * The zero-based end line of the range to fold. The folded area ends with the line's last character.
     * To be valid, the end must be zero or larger and smaller than the number of lines in the document.
     */
    int _endLine{};
    std::optional<int> _endCharacter{};  //!< The zero-based character offset before the folded range ends. If not defined, defaults to the length of the end line.
    /**
     * Describes the kind of the folding range such as 'comment' or 'region'. The kind
     * is used to categorize folding ranges and used by commands like 'Fold all comments'.
     * See {@link FoldingRangeKind} for an enumeration of standardized kinds.
     */
    std::optional<FoldingRangeKind> _kind{};
    /**
     * The text that the client should show when the specified range is
     * collapsed. If not defined or not supported by the client, a default
     * will be chosen by the client.
     *
     * @since 3.17.0
     */
    std::optional<QString> _collapsedText{};

    FoldingRange& startLine(int v) { _startLine = v; return *this; }
    FoldingRange& startCharacter(std::optional<int> v) { _startCharacter = v; return *this; }
    FoldingRange& endLine(int v) { _endLine = v; return *this; }
    FoldingRange& endCharacter(std::optional<int> v) { _endCharacter = v; return *this; }
    FoldingRange& kind(const std::optional<FoldingRangeKind> & v) { _kind = v; return *this; }
    FoldingRange& collapsedText(const std::optional<QString> & v) { _collapsedText = v; return *this; }

    const int& startLine() const { return _startLine; }
    const std::optional<int>& startCharacter() const { return _startCharacter; }
    const int& endLine() const { return _endLine; }
    const std::optional<int>& endCharacter() const { return _endCharacter; }
    const std::optional<FoldingRangeKind>& kind() const { return _kind; }
    const std::optional<QString>& collapsedText() const { return _collapsedText; }

    bool operator==(const FoldingRange &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FoldingRange> fromJson<FoldingRange>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FoldingRange &data);

struct FoldingRangeRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    FoldingRangeRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    FoldingRangeRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    FoldingRangeRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const FoldingRangeRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FoldingRangeRegistrationOptions> fromJson<FoldingRangeRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FoldingRangeRegistrationOptions &data);

struct DeclarationParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};

    DeclarationParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    DeclarationParams& position(const Position & v) { _position = v; return *this; }
    DeclarationParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    DeclarationParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }

    bool operator==(const DeclarationParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DeclarationParams> fromJson<DeclarationParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DeclarationParams &data);

struct DeclarationRegistrationOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    DeclarationRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    DeclarationRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    DeclarationRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const DeclarationRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DeclarationRegistrationOptions> fromJson<DeclarationRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DeclarationRegistrationOptions &data);

/** A parameter literal used in selection range requests. */
struct SelectionRangeParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    QList<Position> _positions{};  //!< The positions inside the text document.

    SelectionRangeParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    SelectionRangeParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    SelectionRangeParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    SelectionRangeParams& positions(const QList<Position> & v) { _positions = v; return *this; }
    SelectionRangeParams& addPosition(const Position & v) { _positions.append(v); return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const QList<Position>& positions() const { return _positions; }

    bool operator==(const SelectionRangeParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SelectionRangeParams> fromJson<SelectionRangeParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SelectionRangeParams &data);

/**
 * A selection range represents a part of a selection hierarchy. A selection range
 * may have a parent selection range that contains it.
 */
struct SelectionRange {
    Range _range{};  //!< The {@link Range range} of this selection range.
    Recursive<SelectionRange> _parent{};  //!< The parent selection range containing this range. Therefore `parent.range` must contain `this.range`.

    SelectionRange& range(const Range & v) { _range = v; return *this; }
    SelectionRange& parent(const SelectionRange & v) { _parent = v; return *this; }

    const Range& range() const { return _range; }
    const Recursive<SelectionRange>& parent() const { return _parent; }

    LANGUAGESERVERPROTOCOL_EXPORT bool operator==(const SelectionRange &other) const;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SelectionRange> fromJson<SelectionRange>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SelectionRange &data);

struct SelectionRangeRegistrationOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    SelectionRangeRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    SelectionRangeRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    SelectionRangeRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const SelectionRangeRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SelectionRangeRegistrationOptions> fromJson<SelectionRangeRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SelectionRangeRegistrationOptions &data);

struct WorkDoneProgressCreateParams {
    ProgressToken _token{};  //!< The token to be used to report progress.

    WorkDoneProgressCreateParams& token(const ProgressToken & v) { _token = v; return *this; }

    const ProgressToken& token() const { return _token; }

    bool operator==(const WorkDoneProgressCreateParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkDoneProgressCreateParams> fromJson<WorkDoneProgressCreateParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkDoneProgressCreateParams &data);

struct WorkDoneProgressCancelParams {
    ProgressToken _token{};  //!< The token to be used to report progress.

    WorkDoneProgressCancelParams& token(const ProgressToken & v) { _token = v; return *this; }

    const ProgressToken& token() const { return _token; }

    bool operator==(const WorkDoneProgressCancelParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkDoneProgressCancelParams> fromJson<WorkDoneProgressCancelParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkDoneProgressCancelParams &data);

/**
 * The parameter of a `textDocument/prepareCallHierarchy` request.
 *
 * @since 3.16.0
 */
struct CallHierarchyPrepareParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.

    CallHierarchyPrepareParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    CallHierarchyPrepareParams& position(const Position & v) { _position = v; return *this; }
    CallHierarchyPrepareParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }

    bool operator==(const CallHierarchyPrepareParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyPrepareParams> fromJson<CallHierarchyPrepareParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CallHierarchyPrepareParams &data);

/** A symbol kind. */
namespace SymbolKind {
    constexpr int File = 1;
    constexpr int Module = 2;
    constexpr int Namespace = 3;
    constexpr int Package = 4;
    constexpr int Class = 5;
    constexpr int Method = 6;
    constexpr int Property = 7;
    constexpr int Field = 8;
    constexpr int Constructor = 9;
    constexpr int Enum = 10;
    constexpr int Interface = 11;
    constexpr int Function = 12;
    constexpr int Variable = 13;
    constexpr int Constant = 14;
    constexpr int String = 15;
    constexpr int Number = 16;
    constexpr int Boolean = 17;
    constexpr int Array = 18;
    constexpr int Object = 19;
    constexpr int Key = 20;
    constexpr int Null = 21;
    constexpr int EnumMember = 22;
    constexpr int Struct = 23;
    constexpr int Event = 24;
    constexpr int Operator = 25;
    constexpr int TypeParameter = 26;
} // namespace SymbolKind
/**
 * Symbol tags are extra annotations that tweak the rendering of a symbol.
 *
 * @since 3.16
 */
namespace SymbolTag {
    constexpr int Deprecated = 1;
} // namespace SymbolTag
/**
 * Represents programming constructs like functions or constructors in the context
 * of call hierarchy.
 *
 * @since 3.16.0
 */
struct CallHierarchyItem {
    QString _name{};  //!< The name of this item.
    int _kind{};  //!< The kind of this item.
    std::optional<QList<int>> _tags{};  //!< Tags for this item.
    std::optional<QString> _detail{};  //!< More detail for this item, e.g. the signature of a function.
    QString _uri{};  //!< The resource identifier of this item.
    Range _range{};  //!< The range enclosing this symbol not including leading/trailing whitespace but everything else, e.g. comments and code.
    /**
     * The range that should be selected and revealed when this symbol is being picked, e.g. the name of a function.
     * Must be contained by the {@link CallHierarchyItem.range `range`}.
     */
    Range _selectionRange{};
    /**
     * A data entry field that is preserved between a call hierarchy prepare and
     * incoming calls or outgoing calls requests.
     */
    std::optional<QJsonValue> _data{};

    CallHierarchyItem& name(const QString & v) { _name = v; return *this; }
    CallHierarchyItem& kind(int v) { _kind = v; return *this; }
    CallHierarchyItem& tags(const std::optional<QList<int>> & v) { _tags = v; return *this; }
    CallHierarchyItem& addTag(int v) { if (!_tags) _tags = QList<int>{}; (*_tags).append(v); return *this; }
    CallHierarchyItem& detail(const std::optional<QString> & v) { _detail = v; return *this; }
    CallHierarchyItem& uri(const QString & v) { _uri = v; return *this; }
    CallHierarchyItem& range(const Range & v) { _range = v; return *this; }
    CallHierarchyItem& selectionRange(const Range & v) { _selectionRange = v; return *this; }
    CallHierarchyItem& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const QString& name() const { return _name; }
    const int& kind() const { return _kind; }
    const std::optional<QList<int>>& tags() const { return _tags; }
    const std::optional<QString>& detail() const { return _detail; }
    const QString& uri() const { return _uri; }
    const Range& range() const { return _range; }
    const Range& selectionRange() const { return _selectionRange; }
    const std::optional<QJsonValue>& data() const { return _data; }

    bool operator==(const CallHierarchyItem &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyItem> fromJson<CallHierarchyItem>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CallHierarchyItem &data);

/**
 * Call hierarchy options used during static or dynamic registration.
 *
 * @since 3.16.0
 */
struct CallHierarchyRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    CallHierarchyRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    CallHierarchyRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    CallHierarchyRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const CallHierarchyRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyRegistrationOptions> fromJson<CallHierarchyRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CallHierarchyRegistrationOptions &data);

/**
 * The parameter of a `callHierarchy/incomingCalls` request.
 *
 * @since 3.16.0
 */
struct CallHierarchyIncomingCallsParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    CallHierarchyItem _item{};

    CallHierarchyIncomingCallsParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    CallHierarchyIncomingCallsParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    CallHierarchyIncomingCallsParams& item(const CallHierarchyItem & v) { _item = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const CallHierarchyItem& item() const { return _item; }

    bool operator==(const CallHierarchyIncomingCallsParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyIncomingCallsParams> fromJson<CallHierarchyIncomingCallsParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CallHierarchyIncomingCallsParams &data);

/**
 * Represents an incoming call, e.g. a caller of a method or constructor.
 *
 * @since 3.16.0
 */
struct CallHierarchyIncomingCall {
    CallHierarchyItem _from{};  //!< The item that makes the call.
    /**
     * The ranges at which the calls appear. This is relative to the caller
     * denoted by {@link CallHierarchyIncomingCall.from `this.from`}.
     */
    QList<Range> _fromRanges{};

    CallHierarchyIncomingCall& from(const CallHierarchyItem & v) { _from = v; return *this; }
    CallHierarchyIncomingCall& fromRanges(const QList<Range> & v) { _fromRanges = v; return *this; }
    CallHierarchyIncomingCall& addFromRange(const Range & v) { _fromRanges.append(v); return *this; }

    const CallHierarchyItem& from() const { return _from; }
    const QList<Range>& fromRanges() const { return _fromRanges; }

    bool operator==(const CallHierarchyIncomingCall &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyIncomingCall> fromJson<CallHierarchyIncomingCall>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CallHierarchyIncomingCall &data);

/**
 * The parameter of a `callHierarchy/outgoingCalls` request.
 *
 * @since 3.16.0
 */
struct CallHierarchyOutgoingCallsParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    CallHierarchyItem _item{};

    CallHierarchyOutgoingCallsParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    CallHierarchyOutgoingCallsParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    CallHierarchyOutgoingCallsParams& item(const CallHierarchyItem & v) { _item = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const CallHierarchyItem& item() const { return _item; }

    bool operator==(const CallHierarchyOutgoingCallsParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyOutgoingCallsParams> fromJson<CallHierarchyOutgoingCallsParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CallHierarchyOutgoingCallsParams &data);

/**
 * Represents an outgoing call, e.g. calling a getter from a method or a method from a constructor etc.
 *
 * @since 3.16.0
 */
struct CallHierarchyOutgoingCall {
    CallHierarchyItem _to{};  //!< The item that is called.
    /**
     * The range at which this item is called. This is the range relative to the caller, e.g the item
     * passed to {@link CallHierarchyItemProvider.provideCallHierarchyOutgoingCalls `provideCallHierarchyOutgoingCalls`}
     * and not {@link CallHierarchyOutgoingCall.to `this.to`}.
     */
    QList<Range> _fromRanges{};

    CallHierarchyOutgoingCall& to(const CallHierarchyItem & v) { _to = v; return *this; }
    CallHierarchyOutgoingCall& fromRanges(const QList<Range> & v) { _fromRanges = v; return *this; }
    CallHierarchyOutgoingCall& addFromRange(const Range & v) { _fromRanges.append(v); return *this; }

    const CallHierarchyItem& to() const { return _to; }
    const QList<Range>& fromRanges() const { return _fromRanges; }

    bool operator==(const CallHierarchyOutgoingCall &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyOutgoingCall> fromJson<CallHierarchyOutgoingCall>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CallHierarchyOutgoingCall &data);

/** @since 3.16.0 */
struct SemanticTokensParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The text document.

    SemanticTokensParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    SemanticTokensParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    SemanticTokensParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }

    bool operator==(const SemanticTokensParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensParams> fromJson<SemanticTokensParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensParams &data);

/** @since 3.16.0 */
struct SemanticTokens {
    /**
     * An optional result id. If provided and clients support delta updating
     * the client will include the result id in the next semantic token request.
     * A server can then instead of computing all semantic tokens again simply
     * send a delta.
     */
    std::optional<QString> _resultId{};
    QList<int> _data{};  //!< The actual tokens.

    SemanticTokens& resultId(const std::optional<QString> & v) { _resultId = v; return *this; }
    SemanticTokens& data(const QList<int> & v) { _data = v; return *this; }
    SemanticTokens& addData(int v) { _data.append(v); return *this; }

    const std::optional<QString>& resultId() const { return _resultId; }
    const QList<int>& data() const { return _data; }

    bool operator==(const SemanticTokens &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokens> fromJson<SemanticTokens>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokens &data);

/** @since 3.16.0 */
struct SemanticTokensPartialResult {
    QList<int> _data{};

    SemanticTokensPartialResult& data(const QList<int> & v) { _data = v; return *this; }
    SemanticTokensPartialResult& addData(int v) { _data.append(v); return *this; }

    const QList<int>& data() const { return _data; }

    bool operator==(const SemanticTokensPartialResult &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensPartialResult> fromJson<SemanticTokensPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensPartialResult &data);

/**
 * Semantic tokens options to support deltas for full documents
 *
 * @since 3.18.0
 */
struct SemanticTokensFullDelta {
    std::optional<bool> _delta{};  //!< The server supports deltas for full documents.

    SemanticTokensFullDelta& delta(std::optional<bool> v) { _delta = v; return *this; }

    const std::optional<bool>& delta() const { return _delta; }

    bool operator==(const SemanticTokensFullDelta &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensFullDelta> fromJson<SemanticTokensFullDelta>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensFullDelta &data);

/** @since 3.16.0 */
struct SemanticTokensLegend {
    QStringList _tokenTypes{};  //!< The token types a server uses.
    QStringList _tokenModifiers{};  //!< The token modifiers a server uses.

    SemanticTokensLegend& tokenTypes(const QStringList & v) { _tokenTypes = v; return *this; }
    SemanticTokensLegend& addTokenType(const QString & v) { _tokenTypes.append(v); return *this; }
    SemanticTokensLegend& tokenModifiers(const QStringList & v) { _tokenModifiers = v; return *this; }
    SemanticTokensLegend& addTokenModifier(const QString & v) { _tokenModifiers.append(v); return *this; }

    const QStringList& tokenTypes() const { return _tokenTypes; }
    const QStringList& tokenModifiers() const { return _tokenModifiers; }

    bool operator==(const SemanticTokensLegend &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensLegend> fromJson<SemanticTokensLegend>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensLegend &data);

using SemanticTokensRegistrationOptionsRange = std::variant<bool, QJsonObject>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensRegistrationOptionsRange> fromJson<SemanticTokensRegistrationOptionsRange>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const SemanticTokensRegistrationOptionsRange &val);

using SemanticTokensRegistrationOptionsFull = std::variant<bool, SemanticTokensFullDelta>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensRegistrationOptionsFull> fromJson<SemanticTokensRegistrationOptionsFull>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const SemanticTokensRegistrationOptionsFull &val);

/** @since 3.16.0 */
struct SemanticTokensRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    SemanticTokensLegend _legend{};  //!< The legend used by the server
    /**
     * Server supports providing semantic tokens for a specific range
     * of a document.
     */
    std::optional<SemanticTokensRegistrationOptionsRange> _range{};
    std::optional<SemanticTokensRegistrationOptionsFull> _full{};  //!< Server supports providing semantic tokens for a full document.
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    SemanticTokensRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    SemanticTokensRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    SemanticTokensRegistrationOptions& legend(const SemanticTokensLegend & v) { _legend = v; return *this; }
    SemanticTokensRegistrationOptions& range(const std::optional<SemanticTokensRegistrationOptionsRange> & v) { _range = v; return *this; }
    SemanticTokensRegistrationOptions& full(const std::optional<SemanticTokensRegistrationOptionsFull> & v) { _full = v; return *this; }
    SemanticTokensRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const SemanticTokensLegend& legend() const { return _legend; }
    const std::optional<SemanticTokensRegistrationOptionsRange>& range() const { return _range; }
    const std::optional<SemanticTokensRegistrationOptionsFull>& full() const { return _full; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const SemanticTokensRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensRegistrationOptions> fromJson<SemanticTokensRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensRegistrationOptions &data);

/** @since 3.16.0 */
struct SemanticTokensDeltaParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    /**
     * The result id of a previous response. The result Id can either point to a full response
     * or a delta response depending on what was received last.
     */
    QString _previousResultId{};

    SemanticTokensDeltaParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    SemanticTokensDeltaParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    SemanticTokensDeltaParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    SemanticTokensDeltaParams& previousResultId(const QString & v) { _previousResultId = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const QString& previousResultId() const { return _previousResultId; }

    bool operator==(const SemanticTokensDeltaParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensDeltaParams> fromJson<SemanticTokensDeltaParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensDeltaParams &data);

/** @since 3.16.0 */
struct SemanticTokensEdit {
    int _start{};  //!< The start offset of the edit.
    int _deleteCount{};  //!< The count of elements to remove.
    std::optional<QList<int>> _data{};  //!< The elements to insert.

    SemanticTokensEdit& start(int v) { _start = v; return *this; }
    SemanticTokensEdit& deleteCount(int v) { _deleteCount = v; return *this; }
    SemanticTokensEdit& data(const std::optional<QList<int>> & v) { _data = v; return *this; }
    SemanticTokensEdit& addData(int v) { if (!_data) _data = QList<int>{}; (*_data).append(v); return *this; }

    const int& start() const { return _start; }
    const int& deleteCount() const { return _deleteCount; }
    const std::optional<QList<int>>& data() const { return _data; }

    bool operator==(const SemanticTokensEdit &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensEdit> fromJson<SemanticTokensEdit>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensEdit &data);

/** @since 3.16.0 */
struct SemanticTokensDelta {
    std::optional<QString> _resultId{};
    QList<SemanticTokensEdit> _edits{};  //!< The semantic token edits to transform a previous result into a new result.

    SemanticTokensDelta& resultId(const std::optional<QString> & v) { _resultId = v; return *this; }
    SemanticTokensDelta& edits(const QList<SemanticTokensEdit> & v) { _edits = v; return *this; }
    SemanticTokensDelta& addEdit(const SemanticTokensEdit & v) { _edits.append(v); return *this; }

    const std::optional<QString>& resultId() const { return _resultId; }
    const QList<SemanticTokensEdit>& edits() const { return _edits; }

    bool operator==(const SemanticTokensDelta &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensDelta> fromJson<SemanticTokensDelta>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensDelta &data);

/** @since 3.16.0 */
struct SemanticTokensDeltaPartialResult {
    QList<SemanticTokensEdit> _edits{};

    SemanticTokensDeltaPartialResult& edits(const QList<SemanticTokensEdit> & v) { _edits = v; return *this; }
    SemanticTokensDeltaPartialResult& addEdit(const SemanticTokensEdit & v) { _edits.append(v); return *this; }

    const QList<SemanticTokensEdit>& edits() const { return _edits; }

    bool operator==(const SemanticTokensDeltaPartialResult &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensDeltaPartialResult> fromJson<SemanticTokensDeltaPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensDeltaPartialResult &data);

/** @since 3.16.0 */
struct SemanticTokensRangeParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Range _range{};  //!< The range the semantic tokens are requested for.

    SemanticTokensRangeParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    SemanticTokensRangeParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    SemanticTokensRangeParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    SemanticTokensRangeParams& range(const Range & v) { _range = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Range& range() const { return _range; }

    bool operator==(const SemanticTokensRangeParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensRangeParams> fromJson<SemanticTokensRangeParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensRangeParams &data);

/**
 * Params to show a resource in the UI.
 *
 * @since 3.16.0
 */
struct ShowDocumentParams {
    QString _uri{};  //!< The uri to show.
    /**
     * Indicates to show the resource in an external program.
     * To show, for example, `https://code.visualstudio.com/`
     * in the default WEB browser set `external` to `true`.
     */
    std::optional<bool> _external{};
    /**
     * An optional property to indicate whether the editor
     * showing the document should take focus or not.
     * Clients might ignore this property if an external
     * program is started.
     */
    std::optional<bool> _takeFocus{};
    /**
     * An optional selection range if the document is a text
     * document. Clients might ignore the property if an
     * external program is started or the file is not a text
     * file.
     */
    std::optional<Range> _selection{};

    ShowDocumentParams& uri(const QString & v) { _uri = v; return *this; }
    ShowDocumentParams& external(std::optional<bool> v) { _external = v; return *this; }
    ShowDocumentParams& takeFocus(std::optional<bool> v) { _takeFocus = v; return *this; }
    ShowDocumentParams& selection(const std::optional<Range> & v) { _selection = v; return *this; }

    const QString& uri() const { return _uri; }
    const std::optional<bool>& external() const { return _external; }
    const std::optional<bool>& takeFocus() const { return _takeFocus; }
    const std::optional<Range>& selection() const { return _selection; }

    bool operator==(const ShowDocumentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ShowDocumentParams> fromJson<ShowDocumentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ShowDocumentParams &data);

/**
 * The result of a showDocument request.
 *
 * @since 3.16.0
 */
struct ShowDocumentResult {
    bool _success{};  //!< A boolean indicating if the show was successful.

    ShowDocumentResult& success(bool v) { _success = v; return *this; }

    const bool& success() const { return _success; }

    bool operator==(const ShowDocumentResult &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ShowDocumentResult> fromJson<ShowDocumentResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ShowDocumentResult &data);

struct LinkedEditingRangeParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.

    LinkedEditingRangeParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    LinkedEditingRangeParams& position(const Position & v) { _position = v; return *this; }
    LinkedEditingRangeParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }

    bool operator==(const LinkedEditingRangeParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<LinkedEditingRangeParams> fromJson<LinkedEditingRangeParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const LinkedEditingRangeParams &data);

/**
 * The result of a linked editing range request.
 *
 * @since 3.16.0
 */
struct LinkedEditingRanges {
    /**
     * A list of ranges that can be edited together. The ranges must have
     * identical length and contain identical text content. The ranges cannot overlap.
     */
    QList<Range> _ranges{};
    /**
     * An optional word pattern (regular expression) that describes valid contents for
     * the given ranges. If no pattern is provided, the client configuration's word
     * pattern will be used.
     */
    std::optional<QString> _wordPattern{};

    LinkedEditingRanges& ranges(const QList<Range> & v) { _ranges = v; return *this; }
    LinkedEditingRanges& addRange(const Range & v) { _ranges.append(v); return *this; }
    LinkedEditingRanges& wordPattern(const std::optional<QString> & v) { _wordPattern = v; return *this; }

    const QList<Range>& ranges() const { return _ranges; }
    const std::optional<QString>& wordPattern() const { return _wordPattern; }

    bool operator==(const LinkedEditingRanges &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<LinkedEditingRanges> fromJson<LinkedEditingRanges>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const LinkedEditingRanges &data);

struct LinkedEditingRangeRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    LinkedEditingRangeRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    LinkedEditingRangeRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    LinkedEditingRangeRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const LinkedEditingRangeRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<LinkedEditingRangeRegistrationOptions> fromJson<LinkedEditingRangeRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const LinkedEditingRangeRegistrationOptions &data);

/**
 * Represents information on a file/folder create.
 *
 * @since 3.16.0
 */
struct FileCreate {
    QString _uri{};  //!< A URI for the location of the file/folder being created.

    FileCreate& uri(const QString & v) { _uri = v; return *this; }

    const QString& uri() const { return _uri; }

    bool operator==(const FileCreate &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileCreate> fromJson<FileCreate>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileCreate &data);

/**
 * The parameters sent in notifications/requests for user-initiated creation of
 * files.
 *
 * @since 3.16.0
 */
struct CreateFilesParams {
    QList<FileCreate> _files{};  //!< An array of all files/folders created in this operation.

    CreateFilesParams& files(const QList<FileCreate> & v) { _files = v; return *this; }
    CreateFilesParams& addFile(const FileCreate & v) { _files.append(v); return *this; }

    const QList<FileCreate>& files() const { return _files; }

    bool operator==(const CreateFilesParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CreateFilesParams> fromJson<CreateFilesParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CreateFilesParams &data);

/**
 * Additional information that describes document changes.
 *
 * @since 3.16.0
 */
struct ChangeAnnotation {
    /**
     * A human-readable string describing the actual change. The string
     * is rendered prominent in the user interface.
     */
    QString _label{};
    /**
     * A flag which indicates that user confirmation is needed
     * before applying the change.
     */
    std::optional<bool> _needsConfirmation{};
    /**
     * A human-readable string which is rendered less prominent in
     * the user interface.
     */
    std::optional<QString> _description{};

    ChangeAnnotation& label(const QString & v) { _label = v; return *this; }
    ChangeAnnotation& needsConfirmation(std::optional<bool> v) { _needsConfirmation = v; return *this; }
    ChangeAnnotation& description(const std::optional<QString> & v) { _description = v; return *this; }

    const QString& label() const { return _label; }
    const std::optional<bool>& needsConfirmation() const { return _needsConfirmation; }
    const std::optional<QString>& description() const { return _description; }

    bool operator==(const ChangeAnnotation &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ChangeAnnotation> fromJson<ChangeAnnotation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ChangeAnnotation &data);

using ChangeAnnotationIdentifier = QString;

/** Options to create a file. */
struct CreateFileOptions {
    std::optional<bool> _overwrite{};  //!< Overwrite existing file. Overwrite wins over `ignoreIfExists`
    std::optional<bool> _ignoreIfExists{};  //!< Ignore if exists.

    CreateFileOptions& overwrite(std::optional<bool> v) { _overwrite = v; return *this; }
    CreateFileOptions& ignoreIfExists(std::optional<bool> v) { _ignoreIfExists = v; return *this; }

    const std::optional<bool>& overwrite() const { return _overwrite; }
    const std::optional<bool>& ignoreIfExists() const { return _ignoreIfExists; }

    bool operator==(const CreateFileOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CreateFileOptions> fromJson<CreateFileOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CreateFileOptions &data);

/** Create file operation. */
struct CreateFile {
    /**
     * An optional annotation identifier describing the operation.
     *
     * @since 3.16.0
     */
    std::optional<ChangeAnnotationIdentifier> _annotationId{};
    QString _uri{};  //!< The resource to create.
    std::optional<CreateFileOptions> _options{};  //!< Additional options

    CreateFile& annotationId(const std::optional<ChangeAnnotationIdentifier> & v) { _annotationId = v; return *this; }
    CreateFile& uri(const QString & v) { _uri = v; return *this; }
    CreateFile& options(const std::optional<CreateFileOptions> & v) { _options = v; return *this; }

    const std::optional<ChangeAnnotationIdentifier>& annotationId() const { return _annotationId; }
    const QString& uri() const { return _uri; }
    const std::optional<CreateFileOptions>& options() const { return _options; }

    bool operator==(const CreateFile &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CreateFile> fromJson<CreateFile>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CreateFile &data);

/** Delete file options */
struct DeleteFileOptions {
    std::optional<bool> _recursive{};  //!< Delete the content recursively if a folder is denoted.
    std::optional<bool> _ignoreIfNotExists{};  //!< Ignore the operation if the file doesn't exist.

    DeleteFileOptions& recursive(std::optional<bool> v) { _recursive = v; return *this; }
    DeleteFileOptions& ignoreIfNotExists(std::optional<bool> v) { _ignoreIfNotExists = v; return *this; }

    const std::optional<bool>& recursive() const { return _recursive; }
    const std::optional<bool>& ignoreIfNotExists() const { return _ignoreIfNotExists; }

    bool operator==(const DeleteFileOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DeleteFileOptions> fromJson<DeleteFileOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DeleteFileOptions &data);

/** Delete file operation */
struct DeleteFile {
    /**
     * An optional annotation identifier describing the operation.
     *
     * @since 3.16.0
     */
    std::optional<ChangeAnnotationIdentifier> _annotationId{};
    QString _uri{};  //!< The file to delete.
    std::optional<DeleteFileOptions> _options{};  //!< Delete options.

    DeleteFile& annotationId(const std::optional<ChangeAnnotationIdentifier> & v) { _annotationId = v; return *this; }
    DeleteFile& uri(const QString & v) { _uri = v; return *this; }
    DeleteFile& options(const std::optional<DeleteFileOptions> & v) { _options = v; return *this; }

    const std::optional<ChangeAnnotationIdentifier>& annotationId() const { return _annotationId; }
    const QString& uri() const { return _uri; }
    const std::optional<DeleteFileOptions>& options() const { return _options; }

    bool operator==(const DeleteFile &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DeleteFile> fromJson<DeleteFile>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DeleteFile &data);

/** Rename file options */
struct RenameFileOptions {
    std::optional<bool> _overwrite{};  //!< Overwrite target if existing. Overwrite wins over `ignoreIfExists`
    std::optional<bool> _ignoreIfExists{};  //!< Ignores if target exists.

    RenameFileOptions& overwrite(std::optional<bool> v) { _overwrite = v; return *this; }
    RenameFileOptions& ignoreIfExists(std::optional<bool> v) { _ignoreIfExists = v; return *this; }

    const std::optional<bool>& overwrite() const { return _overwrite; }
    const std::optional<bool>& ignoreIfExists() const { return _ignoreIfExists; }

    bool operator==(const RenameFileOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RenameFileOptions> fromJson<RenameFileOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RenameFileOptions &data);

/** Rename file operation */
struct RenameFile {
    /**
     * An optional annotation identifier describing the operation.
     *
     * @since 3.16.0
     */
    std::optional<ChangeAnnotationIdentifier> _annotationId{};
    QString _oldUri{};  //!< The old (existing) location.
    QString _newUri{};  //!< The new location.
    std::optional<RenameFileOptions> _options{};  //!< Rename options.

    RenameFile& annotationId(const std::optional<ChangeAnnotationIdentifier> & v) { _annotationId = v; return *this; }
    RenameFile& oldUri(const QString & v) { _oldUri = v; return *this; }
    RenameFile& newUri(const QString & v) { _newUri = v; return *this; }
    RenameFile& options(const std::optional<RenameFileOptions> & v) { _options = v; return *this; }

    const std::optional<ChangeAnnotationIdentifier>& annotationId() const { return _annotationId; }
    const QString& oldUri() const { return _oldUri; }
    const QString& newUri() const { return _newUri; }
    const std::optional<RenameFileOptions>& options() const { return _options; }

    bool operator==(const RenameFile &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RenameFile> fromJson<RenameFile>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RenameFile &data);

/**
 * A special text edit with an additional change annotation.
 *
 * @since 3.16.0.
 */
struct AnnotatedTextEdit {
    /**
     * The range of the text document to be manipulated. To insert
     * text into a document create a range where start === end.
     */
    Range _range{};
    /**
     * The string to be inserted. For delete operations use an
     * empty string.
     */
    QString _newText{};
    ChangeAnnotationIdentifier _annotationId{};  //!< The actual identifier of the change annotation

    AnnotatedTextEdit& range(const Range & v) { _range = v; return *this; }
    AnnotatedTextEdit& newText(const QString & v) { _newText = v; return *this; }
    AnnotatedTextEdit& annotationId(const ChangeAnnotationIdentifier & v) { _annotationId = v; return *this; }

    const Range& range() const { return _range; }
    const QString& newText() const { return _newText; }
    const ChangeAnnotationIdentifier& annotationId() const { return _annotationId; }

    bool operator==(const AnnotatedTextEdit &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<AnnotatedTextEdit> fromJson<AnnotatedTextEdit>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const AnnotatedTextEdit &data);

/** A text document identifier to optionally denote a specific version of a text document. */
struct OptionalVersionedTextDocumentIdentifier {
    QString _uri{};  //!< The text document's uri.
    /**
     * The version number of this document. If a versioned text document identifier
     * is sent from the server to the client and the file is not open in the editor
     * (the server has not received an open notification before) the server can send
     * `null` to indicate that the version is unknown and the content on disk is the
     * truth (as specified with document content ownership).
     */
    std::optional<int> _version{};

    OptionalVersionedTextDocumentIdentifier& uri(const QString & v) { _uri = v; return *this; }
    OptionalVersionedTextDocumentIdentifier& version(std::optional<int> v) { _version = v; return *this; }

    const QString& uri() const { return _uri; }
    const std::optional<int>& version() const { return _version; }

    bool operator==(const OptionalVersionedTextDocumentIdentifier &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<OptionalVersionedTextDocumentIdentifier> fromJson<OptionalVersionedTextDocumentIdentifier>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const OptionalVersionedTextDocumentIdentifier &data);

/**
 * A string value used as a snippet is a template which allows to insert text
 * and to control the editor cursor when insertion happens.
 *
 * A snippet can define tab stops and placeholders with `$1`, `$2`
 * and `${3:foo}`. `$0` defines the final tab stop, it defaults to
 * the end of the snippet. Variables are defined with `$name` and
 * `${name:default value}`.
 *
 * @since 3.18.0
 */
struct StringValue {
    QString _value{};  //!< The snippet string.

    StringValue& value(const QString & v) { _value = v; return *this; }

    const QString& value() const { return _value; }

    bool operator==(const StringValue &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<StringValue> fromJson<StringValue>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const StringValue &data);

/**
 * An interactive text edit.
 *
 * @since 3.18.0
 */
struct SnippetTextEdit {
    Range _range{};  //!< The range of the text document to be manipulated.
    StringValue _snippet{};  //!< The snippet to be inserted.
    std::optional<ChangeAnnotationIdentifier> _annotationId{};  //!< The actual identifier of the snippet edit.

    SnippetTextEdit& range(const Range & v) { _range = v; return *this; }
    SnippetTextEdit& snippet(const StringValue & v) { _snippet = v; return *this; }
    SnippetTextEdit& annotationId(const std::optional<ChangeAnnotationIdentifier> & v) { _annotationId = v; return *this; }

    const Range& range() const { return _range; }
    const StringValue& snippet() const { return _snippet; }
    const std::optional<ChangeAnnotationIdentifier>& annotationId() const { return _annotationId; }

    bool operator==(const SnippetTextEdit &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SnippetTextEdit> fromJson<SnippetTextEdit>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SnippetTextEdit &data);

using TextDocumentEditEditsItem = std::variant<TextEdit, AnnotatedTextEdit, SnippetTextEdit>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentEditEditsItem> fromJson<TextDocumentEditEditsItem>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const TextDocumentEditEditsItem &val);

/**
 * Describes textual changes on a text document. A TextDocumentEdit describes all changes
 * on a document version Si and after they are applied move the document to version Si+1.
 * So the creator of a TextDocumentEdit doesn't need to sort the array of edits or do any
 * kind of ordering. However the edits must be non overlapping.
 */
struct TextDocumentEdit {
    OptionalVersionedTextDocumentIdentifier _textDocument{};  //!< The text document to change.
    /**
     * The edits to be applied.
     *
     * @since 3.16.0 - support for AnnotatedTextEdit. This is guarded using a
     * client capability.
     *
     * @since 3.18.0 - support for SnippetTextEdit. This is guarded using a
     * client capability.
     */
    QList<TextDocumentEditEditsItem> _edits{};

    TextDocumentEdit& textDocument(const OptionalVersionedTextDocumentIdentifier & v) { _textDocument = v; return *this; }
    TextDocumentEdit& edits(const QList<TextDocumentEditEditsItem> & v) { _edits = v; return *this; }
    TextDocumentEdit& addEdit(const TextDocumentEditEditsItem & v) { _edits.append(v); return *this; }

    const OptionalVersionedTextDocumentIdentifier& textDocument() const { return _textDocument; }
    const QList<TextDocumentEditEditsItem>& edits() const { return _edits; }

    bool operator==(const TextDocumentEdit &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentEdit> fromJson<TextDocumentEdit>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentEdit &data);

using WorkspaceEditDocumentChangesItem = std::variant<TextDocumentEdit, CreateFile, RenameFile, DeleteFile>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceEditDocumentChangesItem> fromJson<WorkspaceEditDocumentChangesItem>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const WorkspaceEditDocumentChangesItem &val);

/**
 * A workspace edit represents changes to many resources managed in the workspace. The edit
 * should either provide `changes` or `documentChanges`. If documentChanges are present
 * they are preferred over `changes` if the client can handle versioned document edits.
 *
 * Since version 3.13.0 a workspace edit can contain resource operations as well. If resource
 * operations are present clients need to execute the operations in the order in which they
 * are provided. So a workspace edit for example can consist of the following two changes:
 * (1) a create file a.txt and (2) a text document edit which insert text into file a.txt.
 *
 * An invalid sequence (e.g. (1) delete file a.txt and (2) insert text into file a.txt) will
 * cause failure of the operation. How the client recovers from the failure is described by
 * the client capability: `workspace.workspaceEdit.failureHandling`
 */
struct WorkspaceEdit {
    std::optional<QMap<QString, QList<TextEdit>>> _changes{};  //!< Holds changes to existing resources.
    /**
     * Depending on the client capability `workspace.workspaceEdit.resourceOperations` document changes
     * are either an array of `TextDocumentEdit`s to express changes to n different text documents
     * where each text document edit addresses a specific version of a text document. Or it can contain
     * above `TextDocumentEdit`s mixed with create, rename and delete file / folder operations.
     *
     * Whether a client supports versioned document edits is expressed via
     * `workspace.workspaceEdit.documentChanges` client capability.
     *
     * If a client neither supports `documentChanges` nor `workspace.workspaceEdit.resourceOperations` then
     * only plain `TextEdit`s using the `changes` property are supported.
     */
    std::optional<QList<WorkspaceEditDocumentChangesItem>> _documentChanges{};
    /**
     * A map of change annotations that can be referenced in `AnnotatedTextEdit`s or create, rename and
     * delete file / folder operations.
     *
     * Whether clients honor this property depends on the client capability `workspace.changeAnnotationSupport`.
     *
     * @since 3.16.0
     */
    std::optional<QMap<QString, ChangeAnnotation>> _changeAnnotations{};

    WorkspaceEdit& changes(const std::optional<QMap<QString, QList<TextEdit>>> & v) { _changes = v; return *this; }
    WorkspaceEdit& addChange(const QString &key, const QList<TextEdit> & v) { if (!_changes) _changes = QMap<QString, QList<TextEdit>>{}; (*_changes)[key] = v; return *this; }
    WorkspaceEdit& documentChanges(const std::optional<QList<WorkspaceEditDocumentChangesItem>> & v) { _documentChanges = v; return *this; }
    WorkspaceEdit& addDocumentChange(const WorkspaceEditDocumentChangesItem & v) { if (!_documentChanges) _documentChanges = QList<WorkspaceEditDocumentChangesItem>{}; (*_documentChanges).append(v); return *this; }
    WorkspaceEdit& changeAnnotations(const std::optional<QMap<QString, ChangeAnnotation>> & v) { _changeAnnotations = v; return *this; }
    WorkspaceEdit& addChangeAnnotation(const QString &key, const ChangeAnnotation & v) { if (!_changeAnnotations) _changeAnnotations = QMap<QString, ChangeAnnotation>{}; (*_changeAnnotations)[key] = v; return *this; }

    const std::optional<QMap<QString, QList<TextEdit>>>& changes() const { return _changes; }
    const std::optional<QList<WorkspaceEditDocumentChangesItem>>& documentChanges() const { return _documentChanges; }
    const std::optional<QMap<QString, ChangeAnnotation>>& changeAnnotations() const { return _changeAnnotations; }

    bool operator==(const WorkspaceEdit &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceEdit> fromJson<WorkspaceEdit>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceEdit &data);

/**
 * A pattern kind describing if a glob pattern matches a file a folder or
 * both.
 *
 * @since 3.16.0
 */
enum class FileOperationPatternKind {
    file,
    folder
};

LANGUAGESERVERPROTOCOL_EXPORT QString toString(FileOperationPatternKind v);

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileOperationPatternKind> fromJson<FileOperationPatternKind>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const FileOperationPatternKind &v);

/**
 * Matching options for the file operation pattern.
 *
 * @since 3.16.0
 */
struct FileOperationPatternOptions {
    std::optional<bool> _ignoreCase{};  //!< The pattern should be matched ignoring casing.

    FileOperationPatternOptions& ignoreCase(std::optional<bool> v) { _ignoreCase = v; return *this; }

    const std::optional<bool>& ignoreCase() const { return _ignoreCase; }

    bool operator==(const FileOperationPatternOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileOperationPatternOptions> fromJson<FileOperationPatternOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileOperationPatternOptions &data);

/**
 * A pattern to describe in which file operation requests or notifications
 * the server is interested in receiving.
 *
 * @since 3.16.0
 */
struct FileOperationPattern {
    /**
     * The glob pattern to match. Glob patterns can have the following syntax:
     * - `*` to match zero or more characters in a path segment
     * - `?` to match on one character in a path segment
     * - `**` to match any number of path segments, including none
     * - `{}` to group sub patterns into an OR expression. (e.g. `**\/\*.{ts,js}` matches all TypeScript and JavaScript files)
     * - `[]` to declare a range of characters to match in a path segment (e.g., `example.[0-9]` to match on `example.0`, `example.1`, ...)
     * - `[!...]` to negate a range of characters to match in a path segment (e.g., `example.[!0-9]` to match on `example.a`, `example.b`, but not `example.0`)
     */
    QString _glob{};
    /**
     * Whether to match files or folders with this pattern.
     *
     * Matches both if undefined.
     */
    std::optional<FileOperationPatternKind> _matches{};
    std::optional<FileOperationPatternOptions> _options{};  //!< Additional options used during matching.

    FileOperationPattern& glob(const QString & v) { _glob = v; return *this; }
    FileOperationPattern& matches(const std::optional<FileOperationPatternKind> & v) { _matches = v; return *this; }
    FileOperationPattern& options(const std::optional<FileOperationPatternOptions> & v) { _options = v; return *this; }

    const QString& glob() const { return _glob; }
    const std::optional<FileOperationPatternKind>& matches() const { return _matches; }
    const std::optional<FileOperationPatternOptions>& options() const { return _options; }

    bool operator==(const FileOperationPattern &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileOperationPattern> fromJson<FileOperationPattern>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileOperationPattern &data);

/**
 * A filter to describe in which file operation requests or notifications
 * the server is interested in receiving.
 *
 * @since 3.16.0
 */
struct FileOperationFilter {
    std::optional<QString> _scheme{};  //!< A Uri scheme like `file` or `untitled`.
    FileOperationPattern _pattern{};  //!< The actual file operation pattern.

    FileOperationFilter& scheme(const std::optional<QString> & v) { _scheme = v; return *this; }
    FileOperationFilter& pattern(const FileOperationPattern & v) { _pattern = v; return *this; }

    const std::optional<QString>& scheme() const { return _scheme; }
    const FileOperationPattern& pattern() const { return _pattern; }

    bool operator==(const FileOperationFilter &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileOperationFilter> fromJson<FileOperationFilter>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileOperationFilter &data);

/**
 * The options to register for file operations.
 *
 * @since 3.16.0
 */
struct FileOperationRegistrationOptions {
    QList<FileOperationFilter> _filters{};  //!< The actual filters.

    FileOperationRegistrationOptions& filters(const QList<FileOperationFilter> & v) { _filters = v; return *this; }
    FileOperationRegistrationOptions& addFilter(const FileOperationFilter & v) { _filters.append(v); return *this; }

    const QList<FileOperationFilter>& filters() const { return _filters; }

    bool operator==(const FileOperationRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileOperationRegistrationOptions> fromJson<FileOperationRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileOperationRegistrationOptions &data);

/**
 * Represents information on a file/folder rename.
 *
 * @since 3.16.0
 */
struct FileRename {
    QString _oldUri{};  //!< A URI for the original location of the file/folder being renamed.
    QString _newUri{};  //!< A URI for the new location of the file/folder being renamed.

    FileRename& oldUri(const QString & v) { _oldUri = v; return *this; }
    FileRename& newUri(const QString & v) { _newUri = v; return *this; }

    const QString& oldUri() const { return _oldUri; }
    const QString& newUri() const { return _newUri; }

    bool operator==(const FileRename &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileRename> fromJson<FileRename>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileRename &data);

/**
 * The parameters sent in notifications/requests for user-initiated renames of
 * files.
 *
 * @since 3.16.0
 */
struct RenameFilesParams {
    /**
     * An array of all files/folders renamed in this operation. When a folder is renamed, only
     * the folder will be included, and not its children.
     */
    QList<FileRename> _files{};

    RenameFilesParams& files(const QList<FileRename> & v) { _files = v; return *this; }
    RenameFilesParams& addFile(const FileRename & v) { _files.append(v); return *this; }

    const QList<FileRename>& files() const { return _files; }

    bool operator==(const RenameFilesParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RenameFilesParams> fromJson<RenameFilesParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RenameFilesParams &data);

/**
 * Represents information on a file/folder delete.
 *
 * @since 3.16.0
 */
struct FileDelete {
    QString _uri{};  //!< A URI for the location of the file/folder being deleted.

    FileDelete& uri(const QString & v) { _uri = v; return *this; }

    const QString& uri() const { return _uri; }

    bool operator==(const FileDelete &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileDelete> fromJson<FileDelete>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileDelete &data);

/**
 * The parameters sent in notifications/requests for user-initiated deletes of
 * files.
 *
 * @since 3.16.0
 */
struct DeleteFilesParams {
    QList<FileDelete> _files{};  //!< An array of all files/folders deleted in this operation.

    DeleteFilesParams& files(const QList<FileDelete> & v) { _files = v; return *this; }
    DeleteFilesParams& addFile(const FileDelete & v) { _files.append(v); return *this; }

    const QList<FileDelete>& files() const { return _files; }

    bool operator==(const DeleteFilesParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DeleteFilesParams> fromJson<DeleteFilesParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DeleteFilesParams &data);

struct MonikerParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};

    MonikerParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    MonikerParams& position(const Position & v) { _position = v; return *this; }
    MonikerParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    MonikerParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }

    bool operator==(const MonikerParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MonikerParams> fromJson<MonikerParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const MonikerParams &data);

/**
 * The moniker kind.
 *
 * @since 3.16.0
 */
enum class MonikerKind {
    import,
    export_,
    local
};

LANGUAGESERVERPROTOCOL_EXPORT QString toString(MonikerKind v);

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MonikerKind> fromJson<MonikerKind>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const MonikerKind &v);

/**
 * Moniker uniqueness level to define scope of the moniker.
 *
 * @since 3.16.0
 */
enum class UniquenessLevel {
    document,
    project,
    group,
    scheme,
    global
};

LANGUAGESERVERPROTOCOL_EXPORT QString toString(UniquenessLevel v);

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<UniquenessLevel> fromJson<UniquenessLevel>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const UniquenessLevel &v);

/**
 * Moniker definition to match LSIF 0.5 moniker definition.
 *
 * @since 3.16.0
 */
struct Moniker {
    QString _scheme{};  //!< The scheme of the moniker. For example tsc or .Net
    /**
     * The identifier of the moniker. The value is opaque in LSIF however
     * schema owners are allowed to define the structure if they want.
     */
    QString _identifier{};
    UniquenessLevel _unique{};  //!< The scope in which the moniker is unique
    std::optional<MonikerKind> _kind{};  //!< The moniker kind if known.

    Moniker& scheme(const QString & v) { _scheme = v; return *this; }
    Moniker& identifier(const QString & v) { _identifier = v; return *this; }
    Moniker& unique(const UniquenessLevel & v) { _unique = v; return *this; }
    Moniker& kind(const std::optional<MonikerKind> & v) { _kind = v; return *this; }

    const QString& scheme() const { return _scheme; }
    const QString& identifier() const { return _identifier; }
    const UniquenessLevel& unique() const { return _unique; }
    const std::optional<MonikerKind>& kind() const { return _kind; }

    bool operator==(const Moniker &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Moniker> fromJson<Moniker>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const Moniker &data);

struct MonikerRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};

    MonikerRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    MonikerRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const MonikerRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MonikerRegistrationOptions> fromJson<MonikerRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const MonikerRegistrationOptions &data);

/**
 * The parameter of a `textDocument/prepareTypeHierarchy` request.
 *
 * @since 3.17.0
 */
struct TypeHierarchyPrepareParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.

    TypeHierarchyPrepareParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    TypeHierarchyPrepareParams& position(const Position & v) { _position = v; return *this; }
    TypeHierarchyPrepareParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }

    bool operator==(const TypeHierarchyPrepareParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeHierarchyPrepareParams> fromJson<TypeHierarchyPrepareParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeHierarchyPrepareParams &data);

/** @since 3.17.0 */
struct TypeHierarchyItem {
    QString _name{};  //!< The name of this item.
    int _kind{};  //!< The kind of this item.
    std::optional<QList<int>> _tags{};  //!< Tags for this item.
    std::optional<QString> _detail{};  //!< More detail for this item, e.g. the signature of a function.
    QString _uri{};  //!< The resource identifier of this item.
    /**
     * The range enclosing this symbol not including leading/trailing whitespace
     * but everything else, e.g. comments and code.
     */
    Range _range{};
    /**
     * The range that should be selected and revealed when this symbol is being
     * picked, e.g. the name of a function. Must be contained by the
     * {@link TypeHierarchyItem.range `range`}.
     */
    Range _selectionRange{};
    /**
     * A data entry field that is preserved between a type hierarchy prepare and
     * supertypes or subtypes requests. It could also be used to identify the
     * type hierarchy in the server, helping improve the performance on
     * resolving supertypes and subtypes.
     */
    std::optional<QJsonValue> _data{};

    TypeHierarchyItem& name(const QString & v) { _name = v; return *this; }
    TypeHierarchyItem& kind(int v) { _kind = v; return *this; }
    TypeHierarchyItem& tags(const std::optional<QList<int>> & v) { _tags = v; return *this; }
    TypeHierarchyItem& addTag(int v) { if (!_tags) _tags = QList<int>{}; (*_tags).append(v); return *this; }
    TypeHierarchyItem& detail(const std::optional<QString> & v) { _detail = v; return *this; }
    TypeHierarchyItem& uri(const QString & v) { _uri = v; return *this; }
    TypeHierarchyItem& range(const Range & v) { _range = v; return *this; }
    TypeHierarchyItem& selectionRange(const Range & v) { _selectionRange = v; return *this; }
    TypeHierarchyItem& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const QString& name() const { return _name; }
    const int& kind() const { return _kind; }
    const std::optional<QList<int>>& tags() const { return _tags; }
    const std::optional<QString>& detail() const { return _detail; }
    const QString& uri() const { return _uri; }
    const Range& range() const { return _range; }
    const Range& selectionRange() const { return _selectionRange; }
    const std::optional<QJsonValue>& data() const { return _data; }

    bool operator==(const TypeHierarchyItem &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeHierarchyItem> fromJson<TypeHierarchyItem>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeHierarchyItem &data);

/**
 * Type hierarchy options used during static or dynamic registration.
 *
 * @since 3.17.0
 */
struct TypeHierarchyRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    TypeHierarchyRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    TypeHierarchyRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    TypeHierarchyRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const TypeHierarchyRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeHierarchyRegistrationOptions> fromJson<TypeHierarchyRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeHierarchyRegistrationOptions &data);

/**
 * The parameter of a `typeHierarchy/supertypes` request.
 *
 * @since 3.17.0
 */
struct TypeHierarchySupertypesParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TypeHierarchyItem _item{};

    TypeHierarchySupertypesParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    TypeHierarchySupertypesParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    TypeHierarchySupertypesParams& item(const TypeHierarchyItem & v) { _item = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TypeHierarchyItem& item() const { return _item; }

    bool operator==(const TypeHierarchySupertypesParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeHierarchySupertypesParams> fromJson<TypeHierarchySupertypesParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeHierarchySupertypesParams &data);

/**
 * The parameter of a `typeHierarchy/subtypes` request.
 *
 * @since 3.17.0
 */
struct TypeHierarchySubtypesParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TypeHierarchyItem _item{};

    TypeHierarchySubtypesParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    TypeHierarchySubtypesParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    TypeHierarchySubtypesParams& item(const TypeHierarchyItem & v) { _item = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TypeHierarchyItem& item() const { return _item; }

    bool operator==(const TypeHierarchySubtypesParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeHierarchySubtypesParams> fromJson<TypeHierarchySubtypesParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeHierarchySubtypesParams &data);

/** @since 3.17.0 */
struct InlineValueContext {
    int _frameId{};  //!< The stack frame (as a DAP Id) where the execution has stopped.
    /**
     * The document range where execution has stopped.
     * Typically the end position of the range denotes the line where the inline values are shown.
     */
    Range _stoppedLocation{};

    InlineValueContext& frameId(int v) { _frameId = v; return *this; }
    InlineValueContext& stoppedLocation(const Range & v) { _stoppedLocation = v; return *this; }

    const int& frameId() const { return _frameId; }
    const Range& stoppedLocation() const { return _stoppedLocation; }

    bool operator==(const InlineValueContext &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueContext> fromJson<InlineValueContext>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineValueContext &data);

/**
 * A parameter literal used in inline value requests.
 *
 * @since 3.17.0
 */
struct InlineValueParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Range _range{};  //!< The document range for which inline values information will be returned.
    /**
     * Additional information about the context in which inline values information was
     * requested.
     */
    InlineValueContext _context{};

    InlineValueParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    InlineValueParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    InlineValueParams& range(const Range & v) { _range = v; return *this; }
    InlineValueParams& context(const InlineValueContext & v) { _context = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Range& range() const { return _range; }
    const InlineValueContext& context() const { return _context; }

    bool operator==(const InlineValueParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueParams> fromJson<InlineValueParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineValueParams &data);

/**
 * Inline value options used during static or dynamic registration.
 *
 * @since 3.17.0
 */
struct InlineValueRegistrationOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    InlineValueRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    InlineValueRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    InlineValueRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const InlineValueRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueRegistrationOptions> fromJson<InlineValueRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineValueRegistrationOptions &data);

/**
 * A parameter literal used in inlay hint requests.
 *
 * @since 3.17.0
 */
struct InlayHintParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Range _range{};  //!< The document range for which inlay hints should be computed.

    InlayHintParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    InlayHintParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    InlayHintParams& range(const Range & v) { _range = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Range& range() const { return _range; }

    bool operator==(const InlayHintParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHintParams> fromJson<InlayHintParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlayHintParams &data);

/**
 * Inlay hint kinds.
 *
 * @since 3.17.0
 */
namespace InlayHintKind {
    constexpr int Type = 1;
    constexpr int Parameter = 2;
} // namespace InlayHintKind
/**
 * Represents a reference to a command. Provides a title which
 * will be used to represent a command in the UI and, optionally,
 * an array of arguments which will be passed to the command handler
 * function when invoked.
 */
struct Command {
    QString _title{};  //!< Title of the command, like `save`.
    /**
     * An optional tooltip.
     *
     * @since 3.18.0
     */
    std::optional<QString> _tooltip{};
    QString _command{};  //!< The identifier of the actual command handler.
    /**
     * Arguments that the command handler should be
     * invoked with.
     */
    std::optional<QList<QJsonValue>> _arguments{};

    Command& title(const QString & v) { _title = v; return *this; }
    Command& tooltip(const std::optional<QString> & v) { _tooltip = v; return *this; }
    Command& command(const QString & v) { _command = v; return *this; }
    Command& arguments(const std::optional<QList<QJsonValue>> & v) { _arguments = v; return *this; }
    Command& addArgument(const QJsonValue & v) { if (!_arguments) _arguments = QList<QJsonValue>{}; (*_arguments).append(v); return *this; }

    const QString& title() const { return _title; }
    const std::optional<QString>& tooltip() const { return _tooltip; }
    const QString& command() const { return _command; }
    const std::optional<QList<QJsonValue>>& arguments() const { return _arguments; }

    bool operator==(const Command &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Command> fromJson<Command>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const Command &data);

/**
 * Describes the content type that a client supports in various
 * result literals like `Hover`, `ParameterInfo` or `CompletionItem`.
 *
 * Please note that `MarkupKinds` must not start with a `$`. This kinds
 * are reserved for internal usage.
 */
enum class MarkupKind {
    plaintext,
    markdown
};

LANGUAGESERVERPROTOCOL_EXPORT QString toString(MarkupKind v);

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MarkupKind> fromJson<MarkupKind>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const MarkupKind &v);

/**
 * A `MarkupContent` literal represents a string value which content is interpreted base on its
 * kind flag. Currently the protocol supports `plaintext` and `markdown` as markup kinds.
 *
 * If the kind is `markdown` then the value can contain fenced code blocks like in GitHub issues.
 * See https://help.github.com/articles/creating-and-highlighting-code-blocks/#syntax-highlighting
 *
 * Here is an example how such a string can be constructed using JavaScript / TypeScript:
 * ```ts
 * let markdown: MarkdownContent = {
 * kind: MarkupKind.Markdown,
 * value: [
 * '# Header',
 * 'Some text',
 * '```typescript',
 * 'someCode();',
 * '```'
 * ].join('\n')
 * };
 * ```
 *
 * *Please Note* that clients might sanitize the return markdown. A client could decide to
 * remove HTML from the markdown to avoid script execution.
 */
struct MarkupContent {
    MarkupKind _kind{};  //!< The type of the Markup
    QString _value{};  //!< The content itself

    MarkupContent& kind(const MarkupKind & v) { _kind = v; return *this; }
    MarkupContent& value(const QString & v) { _value = v; return *this; }

    const MarkupKind& kind() const { return _kind; }
    const QString& value() const { return _value; }

    bool operator==(const MarkupContent &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MarkupContent> fromJson<MarkupContent>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const MarkupContent &data);

using InlayHintLabelPartTooltip = std::variant<QString, MarkupContent>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHintLabelPartTooltip> fromJson<InlayHintLabelPartTooltip>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const InlayHintLabelPartTooltip &val);

/**
 * An inlay hint label part allows for interactive and composite labels
 * of inlay hints.
 *
 * @since 3.17.0
 */
struct InlayHintLabelPart {
    QString _value{};  //!< The value of this label part.
    /**
     * The tooltip text when you hover over this label part. Depending on
     * the client capability `inlayHint.resolveSupport` clients might resolve
     * this property late using the resolve request.
     */
    std::optional<InlayHintLabelPartTooltip> _tooltip{};
    /**
     * An optional source code location that represents this
     * label part.
     *
     * The editor will use this location for the hover and for code navigation
     * features: This part will become a clickable link that resolves to the
     * definition of the symbol at the given location (not necessarily the
     * location itself), it shows the hover that shows at the given location,
     * and it shows a context menu with further code navigation commands.
     *
     * Depending on the client capability `inlayHint.resolveSupport` clients
     * might resolve this property late using the resolve request.
     */
    std::optional<Location> _location{};
    /**
     * An optional command for this label part.
     *
     * Depending on the client capability `inlayHint.resolveSupport` clients
     * might resolve this property late using the resolve request.
     */
    std::optional<Command> _command{};

    InlayHintLabelPart& value(const QString & v) { _value = v; return *this; }
    InlayHintLabelPart& tooltip(const std::optional<InlayHintLabelPartTooltip> & v) { _tooltip = v; return *this; }
    InlayHintLabelPart& location(const std::optional<Location> & v) { _location = v; return *this; }
    InlayHintLabelPart& command(const std::optional<Command> & v) { _command = v; return *this; }

    const QString& value() const { return _value; }
    const std::optional<InlayHintLabelPartTooltip>& tooltip() const { return _tooltip; }
    const std::optional<Location>& location() const { return _location; }
    const std::optional<Command>& command() const { return _command; }

    bool operator==(const InlayHintLabelPart &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHintLabelPart> fromJson<InlayHintLabelPart>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlayHintLabelPart &data);

using InlayHintLabel = std::variant<QString, QList<InlayHintLabelPart>>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHintLabel> fromJson<InlayHintLabel>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const InlayHintLabel &val);

/**
 * Inlay hint information.
 *
 * @since 3.17.0
 */
struct InlayHint {
    /**
     * The position of this hint.
     *
     * If multiple hints have the same position, they will be shown in the order
     * they appear in the response.
     */
    Position _position{};
    /**
     * The label of this hint. A human readable string or an array of
     * InlayHintLabelPart label parts.
     *
     * *Note* that neither the string nor the label part can be empty.
     */
    InlayHintLabel _label{};
    /**
     * The kind of this hint. Can be omitted in which case the client
     * should fall back to a reasonable default.
     */
    std::optional<int> _kind{};
    /**
     * Optional text edits that are performed when accepting this inlay hint.
     *
     * *Note* that edits are expected to change the document so that the inlay
     * hint (or its nearest variant) is now part of the document and the inlay
     * hint itself is now obsolete.
     */
    std::optional<QList<TextEdit>> _textEdits{};
    std::optional<InlayHintLabelPartTooltip> _tooltip{};  //!< The tooltip text when you hover over this item.
    /**
     * Render padding before the hint.
     *
     * Note: Padding should use the editor's background color, not the
     * background color of the hint itself. That means padding can be used
     * to visually align/separate an inlay hint.
     */
    std::optional<bool> _paddingLeft{};
    /**
     * Render padding after the hint.
     *
     * Note: Padding should use the editor's background color, not the
     * background color of the hint itself. That means padding can be used
     * to visually align/separate an inlay hint.
     */
    std::optional<bool> _paddingRight{};
    /**
     * A data entry field that is preserved on an inlay hint between
     * a `textDocument/inlayHint` and a `inlayHint/resolve` request.
     */
    std::optional<QJsonValue> _data{};

    InlayHint& position(const Position & v) { _position = v; return *this; }
    InlayHint& label(const InlayHintLabel & v) { _label = v; return *this; }
    InlayHint& kind(std::optional<int> v) { _kind = v; return *this; }
    InlayHint& textEdits(const std::optional<QList<TextEdit>> & v) { _textEdits = v; return *this; }
    InlayHint& addTextEdit(const TextEdit & v) { if (!_textEdits) _textEdits = QList<TextEdit>{}; (*_textEdits).append(v); return *this; }
    InlayHint& tooltip(const std::optional<InlayHintLabelPartTooltip> & v) { _tooltip = v; return *this; }
    InlayHint& paddingLeft(std::optional<bool> v) { _paddingLeft = v; return *this; }
    InlayHint& paddingRight(std::optional<bool> v) { _paddingRight = v; return *this; }
    InlayHint& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const Position& position() const { return _position; }
    const InlayHintLabel& label() const { return _label; }
    const std::optional<int>& kind() const { return _kind; }
    const std::optional<QList<TextEdit>>& textEdits() const { return _textEdits; }
    const std::optional<InlayHintLabelPartTooltip>& tooltip() const { return _tooltip; }
    const std::optional<bool>& paddingLeft() const { return _paddingLeft; }
    const std::optional<bool>& paddingRight() const { return _paddingRight; }
    const std::optional<QJsonValue>& data() const { return _data; }

    bool operator==(const InlayHint &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHint> fromJson<InlayHint>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlayHint &data);

/**
 * Inlay hint options used during static or dynamic registration.
 *
 * @since 3.17.0
 */
struct InlayHintRegistrationOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * The server provides support to resolve additional
     * information for an inlay hint item.
     */
    std::optional<bool> _resolveProvider{};
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    InlayHintRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    InlayHintRegistrationOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }
    InlayHintRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    InlayHintRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }
    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const InlayHintRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHintRegistrationOptions> fromJson<InlayHintRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlayHintRegistrationOptions &data);

/**
 * Parameters of the document diagnostic request.
 *
 * @since 3.17.0
 */
struct DocumentDiagnosticParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    std::optional<QString> _identifier{};  //!< The additional identifier  provided during registration.
    std::optional<QString> _previousResultId{};  //!< The result id of a previous response if provided.

    DocumentDiagnosticParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    DocumentDiagnosticParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    DocumentDiagnosticParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    DocumentDiagnosticParams& identifier(const std::optional<QString> & v) { _identifier = v; return *this; }
    DocumentDiagnosticParams& previousResultId(const std::optional<QString> & v) { _previousResultId = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const std::optional<QString>& identifier() const { return _identifier; }
    const std::optional<QString>& previousResultId() const { return _previousResultId; }

    bool operator==(const DocumentDiagnosticParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentDiagnosticParams> fromJson<DocumentDiagnosticParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentDiagnosticParams &data);

/**
 * Cancellation data returned from a diagnostic request.
 *
 * @since 3.17.0
 */
struct DiagnosticServerCancellationData {
    bool _retriggerRequest{};

    DiagnosticServerCancellationData& retriggerRequest(bool v) { _retriggerRequest = v; return *this; }

    const bool& retriggerRequest() const { return _retriggerRequest; }

    bool operator==(const DiagnosticServerCancellationData &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DiagnosticServerCancellationData> fromJson<DiagnosticServerCancellationData>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DiagnosticServerCancellationData &data);

/**
 * Diagnostic registration options.
 *
 * @since 3.17.0
 */
struct DiagnosticRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * An optional identifier under which the diagnostics are
     * managed by the client.
     */
    std::optional<QString> _identifier{};
    /**
     * Whether the language has inter file dependencies meaning that
     * editing code in one file can result in a different diagnostic
     * set in another file. Inter file dependencies are common for
     * most programming languages and typically uncommon for linters.
     */
    bool _interFileDependencies{};
    bool _workspaceDiagnostics{};  //!< The server provides support for workspace diagnostics as well.
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    DiagnosticRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    DiagnosticRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    DiagnosticRegistrationOptions& identifier(const std::optional<QString> & v) { _identifier = v; return *this; }
    DiagnosticRegistrationOptions& interFileDependencies(bool v) { _interFileDependencies = v; return *this; }
    DiagnosticRegistrationOptions& workspaceDiagnostics(bool v) { _workspaceDiagnostics = v; return *this; }
    DiagnosticRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& identifier() const { return _identifier; }
    const bool& interFileDependencies() const { return _interFileDependencies; }
    const bool& workspaceDiagnostics() const { return _workspaceDiagnostics; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const DiagnosticRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DiagnosticRegistrationOptions> fromJson<DiagnosticRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DiagnosticRegistrationOptions &data);

/**
 * A previous result id in a workspace pull request.
 *
 * @since 3.17.0
 */
struct PreviousResultId {
    /**
     * The URI for which the client knowns a
     * result id.
     */
    QString _uri{};
    QString _value{};  //!< The value of the previous result id.

    PreviousResultId& uri(const QString & v) { _uri = v; return *this; }
    PreviousResultId& value(const QString & v) { _value = v; return *this; }

    const QString& uri() const { return _uri; }
    const QString& value() const { return _value; }

    bool operator==(const PreviousResultId &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<PreviousResultId> fromJson<PreviousResultId>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const PreviousResultId &data);

/**
 * Parameters of the workspace diagnostic request.
 *
 * @since 3.17.0
 */
struct WorkspaceDiagnosticParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    std::optional<QString> _identifier{};  //!< The additional identifier provided during registration.
    /**
     * The currently known diagnostic reports with their
     * previous result ids.
     */
    QList<PreviousResultId> _previousResultIds{};

    WorkspaceDiagnosticParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    WorkspaceDiagnosticParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    WorkspaceDiagnosticParams& identifier(const std::optional<QString> & v) { _identifier = v; return *this; }
    WorkspaceDiagnosticParams& previousResultIds(const QList<PreviousResultId> & v) { _previousResultIds = v; return *this; }
    WorkspaceDiagnosticParams& addPreviousResultId(const PreviousResultId & v) { _previousResultIds.append(v); return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const std::optional<QString>& identifier() const { return _identifier; }
    const QList<PreviousResultId>& previousResultIds() const { return _previousResultIds; }

    bool operator==(const WorkspaceDiagnosticParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceDiagnosticParams> fromJson<WorkspaceDiagnosticParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceDiagnosticParams &data);

/**
 * Structure to capture a description for an error code.
 *
 * @since 3.16.0
 */
struct CodeDescription {
    QString _href{};  //!< An URI to open with more information about the diagnostic error.

    CodeDescription& href(const QString & v) { _href = v; return *this; }

    const QString& href() const { return _href; }

    bool operator==(const CodeDescription &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeDescription> fromJson<CodeDescription>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeDescription &data);

/**
 * Represents a related message and source code location for a diagnostic. This should be
 * used to point to code locations that cause or related to a diagnostics, e.g when duplicating
 * a symbol in a scope.
 */
struct DiagnosticRelatedInformation {
    Location _location{};  //!< The location of this related diagnostic information.
    QString _message{};  //!< The message of this related diagnostic information.

    DiagnosticRelatedInformation& location(const Location & v) { _location = v; return *this; }
    DiagnosticRelatedInformation& message(const QString & v) { _message = v; return *this; }

    const Location& location() const { return _location; }
    const QString& message() const { return _message; }

    bool operator==(const DiagnosticRelatedInformation &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DiagnosticRelatedInformation> fromJson<DiagnosticRelatedInformation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DiagnosticRelatedInformation &data);

/** The diagnostic's severity. */
namespace DiagnosticSeverity {
    constexpr int Error = 1;
    constexpr int Warning = 2;
    constexpr int Information = 3;
    constexpr int Hint = 4;
} // namespace DiagnosticSeverity
/**
 * The diagnostic tags.
 *
 * @since 3.15.0
 */
namespace DiagnosticTag {
    constexpr int Unnecessary = 1;
    constexpr int Deprecated = 2;
} // namespace DiagnosticTag
/**
 * Represents a diagnostic, such as a compiler error or warning. Diagnostic objects
 * are only valid in the scope of a resource.
 */
struct Diagnostic {
    Range _range{};  //!< The range at which the message applies
    /**
     * The diagnostic's severity. To avoid interpretation mismatches when a
     * server is used with different clients it is highly recommended that servers
     * always provide a severity value.
     */
    std::optional<int> _severity{};
    std::optional<ProgressToken> _code{};  //!< The diagnostic's code, which usually appear in the user interface.
    /**
     * An optional property to describe the error code.
     * Requires the code field (above) to be present/not null.
     *
     * @since 3.16.0
     */
    std::optional<CodeDescription> _codeDescription{};
    /**
     * A human-readable string describing the source of this
     * diagnostic, e.g. 'typescript' or 'super lint'. It usually
     * appears in the user interface.
     */
    std::optional<QString> _source{};
    /**
     * The diagnostic's message. It usually appears in the user interface.
     *
     * @since 3.18.0 - support for MarkupContent. This is guarded by the client
     * capability `textDocument.diagnostic.markupMessageSupport`.
     */
    InlayHintLabelPartTooltip _message{};
    /**
     * Additional metadata about the diagnostic.
     *
     * @since 3.15.0
     */
    std::optional<QList<int>> _tags{};
    /**
     * An array of related diagnostic information, e.g. when symbol-names within
     * a scope collide all definitions can be marked via this property.
     */
    std::optional<QList<DiagnosticRelatedInformation>> _relatedInformation{};
    /**
     * A data entry field that is preserved between a `textDocument/publishDiagnostics`
     * notification and `textDocument/codeAction` request.
     *
     * @since 3.16.0
     */
    std::optional<QJsonValue> _data{};

    Diagnostic& range(const Range & v) { _range = v; return *this; }
    Diagnostic& severity(std::optional<int> v) { _severity = v; return *this; }
    Diagnostic& code(const std::optional<ProgressToken> & v) { _code = v; return *this; }
    Diagnostic& codeDescription(const std::optional<CodeDescription> & v) { _codeDescription = v; return *this; }
    Diagnostic& source(const std::optional<QString> & v) { _source = v; return *this; }
    Diagnostic& message(const InlayHintLabelPartTooltip & v) { _message = v; return *this; }
    Diagnostic& tags(const std::optional<QList<int>> & v) { _tags = v; return *this; }
    Diagnostic& addTag(int v) { if (!_tags) _tags = QList<int>{}; (*_tags).append(v); return *this; }
    Diagnostic& relatedInformation(const std::optional<QList<DiagnosticRelatedInformation>> & v) { _relatedInformation = v; return *this; }
    Diagnostic& addRelatedInformation(const DiagnosticRelatedInformation & v) { if (!_relatedInformation) _relatedInformation = QList<DiagnosticRelatedInformation>{}; (*_relatedInformation).append(v); return *this; }
    Diagnostic& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const Range& range() const { return _range; }
    const std::optional<int>& severity() const { return _severity; }
    const std::optional<ProgressToken>& code() const { return _code; }
    const std::optional<CodeDescription>& codeDescription() const { return _codeDescription; }
    const std::optional<QString>& source() const { return _source; }
    const InlayHintLabelPartTooltip& message() const { return _message; }
    const std::optional<QList<int>>& tags() const { return _tags; }
    const std::optional<QList<DiagnosticRelatedInformation>>& relatedInformation() const { return _relatedInformation; }
    const std::optional<QJsonValue>& data() const { return _data; }

    bool operator==(const Diagnostic &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Diagnostic> fromJson<Diagnostic>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const Diagnostic &data);

/**
 * A full document diagnostic report for a workspace diagnostic result.
 *
 * @since 3.17.0
 */
struct WorkspaceFullDocumentDiagnosticReport {
    /**
     * An optional result id. If provided it will
     * be sent on the next diagnostic request for the
     * same document.
     */
    std::optional<QString> _resultId{};
    QList<Diagnostic> _items{};  //!< The actual items.
    QString _uri{};  //!< The URI for which diagnostic information is reported.
    /**
     * The version number for which the diagnostics are reported.
     * If the document is not marked as open `null` can be provided.
     */
    std::optional<int> _version{};

    WorkspaceFullDocumentDiagnosticReport& resultId(const std::optional<QString> & v) { _resultId = v; return *this; }
    WorkspaceFullDocumentDiagnosticReport& items(const QList<Diagnostic> & v) { _items = v; return *this; }
    WorkspaceFullDocumentDiagnosticReport& addItem(const Diagnostic & v) { _items.append(v); return *this; }
    WorkspaceFullDocumentDiagnosticReport& uri(const QString & v) { _uri = v; return *this; }
    WorkspaceFullDocumentDiagnosticReport& version(std::optional<int> v) { _version = v; return *this; }

    const std::optional<QString>& resultId() const { return _resultId; }
    const QList<Diagnostic>& items() const { return _items; }
    const QString& uri() const { return _uri; }
    const std::optional<int>& version() const { return _version; }

    bool operator==(const WorkspaceFullDocumentDiagnosticReport &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceFullDocumentDiagnosticReport> fromJson<WorkspaceFullDocumentDiagnosticReport>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceFullDocumentDiagnosticReport &data);

/**
 * An unchanged document diagnostic report for a workspace diagnostic result.
 *
 * @since 3.17.0
 */
struct WorkspaceUnchangedDocumentDiagnosticReport {
    /**
     * A result id which will be sent on the next
     * diagnostic request for the same document.
     */
    QString _resultId{};
    QString _uri{};  //!< The URI for which diagnostic information is reported.
    /**
     * The version number for which the diagnostics are reported.
     * If the document is not marked as open `null` can be provided.
     */
    std::optional<int> _version{};

    WorkspaceUnchangedDocumentDiagnosticReport& resultId(const QString & v) { _resultId = v; return *this; }
    WorkspaceUnchangedDocumentDiagnosticReport& uri(const QString & v) { _uri = v; return *this; }
    WorkspaceUnchangedDocumentDiagnosticReport& version(std::optional<int> v) { _version = v; return *this; }

    const QString& resultId() const { return _resultId; }
    const QString& uri() const { return _uri; }
    const std::optional<int>& version() const { return _version; }

    bool operator==(const WorkspaceUnchangedDocumentDiagnosticReport &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceUnchangedDocumentDiagnosticReport> fromJson<WorkspaceUnchangedDocumentDiagnosticReport>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceUnchangedDocumentDiagnosticReport &data);

/**
 * A workspace diagnostic document report.
 *
 * @since 3.17.0
 */
using WorkspaceDocumentDiagnosticReport = std::variant<WorkspaceFullDocumentDiagnosticReport, WorkspaceUnchangedDocumentDiagnosticReport>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceDocumentDiagnosticReport> fromJson<WorkspaceDocumentDiagnosticReport>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceDocumentDiagnosticReport &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const WorkspaceDocumentDiagnosticReport &val);

/** Returns the 'kind' dispatch field value for the active variant. */
LANGUAGESERVERPROTOCOL_EXPORT QString dispatchValue(const WorkspaceDocumentDiagnosticReport &val);

/** Returns the 'uri' field from the active variant. */
LANGUAGESERVERPROTOCOL_EXPORT QString uri(const WorkspaceDocumentDiagnosticReport &val);

/**
 * A workspace diagnostic report.
 *
 * @since 3.17.0
 */
struct WorkspaceDiagnosticReport {
    QList<WorkspaceDocumentDiagnosticReport> _items{};

    WorkspaceDiagnosticReport& items(const QList<WorkspaceDocumentDiagnosticReport> & v) { _items = v; return *this; }
    WorkspaceDiagnosticReport& addItem(const WorkspaceDocumentDiagnosticReport & v) { _items.append(v); return *this; }

    const QList<WorkspaceDocumentDiagnosticReport>& items() const { return _items; }

    bool operator==(const WorkspaceDiagnosticReport &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceDiagnosticReport> fromJson<WorkspaceDiagnosticReport>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceDiagnosticReport &data);

/**
 * A partial result for a workspace diagnostic report.
 *
 * @since 3.17.0
 */
struct WorkspaceDiagnosticReportPartialResult {
    QList<WorkspaceDocumentDiagnosticReport> _items{};

    WorkspaceDiagnosticReportPartialResult& items(const QList<WorkspaceDocumentDiagnosticReport> & v) { _items = v; return *this; }
    WorkspaceDiagnosticReportPartialResult& addItem(const WorkspaceDocumentDiagnosticReport & v) { _items.append(v); return *this; }

    const QList<WorkspaceDocumentDiagnosticReport>& items() const { return _items; }

    bool operator==(const WorkspaceDiagnosticReportPartialResult &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceDiagnosticReportPartialResult> fromJson<WorkspaceDiagnosticReportPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceDiagnosticReportPartialResult &data);

struct ExecutionSummary {
    /**
     * A strict monotonically increasing value
     * indicating the execution order of a cell
     * inside a notebook.
     */
    int _executionOrder{};
    /**
     * Whether the execution was successful or
     * not if known by the client.
     */
    std::optional<bool> _success{};

    ExecutionSummary& executionOrder(int v) { _executionOrder = v; return *this; }
    ExecutionSummary& success(std::optional<bool> v) { _success = v; return *this; }

    const int& executionOrder() const { return _executionOrder; }
    const std::optional<bool>& success() const { return _success; }

    bool operator==(const ExecutionSummary &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ExecutionSummary> fromJson<ExecutionSummary>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ExecutionSummary &data);

/**
 * A notebook cell kind.
 *
 * @since 3.17.0
 */
namespace NotebookCellKind {
    constexpr int Markup = 1;
    constexpr int Code = 2;
} // namespace NotebookCellKind
/**
 * A notebook cell.
 *
 * A cell's document URI must be unique across ALL notebook
 * cells and can therefore be used to uniquely identify a
 * notebook cell or the cell's text document.
 *
 * @since 3.17.0
 */
struct NotebookCell {
    int _kind{};  //!< The cell's kind
    /**
     * The URI of the cell's text document
     * content.
     */
    QString _document{};
    /**
     * Additional metadata stored with the cell.
     *
     * Note: should always be an object literal (e.g. LSPObject)
     */
    std::optional<QJsonObject> _metadata{};
    /**
     * Additional execution summary information
     * if supported by the client.
     */
    std::optional<ExecutionSummary> _executionSummary{};

    NotebookCell& kind(int v) { _kind = v; return *this; }
    NotebookCell& document(const QString & v) { _document = v; return *this; }
    NotebookCell& metadata(const std::optional<QJsonObject> & v) { _metadata = v; return *this; }
    NotebookCell& executionSummary(const std::optional<ExecutionSummary> & v) { _executionSummary = v; return *this; }

    const int& kind() const { return _kind; }
    const QString& document() const { return _document; }
    const std::optional<QJsonObject>& metadata() const { return _metadata; }
    const std::optional<ExecutionSummary>& executionSummary() const { return _executionSummary; }

    bool operator==(const NotebookCell &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookCell> fromJson<NotebookCell>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookCell &data);

/**
 * A notebook document.
 *
 * @since 3.17.0
 */
struct NotebookDocument {
    QString _uri{};  //!< The notebook document's uri.
    QString _notebookType{};  //!< The type of the notebook.
    /**
     * The version number of this document (it will increase after each
     * change, including undo/redo).
     */
    int _version{};
    /**
     * Additional metadata stored with the notebook
     * document.
     *
     * Note: should always be an object literal (e.g. LSPObject)
     */
    std::optional<QJsonObject> _metadata{};
    QList<NotebookCell> _cells{};  //!< The cells of a notebook.

    NotebookDocument& uri(const QString & v) { _uri = v; return *this; }
    NotebookDocument& notebookType(const QString & v) { _notebookType = v; return *this; }
    NotebookDocument& version(int v) { _version = v; return *this; }
    NotebookDocument& metadata(const std::optional<QJsonObject> & v) { _metadata = v; return *this; }
    NotebookDocument& cells(const QList<NotebookCell> & v) { _cells = v; return *this; }
    NotebookDocument& addCell(const NotebookCell & v) { _cells.append(v); return *this; }

    const QString& uri() const { return _uri; }
    const QString& notebookType() const { return _notebookType; }
    const int& version() const { return _version; }
    const std::optional<QJsonObject>& metadata() const { return _metadata; }
    const QList<NotebookCell>& cells() const { return _cells; }

    bool operator==(const NotebookDocument &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocument> fromJson<NotebookDocument>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocument &data);

/**
 * Predefined Language kinds
 * @since 3.18.0
 */
using LanguageKind = QString;

namespace LanguageKinds {
    constexpr char ABAP[] = "abap";
    constexpr char WindowsBat[] = "bat";
    constexpr char BibTeX[] = "bibtex";
    constexpr char Clojure[] = "clojure";
    constexpr char Coffeescript[] = "coffeescript";
    constexpr char C[] = "c";
    constexpr char CPP[] = "cpp";
    constexpr char CSharp[] = "csharp";
    constexpr char CSS[] = "css";
    constexpr char D[] = "d";
    constexpr char Delphi[] = "pascal";
    constexpr char Diff[] = "diff";
    constexpr char Dart[] = "dart";
    constexpr char Dockerfile[] = "dockerfile";
    constexpr char Elixir[] = "elixir";
    constexpr char Erlang[] = "erlang";
    constexpr char FSharp[] = "fsharp";
    constexpr char GitCommit[] = "git-commit";
    constexpr char GitRebase[] = "git-rebase";
    constexpr char Go[] = "go";
    constexpr char Groovy[] = "groovy";
    constexpr char Handlebars[] = "handlebars";
    constexpr char Haskell[] = "haskell";
    constexpr char HTML[] = "html";
    constexpr char Ini[] = "ini";
    constexpr char Java[] = "java";
    constexpr char JavaScript[] = "javascript";
    constexpr char JavaScriptReact[] = "javascriptreact";
    constexpr char JSON[] = "json";
    constexpr char LaTeX[] = "latex";
    constexpr char Less[] = "less";
    constexpr char Lua[] = "lua";
    constexpr char Makefile[] = "makefile";
    constexpr char Markdown[] = "markdown";
    constexpr char ObjectiveC[] = "objective-c";
    constexpr char ObjectiveCPP[] = "objective-cpp";
    constexpr char Pascal[] = "pascal";
    constexpr char Perl[] = "perl";
    constexpr char Perl6[] = "perl6";
    constexpr char PHP[] = "php";
    constexpr char Plaintext[] = "plaintext";
    constexpr char Powershell[] = "powershell";
    constexpr char Pug[] = "jade";
    constexpr char Python[] = "python";
    constexpr char R[] = "r";
    constexpr char Razor[] = "razor";
    constexpr char Ruby[] = "ruby";
    constexpr char Rust[] = "rust";
    constexpr char SCSS[] = "scss";
    constexpr char SASS[] = "sass";
    constexpr char Scala[] = "scala";
    constexpr char ShaderLab[] = "shaderlab";
    constexpr char ShellScript[] = "shellscript";
    constexpr char SQL[] = "sql";
    constexpr char Swift[] = "swift";
    constexpr char TypeScript[] = "typescript";
    constexpr char TypeScriptReact[] = "typescriptreact";
    constexpr char TeX[] = "tex";
    constexpr char VisualBasic[] = "vb";
    constexpr char XML[] = "xml";
    constexpr char XSL[] = "xsl";
    constexpr char YAML[] = "yaml";
} // namespace LanguageKinds
/**
 * An item to transfer a text document from the client to the
 * server.
 */
struct TextDocumentItem {
    QString _uri{};  //!< The text document's uri.
    LanguageKind _languageId{};  //!< The text document's language identifier.
    /**
     * The version number of this document (it will increase after each
     * change, including undo/redo).
     */
    int _version{};
    QString _text{};  //!< The content of the opened text document.

    TextDocumentItem& uri(const QString & v) { _uri = v; return *this; }
    TextDocumentItem& languageId(const LanguageKind & v) { _languageId = v; return *this; }
    TextDocumentItem& version(int v) { _version = v; return *this; }
    TextDocumentItem& text(const QString & v) { _text = v; return *this; }

    const QString& uri() const { return _uri; }
    const LanguageKind& languageId() const { return _languageId; }
    const int& version() const { return _version; }
    const QString& text() const { return _text; }

    bool operator==(const TextDocumentItem &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentItem> fromJson<TextDocumentItem>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentItem &data);

/**
 * The params sent in an open notebook document notification.
 *
 * @since 3.17.0
 */
struct DidOpenNotebookDocumentParams {
    NotebookDocument _notebookDocument{};  //!< The notebook document that got opened.
    /**
     * The text documents that represent the content
     * of a notebook cell.
     */
    QList<TextDocumentItem> _cellTextDocuments{};

    DidOpenNotebookDocumentParams& notebookDocument(const NotebookDocument & v) { _notebookDocument = v; return *this; }
    DidOpenNotebookDocumentParams& cellTextDocuments(const QList<TextDocumentItem> & v) { _cellTextDocuments = v; return *this; }
    DidOpenNotebookDocumentParams& addCellTextDocument(const TextDocumentItem & v) { _cellTextDocuments.append(v); return *this; }

    const NotebookDocument& notebookDocument() const { return _notebookDocument; }
    const QList<TextDocumentItem>& cellTextDocuments() const { return _cellTextDocuments; }

    bool operator==(const DidOpenNotebookDocumentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidOpenNotebookDocumentParams> fromJson<DidOpenNotebookDocumentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidOpenNotebookDocumentParams &data);

/** @since 3.18.0 */
struct NotebookCellLanguage {
    QString _language{};

    NotebookCellLanguage& language(const QString & v) { _language = v; return *this; }

    const QString& language() const { return _language; }

    bool operator==(const NotebookCellLanguage &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookCellLanguage> fromJson<NotebookCellLanguage>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookCellLanguage &data);

/** @since 3.18.0 */
struct NotebookDocumentFilterWithCells {
    /**
     * The notebook to be synced If a string
     * value is provided it matches against the
     * notebook type. '*' matches every notebook.
     */
    std::optional<NotebookCellTextDocumentFilterNotebook> _notebook{};
    QList<NotebookCellLanguage> _cells{};  //!< The cells of the matching notebook to be synced.

    NotebookDocumentFilterWithCells& notebook(const std::optional<NotebookCellTextDocumentFilterNotebook> & v) { _notebook = v; return *this; }
    NotebookDocumentFilterWithCells& cells(const QList<NotebookCellLanguage> & v) { _cells = v; return *this; }
    NotebookDocumentFilterWithCells& addCell(const NotebookCellLanguage & v) { _cells.append(v); return *this; }

    const std::optional<NotebookCellTextDocumentFilterNotebook>& notebook() const { return _notebook; }
    const QList<NotebookCellLanguage>& cells() const { return _cells; }

    bool operator==(const NotebookDocumentFilterWithCells &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentFilterWithCells> fromJson<NotebookDocumentFilterWithCells>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentFilterWithCells &data);

/** @since 3.18.0 */
struct NotebookDocumentFilterWithNotebook {
    /**
     * The notebook to be synced If a string
     * value is provided it matches against the
     * notebook type. '*' matches every notebook.
     */
    NotebookCellTextDocumentFilterNotebook _notebook{};
    std::optional<QList<NotebookCellLanguage>> _cells{};  //!< The cells of the matching notebook to be synced.

    NotebookDocumentFilterWithNotebook& notebook(const NotebookCellTextDocumentFilterNotebook & v) { _notebook = v; return *this; }
    NotebookDocumentFilterWithNotebook& cells(const std::optional<QList<NotebookCellLanguage>> & v) { _cells = v; return *this; }
    NotebookDocumentFilterWithNotebook& addCell(const NotebookCellLanguage & v) { if (!_cells) _cells = QList<NotebookCellLanguage>{}; (*_cells).append(v); return *this; }

    const NotebookCellTextDocumentFilterNotebook& notebook() const { return _notebook; }
    const std::optional<QList<NotebookCellLanguage>>& cells() const { return _cells; }

    bool operator==(const NotebookDocumentFilterWithNotebook &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentFilterWithNotebook> fromJson<NotebookDocumentFilterWithNotebook>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentFilterWithNotebook &data);

using NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem = std::variant<NotebookDocumentFilterWithNotebook, NotebookDocumentFilterWithCells>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem> fromJson<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem &val);

/**
 * Registration options specific to a notebook.
 *
 * @since 3.17.0
 */
struct NotebookDocumentSyncRegistrationOptions {
    QList<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem> _notebookSelector{};  //!< The notebooks to be synced
    /**
     * Whether save notification should be forwarded to
     * the server. Will only be honored if mode === `notebook`.
     */
    std::optional<bool> _save{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    NotebookDocumentSyncRegistrationOptions& notebookSelector(const QList<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem> & v) { _notebookSelector = v; return *this; }
    NotebookDocumentSyncRegistrationOptions& addNotebookSelector(const NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem & v) { _notebookSelector.append(v); return *this; }
    NotebookDocumentSyncRegistrationOptions& save(std::optional<bool> v) { _save = v; return *this; }
    NotebookDocumentSyncRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const QList<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem>& notebookSelector() const { return _notebookSelector; }
    const std::optional<bool>& save() const { return _save; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const NotebookDocumentSyncRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentSyncRegistrationOptions> fromJson<NotebookDocumentSyncRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentSyncRegistrationOptions &data);

/**
 * A change describing how to move a `NotebookCell`
 * array from state S to S'.
 *
 * @since 3.17.0
 */
struct NotebookCellArrayChange {
    int _start{};  //!< The start oftest of the cell that changed.
    int _deleteCount{};  //!< The deleted cells
    std::optional<QList<NotebookCell>> _cells{};  //!< The new cells, if any

    NotebookCellArrayChange& start(int v) { _start = v; return *this; }
    NotebookCellArrayChange& deleteCount(int v) { _deleteCount = v; return *this; }
    NotebookCellArrayChange& cells(const std::optional<QList<NotebookCell>> & v) { _cells = v; return *this; }
    NotebookCellArrayChange& addCell(const NotebookCell & v) { if (!_cells) _cells = QList<NotebookCell>{}; (*_cells).append(v); return *this; }

    const int& start() const { return _start; }
    const int& deleteCount() const { return _deleteCount; }
    const std::optional<QList<NotebookCell>>& cells() const { return _cells; }

    bool operator==(const NotebookCellArrayChange &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookCellArrayChange> fromJson<NotebookCellArrayChange>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookCellArrayChange &data);

/**
 * Structural changes to cells in a notebook document.
 *
 * @since 3.18.0
 */
struct NotebookDocumentCellChangeStructure {
    NotebookCellArrayChange _array{};  //!< The change to the cell array.
    std::optional<QList<TextDocumentItem>> _didOpen{};  //!< Additional opened cell text documents.
    std::optional<QList<TextDocumentIdentifier>> _didClose{};  //!< Additional closed cell text documents.

    NotebookDocumentCellChangeStructure& array(const NotebookCellArrayChange & v) { _array = v; return *this; }
    NotebookDocumentCellChangeStructure& didOpen(const std::optional<QList<TextDocumentItem>> & v) { _didOpen = v; return *this; }
    NotebookDocumentCellChangeStructure& addDidOpen(const TextDocumentItem & v) { if (!_didOpen) _didOpen = QList<TextDocumentItem>{}; (*_didOpen).append(v); return *this; }
    NotebookDocumentCellChangeStructure& didClose(const std::optional<QList<TextDocumentIdentifier>> & v) { _didClose = v; return *this; }
    NotebookDocumentCellChangeStructure& addDidClose(const TextDocumentIdentifier & v) { if (!_didClose) _didClose = QList<TextDocumentIdentifier>{}; (*_didClose).append(v); return *this; }

    const NotebookCellArrayChange& array() const { return _array; }
    const std::optional<QList<TextDocumentItem>>& didOpen() const { return _didOpen; }
    const std::optional<QList<TextDocumentIdentifier>>& didClose() const { return _didClose; }

    bool operator==(const NotebookDocumentCellChangeStructure &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentCellChangeStructure> fromJson<NotebookDocumentCellChangeStructure>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentCellChangeStructure &data);

/** @since 3.18.0 */
struct TextDocumentContentChangePartial {
    Range _range{};  //!< The range of the document that changed.
    /**
     * The optional length of the range that got replaced.
     *
     * @deprecated use range instead.
     */
    std::optional<int> _rangeLength{};
    QString _text{};  //!< The new text for the provided range.

    TextDocumentContentChangePartial& range(const Range & v) { _range = v; return *this; }
    TextDocumentContentChangePartial& rangeLength(std::optional<int> v) { _rangeLength = v; return *this; }
    TextDocumentContentChangePartial& text(const QString & v) { _text = v; return *this; }

    const Range& range() const { return _range; }
    const std::optional<int>& rangeLength() const { return _rangeLength; }
    const QString& text() const { return _text; }

    bool operator==(const TextDocumentContentChangePartial &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentContentChangePartial> fromJson<TextDocumentContentChangePartial>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentContentChangePartial &data);

/** @since 3.18.0 */
struct TextDocumentContentChangeWholeDocument {
    QString _text{};  //!< The new text of the whole document.

    TextDocumentContentChangeWholeDocument& text(const QString & v) { _text = v; return *this; }

    const QString& text() const { return _text; }

    bool operator==(const TextDocumentContentChangeWholeDocument &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentContentChangeWholeDocument> fromJson<TextDocumentContentChangeWholeDocument>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentContentChangeWholeDocument &data);

/**
 * An event describing a change to a text document. If only a text is provided
 * it is considered to be the full content of the document.
 */
using TextDocumentContentChangeEvent = std::variant<TextDocumentContentChangePartial, TextDocumentContentChangeWholeDocument>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentContentChangeEvent> fromJson<TextDocumentContentChangeEvent>(const QJsonValue &val);

/** Returns the 'text' field from the active variant. */
LANGUAGESERVERPROTOCOL_EXPORT QString text(const TextDocumentContentChangeEvent &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentContentChangeEvent &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const TextDocumentContentChangeEvent &val);

/** A text document identifier to denote a specific version of a text document. */
struct VersionedTextDocumentIdentifier {
    QString _uri{};  //!< The text document's uri.
    int _version{};  //!< The version number of this document.

    VersionedTextDocumentIdentifier& uri(const QString & v) { _uri = v; return *this; }
    VersionedTextDocumentIdentifier& version(int v) { _version = v; return *this; }

    const QString& uri() const { return _uri; }
    const int& version() const { return _version; }

    bool operator==(const VersionedTextDocumentIdentifier &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<VersionedTextDocumentIdentifier> fromJson<VersionedTextDocumentIdentifier>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const VersionedTextDocumentIdentifier &data);

/**
 * Content changes to a cell in a notebook document.
 *
 * @since 3.18.0
 */
struct NotebookDocumentCellContentChanges {
    VersionedTextDocumentIdentifier _document{};
    QList<TextDocumentContentChangeEvent> _changes{};

    NotebookDocumentCellContentChanges& document(const VersionedTextDocumentIdentifier & v) { _document = v; return *this; }
    NotebookDocumentCellContentChanges& changes(const QList<TextDocumentContentChangeEvent> & v) { _changes = v; return *this; }
    NotebookDocumentCellContentChanges& addChange(const TextDocumentContentChangeEvent & v) { _changes.append(v); return *this; }

    const VersionedTextDocumentIdentifier& document() const { return _document; }
    const QList<TextDocumentContentChangeEvent>& changes() const { return _changes; }

    bool operator==(const NotebookDocumentCellContentChanges &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentCellContentChanges> fromJson<NotebookDocumentCellContentChanges>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentCellContentChanges &data);

/**
 * Cell changes to a notebook document.
 *
 * @since 3.18.0
 */
struct NotebookDocumentCellChanges {
    /**
     * Changes to the cell structure to add or
     * remove cells.
     */
    std::optional<NotebookDocumentCellChangeStructure> _structure{};
    /**
     * Changes to notebook cells properties like its
     * kind, execution summary or metadata.
     */
    std::optional<QList<NotebookCell>> _data{};
    std::optional<QList<NotebookDocumentCellContentChanges>> _textContent{};  //!< Changes to the text content of notebook cells.

    NotebookDocumentCellChanges& structure(const std::optional<NotebookDocumentCellChangeStructure> & v) { _structure = v; return *this; }
    NotebookDocumentCellChanges& data(const std::optional<QList<NotebookCell>> & v) { _data = v; return *this; }
    NotebookDocumentCellChanges& addData(const NotebookCell & v) { if (!_data) _data = QList<NotebookCell>{}; (*_data).append(v); return *this; }
    NotebookDocumentCellChanges& textContent(const std::optional<QList<NotebookDocumentCellContentChanges>> & v) { _textContent = v; return *this; }
    NotebookDocumentCellChanges& addTextContent(const NotebookDocumentCellContentChanges & v) { if (!_textContent) _textContent = QList<NotebookDocumentCellContentChanges>{}; (*_textContent).append(v); return *this; }

    const std::optional<NotebookDocumentCellChangeStructure>& structure() const { return _structure; }
    const std::optional<QList<NotebookCell>>& data() const { return _data; }
    const std::optional<QList<NotebookDocumentCellContentChanges>>& textContent() const { return _textContent; }

    bool operator==(const NotebookDocumentCellChanges &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentCellChanges> fromJson<NotebookDocumentCellChanges>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentCellChanges &data);

/**
 * A change event for a notebook document.
 *
 * @since 3.17.0
 */
struct NotebookDocumentChangeEvent {
    /**
     * The changed meta data if any.
     *
     * Note: should always be an object literal (e.g. LSPObject)
     */
    std::optional<QJsonObject> _metadata{};
    std::optional<NotebookDocumentCellChanges> _cells{};  //!< Changes to cells

    NotebookDocumentChangeEvent& metadata(const std::optional<QJsonObject> & v) { _metadata = v; return *this; }
    NotebookDocumentChangeEvent& cells(const std::optional<NotebookDocumentCellChanges> & v) { _cells = v; return *this; }

    const std::optional<QJsonObject>& metadata() const { return _metadata; }
    const std::optional<NotebookDocumentCellChanges>& cells() const { return _cells; }

    bool operator==(const NotebookDocumentChangeEvent &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentChangeEvent> fromJson<NotebookDocumentChangeEvent>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentChangeEvent &data);

/**
 * A versioned notebook document identifier.
 *
 * @since 3.17.0
 */
struct VersionedNotebookDocumentIdentifier {
    int _version{};  //!< The version number of this notebook document.
    QString _uri{};  //!< The notebook document's uri.

    VersionedNotebookDocumentIdentifier& version(int v) { _version = v; return *this; }
    VersionedNotebookDocumentIdentifier& uri(const QString & v) { _uri = v; return *this; }

    const int& version() const { return _version; }
    const QString& uri() const { return _uri; }

    bool operator==(const VersionedNotebookDocumentIdentifier &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<VersionedNotebookDocumentIdentifier> fromJson<VersionedNotebookDocumentIdentifier>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const VersionedNotebookDocumentIdentifier &data);

/**
 * The params sent in a change notebook document notification.
 *
 * @since 3.17.0
 */
struct DidChangeNotebookDocumentParams {
    /**
     * The notebook document that did change. The version number points
     * to the version after all provided changes have been applied. If
     * only the text document content of a cell changes the notebook version
     * doesn't necessarily have to change.
     */
    VersionedNotebookDocumentIdentifier _notebookDocument{};
    /**
     * The actual changes to the notebook document.
     *
     * The changes describe single state changes to the notebook document.
     * So if there are two changes c1 (at array index 0) and c2 (at array
     * index 1) for a notebook in state S then c1 moves the notebook from
     * S to S' and c2 from S' to S''. So c1 is computed on the state S and
     * c2 is computed on the state S'.
     *
     * To mirror the content of a notebook using change events use the following approach:
     * - start with the same initial content
     * - apply the 'notebookDocument/didChange' notifications in the order you receive them.
     * - apply the `NotebookChangeEvent`s in a single notification in the order
     * you receive them.
     */
    NotebookDocumentChangeEvent _change{};

    DidChangeNotebookDocumentParams& notebookDocument(const VersionedNotebookDocumentIdentifier & v) { _notebookDocument = v; return *this; }
    DidChangeNotebookDocumentParams& change(const NotebookDocumentChangeEvent & v) { _change = v; return *this; }

    const VersionedNotebookDocumentIdentifier& notebookDocument() const { return _notebookDocument; }
    const NotebookDocumentChangeEvent& change() const { return _change; }

    bool operator==(const DidChangeNotebookDocumentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidChangeNotebookDocumentParams> fromJson<DidChangeNotebookDocumentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidChangeNotebookDocumentParams &data);

/**
 * A literal to identify a notebook document in the client.
 *
 * @since 3.17.0
 */
struct NotebookDocumentIdentifier {
    QString _uri{};  //!< The notebook document's uri.

    NotebookDocumentIdentifier& uri(const QString & v) { _uri = v; return *this; }

    const QString& uri() const { return _uri; }

    bool operator==(const NotebookDocumentIdentifier &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentIdentifier> fromJson<NotebookDocumentIdentifier>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentIdentifier &data);

/**
 * The params sent in a save notebook document notification.
 *
 * @since 3.17.0
 */
struct DidSaveNotebookDocumentParams {
    NotebookDocumentIdentifier _notebookDocument{};  //!< The notebook document that got saved.

    DidSaveNotebookDocumentParams& notebookDocument(const NotebookDocumentIdentifier & v) { _notebookDocument = v; return *this; }

    const NotebookDocumentIdentifier& notebookDocument() const { return _notebookDocument; }

    bool operator==(const DidSaveNotebookDocumentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidSaveNotebookDocumentParams> fromJson<DidSaveNotebookDocumentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidSaveNotebookDocumentParams &data);

/**
 * The params sent in a close notebook document notification.
 *
 * @since 3.17.0
 */
struct DidCloseNotebookDocumentParams {
    NotebookDocumentIdentifier _notebookDocument{};  //!< The notebook document that got closed.
    /**
     * The text documents that represent the content
     * of a notebook cell that got closed.
     */
    QList<TextDocumentIdentifier> _cellTextDocuments{};

    DidCloseNotebookDocumentParams& notebookDocument(const NotebookDocumentIdentifier & v) { _notebookDocument = v; return *this; }
    DidCloseNotebookDocumentParams& cellTextDocuments(const QList<TextDocumentIdentifier> & v) { _cellTextDocuments = v; return *this; }
    DidCloseNotebookDocumentParams& addCellTextDocument(const TextDocumentIdentifier & v) { _cellTextDocuments.append(v); return *this; }

    const NotebookDocumentIdentifier& notebookDocument() const { return _notebookDocument; }
    const QList<TextDocumentIdentifier>& cellTextDocuments() const { return _cellTextDocuments; }

    bool operator==(const DidCloseNotebookDocumentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidCloseNotebookDocumentParams> fromJson<DidCloseNotebookDocumentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidCloseNotebookDocumentParams &data);

/**
 * Describes how an {@link InlineCompletionItemProvider inline completion provider} was triggered.
 *
 * @since 3.18.0
 */
namespace InlineCompletionTriggerKind {
    constexpr int Invoked = 1;
    constexpr int Automatic = 2;
} // namespace InlineCompletionTriggerKind
/**
 * Describes the currently selected completion item.
 *
 * @since 3.18.0
 */
struct SelectedCompletionInfo {
    Range _range{};  //!< The range that will be replaced if this completion item is accepted.
    QString _text{};  //!< The text the range will be replaced with if this completion is accepted.

    SelectedCompletionInfo& range(const Range & v) { _range = v; return *this; }
    SelectedCompletionInfo& text(const QString & v) { _text = v; return *this; }

    const Range& range() const { return _range; }
    const QString& text() const { return _text; }

    bool operator==(const SelectedCompletionInfo &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SelectedCompletionInfo> fromJson<SelectedCompletionInfo>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SelectedCompletionInfo &data);

/**
 * Provides information about the context in which an inline completion was requested.
 *
 * @since 3.18.0
 */
struct InlineCompletionContext {
    int _triggerKind{};  //!< Describes how the inline completion was triggered.
    std::optional<SelectedCompletionInfo> _selectedCompletionInfo{};  //!< Provides information about the currently selected item in the autocomplete widget if it is visible.

    InlineCompletionContext& triggerKind(int v) { _triggerKind = v; return *this; }
    InlineCompletionContext& selectedCompletionInfo(const std::optional<SelectedCompletionInfo> & v) { _selectedCompletionInfo = v; return *this; }

    const int& triggerKind() const { return _triggerKind; }
    const std::optional<SelectedCompletionInfo>& selectedCompletionInfo() const { return _selectedCompletionInfo; }

    bool operator==(const InlineCompletionContext &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineCompletionContext> fromJson<InlineCompletionContext>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineCompletionContext &data);

/**
 * A parameter literal used in inline completion requests.
 *
 * @since 3.18.0
 */
struct InlineCompletionParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * Additional information about the context in which inline completions were
     * requested.
     */
    InlineCompletionContext _context{};

    InlineCompletionParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    InlineCompletionParams& position(const Position & v) { _position = v; return *this; }
    InlineCompletionParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    InlineCompletionParams& context(const InlineCompletionContext & v) { _context = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const InlineCompletionContext& context() const { return _context; }

    bool operator==(const InlineCompletionParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineCompletionParams> fromJson<InlineCompletionParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineCompletionParams &data);

using InlineCompletionItemInsertText = std::variant<QString, StringValue>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineCompletionItemInsertText> fromJson<InlineCompletionItemInsertText>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const InlineCompletionItemInsertText &val);

/**
 * An inline completion item represents a text snippet that is proposed inline to complete text that is being typed.
 *
 * @since 3.18.0
 */
struct InlineCompletionItem {
    InlineCompletionItemInsertText _insertText{};  //!< The text to replace the range with. Must be set.
    std::optional<QString> _filterText{};  //!< A text that is used to decide if this inline completion should be shown. When `falsy` the {@link InlineCompletionItem.insertText} is used.
    std::optional<Range> _range{};  //!< The range to replace. Must begin and end on the same line.
    std::optional<Command> _command{};  //!< An optional {@link Command} that is executed *after* inserting this completion.

    InlineCompletionItem& insertText(const InlineCompletionItemInsertText & v) { _insertText = v; return *this; }
    InlineCompletionItem& filterText(const std::optional<QString> & v) { _filterText = v; return *this; }
    InlineCompletionItem& range(const std::optional<Range> & v) { _range = v; return *this; }
    InlineCompletionItem& command(const std::optional<Command> & v) { _command = v; return *this; }

    const InlineCompletionItemInsertText& insertText() const { return _insertText; }
    const std::optional<QString>& filterText() const { return _filterText; }
    const std::optional<Range>& range() const { return _range; }
    const std::optional<Command>& command() const { return _command; }

    bool operator==(const InlineCompletionItem &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineCompletionItem> fromJson<InlineCompletionItem>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineCompletionItem &data);

/**
 * Represents a collection of {@link InlineCompletionItem inline completion items} to be presented in the editor.
 *
 * @since 3.18.0
 */
struct InlineCompletionList {
    QList<InlineCompletionItem> _items{};  //!< The inline completion items

    InlineCompletionList& items(const QList<InlineCompletionItem> & v) { _items = v; return *this; }
    InlineCompletionList& addItem(const InlineCompletionItem & v) { _items.append(v); return *this; }

    const QList<InlineCompletionItem>& items() const { return _items; }

    bool operator==(const InlineCompletionList &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineCompletionList> fromJson<InlineCompletionList>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineCompletionList &data);

/**
 * Inline completion options used during static or dynamic registration.
 *
 * @since 3.18.0
 */
struct InlineCompletionRegistrationOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    InlineCompletionRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    InlineCompletionRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    InlineCompletionRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const InlineCompletionRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineCompletionRegistrationOptions> fromJson<InlineCompletionRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineCompletionRegistrationOptions &data);

/**
 * Parameters for the `workspace/textDocumentContent` request.
 *
 * @since 3.18.0
 */
struct TextDocumentContentParams {
    QString _uri{};  //!< The uri of the text document.

    TextDocumentContentParams& uri(const QString & v) { _uri = v; return *this; }

    const QString& uri() const { return _uri; }

    bool operator==(const TextDocumentContentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentContentParams> fromJson<TextDocumentContentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentContentParams &data);

/**
 * Result of the `workspace/textDocumentContent` request.
 *
 * @since 3.18.0
 */
struct TextDocumentContentResult {
    /**
     * The text content of the text document. Please note, that the content of
     * any subsequent open notifications for the text document might differ
     * from the returned content due to whitespace and line ending
     * normalizations done on the client
     */
    QString _text{};

    TextDocumentContentResult& text(const QString & v) { _text = v; return *this; }

    const QString& text() const { return _text; }

    bool operator==(const TextDocumentContentResult &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentContentResult> fromJson<TextDocumentContentResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentContentResult &data);

/**
 * Text document content provider registration options.
 *
 * @since 3.18.0
 */
struct TextDocumentContentRegistrationOptions {
    QStringList _schemes{};  //!< The schemes for which the server provides content.
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    TextDocumentContentRegistrationOptions& schemes(const QStringList & v) { _schemes = v; return *this; }
    TextDocumentContentRegistrationOptions& addScheme(const QString & v) { _schemes.append(v); return *this; }
    TextDocumentContentRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const QStringList& schemes() const { return _schemes; }
    const std::optional<QString>& id() const { return _id; }

    bool operator==(const TextDocumentContentRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentContentRegistrationOptions> fromJson<TextDocumentContentRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentContentRegistrationOptions &data);

/**
 * Parameters for the `workspace/textDocumentContent/refresh` request.
 *
 * @since 3.18.0
 */
struct TextDocumentContentRefreshParams {
    QString _uri{};  //!< The uri of the text document to refresh.

    TextDocumentContentRefreshParams& uri(const QString & v) { _uri = v; return *this; }

    const QString& uri() const { return _uri; }

    bool operator==(const TextDocumentContentRefreshParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentContentRefreshParams> fromJson<TextDocumentContentRefreshParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentContentRefreshParams &data);

/** General parameters to register for a notification or to register a provider. */
struct Registration {
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again.
     */
    QString _id{};
    QString _method{};  //!< The method / capability to register for.
    std::optional<QJsonValue> _registerOptions{};  //!< Options necessary for the registration.

    Registration& id(const QString & v) { _id = v; return *this; }
    Registration& method(const QString & v) { _method = v; return *this; }
    Registration& registerOptions(const std::optional<QJsonValue> & v) { _registerOptions = v; return *this; }

    const QString& id() const { return _id; }
    const QString& method() const { return _method; }
    const std::optional<QJsonValue>& registerOptions() const { return _registerOptions; }

    bool operator==(const Registration &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Registration> fromJson<Registration>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const Registration &data);

struct RegistrationParams {
    QList<Registration> _registrations{};

    RegistrationParams& registrations(const QList<Registration> & v) { _registrations = v; return *this; }
    RegistrationParams& addRegistration(const Registration & v) { _registrations.append(v); return *this; }

    const QList<Registration>& registrations() const { return _registrations; }

    bool operator==(const RegistrationParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RegistrationParams> fromJson<RegistrationParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RegistrationParams &data);

/** General parameters to unregister a request or notification. */
struct Unregistration {
    /**
     * The id used to unregister the request or notification. Usually an id
     * provided during the register request.
     */
    QString _id{};
    QString _method{};  //!< The method to unregister for.

    Unregistration& id(const QString & v) { _id = v; return *this; }
    Unregistration& method(const QString & v) { _method = v; return *this; }

    const QString& id() const { return _id; }
    const QString& method() const { return _method; }

    bool operator==(const Unregistration &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Unregistration> fromJson<Unregistration>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const Unregistration &data);

struct UnregistrationParams {
    QList<Unregistration> _unregisterations{};

    UnregistrationParams& unregisterations(const QList<Unregistration> & v) { _unregisterations = v; return *this; }
    UnregistrationParams& addUnregisteration(const Unregistration & v) { _unregisterations.append(v); return *this; }

    const QList<Unregistration>& unregisterations() const { return _unregisterations; }

    bool operator==(const UnregistrationParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<UnregistrationParams> fromJson<UnregistrationParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const UnregistrationParams &data);

/**
 * Client capabilities specific to the used markdown parser.
 *
 * @since 3.16.0
 */
struct MarkdownClientCapabilities {
    QString _parser{};  //!< The name of the parser.
    std::optional<QString> _version{};  //!< The version of the parser.
    /**
     * A list of HTML tags that the client allows / supports in
     * Markdown.
     *
     * @since 3.17.0
     */
    std::optional<QStringList> _allowedTags{};

    MarkdownClientCapabilities& parser(const QString & v) { _parser = v; return *this; }
    MarkdownClientCapabilities& version(const std::optional<QString> & v) { _version = v; return *this; }
    MarkdownClientCapabilities& allowedTags(const std::optional<QStringList> & v) { _allowedTags = v; return *this; }
    MarkdownClientCapabilities& addAllowedTag(const QString & v) { if (!_allowedTags) _allowedTags = QStringList{}; (*_allowedTags).append(v); return *this; }

    const QString& parser() const { return _parser; }
    const std::optional<QString>& version() const { return _version; }
    const std::optional<QStringList>& allowedTags() const { return _allowedTags; }

    bool operator==(const MarkdownClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MarkdownClientCapabilities> fromJson<MarkdownClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const MarkdownClientCapabilities &data);

/**
 * A set of predefined position encoding kinds.
 *
 * @since 3.17.0
 */
using PositionEncodingKind = QString;

namespace PositionEncodingKinds {
    constexpr char UTF8[] = "utf-8";
    constexpr char UTF16[] = "utf-16";
    constexpr char UTF32[] = "utf-32";
} // namespace PositionEncodingKinds
using RegularExpressionEngineKind = QString;

/**
 * Client capabilities specific to regular expressions.
 *
 * @since 3.16.0
 */
struct RegularExpressionsClientCapabilities {
    RegularExpressionEngineKind _engine{};  //!< The engine's name.
    std::optional<QString> _version{};  //!< The engine's version.

    RegularExpressionsClientCapabilities& engine(const RegularExpressionEngineKind & v) { _engine = v; return *this; }
    RegularExpressionsClientCapabilities& version(const std::optional<QString> & v) { _version = v; return *this; }

    const RegularExpressionEngineKind& engine() const { return _engine; }
    const std::optional<QString>& version() const { return _version; }

    bool operator==(const RegularExpressionsClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RegularExpressionsClientCapabilities> fromJson<RegularExpressionsClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RegularExpressionsClientCapabilities &data);

/** @since 3.18.0 */
struct StaleRequestSupportOptions {
    bool _cancel{};  //!< The client will actively cancel the request.
    /**
     * The list of requests for which the client
     * will retry the request if it receives a
     * response with error code `ContentModified`
     */
    QStringList _retryOnContentModified{};

    StaleRequestSupportOptions& cancel(bool v) { _cancel = v; return *this; }
    StaleRequestSupportOptions& retryOnContentModified(const QStringList & v) { _retryOnContentModified = v; return *this; }
    StaleRequestSupportOptions& addRetryOnContentModified(const QString & v) { _retryOnContentModified.append(v); return *this; }

    const bool& cancel() const { return _cancel; }
    const QStringList& retryOnContentModified() const { return _retryOnContentModified; }

    bool operator==(const StaleRequestSupportOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<StaleRequestSupportOptions> fromJson<StaleRequestSupportOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const StaleRequestSupportOptions &data);

/**
 * General client capabilities.
 *
 * @since 3.16.0
 */
struct GeneralClientCapabilities {
    /**
     * Client capability that signals how the client
     * handles stale requests (e.g. a request
     * for which the client will not process the response
     * anymore since the information is outdated).
     *
     * @since 3.17.0
     */
    std::optional<StaleRequestSupportOptions> _staleRequestSupport{};
    /**
     * Client capabilities specific to regular expressions.
     *
     * @since 3.16.0
     */
    std::optional<RegularExpressionsClientCapabilities> _regularExpressions{};
    /**
     * Client capabilities specific to the client's markdown parser.
     *
     * @since 3.16.0
     */
    std::optional<MarkdownClientCapabilities> _markdown{};
    /**
     * The position encodings supported by the client. Client and server
     * have to agree on the same position encoding to ensure that offsets
     * (e.g. character position in a line) are interpreted the same on both
     * sides.
     *
     * To keep the protocol backwards compatible the following applies: if
     * the value 'utf-16' is missing from the array of position encodings
     * servers can assume that the client supports UTF-16. UTF-16 is
     * therefore a mandatory encoding.
     *
     * If omitted it defaults to ['utf-16'].
     *
     * Implementation considerations: since the conversion from one encoding
     * into another requires the content of the file / line the conversion
     * is best done where the file is read which is usually on the server
     * side.
     *
     * @since 3.17.0
     */
    std::optional<QList<PositionEncodingKind>> _positionEncodings{};

    GeneralClientCapabilities& staleRequestSupport(const std::optional<StaleRequestSupportOptions> & v) { _staleRequestSupport = v; return *this; }
    GeneralClientCapabilities& regularExpressions(const std::optional<RegularExpressionsClientCapabilities> & v) { _regularExpressions = v; return *this; }
    GeneralClientCapabilities& markdown(const std::optional<MarkdownClientCapabilities> & v) { _markdown = v; return *this; }
    GeneralClientCapabilities& positionEncodings(const std::optional<QList<PositionEncodingKind>> & v) { _positionEncodings = v; return *this; }
    GeneralClientCapabilities& addPositionEncoding(const PositionEncodingKind & v) { if (!_positionEncodings) _positionEncodings = QList<PositionEncodingKind>{}; (*_positionEncodings).append(v); return *this; }

    const std::optional<StaleRequestSupportOptions>& staleRequestSupport() const { return _staleRequestSupport; }
    const std::optional<RegularExpressionsClientCapabilities>& regularExpressions() const { return _regularExpressions; }
    const std::optional<MarkdownClientCapabilities>& markdown() const { return _markdown; }
    const std::optional<QList<PositionEncodingKind>>& positionEncodings() const { return _positionEncodings; }

    bool operator==(const GeneralClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<GeneralClientCapabilities> fromJson<GeneralClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const GeneralClientCapabilities &data);

/**
 * Notebook specific client capabilities.
 *
 * @since 3.17.0
 */
struct NotebookDocumentSyncClientCapabilities {
    /**
     * Whether implementation supports dynamic registration. If this is
     * set to `true` the client supports the new
     * `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};
    std::optional<bool> _executionSummarySupport{};  //!< The client supports sending execution summary data per cell.

    NotebookDocumentSyncClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    NotebookDocumentSyncClientCapabilities& executionSummarySupport(std::optional<bool> v) { _executionSummarySupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& executionSummarySupport() const { return _executionSummarySupport; }

    bool operator==(const NotebookDocumentSyncClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentSyncClientCapabilities> fromJson<NotebookDocumentSyncClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentSyncClientCapabilities &data);

/**
 * Capabilities specific to the notebook document support.
 *
 * @since 3.17.0
 */
struct NotebookDocumentClientCapabilities {
    /**
     * Capabilities specific to notebook document synchronization
     *
     * @since 3.17.0
     */
    NotebookDocumentSyncClientCapabilities _synchronization{};

    NotebookDocumentClientCapabilities& synchronization(const NotebookDocumentSyncClientCapabilities & v) { _synchronization = v; return *this; }

    const NotebookDocumentSyncClientCapabilities& synchronization() const { return _synchronization; }

    bool operator==(const NotebookDocumentClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentClientCapabilities> fromJson<NotebookDocumentClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentClientCapabilities &data);

/** @since 3.16.0 */
struct CallHierarchyClientCapabilities {
    /**
     * Whether implementation supports dynamic registration. If this is set to `true`
     * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};

    CallHierarchyClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const CallHierarchyClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyClientCapabilities> fromJson<CallHierarchyClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CallHierarchyClientCapabilities &data);

/** A set of predefined code action kinds */
using CodeActionKind = QString;

namespace CodeActionKinds {
    constexpr char Empty[] = "";
    constexpr char QuickFix[] = "quickfix";
    constexpr char Refactor[] = "refactor";
    constexpr char RefactorExtract[] = "refactor.extract";
    constexpr char RefactorInline[] = "refactor.inline";
    constexpr char RefactorMove[] = "refactor.move";
    constexpr char RefactorRewrite[] = "refactor.rewrite";
    constexpr char Source[] = "source";
    constexpr char SourceOrganizeImports[] = "source.organizeImports";
    constexpr char SourceFixAll[] = "source.fixAll";
    constexpr char Notebook[] = "notebook";
} // namespace CodeActionKinds
/** @since 3.18.0 */
struct ClientCodeActionKindOptions {
    /**
     * The code action kind values the client supports. When this
     * property exists the client also guarantees that it will
     * handle values outside its set gracefully and falls back
     * to a default value when unknown.
     */
    QList<CodeActionKind> _valueSet{};

    ClientCodeActionKindOptions& valueSet(const QList<CodeActionKind> & v) { _valueSet = v; return *this; }
    ClientCodeActionKindOptions& addValueSet(const CodeActionKind & v) { _valueSet.append(v); return *this; }

    const QList<CodeActionKind>& valueSet() const { return _valueSet; }

    bool operator==(const ClientCodeActionKindOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientCodeActionKindOptions> fromJson<ClientCodeActionKindOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientCodeActionKindOptions &data);

/** @since 3.18.0 */
struct ClientCodeActionLiteralOptions {
    /**
     * The code action kind is support with the following value
     * set.
     */
    ClientCodeActionKindOptions _codeActionKind{};

    ClientCodeActionLiteralOptions& codeActionKind(const ClientCodeActionKindOptions & v) { _codeActionKind = v; return *this; }

    const ClientCodeActionKindOptions& codeActionKind() const { return _codeActionKind; }

    bool operator==(const ClientCodeActionLiteralOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientCodeActionLiteralOptions> fromJson<ClientCodeActionLiteralOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientCodeActionLiteralOptions &data);

/** @since 3.18.0 */
struct ClientCodeActionResolveOptions {
    QStringList _properties{};  //!< The properties that a client can resolve lazily.

    ClientCodeActionResolveOptions& properties(const QStringList & v) { _properties = v; return *this; }
    ClientCodeActionResolveOptions& addProperty(const QString & v) { _properties.append(v); return *this; }

    const QStringList& properties() const { return _properties; }

    bool operator==(const ClientCodeActionResolveOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientCodeActionResolveOptions> fromJson<ClientCodeActionResolveOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientCodeActionResolveOptions &data);

/**
 * Code action tags are extra annotations that tweak the behavior of a code action.
 *
 * @since 3.18.0
 */
namespace CodeActionTag {
    constexpr int LLMGenerated = 1;
} // namespace CodeActionTag
/** @since 3.18.0 */
struct CodeActionTagOptions {
    QList<int> _valueSet{};  //!< The tags supported by the client.

    CodeActionTagOptions& valueSet(const QList<int> & v) { _valueSet = v; return *this; }
    CodeActionTagOptions& addValueSet(int v) { _valueSet.append(v); return *this; }

    const QList<int>& valueSet() const { return _valueSet; }

    bool operator==(const CodeActionTagOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeActionTagOptions> fromJson<CodeActionTagOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeActionTagOptions &data);

/** The Client Capabilities of a {@link CodeActionRequest}. */
struct CodeActionClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether code action supports dynamic registration.
    /**
     * The client support code action literals of type `CodeAction` as a valid
     * response of the `textDocument/codeAction` request. If the property is not
     * set the request can only return `Command` literals.
     *
     * @since 3.8.0
     */
    std::optional<ClientCodeActionLiteralOptions> _codeActionLiteralSupport{};
    /**
     * Whether code action supports the `isPreferred` property.
     *
     * @since 3.15.0
     */
    std::optional<bool> _isPreferredSupport{};
    /**
     * Whether code action supports the `disabled` property.
     *
     * @since 3.16.0
     */
    std::optional<bool> _disabledSupport{};
    /**
     * Whether code action supports the `data` property which is
     * preserved between a `textDocument/codeAction` and a
     * `codeAction/resolve` request.
     *
     * @since 3.16.0
     */
    std::optional<bool> _dataSupport{};
    /**
     * Whether the client supports resolving additional code action
     * properties via a separate `codeAction/resolve` request.
     *
     * @since 3.16.0
     */
    std::optional<ClientCodeActionResolveOptions> _resolveSupport{};
    /**
     * Whether the client honors the change annotations in
     * text edits and resource operations returned via the
     * `CodeAction#edit` property by for example presenting
     * the workspace edit in the user interface and asking
     * for confirmation.
     *
     * @since 3.16.0
     */
    std::optional<bool> _honorsChangeAnnotations{};
    /**
     * Whether the client supports documentation for a class of
     * code actions.
     *
     * @since 3.18.0
     */
    std::optional<bool> _documentationSupport{};
    /**
     * Client supports the tag property on a code action. Clients
     * supporting tags have to handle unknown tags gracefully.
     *
     * @since 3.18.0
     */
    std::optional<CodeActionTagOptions> _tagSupport{};

    CodeActionClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    CodeActionClientCapabilities& codeActionLiteralSupport(const std::optional<ClientCodeActionLiteralOptions> & v) { _codeActionLiteralSupport = v; return *this; }
    CodeActionClientCapabilities& isPreferredSupport(std::optional<bool> v) { _isPreferredSupport = v; return *this; }
    CodeActionClientCapabilities& disabledSupport(std::optional<bool> v) { _disabledSupport = v; return *this; }
    CodeActionClientCapabilities& dataSupport(std::optional<bool> v) { _dataSupport = v; return *this; }
    CodeActionClientCapabilities& resolveSupport(const std::optional<ClientCodeActionResolveOptions> & v) { _resolveSupport = v; return *this; }
    CodeActionClientCapabilities& honorsChangeAnnotations(std::optional<bool> v) { _honorsChangeAnnotations = v; return *this; }
    CodeActionClientCapabilities& documentationSupport(std::optional<bool> v) { _documentationSupport = v; return *this; }
    CodeActionClientCapabilities& tagSupport(const std::optional<CodeActionTagOptions> & v) { _tagSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<ClientCodeActionLiteralOptions>& codeActionLiteralSupport() const { return _codeActionLiteralSupport; }
    const std::optional<bool>& isPreferredSupport() const { return _isPreferredSupport; }
    const std::optional<bool>& disabledSupport() const { return _disabledSupport; }
    const std::optional<bool>& dataSupport() const { return _dataSupport; }
    const std::optional<ClientCodeActionResolveOptions>& resolveSupport() const { return _resolveSupport; }
    const std::optional<bool>& honorsChangeAnnotations() const { return _honorsChangeAnnotations; }
    const std::optional<bool>& documentationSupport() const { return _documentationSupport; }
    const std::optional<CodeActionTagOptions>& tagSupport() const { return _tagSupport; }

    bool operator==(const CodeActionClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeActionClientCapabilities> fromJson<CodeActionClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeActionClientCapabilities &data);

/** @since 3.18.0 */
struct ClientCodeLensResolveOptions {
    QStringList _properties{};  //!< The properties that a client can resolve lazily.

    ClientCodeLensResolveOptions& properties(const QStringList & v) { _properties = v; return *this; }
    ClientCodeLensResolveOptions& addProperty(const QString & v) { _properties.append(v); return *this; }

    const QStringList& properties() const { return _properties; }

    bool operator==(const ClientCodeLensResolveOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientCodeLensResolveOptions> fromJson<ClientCodeLensResolveOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientCodeLensResolveOptions &data);

/** The client capabilities  of a {@link CodeLensRequest}. */
struct CodeLensClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether code lens supports dynamic registration.
    /**
     * Whether the client supports resolving additional code lens
     * properties via a separate `codeLens/resolve` request.
     *
     * @since 3.18.0
     */
    std::optional<ClientCodeLensResolveOptions> _resolveSupport{};

    CodeLensClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    CodeLensClientCapabilities& resolveSupport(const std::optional<ClientCodeLensResolveOptions> & v) { _resolveSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<ClientCodeLensResolveOptions>& resolveSupport() const { return _resolveSupport; }

    bool operator==(const CodeLensClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeLensClientCapabilities> fromJson<CodeLensClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeLensClientCapabilities &data);

/**
 * How whitespace and indentation is handled during completion
 * item insertion.
 *
 * @since 3.16.0
 */
namespace InsertTextMode {
    constexpr int asIs = 1;
    constexpr int adjustIndentation = 2;
} // namespace InsertTextMode
/** @since 3.18.0 */
struct ClientCompletionItemInsertTextModeOptions {
    QList<int> _valueSet{};

    ClientCompletionItemInsertTextModeOptions& valueSet(const QList<int> & v) { _valueSet = v; return *this; }
    ClientCompletionItemInsertTextModeOptions& addValueSet(int v) { _valueSet.append(v); return *this; }

    const QList<int>& valueSet() const { return _valueSet; }

    bool operator==(const ClientCompletionItemInsertTextModeOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientCompletionItemInsertTextModeOptions> fromJson<ClientCompletionItemInsertTextModeOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientCompletionItemInsertTextModeOptions &data);

/** @since 3.18.0 */
struct ClientCompletionItemResolveOptions {
    QStringList _properties{};  //!< The properties that a client can resolve lazily.

    ClientCompletionItemResolveOptions& properties(const QStringList & v) { _properties = v; return *this; }
    ClientCompletionItemResolveOptions& addProperty(const QString & v) { _properties.append(v); return *this; }

    const QStringList& properties() const { return _properties; }

    bool operator==(const ClientCompletionItemResolveOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientCompletionItemResolveOptions> fromJson<ClientCompletionItemResolveOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientCompletionItemResolveOptions &data);

/**
 * Completion item tags are extra annotations that tweak the rendering of a completion
 * item.
 *
 * @since 3.15.0
 */
namespace CompletionItemTag {
    constexpr int Deprecated = 1;
} // namespace CompletionItemTag
/** @since 3.18.0 */
struct CompletionItemTagOptions {
    QList<int> _valueSet{};  //!< The tags supported by the client.

    CompletionItemTagOptions& valueSet(const QList<int> & v) { _valueSet = v; return *this; }
    CompletionItemTagOptions& addValueSet(int v) { _valueSet.append(v); return *this; }

    const QList<int>& valueSet() const { return _valueSet; }

    bool operator==(const CompletionItemTagOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionItemTagOptions> fromJson<CompletionItemTagOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionItemTagOptions &data);

/** @since 3.18.0 */
struct ClientCompletionItemOptions {
    /**
     * Client supports snippets as insert text.
     *
     * A snippet can define tab stops and placeholders with `$1`, `$2`
     * and `${3:foo}`. `$0` defines the final tab stop, it defaults to
     * the end of the snippet. Placeholders with equal identifiers are linked,
     * that is typing in one will update others too.
     */
    std::optional<bool> _snippetSupport{};
    std::optional<bool> _commitCharactersSupport{};  //!< Client supports commit characters on a completion item.
    /**
     * Client supports the following content formats for the documentation
     * property. The order describes the preferred format of the client.
     */
    std::optional<QList<MarkupKind>> _documentationFormat{};
    std::optional<bool> _deprecatedSupport{};  //!< Client supports the deprecated property on a completion item.
    std::optional<bool> _preselectSupport{};  //!< Client supports the preselect property on a completion item.
    /**
     * Client supports the tag property on a completion item. Clients supporting
     * tags have to handle unknown tags gracefully. Clients especially need to
     * preserve unknown tags when sending a completion item back to the server in
     * a resolve call.
     *
     * @since 3.15.0
     */
    std::optional<CompletionItemTagOptions> _tagSupport{};
    /**
     * Client support insert replace edit to control different behavior if a
     * completion item is inserted in the text or should replace text.
     *
     * @since 3.16.0
     */
    std::optional<bool> _insertReplaceSupport{};
    /**
     * Indicates which properties a client can resolve lazily on a completion
     * item. Before version 3.16.0 only the predefined properties `documentation`
     * and `details` could be resolved lazily.
     *
     * @since 3.16.0
     */
    std::optional<ClientCompletionItemResolveOptions> _resolveSupport{};
    /**
     * The client supports the `insertTextMode` property on
     * a completion item to override the whitespace handling mode
     * as defined by the client (see `insertTextMode`).
     *
     * @since 3.16.0
     */
    std::optional<ClientCompletionItemInsertTextModeOptions> _insertTextModeSupport{};
    /**
     * The client has support for completion item label
     * details (see also `CompletionItemLabelDetails`).
     *
     * @since 3.17.0
     */
    std::optional<bool> _labelDetailsSupport{};

    ClientCompletionItemOptions& snippetSupport(std::optional<bool> v) { _snippetSupport = v; return *this; }
    ClientCompletionItemOptions& commitCharactersSupport(std::optional<bool> v) { _commitCharactersSupport = v; return *this; }
    ClientCompletionItemOptions& documentationFormat(const std::optional<QList<MarkupKind>> & v) { _documentationFormat = v; return *this; }
    ClientCompletionItemOptions& addDocumentationFormat(const MarkupKind & v) { if (!_documentationFormat) _documentationFormat = QList<MarkupKind>{}; (*_documentationFormat).append(v); return *this; }
    ClientCompletionItemOptions& deprecatedSupport(std::optional<bool> v) { _deprecatedSupport = v; return *this; }
    ClientCompletionItemOptions& preselectSupport(std::optional<bool> v) { _preselectSupport = v; return *this; }
    ClientCompletionItemOptions& tagSupport(const std::optional<CompletionItemTagOptions> & v) { _tagSupport = v; return *this; }
    ClientCompletionItemOptions& insertReplaceSupport(std::optional<bool> v) { _insertReplaceSupport = v; return *this; }
    ClientCompletionItemOptions& resolveSupport(const std::optional<ClientCompletionItemResolveOptions> & v) { _resolveSupport = v; return *this; }
    ClientCompletionItemOptions& insertTextModeSupport(const std::optional<ClientCompletionItemInsertTextModeOptions> & v) { _insertTextModeSupport = v; return *this; }
    ClientCompletionItemOptions& labelDetailsSupport(std::optional<bool> v) { _labelDetailsSupport = v; return *this; }

    const std::optional<bool>& snippetSupport() const { return _snippetSupport; }
    const std::optional<bool>& commitCharactersSupport() const { return _commitCharactersSupport; }
    const std::optional<QList<MarkupKind>>& documentationFormat() const { return _documentationFormat; }
    const std::optional<bool>& deprecatedSupport() const { return _deprecatedSupport; }
    const std::optional<bool>& preselectSupport() const { return _preselectSupport; }
    const std::optional<CompletionItemTagOptions>& tagSupport() const { return _tagSupport; }
    const std::optional<bool>& insertReplaceSupport() const { return _insertReplaceSupport; }
    const std::optional<ClientCompletionItemResolveOptions>& resolveSupport() const { return _resolveSupport; }
    const std::optional<ClientCompletionItemInsertTextModeOptions>& insertTextModeSupport() const { return _insertTextModeSupport; }
    const std::optional<bool>& labelDetailsSupport() const { return _labelDetailsSupport; }

    bool operator==(const ClientCompletionItemOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientCompletionItemOptions> fromJson<ClientCompletionItemOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientCompletionItemOptions &data);

/** The kind of a completion entry. */
namespace CompletionItemKind {
    constexpr int Text = 1;
    constexpr int Method = 2;
    constexpr int Function = 3;
    constexpr int Constructor = 4;
    constexpr int Field = 5;
    constexpr int Variable = 6;
    constexpr int Class = 7;
    constexpr int Interface = 8;
    constexpr int Module = 9;
    constexpr int Property = 10;
    constexpr int Unit = 11;
    constexpr int Value = 12;
    constexpr int Enum = 13;
    constexpr int Keyword = 14;
    constexpr int Snippet = 15;
    constexpr int Color = 16;
    constexpr int File = 17;
    constexpr int Reference = 18;
    constexpr int Folder = 19;
    constexpr int EnumMember = 20;
    constexpr int Constant = 21;
    constexpr int Struct = 22;
    constexpr int Event = 23;
    constexpr int Operator = 24;
    constexpr int TypeParameter = 25;
} // namespace CompletionItemKind
/** @since 3.18.0 */
struct ClientCompletionItemOptionsKind {
    /**
     * The completion item kind values the client supports. When this
     * property exists the client also guarantees that it will
     * handle values outside its set gracefully and falls back
     * to a default value when unknown.
     *
     * If this property is not present the client only supports
     * the completion items kinds from `Text` to `Reference` as defined in
     * the initial version of the protocol.
     */
    std::optional<QList<int>> _valueSet{};

    ClientCompletionItemOptionsKind& valueSet(const std::optional<QList<int>> & v) { _valueSet = v; return *this; }
    ClientCompletionItemOptionsKind& addValueSet(int v) { if (!_valueSet) _valueSet = QList<int>{}; (*_valueSet).append(v); return *this; }

    const std::optional<QList<int>>& valueSet() const { return _valueSet; }

    bool operator==(const ClientCompletionItemOptionsKind &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientCompletionItemOptionsKind> fromJson<ClientCompletionItemOptionsKind>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientCompletionItemOptionsKind &data);

/**
 * The client supports the following `CompletionList` specific
 * capabilities.
 *
 * @since 3.17.0
 */
struct CompletionListCapabilities {
    /**
     * The client supports the following itemDefaults on
     * a completion list.
     *
     * The value lists the supported property names of the
     * `CompletionList.itemDefaults` object. If omitted
     * no properties are supported.
     *
     * @since 3.17.0
     */
    std::optional<QStringList> _itemDefaults{};
    /**
     * Specifies whether the client supports `CompletionList.applyKind` to
     * indicate how supported values from `completionList.itemDefaults`
     * and `completion` will be combined.
     *
     * If a client supports `applyKind` it must support it for all fields
     * that it supports that are listed in `CompletionList.applyKind`. This
     * means when clients add support for new/future fields in completion
     * items the MUST also support merge for them if those fields are
     * defined in `CompletionList.applyKind`.
     *
     * @since 3.18.0
     */
    std::optional<bool> _applyKindSupport{};

    CompletionListCapabilities& itemDefaults(const std::optional<QStringList> & v) { _itemDefaults = v; return *this; }
    CompletionListCapabilities& addItemDefault(const QString & v) { if (!_itemDefaults) _itemDefaults = QStringList{}; (*_itemDefaults).append(v); return *this; }
    CompletionListCapabilities& applyKindSupport(std::optional<bool> v) { _applyKindSupport = v; return *this; }

    const std::optional<QStringList>& itemDefaults() const { return _itemDefaults; }
    const std::optional<bool>& applyKindSupport() const { return _applyKindSupport; }

    bool operator==(const CompletionListCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionListCapabilities> fromJson<CompletionListCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionListCapabilities &data);

/** Completion client capabilities */
struct CompletionClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether completion supports dynamic registration.
    /**
     * The client supports the following `CompletionItem` specific
     * capabilities.
     */
    std::optional<ClientCompletionItemOptions> _completionItem{};
    std::optional<ClientCompletionItemOptionsKind> _completionItemKind{};  //!< The client supports the following completion item kinds.
    /**
     * Defines how the client handles whitespace and indentation
     * when accepting a completion item that uses multi line
     * text in either `insertText` or `textEdit`.
     *
     * @since 3.17.0
     */
    std::optional<int> _insertTextMode{};
    /**
     * The client supports to send additional context information for a
     * `textDocument/completion` request.
     */
    std::optional<bool> _contextSupport{};
    /**
     * The client supports the following `CompletionList` specific
     * capabilities.
     *
     * @since 3.17.0
     */
    std::optional<CompletionListCapabilities> _completionList{};

    CompletionClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    CompletionClientCapabilities& completionItem(const std::optional<ClientCompletionItemOptions> & v) { _completionItem = v; return *this; }
    CompletionClientCapabilities& completionItemKind(const std::optional<ClientCompletionItemOptionsKind> & v) { _completionItemKind = v; return *this; }
    CompletionClientCapabilities& insertTextMode(std::optional<int> v) { _insertTextMode = v; return *this; }
    CompletionClientCapabilities& contextSupport(std::optional<bool> v) { _contextSupport = v; return *this; }
    CompletionClientCapabilities& completionList(const std::optional<CompletionListCapabilities> & v) { _completionList = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<ClientCompletionItemOptions>& completionItem() const { return _completionItem; }
    const std::optional<ClientCompletionItemOptionsKind>& completionItemKind() const { return _completionItemKind; }
    const std::optional<int>& insertTextMode() const { return _insertTextMode; }
    const std::optional<bool>& contextSupport() const { return _contextSupport; }
    const std::optional<CompletionListCapabilities>& completionList() const { return _completionList; }

    bool operator==(const CompletionClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionClientCapabilities> fromJson<CompletionClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionClientCapabilities &data);

/** @since 3.14.0 */
struct DeclarationClientCapabilities {
    /**
     * Whether declaration supports dynamic registration. If this is set to `true`
     * the client supports the new `DeclarationRegistrationOptions` return value
     * for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};
    std::optional<bool> _linkSupport{};  //!< The client supports additional metadata in the form of declaration links.

    DeclarationClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    DeclarationClientCapabilities& linkSupport(std::optional<bool> v) { _linkSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& linkSupport() const { return _linkSupport; }

    bool operator==(const DeclarationClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DeclarationClientCapabilities> fromJson<DeclarationClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DeclarationClientCapabilities &data);

/** Client Capabilities for a {@link DefinitionRequest}. */
struct DefinitionClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether definition supports dynamic registration.
    /**
     * The client supports additional metadata in the form of definition links.
     *
     * @since 3.14.0
     */
    std::optional<bool> _linkSupport{};

    DefinitionClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    DefinitionClientCapabilities& linkSupport(std::optional<bool> v) { _linkSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& linkSupport() const { return _linkSupport; }

    bool operator==(const DefinitionClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DefinitionClientCapabilities> fromJson<DefinitionClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DefinitionClientCapabilities &data);

/** @since 3.18.0 */
struct ClientDiagnosticsTagOptions {
    QList<int> _valueSet{};  //!< The tags supported by the client.

    ClientDiagnosticsTagOptions& valueSet(const QList<int> & v) { _valueSet = v; return *this; }
    ClientDiagnosticsTagOptions& addValueSet(int v) { _valueSet.append(v); return *this; }

    const QList<int>& valueSet() const { return _valueSet; }

    bool operator==(const ClientDiagnosticsTagOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientDiagnosticsTagOptions> fromJson<ClientDiagnosticsTagOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientDiagnosticsTagOptions &data);

/**
 * Client capabilities specific to diagnostic pull requests.
 *
 * @since 3.17.0
 */
struct DiagnosticClientCapabilities {
    std::optional<bool> _relatedInformation{};  //!< Whether the clients accepts diagnostics with related information.
    /**
     * Client supports the tag property to provide meta data about a diagnostic.
     * Clients supporting tags have to handle unknown tags gracefully.
     *
     * @since 3.15.0
     */
    std::optional<ClientDiagnosticsTagOptions> _tagSupport{};
    /**
     * Client supports a codeDescription property
     *
     * @since 3.16.0
     */
    std::optional<bool> _codeDescriptionSupport{};
    /**
     * Whether code action supports the `data` property which is
     * preserved between a `textDocument/publishDiagnostics` and
     * `textDocument/codeAction` request.
     *
     * @since 3.16.0
     */
    std::optional<bool> _dataSupport{};
    /**
     * Whether implementation supports dynamic registration. If this is set to `true`
     * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};
    std::optional<bool> _relatedDocumentSupport{};  //!< Whether the clients supports related documents for document diagnostic pulls.
    /**
     * Whether the client supports `MarkupContent` in diagnostic messages.
     *
     * @since 3.18.0
     */
    std::optional<bool> _markupMessageSupport{};

    DiagnosticClientCapabilities& relatedInformation(std::optional<bool> v) { _relatedInformation = v; return *this; }
    DiagnosticClientCapabilities& tagSupport(const std::optional<ClientDiagnosticsTagOptions> & v) { _tagSupport = v; return *this; }
    DiagnosticClientCapabilities& codeDescriptionSupport(std::optional<bool> v) { _codeDescriptionSupport = v; return *this; }
    DiagnosticClientCapabilities& dataSupport(std::optional<bool> v) { _dataSupport = v; return *this; }
    DiagnosticClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    DiagnosticClientCapabilities& relatedDocumentSupport(std::optional<bool> v) { _relatedDocumentSupport = v; return *this; }
    DiagnosticClientCapabilities& markupMessageSupport(std::optional<bool> v) { _markupMessageSupport = v; return *this; }

    const std::optional<bool>& relatedInformation() const { return _relatedInformation; }
    const std::optional<ClientDiagnosticsTagOptions>& tagSupport() const { return _tagSupport; }
    const std::optional<bool>& codeDescriptionSupport() const { return _codeDescriptionSupport; }
    const std::optional<bool>& dataSupport() const { return _dataSupport; }
    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& relatedDocumentSupport() const { return _relatedDocumentSupport; }
    const std::optional<bool>& markupMessageSupport() const { return _markupMessageSupport; }

    bool operator==(const DiagnosticClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DiagnosticClientCapabilities> fromJson<DiagnosticClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DiagnosticClientCapabilities &data);

struct DocumentColorClientCapabilities {
    /**
     * Whether implementation supports dynamic registration. If this is set to `true`
     * the client supports the new `DocumentColorRegistrationOptions` return value
     * for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};

    DocumentColorClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const DocumentColorClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentColorClientCapabilities> fromJson<DocumentColorClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentColorClientCapabilities &data);

/** Client capabilities of a {@link DocumentFormattingRequest}. */
struct DocumentFormattingClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether formatting supports dynamic registration.

    DocumentFormattingClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const DocumentFormattingClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentFormattingClientCapabilities> fromJson<DocumentFormattingClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentFormattingClientCapabilities &data);

/** Client Capabilities for a {@link DocumentHighlightRequest}. */
struct DocumentHighlightClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether document highlight supports dynamic registration.

    DocumentHighlightClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const DocumentHighlightClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentHighlightClientCapabilities> fromJson<DocumentHighlightClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentHighlightClientCapabilities &data);

/** The client capabilities of a {@link DocumentLinkRequest}. */
struct DocumentLinkClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether document link supports dynamic registration.
    /**
     * Whether the client supports the `tooltip` property on `DocumentLink`.
     *
     * @since 3.15.0
     */
    std::optional<bool> _tooltipSupport{};

    DocumentLinkClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    DocumentLinkClientCapabilities& tooltipSupport(std::optional<bool> v) { _tooltipSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& tooltipSupport() const { return _tooltipSupport; }

    bool operator==(const DocumentLinkClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentLinkClientCapabilities> fromJson<DocumentLinkClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentLinkClientCapabilities &data);

/** Client capabilities of a {@link DocumentOnTypeFormattingRequest}. */
struct DocumentOnTypeFormattingClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether on type formatting supports dynamic registration.

    DocumentOnTypeFormattingClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const DocumentOnTypeFormattingClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentOnTypeFormattingClientCapabilities> fromJson<DocumentOnTypeFormattingClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentOnTypeFormattingClientCapabilities &data);

/** Client capabilities of a {@link DocumentRangeFormattingRequest}. */
struct DocumentRangeFormattingClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether range formatting supports dynamic registration.
    /**
     * Whether the client supports formatting multiple ranges at once.
     *
     * @since 3.18.0
     */
    std::optional<bool> _rangesSupport{};

    DocumentRangeFormattingClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    DocumentRangeFormattingClientCapabilities& rangesSupport(std::optional<bool> v) { _rangesSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& rangesSupport() const { return _rangesSupport; }

    bool operator==(const DocumentRangeFormattingClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentRangeFormattingClientCapabilities> fromJson<DocumentRangeFormattingClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentRangeFormattingClientCapabilities &data);

/** @since 3.18.0 */
struct ClientSymbolKindOptions {
    /**
     * The symbol kind values the client supports. When this
     * property exists the client also guarantees that it will
     * handle values outside its set gracefully and falls back
     * to a default value when unknown.
     *
     * If this property is not present the client only supports
     * the symbol kinds from `File` to `Array` as defined in
     * the initial version of the protocol.
     */
    std::optional<QList<int>> _valueSet{};

    ClientSymbolKindOptions& valueSet(const std::optional<QList<int>> & v) { _valueSet = v; return *this; }
    ClientSymbolKindOptions& addValueSet(int v) { if (!_valueSet) _valueSet = QList<int>{}; (*_valueSet).append(v); return *this; }

    const std::optional<QList<int>>& valueSet() const { return _valueSet; }

    bool operator==(const ClientSymbolKindOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientSymbolKindOptions> fromJson<ClientSymbolKindOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientSymbolKindOptions &data);

/** @since 3.18.0 */
struct ClientSymbolTagOptions {
    QList<int> _valueSet{};  //!< The tags supported by the client.

    ClientSymbolTagOptions& valueSet(const QList<int> & v) { _valueSet = v; return *this; }
    ClientSymbolTagOptions& addValueSet(int v) { _valueSet.append(v); return *this; }

    const QList<int>& valueSet() const { return _valueSet; }

    bool operator==(const ClientSymbolTagOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientSymbolTagOptions> fromJson<ClientSymbolTagOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientSymbolTagOptions &data);

/** Client Capabilities for a {@link DocumentSymbolRequest}. */
struct DocumentSymbolClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether document symbol supports dynamic registration.
    /**
     * Specific capabilities for the `SymbolKind` in the
     * `textDocument/documentSymbol` request.
     */
    std::optional<ClientSymbolKindOptions> _symbolKind{};
    std::optional<bool> _hierarchicalDocumentSymbolSupport{};  //!< The client supports hierarchical document symbols.
    /**
     * The client supports tags on `SymbolInformation`. Tags are supported on
     * `DocumentSymbol` if `hierarchicalDocumentSymbolSupport` is set to true.
     * Clients supporting tags have to handle unknown tags gracefully.
     *
     * @since 3.16.0
     */
    std::optional<ClientSymbolTagOptions> _tagSupport{};
    /**
     * The client supports an additional label presented in the UI when
     * registering a document symbol provider.
     *
     * @since 3.16.0
     */
    std::optional<bool> _labelSupport{};

    DocumentSymbolClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    DocumentSymbolClientCapabilities& symbolKind(const std::optional<ClientSymbolKindOptions> & v) { _symbolKind = v; return *this; }
    DocumentSymbolClientCapabilities& hierarchicalDocumentSymbolSupport(std::optional<bool> v) { _hierarchicalDocumentSymbolSupport = v; return *this; }
    DocumentSymbolClientCapabilities& tagSupport(const std::optional<ClientSymbolTagOptions> & v) { _tagSupport = v; return *this; }
    DocumentSymbolClientCapabilities& labelSupport(std::optional<bool> v) { _labelSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<ClientSymbolKindOptions>& symbolKind() const { return _symbolKind; }
    const std::optional<bool>& hierarchicalDocumentSymbolSupport() const { return _hierarchicalDocumentSymbolSupport; }
    const std::optional<ClientSymbolTagOptions>& tagSupport() const { return _tagSupport; }
    const std::optional<bool>& labelSupport() const { return _labelSupport; }

    bool operator==(const DocumentSymbolClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentSymbolClientCapabilities> fromJson<DocumentSymbolClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentSymbolClientCapabilities &data);

/** @since 3.18.0 */
struct ClientFoldingRangeKindOptions {
    /**
     * The folding range kind values the client supports. When this
     * property exists the client also guarantees that it will
     * handle values outside its set gracefully and falls back
     * to a default value when unknown.
     */
    std::optional<QList<FoldingRangeKind>> _valueSet{};

    ClientFoldingRangeKindOptions& valueSet(const std::optional<QList<FoldingRangeKind>> & v) { _valueSet = v; return *this; }
    ClientFoldingRangeKindOptions& addValueSet(const FoldingRangeKind & v) { if (!_valueSet) _valueSet = QList<FoldingRangeKind>{}; (*_valueSet).append(v); return *this; }

    const std::optional<QList<FoldingRangeKind>>& valueSet() const { return _valueSet; }

    bool operator==(const ClientFoldingRangeKindOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientFoldingRangeKindOptions> fromJson<ClientFoldingRangeKindOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientFoldingRangeKindOptions &data);

/** @since 3.18.0 */
struct ClientFoldingRangeOptions {
    /**
     * If set, the client signals that it supports setting collapsedText on
     * folding ranges to display custom labels instead of the default text.
     *
     * @since 3.17.0
     */
    std::optional<bool> _collapsedText{};

    ClientFoldingRangeOptions& collapsedText(std::optional<bool> v) { _collapsedText = v; return *this; }

    const std::optional<bool>& collapsedText() const { return _collapsedText; }

    bool operator==(const ClientFoldingRangeOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientFoldingRangeOptions> fromJson<ClientFoldingRangeOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientFoldingRangeOptions &data);

struct FoldingRangeClientCapabilities {
    /**
     * Whether implementation supports dynamic registration for folding range
     * providers. If this is set to `true` the client supports the new
     * `FoldingRangeRegistrationOptions` return value for the corresponding
     * server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};
    /**
     * The maximum number of folding ranges that the client prefers to receive
     * per document. The value serves as a hint, servers are free to follow the
     * limit.
     */
    std::optional<int> _rangeLimit{};
    /**
     * If set, the client signals that it only supports folding complete lines.
     * If set, client will ignore specified `startCharacter` and `endCharacter`
     * properties in a FoldingRange.
     */
    std::optional<bool> _lineFoldingOnly{};
    /**
     * Specific options for the folding range kind.
     *
     * @since 3.17.0
     */
    std::optional<ClientFoldingRangeKindOptions> _foldingRangeKind{};
    /**
     * Specific options for the folding range.
     *
     * @since 3.17.0
     */
    std::optional<ClientFoldingRangeOptions> _foldingRange{};

    FoldingRangeClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    FoldingRangeClientCapabilities& rangeLimit(std::optional<int> v) { _rangeLimit = v; return *this; }
    FoldingRangeClientCapabilities& lineFoldingOnly(std::optional<bool> v) { _lineFoldingOnly = v; return *this; }
    FoldingRangeClientCapabilities& foldingRangeKind(const std::optional<ClientFoldingRangeKindOptions> & v) { _foldingRangeKind = v; return *this; }
    FoldingRangeClientCapabilities& foldingRange(const std::optional<ClientFoldingRangeOptions> & v) { _foldingRange = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<int>& rangeLimit() const { return _rangeLimit; }
    const std::optional<bool>& lineFoldingOnly() const { return _lineFoldingOnly; }
    const std::optional<ClientFoldingRangeKindOptions>& foldingRangeKind() const { return _foldingRangeKind; }
    const std::optional<ClientFoldingRangeOptions>& foldingRange() const { return _foldingRange; }

    bool operator==(const FoldingRangeClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FoldingRangeClientCapabilities> fromJson<FoldingRangeClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FoldingRangeClientCapabilities &data);

struct HoverClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether hover supports dynamic registration.
    /**
     * Client supports the following content formats for the content
     * property. The order describes the preferred format of the client.
     */
    std::optional<QList<MarkupKind>> _contentFormat{};

    HoverClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    HoverClientCapabilities& contentFormat(const std::optional<QList<MarkupKind>> & v) { _contentFormat = v; return *this; }
    HoverClientCapabilities& addContentFormat(const MarkupKind & v) { if (!_contentFormat) _contentFormat = QList<MarkupKind>{}; (*_contentFormat).append(v); return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<QList<MarkupKind>>& contentFormat() const { return _contentFormat; }

    bool operator==(const HoverClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<HoverClientCapabilities> fromJson<HoverClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const HoverClientCapabilities &data);

/** @since 3.6.0 */
struct ImplementationClientCapabilities {
    /**
     * Whether implementation supports dynamic registration. If this is set to `true`
     * the client supports the new `ImplementationRegistrationOptions` return value
     * for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};
    /**
     * The client supports additional metadata in the form of definition links.
     *
     * @since 3.14.0
     */
    std::optional<bool> _linkSupport{};

    ImplementationClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    ImplementationClientCapabilities& linkSupport(std::optional<bool> v) { _linkSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& linkSupport() const { return _linkSupport; }

    bool operator==(const ImplementationClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ImplementationClientCapabilities> fromJson<ImplementationClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ImplementationClientCapabilities &data);

/** @since 3.18.0 */
struct ClientInlayHintResolveOptions {
    QStringList _properties{};  //!< The properties that a client can resolve lazily.

    ClientInlayHintResolveOptions& properties(const QStringList & v) { _properties = v; return *this; }
    ClientInlayHintResolveOptions& addProperty(const QString & v) { _properties.append(v); return *this; }

    const QStringList& properties() const { return _properties; }

    bool operator==(const ClientInlayHintResolveOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientInlayHintResolveOptions> fromJson<ClientInlayHintResolveOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientInlayHintResolveOptions &data);

/**
 * Inlay hint client capabilities.
 *
 * @since 3.17.0
 */
struct InlayHintClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether inlay hints support dynamic registration.
    /**
     * Indicates which properties a client can resolve lazily on an inlay
     * hint.
     */
    std::optional<ClientInlayHintResolveOptions> _resolveSupport{};

    InlayHintClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    InlayHintClientCapabilities& resolveSupport(const std::optional<ClientInlayHintResolveOptions> & v) { _resolveSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<ClientInlayHintResolveOptions>& resolveSupport() const { return _resolveSupport; }

    bool operator==(const InlayHintClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHintClientCapabilities> fromJson<InlayHintClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlayHintClientCapabilities &data);

/**
 * Client capabilities specific to inline completions.
 *
 * @since 3.18.0
 */
struct InlineCompletionClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether implementation supports dynamic registration for inline completion providers.

    InlineCompletionClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const InlineCompletionClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineCompletionClientCapabilities> fromJson<InlineCompletionClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineCompletionClientCapabilities &data);

/**
 * Client capabilities specific to inline values.
 *
 * @since 3.17.0
 */
struct InlineValueClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether implementation supports dynamic registration for inline value providers.

    InlineValueClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const InlineValueClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueClientCapabilities> fromJson<InlineValueClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineValueClientCapabilities &data);

/**
 * Client capabilities for the linked editing range request.
 *
 * @since 3.16.0
 */
struct LinkedEditingRangeClientCapabilities {
    /**
     * Whether implementation supports dynamic registration. If this is set to `true`
     * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};

    LinkedEditingRangeClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const LinkedEditingRangeClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<LinkedEditingRangeClientCapabilities> fromJson<LinkedEditingRangeClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const LinkedEditingRangeClientCapabilities &data);

/**
 * Client capabilities specific to the moniker request.
 *
 * @since 3.16.0
 */
struct MonikerClientCapabilities {
    /**
     * Whether moniker supports dynamic registration. If this is set to `true`
     * the client supports the new `MonikerRegistrationOptions` return value
     * for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};

    MonikerClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const MonikerClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MonikerClientCapabilities> fromJson<MonikerClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const MonikerClientCapabilities &data);

/** The publish diagnostic client capabilities. */
struct PublishDiagnosticsClientCapabilities {
    std::optional<bool> _relatedInformation{};  //!< Whether the clients accepts diagnostics with related information.
    /**
     * Client supports the tag property to provide meta data about a diagnostic.
     * Clients supporting tags have to handle unknown tags gracefully.
     *
     * @since 3.15.0
     */
    std::optional<ClientDiagnosticsTagOptions> _tagSupport{};
    /**
     * Client supports a codeDescription property
     *
     * @since 3.16.0
     */
    std::optional<bool> _codeDescriptionSupport{};
    /**
     * Whether code action supports the `data` property which is
     * preserved between a `textDocument/publishDiagnostics` and
     * `textDocument/codeAction` request.
     *
     * @since 3.16.0
     */
    std::optional<bool> _dataSupport{};
    /**
     * Whether the client interprets the version property of the
     * `textDocument/publishDiagnostics` notification's parameter.
     *
     * @since 3.15.0
     */
    std::optional<bool> _versionSupport{};

    PublishDiagnosticsClientCapabilities& relatedInformation(std::optional<bool> v) { _relatedInformation = v; return *this; }
    PublishDiagnosticsClientCapabilities& tagSupport(const std::optional<ClientDiagnosticsTagOptions> & v) { _tagSupport = v; return *this; }
    PublishDiagnosticsClientCapabilities& codeDescriptionSupport(std::optional<bool> v) { _codeDescriptionSupport = v; return *this; }
    PublishDiagnosticsClientCapabilities& dataSupport(std::optional<bool> v) { _dataSupport = v; return *this; }
    PublishDiagnosticsClientCapabilities& versionSupport(std::optional<bool> v) { _versionSupport = v; return *this; }

    const std::optional<bool>& relatedInformation() const { return _relatedInformation; }
    const std::optional<ClientDiagnosticsTagOptions>& tagSupport() const { return _tagSupport; }
    const std::optional<bool>& codeDescriptionSupport() const { return _codeDescriptionSupport; }
    const std::optional<bool>& dataSupport() const { return _dataSupport; }
    const std::optional<bool>& versionSupport() const { return _versionSupport; }

    bool operator==(const PublishDiagnosticsClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<PublishDiagnosticsClientCapabilities> fromJson<PublishDiagnosticsClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const PublishDiagnosticsClientCapabilities &data);

/** Client Capabilities for a {@link ReferencesRequest}. */
struct ReferenceClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether references supports dynamic registration.

    ReferenceClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const ReferenceClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ReferenceClientCapabilities> fromJson<ReferenceClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ReferenceClientCapabilities &data);

namespace PrepareSupportDefaultBehavior {
    constexpr int Identifier = 1;
} // namespace PrepareSupportDefaultBehavior
struct RenameClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether rename supports dynamic registration.
    /**
     * Client supports testing for validity of rename operations
     * before execution.
     *
     * @since 3.12.0
     */
    std::optional<bool> _prepareSupport{};
    /**
     * Client supports the default behavior result.
     *
     * The value indicates the default behavior used by the
     * client.
     *
     * @since 3.16.0
     */
    std::optional<int> _prepareSupportDefaultBehavior{};
    /**
     * Whether the client honors the change annotations in
     * text edits and resource operations returned via the
     * rename request's workspace edit by for example presenting
     * the workspace edit in the user interface and asking
     * for confirmation.
     *
     * @since 3.16.0
     */
    std::optional<bool> _honorsChangeAnnotations{};

    RenameClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    RenameClientCapabilities& prepareSupport(std::optional<bool> v) { _prepareSupport = v; return *this; }
    RenameClientCapabilities& prepareSupportDefaultBehavior(std::optional<int> v) { _prepareSupportDefaultBehavior = v; return *this; }
    RenameClientCapabilities& honorsChangeAnnotations(std::optional<bool> v) { _honorsChangeAnnotations = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& prepareSupport() const { return _prepareSupport; }
    const std::optional<int>& prepareSupportDefaultBehavior() const { return _prepareSupportDefaultBehavior; }
    const std::optional<bool>& honorsChangeAnnotations() const { return _honorsChangeAnnotations; }

    bool operator==(const RenameClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RenameClientCapabilities> fromJson<RenameClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RenameClientCapabilities &data);

struct SelectionRangeClientCapabilities {
    /**
     * Whether implementation supports dynamic registration for selection range providers. If this is set to `true`
     * the client supports the new `SelectionRangeRegistrationOptions` return value for the corresponding server
     * capability as well.
     */
    std::optional<bool> _dynamicRegistration{};

    SelectionRangeClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const SelectionRangeClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SelectionRangeClientCapabilities> fromJson<SelectionRangeClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SelectionRangeClientCapabilities &data);

/** @since 3.18.0 */
struct ClientSemanticTokensRequestFullDelta {
    /**
     * The client will send the `textDocument/semanticTokens/full/delta` request if
     * the server provides a corresponding handler.
     */
    std::optional<bool> _delta{};

    ClientSemanticTokensRequestFullDelta& delta(std::optional<bool> v) { _delta = v; return *this; }

    const std::optional<bool>& delta() const { return _delta; }

    bool operator==(const ClientSemanticTokensRequestFullDelta &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientSemanticTokensRequestFullDelta> fromJson<ClientSemanticTokensRequestFullDelta>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientSemanticTokensRequestFullDelta &data);

using ClientSemanticTokensRequestOptionsFull = std::variant<bool, ClientSemanticTokensRequestFullDelta>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientSemanticTokensRequestOptionsFull> fromJson<ClientSemanticTokensRequestOptionsFull>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ClientSemanticTokensRequestOptionsFull &val);

/** @since 3.18.0 */
struct ClientSemanticTokensRequestOptions {
    /**
     * The client will send the `textDocument/semanticTokens/range` request if
     * the server provides a corresponding handler.
     */
    std::optional<SemanticTokensRegistrationOptionsRange> _range{};
    /**
     * The client will send the `textDocument/semanticTokens/full` request if
     * the server provides a corresponding handler.
     */
    std::optional<ClientSemanticTokensRequestOptionsFull> _full{};

    ClientSemanticTokensRequestOptions& range(const std::optional<SemanticTokensRegistrationOptionsRange> & v) { _range = v; return *this; }
    ClientSemanticTokensRequestOptions& full(const std::optional<ClientSemanticTokensRequestOptionsFull> & v) { _full = v; return *this; }

    const std::optional<SemanticTokensRegistrationOptionsRange>& range() const { return _range; }
    const std::optional<ClientSemanticTokensRequestOptionsFull>& full() const { return _full; }

    bool operator==(const ClientSemanticTokensRequestOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientSemanticTokensRequestOptions> fromJson<ClientSemanticTokensRequestOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientSemanticTokensRequestOptions &data);

enum class TokenFormat {
    relative
};

LANGUAGESERVERPROTOCOL_EXPORT QString toString(TokenFormat v);

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TokenFormat> fromJson<TokenFormat>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const TokenFormat &v);

/** @since 3.16.0 */
struct SemanticTokensClientCapabilities {
    /**
     * Whether implementation supports dynamic registration. If this is set to `true`
     * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};
    /**
     * Which requests the client supports and might send to the server
     * depending on the server's capability. Please note that clients might not
     * show semantic tokens or degrade some of the user experience if a range
     * or full request is advertised by the client but not provided by the
     * server. If for example the client capability `requests.full` and
     * `request.range` are both set to true but the server only provides a
     * range provider the client might not render a minimap correctly or might
     * even decide to not show any semantic tokens at all.
     */
    ClientSemanticTokensRequestOptions _requests{};
    QStringList _tokenTypes{};  //!< The token types that the client supports.
    QStringList _tokenModifiers{};  //!< The token modifiers that the client supports.
    QList<TokenFormat> _formats{};  //!< The token formats the clients supports.
    std::optional<bool> _overlappingTokenSupport{};  //!< Whether the client supports tokens that can overlap each other.
    std::optional<bool> _multilineTokenSupport{};  //!< Whether the client supports tokens that can span multiple lines.
    /**
     * Whether the client allows the server to actively cancel a
     * semantic token request, e.g. supports returning
     * LSPErrorCodes.ServerCancelled. If a server does the client
     * needs to retrigger the request.
     *
     * @since 3.17.0
     */
    std::optional<bool> _serverCancelSupport{};
    /**
     * Whether the client uses semantic tokens to augment existing
     * syntax tokens. If set to `true` client side created syntax
     * tokens and semantic tokens are both used for colorization. If
     * set to `false` the client only uses the returned semantic tokens
     * for colorization.
     *
     * If the value is `undefined` then the client behavior is not
     * specified.
     *
     * @since 3.17.0
     */
    std::optional<bool> _augmentsSyntaxTokens{};

    SemanticTokensClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    SemanticTokensClientCapabilities& requests(const ClientSemanticTokensRequestOptions & v) { _requests = v; return *this; }
    SemanticTokensClientCapabilities& tokenTypes(const QStringList & v) { _tokenTypes = v; return *this; }
    SemanticTokensClientCapabilities& addTokenType(const QString & v) { _tokenTypes.append(v); return *this; }
    SemanticTokensClientCapabilities& tokenModifiers(const QStringList & v) { _tokenModifiers = v; return *this; }
    SemanticTokensClientCapabilities& addTokenModifier(const QString & v) { _tokenModifiers.append(v); return *this; }
    SemanticTokensClientCapabilities& formats(const QList<TokenFormat> & v) { _formats = v; return *this; }
    SemanticTokensClientCapabilities& addFormat(const TokenFormat & v) { _formats.append(v); return *this; }
    SemanticTokensClientCapabilities& overlappingTokenSupport(std::optional<bool> v) { _overlappingTokenSupport = v; return *this; }
    SemanticTokensClientCapabilities& multilineTokenSupport(std::optional<bool> v) { _multilineTokenSupport = v; return *this; }
    SemanticTokensClientCapabilities& serverCancelSupport(std::optional<bool> v) { _serverCancelSupport = v; return *this; }
    SemanticTokensClientCapabilities& augmentsSyntaxTokens(std::optional<bool> v) { _augmentsSyntaxTokens = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const ClientSemanticTokensRequestOptions& requests() const { return _requests; }
    const QStringList& tokenTypes() const { return _tokenTypes; }
    const QStringList& tokenModifiers() const { return _tokenModifiers; }
    const QList<TokenFormat>& formats() const { return _formats; }
    const std::optional<bool>& overlappingTokenSupport() const { return _overlappingTokenSupport; }
    const std::optional<bool>& multilineTokenSupport() const { return _multilineTokenSupport; }
    const std::optional<bool>& serverCancelSupport() const { return _serverCancelSupport; }
    const std::optional<bool>& augmentsSyntaxTokens() const { return _augmentsSyntaxTokens; }

    bool operator==(const SemanticTokensClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensClientCapabilities> fromJson<SemanticTokensClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensClientCapabilities &data);

/** @since 3.18.0 */
struct ClientSignatureParameterInformationOptions {
    /**
     * The client supports processing label offsets instead of a
     * simple label string.
     *
     * @since 3.14.0
     */
    std::optional<bool> _labelOffsetSupport{};

    ClientSignatureParameterInformationOptions& labelOffsetSupport(std::optional<bool> v) { _labelOffsetSupport = v; return *this; }

    const std::optional<bool>& labelOffsetSupport() const { return _labelOffsetSupport; }

    bool operator==(const ClientSignatureParameterInformationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientSignatureParameterInformationOptions> fromJson<ClientSignatureParameterInformationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientSignatureParameterInformationOptions &data);

/** @since 3.18.0 */
struct ClientSignatureInformationOptions {
    /**
     * Client supports the following content formats for the documentation
     * property. The order describes the preferred format of the client.
     */
    std::optional<QList<MarkupKind>> _documentationFormat{};
    std::optional<ClientSignatureParameterInformationOptions> _parameterInformation{};  //!< Client capabilities specific to parameter information.
    /**
     * The client supports the `activeParameter` property on `SignatureInformation`
     * literal.
     *
     * @since 3.16.0
     */
    std::optional<bool> _activeParameterSupport{};
    /**
     * The client supports the `activeParameter` property on
     * `SignatureHelp`/`SignatureInformation` being set to `null` to
     * indicate that no parameter should be active.
     *
     * @since 3.18.0
     */
    std::optional<bool> _noActiveParameterSupport{};

    ClientSignatureInformationOptions& documentationFormat(const std::optional<QList<MarkupKind>> & v) { _documentationFormat = v; return *this; }
    ClientSignatureInformationOptions& addDocumentationFormat(const MarkupKind & v) { if (!_documentationFormat) _documentationFormat = QList<MarkupKind>{}; (*_documentationFormat).append(v); return *this; }
    ClientSignatureInformationOptions& parameterInformation(const std::optional<ClientSignatureParameterInformationOptions> & v) { _parameterInformation = v; return *this; }
    ClientSignatureInformationOptions& activeParameterSupport(std::optional<bool> v) { _activeParameterSupport = v; return *this; }
    ClientSignatureInformationOptions& noActiveParameterSupport(std::optional<bool> v) { _noActiveParameterSupport = v; return *this; }

    const std::optional<QList<MarkupKind>>& documentationFormat() const { return _documentationFormat; }
    const std::optional<ClientSignatureParameterInformationOptions>& parameterInformation() const { return _parameterInformation; }
    const std::optional<bool>& activeParameterSupport() const { return _activeParameterSupport; }
    const std::optional<bool>& noActiveParameterSupport() const { return _noActiveParameterSupport; }

    bool operator==(const ClientSignatureInformationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientSignatureInformationOptions> fromJson<ClientSignatureInformationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientSignatureInformationOptions &data);

/** Client Capabilities for a {@link SignatureHelpRequest}. */
struct SignatureHelpClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether signature help supports dynamic registration.
    /**
     * The client supports the following `SignatureInformation`
     * specific properties.
     */
    std::optional<ClientSignatureInformationOptions> _signatureInformation{};
    /**
     * The client supports to send additional context information for a
     * `textDocument/signatureHelp` request. A client that opts into
     * contextSupport will also support the `retriggerCharacters` on
     * `SignatureHelpOptions`.
     *
     * @since 3.15.0
     */
    std::optional<bool> _contextSupport{};

    SignatureHelpClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    SignatureHelpClientCapabilities& signatureInformation(const std::optional<ClientSignatureInformationOptions> & v) { _signatureInformation = v; return *this; }
    SignatureHelpClientCapabilities& contextSupport(std::optional<bool> v) { _contextSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<ClientSignatureInformationOptions>& signatureInformation() const { return _signatureInformation; }
    const std::optional<bool>& contextSupport() const { return _contextSupport; }

    bool operator==(const SignatureHelpClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SignatureHelpClientCapabilities> fromJson<SignatureHelpClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SignatureHelpClientCapabilities &data);

struct TextDocumentFilterClientCapabilities {
    /**
     * The client supports Relative Patterns.
     *
     * @since 3.18.0
     */
    std::optional<bool> _relativePatternSupport{};

    TextDocumentFilterClientCapabilities& relativePatternSupport(std::optional<bool> v) { _relativePatternSupport = v; return *this; }

    const std::optional<bool>& relativePatternSupport() const { return _relativePatternSupport; }

    bool operator==(const TextDocumentFilterClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentFilterClientCapabilities> fromJson<TextDocumentFilterClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentFilterClientCapabilities &data);

struct TextDocumentSyncClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether text document synchronization supports dynamic registration.
    std::optional<bool> _willSave{};  //!< The client supports sending will save notifications.
    /**
     * The client supports sending a will save request and
     * waits for a response providing text edits which will
     * be applied to the document before it is saved.
     */
    std::optional<bool> _willSaveWaitUntil{};
    std::optional<bool> _didSave{};  //!< The client supports did save notifications.

    TextDocumentSyncClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    TextDocumentSyncClientCapabilities& willSave(std::optional<bool> v) { _willSave = v; return *this; }
    TextDocumentSyncClientCapabilities& willSaveWaitUntil(std::optional<bool> v) { _willSaveWaitUntil = v; return *this; }
    TextDocumentSyncClientCapabilities& didSave(std::optional<bool> v) { _didSave = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& willSave() const { return _willSave; }
    const std::optional<bool>& willSaveWaitUntil() const { return _willSaveWaitUntil; }
    const std::optional<bool>& didSave() const { return _didSave; }

    bool operator==(const TextDocumentSyncClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentSyncClientCapabilities> fromJson<TextDocumentSyncClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentSyncClientCapabilities &data);

/** Since 3.6.0 */
struct TypeDefinitionClientCapabilities {
    /**
     * Whether implementation supports dynamic registration. If this is set to `true`
     * the client supports the new `TypeDefinitionRegistrationOptions` return value
     * for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};
    /**
     * The client supports additional metadata in the form of definition links.
     *
     * Since 3.14.0
     */
    std::optional<bool> _linkSupport{};

    TypeDefinitionClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    TypeDefinitionClientCapabilities& linkSupport(std::optional<bool> v) { _linkSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& linkSupport() const { return _linkSupport; }

    bool operator==(const TypeDefinitionClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeDefinitionClientCapabilities> fromJson<TypeDefinitionClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeDefinitionClientCapabilities &data);

/** @since 3.17.0 */
struct TypeHierarchyClientCapabilities {
    /**
     * Whether implementation supports dynamic registration. If this is set to `true`
     * the client supports the new `(TextDocumentRegistrationOptions & StaticRegistrationOptions)`
     * return value for the corresponding server capability as well.
     */
    std::optional<bool> _dynamicRegistration{};

    TypeHierarchyClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const TypeHierarchyClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeHierarchyClientCapabilities> fromJson<TypeHierarchyClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeHierarchyClientCapabilities &data);

/** Text document specific client capabilities. */
struct TextDocumentClientCapabilities {
    std::optional<TextDocumentSyncClientCapabilities> _synchronization{};  //!< Defines which synchronization capabilities the client supports.
    /**
     * Defines which filters the client supports.
     *
     * @since 3.18.0
     */
    std::optional<TextDocumentFilterClientCapabilities> _filters{};
    std::optional<CompletionClientCapabilities> _completion{};  //!< Capabilities specific to the `textDocument/completion` request.
    std::optional<HoverClientCapabilities> _hover{};  //!< Capabilities specific to the `textDocument/hover` request.
    std::optional<SignatureHelpClientCapabilities> _signatureHelp{};  //!< Capabilities specific to the `textDocument/signatureHelp` request.
    /**
     * Capabilities specific to the `textDocument/declaration` request.
     *
     * @since 3.14.0
     */
    std::optional<DeclarationClientCapabilities> _declaration{};
    std::optional<DefinitionClientCapabilities> _definition{};  //!< Capabilities specific to the `textDocument/definition` request.
    /**
     * Capabilities specific to the `textDocument/typeDefinition` request.
     *
     * @since 3.6.0
     */
    std::optional<TypeDefinitionClientCapabilities> _typeDefinition{};
    /**
     * Capabilities specific to the `textDocument/implementation` request.
     *
     * @since 3.6.0
     */
    std::optional<ImplementationClientCapabilities> _implementation{};
    std::optional<ReferenceClientCapabilities> _references{};  //!< Capabilities specific to the `textDocument/references` request.
    std::optional<DocumentHighlightClientCapabilities> _documentHighlight{};  //!< Capabilities specific to the `textDocument/documentHighlight` request.
    std::optional<DocumentSymbolClientCapabilities> _documentSymbol{};  //!< Capabilities specific to the `textDocument/documentSymbol` request.
    std::optional<CodeActionClientCapabilities> _codeAction{};  //!< Capabilities specific to the `textDocument/codeAction` request.
    std::optional<CodeLensClientCapabilities> _codeLens{};  //!< Capabilities specific to the `textDocument/codeLens` request.
    std::optional<DocumentLinkClientCapabilities> _documentLink{};  //!< Capabilities specific to the `textDocument/documentLink` request.
    /**
     * Capabilities specific to the `textDocument/documentColor` and the
     * `textDocument/colorPresentation` request.
     *
     * @since 3.6.0
     */
    std::optional<DocumentColorClientCapabilities> _colorProvider{};
    std::optional<DocumentFormattingClientCapabilities> _formatting{};  //!< Capabilities specific to the `textDocument/formatting` request.
    std::optional<DocumentRangeFormattingClientCapabilities> _rangeFormatting{};  //!< Capabilities specific to the `textDocument/rangeFormatting` request.
    std::optional<DocumentOnTypeFormattingClientCapabilities> _onTypeFormatting{};  //!< Capabilities specific to the `textDocument/onTypeFormatting` request.
    std::optional<RenameClientCapabilities> _rename{};  //!< Capabilities specific to the `textDocument/rename` request.
    /**
     * Capabilities specific to the `textDocument/foldingRange` request.
     *
     * @since 3.10.0
     */
    std::optional<FoldingRangeClientCapabilities> _foldingRange{};
    /**
     * Capabilities specific to the `textDocument/selectionRange` request.
     *
     * @since 3.15.0
     */
    std::optional<SelectionRangeClientCapabilities> _selectionRange{};
    std::optional<PublishDiagnosticsClientCapabilities> _publishDiagnostics{};  //!< Capabilities specific to the `textDocument/publishDiagnostics` notification.
    /**
     * Capabilities specific to the various call hierarchy requests.
     *
     * @since 3.16.0
     */
    std::optional<CallHierarchyClientCapabilities> _callHierarchy{};
    /**
     * Capabilities specific to the various semantic token request.
     *
     * @since 3.16.0
     */
    std::optional<SemanticTokensClientCapabilities> _semanticTokens{};
    /**
     * Capabilities specific to the `textDocument/linkedEditingRange` request.
     *
     * @since 3.16.0
     */
    std::optional<LinkedEditingRangeClientCapabilities> _linkedEditingRange{};
    /**
     * Client capabilities specific to the `textDocument/moniker` request.
     *
     * @since 3.16.0
     */
    std::optional<MonikerClientCapabilities> _moniker{};
    /**
     * Capabilities specific to the various type hierarchy requests.
     *
     * @since 3.17.0
     */
    std::optional<TypeHierarchyClientCapabilities> _typeHierarchy{};
    /**
     * Capabilities specific to the `textDocument/inlineValue` request.
     *
     * @since 3.17.0
     */
    std::optional<InlineValueClientCapabilities> _inlineValue{};
    /**
     * Capabilities specific to the `textDocument/inlayHint` request.
     *
     * @since 3.17.0
     */
    std::optional<InlayHintClientCapabilities> _inlayHint{};
    /**
     * Capabilities specific to the diagnostic pull model.
     *
     * @since 3.17.0
     */
    std::optional<DiagnosticClientCapabilities> _diagnostic{};
    /**
     * Client capabilities specific to inline completions.
     *
     * @since 3.18.0
     */
    std::optional<InlineCompletionClientCapabilities> _inlineCompletion{};

    TextDocumentClientCapabilities& synchronization(const std::optional<TextDocumentSyncClientCapabilities> & v) { _synchronization = v; return *this; }
    TextDocumentClientCapabilities& filters(const std::optional<TextDocumentFilterClientCapabilities> & v) { _filters = v; return *this; }
    TextDocumentClientCapabilities& completion(const std::optional<CompletionClientCapabilities> & v) { _completion = v; return *this; }
    TextDocumentClientCapabilities& hover(const std::optional<HoverClientCapabilities> & v) { _hover = v; return *this; }
    TextDocumentClientCapabilities& signatureHelp(const std::optional<SignatureHelpClientCapabilities> & v) { _signatureHelp = v; return *this; }
    TextDocumentClientCapabilities& declaration(const std::optional<DeclarationClientCapabilities> & v) { _declaration = v; return *this; }
    TextDocumentClientCapabilities& definition(const std::optional<DefinitionClientCapabilities> & v) { _definition = v; return *this; }
    TextDocumentClientCapabilities& typeDefinition(const std::optional<TypeDefinitionClientCapabilities> & v) { _typeDefinition = v; return *this; }
    TextDocumentClientCapabilities& implementation(const std::optional<ImplementationClientCapabilities> & v) { _implementation = v; return *this; }
    TextDocumentClientCapabilities& references(const std::optional<ReferenceClientCapabilities> & v) { _references = v; return *this; }
    TextDocumentClientCapabilities& documentHighlight(const std::optional<DocumentHighlightClientCapabilities> & v) { _documentHighlight = v; return *this; }
    TextDocumentClientCapabilities& documentSymbol(const std::optional<DocumentSymbolClientCapabilities> & v) { _documentSymbol = v; return *this; }
    TextDocumentClientCapabilities& codeAction(const std::optional<CodeActionClientCapabilities> & v) { _codeAction = v; return *this; }
    TextDocumentClientCapabilities& codeLens(const std::optional<CodeLensClientCapabilities> & v) { _codeLens = v; return *this; }
    TextDocumentClientCapabilities& documentLink(const std::optional<DocumentLinkClientCapabilities> & v) { _documentLink = v; return *this; }
    TextDocumentClientCapabilities& colorProvider(const std::optional<DocumentColorClientCapabilities> & v) { _colorProvider = v; return *this; }
    TextDocumentClientCapabilities& formatting(const std::optional<DocumentFormattingClientCapabilities> & v) { _formatting = v; return *this; }
    TextDocumentClientCapabilities& rangeFormatting(const std::optional<DocumentRangeFormattingClientCapabilities> & v) { _rangeFormatting = v; return *this; }
    TextDocumentClientCapabilities& onTypeFormatting(const std::optional<DocumentOnTypeFormattingClientCapabilities> & v) { _onTypeFormatting = v; return *this; }
    TextDocumentClientCapabilities& rename(const std::optional<RenameClientCapabilities> & v) { _rename = v; return *this; }
    TextDocumentClientCapabilities& foldingRange(const std::optional<FoldingRangeClientCapabilities> & v) { _foldingRange = v; return *this; }
    TextDocumentClientCapabilities& selectionRange(const std::optional<SelectionRangeClientCapabilities> & v) { _selectionRange = v; return *this; }
    TextDocumentClientCapabilities& publishDiagnostics(const std::optional<PublishDiagnosticsClientCapabilities> & v) { _publishDiagnostics = v; return *this; }
    TextDocumentClientCapabilities& callHierarchy(const std::optional<CallHierarchyClientCapabilities> & v) { _callHierarchy = v; return *this; }
    TextDocumentClientCapabilities& semanticTokens(const std::optional<SemanticTokensClientCapabilities> & v) { _semanticTokens = v; return *this; }
    TextDocumentClientCapabilities& linkedEditingRange(const std::optional<LinkedEditingRangeClientCapabilities> & v) { _linkedEditingRange = v; return *this; }
    TextDocumentClientCapabilities& moniker(const std::optional<MonikerClientCapabilities> & v) { _moniker = v; return *this; }
    TextDocumentClientCapabilities& typeHierarchy(const std::optional<TypeHierarchyClientCapabilities> & v) { _typeHierarchy = v; return *this; }
    TextDocumentClientCapabilities& inlineValue(const std::optional<InlineValueClientCapabilities> & v) { _inlineValue = v; return *this; }
    TextDocumentClientCapabilities& inlayHint(const std::optional<InlayHintClientCapabilities> & v) { _inlayHint = v; return *this; }
    TextDocumentClientCapabilities& diagnostic(const std::optional<DiagnosticClientCapabilities> & v) { _diagnostic = v; return *this; }
    TextDocumentClientCapabilities& inlineCompletion(const std::optional<InlineCompletionClientCapabilities> & v) { _inlineCompletion = v; return *this; }

    const std::optional<TextDocumentSyncClientCapabilities>& synchronization() const { return _synchronization; }
    const std::optional<TextDocumentFilterClientCapabilities>& filters() const { return _filters; }
    const std::optional<CompletionClientCapabilities>& completion() const { return _completion; }
    const std::optional<HoverClientCapabilities>& hover() const { return _hover; }
    const std::optional<SignatureHelpClientCapabilities>& signatureHelp() const { return _signatureHelp; }
    const std::optional<DeclarationClientCapabilities>& declaration() const { return _declaration; }
    const std::optional<DefinitionClientCapabilities>& definition() const { return _definition; }
    const std::optional<TypeDefinitionClientCapabilities>& typeDefinition() const { return _typeDefinition; }
    const std::optional<ImplementationClientCapabilities>& implementation() const { return _implementation; }
    const std::optional<ReferenceClientCapabilities>& references() const { return _references; }
    const std::optional<DocumentHighlightClientCapabilities>& documentHighlight() const { return _documentHighlight; }
    const std::optional<DocumentSymbolClientCapabilities>& documentSymbol() const { return _documentSymbol; }
    const std::optional<CodeActionClientCapabilities>& codeAction() const { return _codeAction; }
    const std::optional<CodeLensClientCapabilities>& codeLens() const { return _codeLens; }
    const std::optional<DocumentLinkClientCapabilities>& documentLink() const { return _documentLink; }
    const std::optional<DocumentColorClientCapabilities>& colorProvider() const { return _colorProvider; }
    const std::optional<DocumentFormattingClientCapabilities>& formatting() const { return _formatting; }
    const std::optional<DocumentRangeFormattingClientCapabilities>& rangeFormatting() const { return _rangeFormatting; }
    const std::optional<DocumentOnTypeFormattingClientCapabilities>& onTypeFormatting() const { return _onTypeFormatting; }
    const std::optional<RenameClientCapabilities>& rename() const { return _rename; }
    const std::optional<FoldingRangeClientCapabilities>& foldingRange() const { return _foldingRange; }
    const std::optional<SelectionRangeClientCapabilities>& selectionRange() const { return _selectionRange; }
    const std::optional<PublishDiagnosticsClientCapabilities>& publishDiagnostics() const { return _publishDiagnostics; }
    const std::optional<CallHierarchyClientCapabilities>& callHierarchy() const { return _callHierarchy; }
    const std::optional<SemanticTokensClientCapabilities>& semanticTokens() const { return _semanticTokens; }
    const std::optional<LinkedEditingRangeClientCapabilities>& linkedEditingRange() const { return _linkedEditingRange; }
    const std::optional<MonikerClientCapabilities>& moniker() const { return _moniker; }
    const std::optional<TypeHierarchyClientCapabilities>& typeHierarchy() const { return _typeHierarchy; }
    const std::optional<InlineValueClientCapabilities>& inlineValue() const { return _inlineValue; }
    const std::optional<InlayHintClientCapabilities>& inlayHint() const { return _inlayHint; }
    const std::optional<DiagnosticClientCapabilities>& diagnostic() const { return _diagnostic; }
    const std::optional<InlineCompletionClientCapabilities>& inlineCompletion() const { return _inlineCompletion; }

    bool operator==(const TextDocumentClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentClientCapabilities> fromJson<TextDocumentClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentClientCapabilities &data);

/**
 * Client capabilities for the showDocument request.
 *
 * @since 3.16.0
 */
struct ShowDocumentClientCapabilities {
    /**
     * The client has support for the showDocument
     * request.
     */
    bool _support{};

    ShowDocumentClientCapabilities& support(bool v) { _support = v; return *this; }

    const bool& support() const { return _support; }

    bool operator==(const ShowDocumentClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ShowDocumentClientCapabilities> fromJson<ShowDocumentClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ShowDocumentClientCapabilities &data);

/** @since 3.18.0 */
struct ClientShowMessageActionItemOptions {
    /**
     * Whether the client supports additional attributes which
     * are preserved and send back to the server in the
     * request's response.
     */
    std::optional<bool> _additionalPropertiesSupport{};

    ClientShowMessageActionItemOptions& additionalPropertiesSupport(std::optional<bool> v) { _additionalPropertiesSupport = v; return *this; }

    const std::optional<bool>& additionalPropertiesSupport() const { return _additionalPropertiesSupport; }

    bool operator==(const ClientShowMessageActionItemOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientShowMessageActionItemOptions> fromJson<ClientShowMessageActionItemOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientShowMessageActionItemOptions &data);

/** Show message request client capabilities */
struct ShowMessageRequestClientCapabilities {
    std::optional<ClientShowMessageActionItemOptions> _messageActionItem{};  //!< Capabilities specific to the `MessageActionItem` type.

    ShowMessageRequestClientCapabilities& messageActionItem(const std::optional<ClientShowMessageActionItemOptions> & v) { _messageActionItem = v; return *this; }

    const std::optional<ClientShowMessageActionItemOptions>& messageActionItem() const { return _messageActionItem; }

    bool operator==(const ShowMessageRequestClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ShowMessageRequestClientCapabilities> fromJson<ShowMessageRequestClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ShowMessageRequestClientCapabilities &data);

struct WindowClientCapabilities {
    /**
     * It indicates whether the client supports server initiated
     * progress using the `window/workDoneProgress/create` request.
     *
     * The capability also controls Whether client supports handling
     * of progress notifications. If set servers are allowed to report a
     * `workDoneProgress` property in the request specific server
     * capabilities.
     *
     * @since 3.15.0
     */
    std::optional<bool> _workDoneProgress{};
    /**
     * Capabilities specific to the showMessage request.
     *
     * @since 3.16.0
     */
    std::optional<ShowMessageRequestClientCapabilities> _showMessage{};
    /**
     * Capabilities specific to the showDocument request.
     *
     * @since 3.16.0
     */
    std::optional<ShowDocumentClientCapabilities> _showDocument{};

    WindowClientCapabilities& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    WindowClientCapabilities& showMessage(const std::optional<ShowMessageRequestClientCapabilities> & v) { _showMessage = v; return *this; }
    WindowClientCapabilities& showDocument(const std::optional<ShowDocumentClientCapabilities> & v) { _showDocument = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<ShowMessageRequestClientCapabilities>& showMessage() const { return _showMessage; }
    const std::optional<ShowDocumentClientCapabilities>& showDocument() const { return _showDocument; }

    bool operator==(const WindowClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WindowClientCapabilities> fromJson<WindowClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WindowClientCapabilities &data);

/** @since 3.16.0 */
struct CodeLensWorkspaceClientCapabilities {
    /**
     * Whether the client implementation supports a refresh request sent from the
     * server to the client.
     *
     * Note that this event is global and will force the client to refresh all
     * code lenses currently shown. It should be used with absolute care and is
     * useful for situation where a server for example detect a project wide
     * change that requires such a calculation.
     */
    std::optional<bool> _refreshSupport{};

    CodeLensWorkspaceClientCapabilities& refreshSupport(std::optional<bool> v) { _refreshSupport = v; return *this; }

    const std::optional<bool>& refreshSupport() const { return _refreshSupport; }

    bool operator==(const CodeLensWorkspaceClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeLensWorkspaceClientCapabilities> fromJson<CodeLensWorkspaceClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeLensWorkspaceClientCapabilities &data);

/**
 * Workspace client capabilities specific to diagnostic pull requests.
 *
 * @since 3.17.0
 */
struct DiagnosticWorkspaceClientCapabilities {
    /**
     * Whether the client implementation supports a refresh request sent from
     * the server to the client.
     *
     * Note that this event is global and will force the client to refresh all
     * pulled diagnostics currently shown. It should be used with absolute care and
     * is useful for situation where a server for example detects a project wide
     * change that requires such a calculation.
     */
    std::optional<bool> _refreshSupport{};

    DiagnosticWorkspaceClientCapabilities& refreshSupport(std::optional<bool> v) { _refreshSupport = v; return *this; }

    const std::optional<bool>& refreshSupport() const { return _refreshSupport; }

    bool operator==(const DiagnosticWorkspaceClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DiagnosticWorkspaceClientCapabilities> fromJson<DiagnosticWorkspaceClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DiagnosticWorkspaceClientCapabilities &data);

struct DidChangeConfigurationClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Did change configuration notification supports dynamic registration.

    DidChangeConfigurationClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const DidChangeConfigurationClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidChangeConfigurationClientCapabilities> fromJson<DidChangeConfigurationClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidChangeConfigurationClientCapabilities &data);

struct DidChangeWatchedFilesClientCapabilities {
    /**
     * Did change watched files notification supports dynamic registration. Please note
     * that the current protocol doesn't support static configuration for file changes
     * from the server side.
     */
    std::optional<bool> _dynamicRegistration{};
    /**
     * Whether the client has support for {@link  RelativePattern relative pattern}
     * or not.
     *
     * @since 3.17.0
     */
    std::optional<bool> _relativePatternSupport{};

    DidChangeWatchedFilesClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    DidChangeWatchedFilesClientCapabilities& relativePatternSupport(std::optional<bool> v) { _relativePatternSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& relativePatternSupport() const { return _relativePatternSupport; }

    bool operator==(const DidChangeWatchedFilesClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidChangeWatchedFilesClientCapabilities> fromJson<DidChangeWatchedFilesClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidChangeWatchedFilesClientCapabilities &data);

/** The client capabilities of a {@link ExecuteCommandRequest}. */
struct ExecuteCommandClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Execute command supports dynamic registration.

    ExecuteCommandClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const ExecuteCommandClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ExecuteCommandClientCapabilities> fromJson<ExecuteCommandClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ExecuteCommandClientCapabilities &data);

/**
 * Capabilities relating to events from file operations by the user in the client.
 *
 * These events do not come from the file system, they come from user operations
 * like renaming a file in the UI.
 *
 * @since 3.16.0
 */
struct FileOperationClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Whether the client supports dynamic registration for file requests/notifications.
    std::optional<bool> _didCreate{};  //!< The client has support for sending didCreateFiles notifications.
    std::optional<bool> _willCreate{};  //!< The client has support for sending willCreateFiles requests.
    std::optional<bool> _didRename{};  //!< The client has support for sending didRenameFiles notifications.
    std::optional<bool> _willRename{};  //!< The client has support for sending willRenameFiles requests.
    std::optional<bool> _didDelete{};  //!< The client has support for sending didDeleteFiles notifications.
    std::optional<bool> _willDelete{};  //!< The client has support for sending willDeleteFiles requests.

    FileOperationClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    FileOperationClientCapabilities& didCreate(std::optional<bool> v) { _didCreate = v; return *this; }
    FileOperationClientCapabilities& willCreate(std::optional<bool> v) { _willCreate = v; return *this; }
    FileOperationClientCapabilities& didRename(std::optional<bool> v) { _didRename = v; return *this; }
    FileOperationClientCapabilities& willRename(std::optional<bool> v) { _willRename = v; return *this; }
    FileOperationClientCapabilities& didDelete(std::optional<bool> v) { _didDelete = v; return *this; }
    FileOperationClientCapabilities& willDelete(std::optional<bool> v) { _willDelete = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<bool>& didCreate() const { return _didCreate; }
    const std::optional<bool>& willCreate() const { return _willCreate; }
    const std::optional<bool>& didRename() const { return _didRename; }
    const std::optional<bool>& willRename() const { return _willRename; }
    const std::optional<bool>& didDelete() const { return _didDelete; }
    const std::optional<bool>& willDelete() const { return _willDelete; }

    bool operator==(const FileOperationClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileOperationClientCapabilities> fromJson<FileOperationClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileOperationClientCapabilities &data);

/**
 * Client workspace capabilities specific to folding ranges
 *
 * @since 3.18.0
 */
struct FoldingRangeWorkspaceClientCapabilities {
    /**
     * Whether the client implementation supports a refresh request sent from the
     * server to the client.
     *
     * Note that this event is global and will force the client to refresh all
     * folding ranges currently shown. It should be used with absolute care and is
     * useful for situation where a server for example detects a project wide
     * change that requires such a calculation.
     *
     * @since 3.18.0
     */
    std::optional<bool> _refreshSupport{};

    FoldingRangeWorkspaceClientCapabilities& refreshSupport(std::optional<bool> v) { _refreshSupport = v; return *this; }

    const std::optional<bool>& refreshSupport() const { return _refreshSupport; }

    bool operator==(const FoldingRangeWorkspaceClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FoldingRangeWorkspaceClientCapabilities> fromJson<FoldingRangeWorkspaceClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FoldingRangeWorkspaceClientCapabilities &data);

/**
 * Client workspace capabilities specific to inlay hints.
 *
 * @since 3.17.0
 */
struct InlayHintWorkspaceClientCapabilities {
    /**
     * Whether the client implementation supports a refresh request sent from
     * the server to the client.
     *
     * Note that this event is global and will force the client to refresh all
     * inlay hints currently shown. It should be used with absolute care and
     * is useful for situation where a server for example detects a project wide
     * change that requires such a calculation.
     */
    std::optional<bool> _refreshSupport{};

    InlayHintWorkspaceClientCapabilities& refreshSupport(std::optional<bool> v) { _refreshSupport = v; return *this; }

    const std::optional<bool>& refreshSupport() const { return _refreshSupport; }

    bool operator==(const InlayHintWorkspaceClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHintWorkspaceClientCapabilities> fromJson<InlayHintWorkspaceClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlayHintWorkspaceClientCapabilities &data);

/**
 * Client workspace capabilities specific to inline values.
 *
 * @since 3.17.0
 */
struct InlineValueWorkspaceClientCapabilities {
    /**
     * Whether the client implementation supports a refresh request sent from the
     * server to the client.
     *
     * Note that this event is global and will force the client to refresh all
     * inline values currently shown. It should be used with absolute care and is
     * useful for situation where a server for example detects a project wide
     * change that requires such a calculation.
     */
    std::optional<bool> _refreshSupport{};

    InlineValueWorkspaceClientCapabilities& refreshSupport(std::optional<bool> v) { _refreshSupport = v; return *this; }

    const std::optional<bool>& refreshSupport() const { return _refreshSupport; }

    bool operator==(const InlineValueWorkspaceClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueWorkspaceClientCapabilities> fromJson<InlineValueWorkspaceClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineValueWorkspaceClientCapabilities &data);

/** @since 3.16.0 */
struct SemanticTokensWorkspaceClientCapabilities {
    /**
     * Whether the client implementation supports a refresh request sent from
     * the server to the client.
     *
     * Note that this event is global and will force the client to refresh all
     * semantic tokens currently shown. It should be used with absolute care
     * and is useful for situation where a server for example detects a project
     * wide change that requires such a calculation.
     */
    std::optional<bool> _refreshSupport{};

    SemanticTokensWorkspaceClientCapabilities& refreshSupport(std::optional<bool> v) { _refreshSupport = v; return *this; }

    const std::optional<bool>& refreshSupport() const { return _refreshSupport; }

    bool operator==(const SemanticTokensWorkspaceClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensWorkspaceClientCapabilities> fromJson<SemanticTokensWorkspaceClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensWorkspaceClientCapabilities &data);

/**
 * Client capabilities for a text document content provider.
 *
 * @since 3.18.0
 */
struct TextDocumentContentClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Text document content provider supports dynamic registration.

    TextDocumentContentClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }

    bool operator==(const TextDocumentContentClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentContentClientCapabilities> fromJson<TextDocumentContentClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentContentClientCapabilities &data);

/** @since 3.18.0 */
struct ChangeAnnotationsSupportOptions {
    /**
     * Whether the client groups edits with equal labels into tree nodes,
     * for instance all edits labelled with "Changes in Strings" would
     * be a tree node.
     */
    std::optional<bool> _groupsOnLabel{};

    ChangeAnnotationsSupportOptions& groupsOnLabel(std::optional<bool> v) { _groupsOnLabel = v; return *this; }

    const std::optional<bool>& groupsOnLabel() const { return _groupsOnLabel; }

    bool operator==(const ChangeAnnotationsSupportOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ChangeAnnotationsSupportOptions> fromJson<ChangeAnnotationsSupportOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ChangeAnnotationsSupportOptions &data);

enum class FailureHandlingKind {
    abort,
    transactional,
    textOnlyTransactional,
    undo
};

LANGUAGESERVERPROTOCOL_EXPORT QString toString(FailureHandlingKind v);

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FailureHandlingKind> fromJson<FailureHandlingKind>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const FailureHandlingKind &v);

enum class ResourceOperationKind {
    create,
    rename,
    delete_
};

LANGUAGESERVERPROTOCOL_EXPORT QString toString(ResourceOperationKind v);

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ResourceOperationKind> fromJson<ResourceOperationKind>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ResourceOperationKind &v);

struct WorkspaceEditClientCapabilities {
    std::optional<bool> _documentChanges{};  //!< The client supports versioned document changes in `WorkspaceEdit`s
    /**
     * The resource operations the client supports. Clients should at least
     * support 'create', 'rename' and 'delete' files and folders.
     *
     * @since 3.13.0
     */
    std::optional<QList<ResourceOperationKind>> _resourceOperations{};
    /**
     * The failure handling strategy of a client if applying the workspace edit
     * fails.
     *
     * @since 3.13.0
     */
    std::optional<FailureHandlingKind> _failureHandling{};
    /**
     * Whether the client normalizes line endings to the client specific
     * setting.
     * If set to `true` the client will normalize line ending characters
     * in a workspace edit to the client-specified new line
     * character.
     *
     * @since 3.16.0
     */
    std::optional<bool> _normalizesLineEndings{};
    /**
     * Whether the client in general supports change annotations on text edits,
     * create file, rename file and delete file changes.
     *
     * @since 3.16.0
     */
    std::optional<ChangeAnnotationsSupportOptions> _changeAnnotationSupport{};
    /**
     * Whether the client supports `WorkspaceEditMetadata` in `WorkspaceEdit`s.
     *
     * @since 3.18.0
     */
    std::optional<bool> _metadataSupport{};
    /**
     * Whether the client supports snippets as text edits.
     *
     * @since 3.18.0
     */
    std::optional<bool> _snippetEditSupport{};

    WorkspaceEditClientCapabilities& documentChanges(std::optional<bool> v) { _documentChanges = v; return *this; }
    WorkspaceEditClientCapabilities& resourceOperations(const std::optional<QList<ResourceOperationKind>> & v) { _resourceOperations = v; return *this; }
    WorkspaceEditClientCapabilities& addResourceOperation(const ResourceOperationKind & v) { if (!_resourceOperations) _resourceOperations = QList<ResourceOperationKind>{}; (*_resourceOperations).append(v); return *this; }
    WorkspaceEditClientCapabilities& failureHandling(const std::optional<FailureHandlingKind> & v) { _failureHandling = v; return *this; }
    WorkspaceEditClientCapabilities& normalizesLineEndings(std::optional<bool> v) { _normalizesLineEndings = v; return *this; }
    WorkspaceEditClientCapabilities& changeAnnotationSupport(const std::optional<ChangeAnnotationsSupportOptions> & v) { _changeAnnotationSupport = v; return *this; }
    WorkspaceEditClientCapabilities& metadataSupport(std::optional<bool> v) { _metadataSupport = v; return *this; }
    WorkspaceEditClientCapabilities& snippetEditSupport(std::optional<bool> v) { _snippetEditSupport = v; return *this; }

    const std::optional<bool>& documentChanges() const { return _documentChanges; }
    const std::optional<QList<ResourceOperationKind>>& resourceOperations() const { return _resourceOperations; }
    const std::optional<FailureHandlingKind>& failureHandling() const { return _failureHandling; }
    const std::optional<bool>& normalizesLineEndings() const { return _normalizesLineEndings; }
    const std::optional<ChangeAnnotationsSupportOptions>& changeAnnotationSupport() const { return _changeAnnotationSupport; }
    const std::optional<bool>& metadataSupport() const { return _metadataSupport; }
    const std::optional<bool>& snippetEditSupport() const { return _snippetEditSupport; }

    bool operator==(const WorkspaceEditClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceEditClientCapabilities> fromJson<WorkspaceEditClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceEditClientCapabilities &data);

/** @since 3.18.0 */
struct ClientSymbolResolveOptions {
    /**
     * The properties that a client can resolve lazily. Usually
     * `location.range`
     */
    QStringList _properties{};

    ClientSymbolResolveOptions& properties(const QStringList & v) { _properties = v; return *this; }
    ClientSymbolResolveOptions& addProperty(const QString & v) { _properties.append(v); return *this; }

    const QStringList& properties() const { return _properties; }

    bool operator==(const ClientSymbolResolveOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientSymbolResolveOptions> fromJson<ClientSymbolResolveOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientSymbolResolveOptions &data);

/** Client capabilities for a {@link WorkspaceSymbolRequest}. */
struct WorkspaceSymbolClientCapabilities {
    std::optional<bool> _dynamicRegistration{};  //!< Symbol request supports dynamic registration.
    std::optional<ClientSymbolKindOptions> _symbolKind{};  //!< Specific capabilities for the `SymbolKind` in the `workspace/symbol` request.
    /**
     * The client supports tags on `SymbolInformation`.
     * Clients supporting tags have to handle unknown tags gracefully.
     *
     * @since 3.16.0
     */
    std::optional<ClientSymbolTagOptions> _tagSupport{};
    /**
     * The client support partial workspace symbols. The client will send the
     * request `workspaceSymbol/resolve` to the server to resolve additional
     * properties.
     *
     * @since 3.17.0
     */
    std::optional<ClientSymbolResolveOptions> _resolveSupport{};

    WorkspaceSymbolClientCapabilities& dynamicRegistration(std::optional<bool> v) { _dynamicRegistration = v; return *this; }
    WorkspaceSymbolClientCapabilities& symbolKind(const std::optional<ClientSymbolKindOptions> & v) { _symbolKind = v; return *this; }
    WorkspaceSymbolClientCapabilities& tagSupport(const std::optional<ClientSymbolTagOptions> & v) { _tagSupport = v; return *this; }
    WorkspaceSymbolClientCapabilities& resolveSupport(const std::optional<ClientSymbolResolveOptions> & v) { _resolveSupport = v; return *this; }

    const std::optional<bool>& dynamicRegistration() const { return _dynamicRegistration; }
    const std::optional<ClientSymbolKindOptions>& symbolKind() const { return _symbolKind; }
    const std::optional<ClientSymbolTagOptions>& tagSupport() const { return _tagSupport; }
    const std::optional<ClientSymbolResolveOptions>& resolveSupport() const { return _resolveSupport; }

    bool operator==(const WorkspaceSymbolClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceSymbolClientCapabilities> fromJson<WorkspaceSymbolClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceSymbolClientCapabilities &data);

/** Workspace specific client capabilities. */
struct WorkspaceClientCapabilities {
    /**
     * The client supports applying batch edits
     * to the workspace by supporting the request
     * 'workspace/applyEdit'
     */
    std::optional<bool> _applyEdit{};
    std::optional<WorkspaceEditClientCapabilities> _workspaceEdit{};  //!< Capabilities specific to `WorkspaceEdit`s.
    std::optional<DidChangeConfigurationClientCapabilities> _didChangeConfiguration{};  //!< Capabilities specific to the `workspace/didChangeConfiguration` notification.
    std::optional<DidChangeWatchedFilesClientCapabilities> _didChangeWatchedFiles{};  //!< Capabilities specific to the `workspace/didChangeWatchedFiles` notification.
    std::optional<WorkspaceSymbolClientCapabilities> _symbol{};  //!< Capabilities specific to the `workspace/symbol` request.
    std::optional<ExecuteCommandClientCapabilities> _executeCommand{};  //!< Capabilities specific to the `workspace/executeCommand` request.
    /**
     * The client has support for workspace folders.
     *
     * @since 3.6.0
     */
    std::optional<bool> _workspaceFolders{};
    /**
     * The client supports `workspace/configuration` requests.
     *
     * @since 3.6.0
     */
    std::optional<bool> _configuration{};
    /**
     * Capabilities specific to the semantic token requests scoped to the
     * workspace.
     *
     * @since 3.16.0.
     */
    std::optional<SemanticTokensWorkspaceClientCapabilities> _semanticTokens{};
    /**
     * Capabilities specific to the code lens requests scoped to the
     * workspace.
     *
     * @since 3.16.0.
     */
    std::optional<CodeLensWorkspaceClientCapabilities> _codeLens{};
    /**
     * The client has support for file notifications/requests for user operations on files.
     *
     * Since 3.16.0
     */
    std::optional<FileOperationClientCapabilities> _fileOperations{};
    /**
     * Capabilities specific to the inline values requests scoped to the
     * workspace.
     *
     * @since 3.17.0.
     */
    std::optional<InlineValueWorkspaceClientCapabilities> _inlineValue{};
    /**
     * Capabilities specific to the inlay hint requests scoped to the
     * workspace.
     *
     * @since 3.17.0.
     */
    std::optional<InlayHintWorkspaceClientCapabilities> _inlayHint{};
    /**
     * Capabilities specific to the diagnostic requests scoped to the
     * workspace.
     *
     * @since 3.17.0.
     */
    std::optional<DiagnosticWorkspaceClientCapabilities> _diagnostics{};
    /**
     * Capabilities specific to the folding range requests scoped to the workspace.
     *
     * @since 3.18.0
     */
    std::optional<FoldingRangeWorkspaceClientCapabilities> _foldingRange{};
    /**
     * Capabilities specific to the `workspace/textDocumentContent` request.
     *
     * @since 3.18.0
     */
    std::optional<TextDocumentContentClientCapabilities> _textDocumentContent{};

    WorkspaceClientCapabilities& applyEdit(std::optional<bool> v) { _applyEdit = v; return *this; }
    WorkspaceClientCapabilities& workspaceEdit(const std::optional<WorkspaceEditClientCapabilities> & v) { _workspaceEdit = v; return *this; }
    WorkspaceClientCapabilities& didChangeConfiguration(const std::optional<DidChangeConfigurationClientCapabilities> & v) { _didChangeConfiguration = v; return *this; }
    WorkspaceClientCapabilities& didChangeWatchedFiles(const std::optional<DidChangeWatchedFilesClientCapabilities> & v) { _didChangeWatchedFiles = v; return *this; }
    WorkspaceClientCapabilities& symbol(const std::optional<WorkspaceSymbolClientCapabilities> & v) { _symbol = v; return *this; }
    WorkspaceClientCapabilities& executeCommand(const std::optional<ExecuteCommandClientCapabilities> & v) { _executeCommand = v; return *this; }
    WorkspaceClientCapabilities& workspaceFolders(std::optional<bool> v) { _workspaceFolders = v; return *this; }
    WorkspaceClientCapabilities& configuration(std::optional<bool> v) { _configuration = v; return *this; }
    WorkspaceClientCapabilities& semanticTokens(const std::optional<SemanticTokensWorkspaceClientCapabilities> & v) { _semanticTokens = v; return *this; }
    WorkspaceClientCapabilities& codeLens(const std::optional<CodeLensWorkspaceClientCapabilities> & v) { _codeLens = v; return *this; }
    WorkspaceClientCapabilities& fileOperations(const std::optional<FileOperationClientCapabilities> & v) { _fileOperations = v; return *this; }
    WorkspaceClientCapabilities& inlineValue(const std::optional<InlineValueWorkspaceClientCapabilities> & v) { _inlineValue = v; return *this; }
    WorkspaceClientCapabilities& inlayHint(const std::optional<InlayHintWorkspaceClientCapabilities> & v) { _inlayHint = v; return *this; }
    WorkspaceClientCapabilities& diagnostics(const std::optional<DiagnosticWorkspaceClientCapabilities> & v) { _diagnostics = v; return *this; }
    WorkspaceClientCapabilities& foldingRange(const std::optional<FoldingRangeWorkspaceClientCapabilities> & v) { _foldingRange = v; return *this; }
    WorkspaceClientCapabilities& textDocumentContent(const std::optional<TextDocumentContentClientCapabilities> & v) { _textDocumentContent = v; return *this; }

    const std::optional<bool>& applyEdit() const { return _applyEdit; }
    const std::optional<WorkspaceEditClientCapabilities>& workspaceEdit() const { return _workspaceEdit; }
    const std::optional<DidChangeConfigurationClientCapabilities>& didChangeConfiguration() const { return _didChangeConfiguration; }
    const std::optional<DidChangeWatchedFilesClientCapabilities>& didChangeWatchedFiles() const { return _didChangeWatchedFiles; }
    const std::optional<WorkspaceSymbolClientCapabilities>& symbol() const { return _symbol; }
    const std::optional<ExecuteCommandClientCapabilities>& executeCommand() const { return _executeCommand; }
    const std::optional<bool>& workspaceFolders() const { return _workspaceFolders; }
    const std::optional<bool>& configuration() const { return _configuration; }
    const std::optional<SemanticTokensWorkspaceClientCapabilities>& semanticTokens() const { return _semanticTokens; }
    const std::optional<CodeLensWorkspaceClientCapabilities>& codeLens() const { return _codeLens; }
    const std::optional<FileOperationClientCapabilities>& fileOperations() const { return _fileOperations; }
    const std::optional<InlineValueWorkspaceClientCapabilities>& inlineValue() const { return _inlineValue; }
    const std::optional<InlayHintWorkspaceClientCapabilities>& inlayHint() const { return _inlayHint; }
    const std::optional<DiagnosticWorkspaceClientCapabilities>& diagnostics() const { return _diagnostics; }
    const std::optional<FoldingRangeWorkspaceClientCapabilities>& foldingRange() const { return _foldingRange; }
    const std::optional<TextDocumentContentClientCapabilities>& textDocumentContent() const { return _textDocumentContent; }

    bool operator==(const WorkspaceClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceClientCapabilities> fromJson<WorkspaceClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceClientCapabilities &data);

/** Defines the capabilities provided by the client. */
struct ClientCapabilities {
    std::optional<WorkspaceClientCapabilities> _workspace{};  //!< Workspace specific client capabilities.
    std::optional<TextDocumentClientCapabilities> _textDocument{};  //!< Text document specific client capabilities.
    /**
     * Capabilities specific to the notebook document support.
     *
     * @since 3.17.0
     */
    std::optional<NotebookDocumentClientCapabilities> _notebookDocument{};
    std::optional<WindowClientCapabilities> _window{};  //!< Window specific client capabilities.
    /**
     * General client capabilities.
     *
     * @since 3.16.0
     */
    std::optional<GeneralClientCapabilities> _general{};
    std::optional<QJsonValue> _experimental{};  //!< Experimental client capabilities.

    ClientCapabilities& workspace(const std::optional<WorkspaceClientCapabilities> & v) { _workspace = v; return *this; }
    ClientCapabilities& textDocument(const std::optional<TextDocumentClientCapabilities> & v) { _textDocument = v; return *this; }
    ClientCapabilities& notebookDocument(const std::optional<NotebookDocumentClientCapabilities> & v) { _notebookDocument = v; return *this; }
    ClientCapabilities& window(const std::optional<WindowClientCapabilities> & v) { _window = v; return *this; }
    ClientCapabilities& general(const std::optional<GeneralClientCapabilities> & v) { _general = v; return *this; }
    ClientCapabilities& experimental(const std::optional<QJsonValue> & v) { _experimental = v; return *this; }

    const std::optional<WorkspaceClientCapabilities>& workspace() const { return _workspace; }
    const std::optional<TextDocumentClientCapabilities>& textDocument() const { return _textDocument; }
    const std::optional<NotebookDocumentClientCapabilities>& notebookDocument() const { return _notebookDocument; }
    const std::optional<WindowClientCapabilities>& window() const { return _window; }
    const std::optional<GeneralClientCapabilities>& general() const { return _general; }
    const std::optional<QJsonValue>& experimental() const { return _experimental; }

    bool operator==(const ClientCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientCapabilities &data);

/**
 * Information about the client
 *
 * @since 3.15.0
 * @since 3.18.0 ClientInfo type name added.
 */
struct ClientInfo {
    QString _name{};  //!< The name of the client as defined by the client.
    std::optional<QString> _version{};  //!< The client's version as defined by the client.

    ClientInfo& name(const QString & v) { _name = v; return *this; }
    ClientInfo& version(const std::optional<QString> & v) { _version = v; return *this; }

    const QString& name() const { return _name; }
    const std::optional<QString>& version() const { return _version; }

    bool operator==(const ClientInfo &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ClientInfo> fromJson<ClientInfo>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ClientInfo &data);

enum class TraceValue {
    off,
    messages,
    verbose
};

LANGUAGESERVERPROTOCOL_EXPORT QString toString(TraceValue v);

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TraceValue> fromJson<TraceValue>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const TraceValue &v);

using InitializeParamsWorkspaceFolders = std::variant<QList<WorkspaceFolder>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InitializeParamsWorkspaceFolders> fromJson<InitializeParamsWorkspaceFolders>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const InitializeParamsWorkspaceFolders &val);

struct InitializeParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * The process Id of the parent process that started
     * the server.
     *
     * Is `null` if the process has not been started by another process.
     * If the parent process is not alive then the server should exit.
     */
    std::optional<int> _processId{};
    /**
     * Information about the client
     *
     * @since 3.15.0
     */
    std::optional<ClientInfo> _clientInfo{};
    /**
     * The locale the client is currently showing the user interface
     * in. This must not necessarily be the locale of the operating
     * system.
     *
     * Uses IETF language tags as the value's syntax
     * (See https://en.wikipedia.org/wiki/IETF_language_tag)
     *
     * @since 3.16.0
     */
    std::optional<QString> _locale{};
    /**
     * The rootPath of the workspace. Is null
     * if no folder is open.
     *
     * @deprecated in favour of rootUri.
     */
    std::optional<QString> _rootPath{};
    /**
     * The rootUri of the workspace. Is null if no
     * folder is open. If both `rootPath` and `rootUri` are set
     * `rootUri` wins.
     *
     * @deprecated in favour of workspaceFolders.
     */
    std::optional<QString> _rootUri{};
    ClientCapabilities _capabilities{};  //!< The capabilities provided by the client (editor or tool)
    std::optional<QJsonValue> _initializationOptions{};  //!< User provided initialization options.
    std::optional<TraceValue> _trace{};  //!< The initial trace setting. If omitted trace is disabled ('off').
    /**
     * The workspace folders configured in the client when the server starts.
     *
     * This property is only available if the client supports workspace folders.
     * It can be `null` if the client supports workspace folders but none are
     * configured.
     *
     * @since 3.6.0
     */
    std::optional<InitializeParamsWorkspaceFolders> _workspaceFolders{};

    InitializeParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    InitializeParams& processId(std::optional<int> v) { _processId = v; return *this; }
    InitializeParams& clientInfo(const std::optional<ClientInfo> & v) { _clientInfo = v; return *this; }
    InitializeParams& locale(const std::optional<QString> & v) { _locale = v; return *this; }
    InitializeParams& rootPath(const std::optional<QString> & v) { _rootPath = v; return *this; }
    InitializeParams& rootUri(const std::optional<QString> & v) { _rootUri = v; return *this; }
    InitializeParams& capabilities(const ClientCapabilities & v) { _capabilities = v; return *this; }
    InitializeParams& initializationOptions(const std::optional<QJsonValue> & v) { _initializationOptions = v; return *this; }
    InitializeParams& trace(const std::optional<TraceValue> & v) { _trace = v; return *this; }
    InitializeParams& workspaceFolders(const std::optional<InitializeParamsWorkspaceFolders> & v) { _workspaceFolders = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<int>& processId() const { return _processId; }
    const std::optional<ClientInfo>& clientInfo() const { return _clientInfo; }
    const std::optional<QString>& locale() const { return _locale; }
    const std::optional<QString>& rootPath() const { return _rootPath; }
    const std::optional<QString>& rootUri() const { return _rootUri; }
    const ClientCapabilities& capabilities() const { return _capabilities; }
    const std::optional<QJsonValue>& initializationOptions() const { return _initializationOptions; }
    const std::optional<TraceValue>& trace() const { return _trace; }
    const std::optional<InitializeParamsWorkspaceFolders>& workspaceFolders() const { return _workspaceFolders; }

    bool operator==(const InitializeParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InitializeParams> fromJson<InitializeParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InitializeParams &data);

/**
 * Call hierarchy options used during static registration.
 *
 * @since 3.16.0
 */
struct CallHierarchyOptions {
    std::optional<bool> _workDoneProgress{};

    CallHierarchyOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const CallHierarchyOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyOptions> fromJson<CallHierarchyOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CallHierarchyOptions &data);

/**
 * Documentation for a class of code actions.
 *
 * @since 3.18.0
 */
struct CodeActionKindDocumentation {
    /**
     * The kind of the code action being documented.
     *
     * If the kind is generic, such as `CodeActionKind.Refactor`, the documentation will be shown whenever any
     * refactorings are returned. If the kind if more specific, such as `CodeActionKind.RefactorExtract`, the
     * documentation will only be shown when extract refactoring code actions are returned.
     */
    CodeActionKind _kind{};
    /**
     * Command that is ued to display the documentation to the user.
     *
     * The title of this documentation code action is taken from {@linkcode Command.title}
     */
    Command _command{};

    CodeActionKindDocumentation& kind(const CodeActionKind & v) { _kind = v; return *this; }
    CodeActionKindDocumentation& command(const Command & v) { _command = v; return *this; }

    const CodeActionKind& kind() const { return _kind; }
    const Command& command() const { return _command; }

    bool operator==(const CodeActionKindDocumentation &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeActionKindDocumentation> fromJson<CodeActionKindDocumentation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeActionKindDocumentation &data);

/** Provider options for a {@link CodeActionRequest}. */
struct CodeActionOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * CodeActionKinds that this server may return.
     *
     * The list of kinds may be generic, such as `CodeActionKind.Refactor`, or the server
     * may list out every specific kind they provide.
     */
    std::optional<QList<CodeActionKind>> _codeActionKinds{};
    /**
     * Static documentation for a class of code actions.
     *
     * Documentation from the provider should be shown in the code actions menu if either:
     *
     * - Code actions of `kind` are requested by the editor. In this case, the editor will show the documentation that
     * most closely matches the requested code action kind. For example, if a provider has documentation for
     * both `Refactor` and `RefactorExtract`, when the user requests code actions for `RefactorExtract`,
     * the editor will use the documentation for `RefactorExtract` instead of the documentation for `Refactor`.
     *
     * - Any code actions of `kind` are returned by the provider.
     *
     * At most one documentation entry should be shown per provider.
     *
     * @since 3.18.0
     */
    std::optional<QList<CodeActionKindDocumentation>> _documentation{};
    /**
     * The server provides support to resolve additional
     * information for a code action.
     *
     * @since 3.16.0
     */
    std::optional<bool> _resolveProvider{};

    CodeActionOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    CodeActionOptions& codeActionKinds(const std::optional<QList<CodeActionKind>> & v) { _codeActionKinds = v; return *this; }
    CodeActionOptions& addCodeActionKind(const CodeActionKind & v) { if (!_codeActionKinds) _codeActionKinds = QList<CodeActionKind>{}; (*_codeActionKinds).append(v); return *this; }
    CodeActionOptions& documentation(const std::optional<QList<CodeActionKindDocumentation>> & v) { _documentation = v; return *this; }
    CodeActionOptions& addDocumentation(const CodeActionKindDocumentation & v) { if (!_documentation) _documentation = QList<CodeActionKindDocumentation>{}; (*_documentation).append(v); return *this; }
    CodeActionOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QList<CodeActionKind>>& codeActionKinds() const { return _codeActionKinds; }
    const std::optional<QList<CodeActionKindDocumentation>>& documentation() const { return _documentation; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }

    bool operator==(const CodeActionOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeActionOptions> fromJson<CodeActionOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeActionOptions &data);

/** Code Lens provider options of a {@link CodeLensRequest}. */
struct CodeLensOptions {
    std::optional<bool> _workDoneProgress{};
    std::optional<bool> _resolveProvider{};  //!< Code lens has a resolve provider as well.

    CodeLensOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    CodeLensOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }

    bool operator==(const CodeLensOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeLensOptions> fromJson<CodeLensOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeLensOptions &data);

/** @since 3.18.0 */
struct ServerCompletionItemOptions {
    /**
     * The server has support for completion item label
     * details (see also `CompletionItemLabelDetails`) when
     * receiving a completion item in a resolve call.
     *
     * @since 3.17.0
     */
    std::optional<bool> _labelDetailsSupport{};

    ServerCompletionItemOptions& labelDetailsSupport(std::optional<bool> v) { _labelDetailsSupport = v; return *this; }

    const std::optional<bool>& labelDetailsSupport() const { return _labelDetailsSupport; }

    bool operator==(const ServerCompletionItemOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCompletionItemOptions> fromJson<ServerCompletionItemOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ServerCompletionItemOptions &data);

/** Completion options. */
struct CompletionOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * Most tools trigger completion request automatically without explicitly requesting
     * it using a keyboard shortcut (e.g. Ctrl+Space). Typically they do so when the user
     * starts to type an identifier. For example if the user types `c` in a JavaScript file
     * code complete will automatically pop up present `console` besides others as a
     * completion item. Characters that make up identifiers don't need to be listed here.
     *
     * If code complete should automatically be trigger on characters not being valid inside
     * an identifier (for example `.` in JavaScript) list them in `triggerCharacters`.
     */
    std::optional<QStringList> _triggerCharacters{};
    /**
     * The list of all possible characters that commit a completion. This field can be used
     * if clients don't support individual commit characters per completion item. See
     * `ClientCapabilities.textDocument.completion.completionItem.commitCharactersSupport`
     *
     * If a server provides both `allCommitCharacters` and commit characters on an individual
     * completion item the ones on the completion item win.
     *
     * @since 3.2.0
     */
    std::optional<QStringList> _allCommitCharacters{};
    /**
     * The server provides support to resolve additional
     * information for a completion item.
     */
    std::optional<bool> _resolveProvider{};
    /**
     * The server supports the following `CompletionItem` specific
     * capabilities.
     *
     * @since 3.17.0
     */
    std::optional<ServerCompletionItemOptions> _completionItem{};

    CompletionOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    CompletionOptions& triggerCharacters(const std::optional<QStringList> & v) { _triggerCharacters = v; return *this; }
    CompletionOptions& addTriggerCharacter(const QString & v) { if (!_triggerCharacters) _triggerCharacters = QStringList{}; (*_triggerCharacters).append(v); return *this; }
    CompletionOptions& allCommitCharacters(const std::optional<QStringList> & v) { _allCommitCharacters = v; return *this; }
    CompletionOptions& addAllCommitCharacter(const QString & v) { if (!_allCommitCharacters) _allCommitCharacters = QStringList{}; (*_allCommitCharacters).append(v); return *this; }
    CompletionOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }
    CompletionOptions& completionItem(const std::optional<ServerCompletionItemOptions> & v) { _completionItem = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QStringList>& triggerCharacters() const { return _triggerCharacters; }
    const std::optional<QStringList>& allCommitCharacters() const { return _allCommitCharacters; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }
    const std::optional<ServerCompletionItemOptions>& completionItem() const { return _completionItem; }

    bool operator==(const CompletionOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionOptions> fromJson<CompletionOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionOptions &data);

struct DeclarationOptions {
    std::optional<bool> _workDoneProgress{};

    DeclarationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const DeclarationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DeclarationOptions> fromJson<DeclarationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DeclarationOptions &data);

/** Server Capabilities for a {@link DefinitionRequest}. */
struct DefinitionOptions {
    std::optional<bool> _workDoneProgress{};

    DefinitionOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const DefinitionOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DefinitionOptions> fromJson<DefinitionOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DefinitionOptions &data);

/**
 * Diagnostic options.
 *
 * @since 3.17.0
 */
struct DiagnosticOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * An optional identifier under which the diagnostics are
     * managed by the client.
     */
    std::optional<QString> _identifier{};
    /**
     * Whether the language has inter file dependencies meaning that
     * editing code in one file can result in a different diagnostic
     * set in another file. Inter file dependencies are common for
     * most programming languages and typically uncommon for linters.
     */
    bool _interFileDependencies{};
    bool _workspaceDiagnostics{};  //!< The server provides support for workspace diagnostics as well.

    DiagnosticOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    DiagnosticOptions& identifier(const std::optional<QString> & v) { _identifier = v; return *this; }
    DiagnosticOptions& interFileDependencies(bool v) { _interFileDependencies = v; return *this; }
    DiagnosticOptions& workspaceDiagnostics(bool v) { _workspaceDiagnostics = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& identifier() const { return _identifier; }
    const bool& interFileDependencies() const { return _interFileDependencies; }
    const bool& workspaceDiagnostics() const { return _workspaceDiagnostics; }

    bool operator==(const DiagnosticOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DiagnosticOptions> fromJson<DiagnosticOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DiagnosticOptions &data);

struct DocumentColorOptions {
    std::optional<bool> _workDoneProgress{};

    DocumentColorOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const DocumentColorOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentColorOptions> fromJson<DocumentColorOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentColorOptions &data);

/** Provider options for a {@link DocumentFormattingRequest}. */
struct DocumentFormattingOptions {
    std::optional<bool> _workDoneProgress{};

    DocumentFormattingOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const DocumentFormattingOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentFormattingOptions> fromJson<DocumentFormattingOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentFormattingOptions &data);

/** Provider options for a {@link DocumentHighlightRequest}. */
struct DocumentHighlightOptions {
    std::optional<bool> _workDoneProgress{};

    DocumentHighlightOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const DocumentHighlightOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentHighlightOptions> fromJson<DocumentHighlightOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentHighlightOptions &data);

/** Provider options for a {@link DocumentLinkRequest}. */
struct DocumentLinkOptions {
    std::optional<bool> _workDoneProgress{};
    std::optional<bool> _resolveProvider{};  //!< Document links have a resolve provider as well.

    DocumentLinkOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    DocumentLinkOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }

    bool operator==(const DocumentLinkOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentLinkOptions> fromJson<DocumentLinkOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentLinkOptions &data);

/** Provider options for a {@link DocumentOnTypeFormattingRequest}. */
struct DocumentOnTypeFormattingOptions {
    QString _firstTriggerCharacter{};  //!< A character on which formatting should be triggered, like `{`.
    std::optional<QStringList> _moreTriggerCharacter{};  //!< More trigger characters.

    DocumentOnTypeFormattingOptions& firstTriggerCharacter(const QString & v) { _firstTriggerCharacter = v; return *this; }
    DocumentOnTypeFormattingOptions& moreTriggerCharacter(const std::optional<QStringList> & v) { _moreTriggerCharacter = v; return *this; }
    DocumentOnTypeFormattingOptions& addMoreTriggerCharacter(const QString & v) { if (!_moreTriggerCharacter) _moreTriggerCharacter = QStringList{}; (*_moreTriggerCharacter).append(v); return *this; }

    const QString& firstTriggerCharacter() const { return _firstTriggerCharacter; }
    const std::optional<QStringList>& moreTriggerCharacter() const { return _moreTriggerCharacter; }

    bool operator==(const DocumentOnTypeFormattingOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentOnTypeFormattingOptions> fromJson<DocumentOnTypeFormattingOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentOnTypeFormattingOptions &data);

/** Provider options for a {@link DocumentRangeFormattingRequest}. */
struct DocumentRangeFormattingOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * Whether the server supports formatting multiple ranges at once.
     *
     * @since 3.18.0
     */
    std::optional<bool> _rangesSupport{};

    DocumentRangeFormattingOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    DocumentRangeFormattingOptions& rangesSupport(std::optional<bool> v) { _rangesSupport = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& rangesSupport() const { return _rangesSupport; }

    bool operator==(const DocumentRangeFormattingOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentRangeFormattingOptions> fromJson<DocumentRangeFormattingOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentRangeFormattingOptions &data);

/** Provider options for a {@link DocumentSymbolRequest}. */
struct DocumentSymbolOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * A human-readable string that is shown when multiple outlines trees
     * are shown for the same document.
     *
     * @since 3.16.0
     */
    std::optional<QString> _label{};

    DocumentSymbolOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    DocumentSymbolOptions& label(const std::optional<QString> & v) { _label = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& label() const { return _label; }

    bool operator==(const DocumentSymbolOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentSymbolOptions> fromJson<DocumentSymbolOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentSymbolOptions &data);

/** The server capabilities of a {@link ExecuteCommandRequest}. */
struct ExecuteCommandOptions {
    std::optional<bool> _workDoneProgress{};
    QStringList _commands{};  //!< The commands to be executed on the server

    ExecuteCommandOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    ExecuteCommandOptions& commands(const QStringList & v) { _commands = v; return *this; }
    ExecuteCommandOptions& addCommand(const QString & v) { _commands.append(v); return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const QStringList& commands() const { return _commands; }

    bool operator==(const ExecuteCommandOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ExecuteCommandOptions> fromJson<ExecuteCommandOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ExecuteCommandOptions &data);

struct FoldingRangeOptions {
    std::optional<bool> _workDoneProgress{};

    FoldingRangeOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const FoldingRangeOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FoldingRangeOptions> fromJson<FoldingRangeOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FoldingRangeOptions &data);

/** Hover options. */
struct HoverOptions {
    std::optional<bool> _workDoneProgress{};

    HoverOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const HoverOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<HoverOptions> fromJson<HoverOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const HoverOptions &data);

struct ImplementationOptions {
    std::optional<bool> _workDoneProgress{};

    ImplementationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const ImplementationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ImplementationOptions> fromJson<ImplementationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ImplementationOptions &data);

/**
 * Inlay hint options used during static registration.
 *
 * @since 3.17.0
 */
struct InlayHintOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * The server provides support to resolve additional
     * information for an inlay hint item.
     */
    std::optional<bool> _resolveProvider{};

    InlayHintOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    InlayHintOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }

    bool operator==(const InlayHintOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHintOptions> fromJson<InlayHintOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlayHintOptions &data);

/**
 * Inline completion options used during static registration.
 *
 * @since 3.18.0
 */
struct InlineCompletionOptions {
    std::optional<bool> _workDoneProgress{};

    InlineCompletionOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const InlineCompletionOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineCompletionOptions> fromJson<InlineCompletionOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineCompletionOptions &data);

/**
 * Inline value options used during static registration.
 *
 * @since 3.17.0
 */
struct InlineValueOptions {
    std::optional<bool> _workDoneProgress{};

    InlineValueOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const InlineValueOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueOptions> fromJson<InlineValueOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineValueOptions &data);

struct LinkedEditingRangeOptions {
    std::optional<bool> _workDoneProgress{};

    LinkedEditingRangeOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const LinkedEditingRangeOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<LinkedEditingRangeOptions> fromJson<LinkedEditingRangeOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const LinkedEditingRangeOptions &data);

struct MonikerOptions {
    std::optional<bool> _workDoneProgress{};

    MonikerOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const MonikerOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MonikerOptions> fromJson<MonikerOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const MonikerOptions &data);

/**
 * Options specific to a notebook plus its cells
 * to be synced to the server.
 *
 * If a selector provides a notebook document
 * filter but no cell selector all cells of a
 * matching notebook document will be synced.
 *
 * If a selector provides no notebook document
 * filter but only a cell selector all notebook
 * document that contain at least one matching
 * cell will be synced.
 *
 * @since 3.17.0
 */
struct NotebookDocumentSyncOptions {
    QList<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem> _notebookSelector{};  //!< The notebooks to be synced
    /**
     * Whether save notification should be forwarded to
     * the server. Will only be honored if mode === `notebook`.
     */
    std::optional<bool> _save{};

    NotebookDocumentSyncOptions& notebookSelector(const QList<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem> & v) { _notebookSelector = v; return *this; }
    NotebookDocumentSyncOptions& addNotebookSelector(const NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem & v) { _notebookSelector.append(v); return *this; }
    NotebookDocumentSyncOptions& save(std::optional<bool> v) { _save = v; return *this; }

    const QList<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem>& notebookSelector() const { return _notebookSelector; }
    const std::optional<bool>& save() const { return _save; }

    bool operator==(const NotebookDocumentSyncOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<NotebookDocumentSyncOptions> fromJson<NotebookDocumentSyncOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const NotebookDocumentSyncOptions &data);

/** Reference options. */
struct ReferenceOptions {
    std::optional<bool> _workDoneProgress{};

    ReferenceOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const ReferenceOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ReferenceOptions> fromJson<ReferenceOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ReferenceOptions &data);

/** Provider options for a {@link RenameRequest}. */
struct RenameOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * Renames should be checked and tested before being executed.
     *
     * @since version 3.12.0
     */
    std::optional<bool> _prepareProvider{};

    RenameOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    RenameOptions& prepareProvider(std::optional<bool> v) { _prepareProvider = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& prepareProvider() const { return _prepareProvider; }

    bool operator==(const RenameOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RenameOptions> fromJson<RenameOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RenameOptions &data);

struct SelectionRangeOptions {
    std::optional<bool> _workDoneProgress{};

    SelectionRangeOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const SelectionRangeOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SelectionRangeOptions> fromJson<SelectionRangeOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SelectionRangeOptions &data);

/** @since 3.16.0 */
struct SemanticTokensOptions {
    std::optional<bool> _workDoneProgress{};
    SemanticTokensLegend _legend{};  //!< The legend used by the server
    /**
     * Server supports providing semantic tokens for a specific range
     * of a document.
     */
    std::optional<SemanticTokensRegistrationOptionsRange> _range{};
    std::optional<SemanticTokensRegistrationOptionsFull> _full{};  //!< Server supports providing semantic tokens for a full document.

    SemanticTokensOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    SemanticTokensOptions& legend(const SemanticTokensLegend & v) { _legend = v; return *this; }
    SemanticTokensOptions& range(const std::optional<SemanticTokensRegistrationOptionsRange> & v) { _range = v; return *this; }
    SemanticTokensOptions& full(const std::optional<SemanticTokensRegistrationOptionsFull> & v) { _full = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const SemanticTokensLegend& legend() const { return _legend; }
    const std::optional<SemanticTokensRegistrationOptionsRange>& range() const { return _range; }
    const std::optional<SemanticTokensRegistrationOptionsFull>& full() const { return _full; }

    bool operator==(const SemanticTokensOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensOptions> fromJson<SemanticTokensOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensOptions &data);

/** Server Capabilities for a {@link SignatureHelpRequest}. */
struct SignatureHelpOptions {
    std::optional<bool> _workDoneProgress{};
    std::optional<QStringList> _triggerCharacters{};  //!< List of characters that trigger signature help automatically.
    /**
     * List of characters that re-trigger signature help.
     *
     * These trigger characters are only active when signature help is already showing. All trigger characters
     * are also counted as re-trigger characters.
     *
     * @since 3.15.0
     */
    std::optional<QStringList> _retriggerCharacters{};

    SignatureHelpOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    SignatureHelpOptions& triggerCharacters(const std::optional<QStringList> & v) { _triggerCharacters = v; return *this; }
    SignatureHelpOptions& addTriggerCharacter(const QString & v) { if (!_triggerCharacters) _triggerCharacters = QStringList{}; (*_triggerCharacters).append(v); return *this; }
    SignatureHelpOptions& retriggerCharacters(const std::optional<QStringList> & v) { _retriggerCharacters = v; return *this; }
    SignatureHelpOptions& addRetriggerCharacter(const QString & v) { if (!_retriggerCharacters) _retriggerCharacters = QStringList{}; (*_retriggerCharacters).append(v); return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QStringList>& triggerCharacters() const { return _triggerCharacters; }
    const std::optional<QStringList>& retriggerCharacters() const { return _retriggerCharacters; }

    bool operator==(const SignatureHelpOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SignatureHelpOptions> fromJson<SignatureHelpOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SignatureHelpOptions &data);

/**
 * Defines how the host (editor) should sync
 * document changes to the language server.
 */
namespace TextDocumentSyncKind {
    constexpr int None = 0;
    constexpr int Full = 1;
    constexpr int Incremental = 2;
} // namespace TextDocumentSyncKind
/** Save options. */
struct SaveOptions {
    std::optional<bool> _includeText{};  //!< The client is supposed to include the content on save.

    SaveOptions& includeText(std::optional<bool> v) { _includeText = v; return *this; }

    const std::optional<bool>& includeText() const { return _includeText; }

    bool operator==(const SaveOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SaveOptions> fromJson<SaveOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SaveOptions &data);

using TextDocumentSyncOptionsSave = std::variant<bool, SaveOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentSyncOptionsSave> fromJson<TextDocumentSyncOptionsSave>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const TextDocumentSyncOptionsSave &val);

struct TextDocumentSyncOptions {
    /**
     * Open and close notifications are sent to the server. If omitted open close notification should not
     * be sent.
     */
    std::optional<bool> _openClose{};
    /**
     * Change notifications are sent to the server. See TextDocumentSyncKind.None, TextDocumentSyncKind.Full
     * and TextDocumentSyncKind.Incremental. If omitted it defaults to TextDocumentSyncKind.None.
     */
    std::optional<int> _change{};
    /**
     * If present will save notifications are sent to the server. If omitted the notification should not be
     * sent.
     */
    std::optional<bool> _willSave{};
    /**
     * If present will save wait until requests are sent to the server. If omitted the request should not be
     * sent.
     */
    std::optional<bool> _willSaveWaitUntil{};
    /**
     * If present save notifications are sent to the server. If omitted the notification should not be
     * sent.
     */
    std::optional<TextDocumentSyncOptionsSave> _save{};

    TextDocumentSyncOptions& openClose(std::optional<bool> v) { _openClose = v; return *this; }
    TextDocumentSyncOptions& change(std::optional<int> v) { _change = v; return *this; }
    TextDocumentSyncOptions& willSave(std::optional<bool> v) { _willSave = v; return *this; }
    TextDocumentSyncOptions& willSaveWaitUntil(std::optional<bool> v) { _willSaveWaitUntil = v; return *this; }
    TextDocumentSyncOptions& save(const std::optional<TextDocumentSyncOptionsSave> & v) { _save = v; return *this; }

    const std::optional<bool>& openClose() const { return _openClose; }
    const std::optional<int>& change() const { return _change; }
    const std::optional<bool>& willSave() const { return _willSave; }
    const std::optional<bool>& willSaveWaitUntil() const { return _willSaveWaitUntil; }
    const std::optional<TextDocumentSyncOptionsSave>& save() const { return _save; }

    bool operator==(const TextDocumentSyncOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentSyncOptions> fromJson<TextDocumentSyncOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentSyncOptions &data);

struct TypeDefinitionOptions {
    std::optional<bool> _workDoneProgress{};

    TypeDefinitionOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const TypeDefinitionOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeDefinitionOptions> fromJson<TypeDefinitionOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeDefinitionOptions &data);

/**
 * Type hierarchy options used during static registration.
 *
 * @since 3.17.0
 */
struct TypeHierarchyOptions {
    std::optional<bool> _workDoneProgress{};

    TypeHierarchyOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const TypeHierarchyOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeHierarchyOptions> fromJson<TypeHierarchyOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TypeHierarchyOptions &data);

/**
 * Options for notifications/requests for user operations on files.
 *
 * @since 3.16.0
 */
struct FileOperationOptions {
    std::optional<FileOperationRegistrationOptions> _didCreate{};  //!< The server is interested in receiving didCreateFiles notifications.
    std::optional<FileOperationRegistrationOptions> _willCreate{};  //!< The server is interested in receiving willCreateFiles requests.
    std::optional<FileOperationRegistrationOptions> _didRename{};  //!< The server is interested in receiving didRenameFiles notifications.
    std::optional<FileOperationRegistrationOptions> _willRename{};  //!< The server is interested in receiving willRenameFiles requests.
    std::optional<FileOperationRegistrationOptions> _didDelete{};  //!< The server is interested in receiving didDeleteFiles file notifications.
    std::optional<FileOperationRegistrationOptions> _willDelete{};  //!< The server is interested in receiving willDeleteFiles file requests.

    FileOperationOptions& didCreate(const std::optional<FileOperationRegistrationOptions> & v) { _didCreate = v; return *this; }
    FileOperationOptions& willCreate(const std::optional<FileOperationRegistrationOptions> & v) { _willCreate = v; return *this; }
    FileOperationOptions& didRename(const std::optional<FileOperationRegistrationOptions> & v) { _didRename = v; return *this; }
    FileOperationOptions& willRename(const std::optional<FileOperationRegistrationOptions> & v) { _willRename = v; return *this; }
    FileOperationOptions& didDelete(const std::optional<FileOperationRegistrationOptions> & v) { _didDelete = v; return *this; }
    FileOperationOptions& willDelete(const std::optional<FileOperationRegistrationOptions> & v) { _willDelete = v; return *this; }

    const std::optional<FileOperationRegistrationOptions>& didCreate() const { return _didCreate; }
    const std::optional<FileOperationRegistrationOptions>& willCreate() const { return _willCreate; }
    const std::optional<FileOperationRegistrationOptions>& didRename() const { return _didRename; }
    const std::optional<FileOperationRegistrationOptions>& willRename() const { return _willRename; }
    const std::optional<FileOperationRegistrationOptions>& didDelete() const { return _didDelete; }
    const std::optional<FileOperationRegistrationOptions>& willDelete() const { return _willDelete; }

    bool operator==(const FileOperationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileOperationOptions> fromJson<FileOperationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileOperationOptions &data);

/**
 * Text document content provider options.
 *
 * @since 3.18.0
 */
struct TextDocumentContentOptions {
    QStringList _schemes{};  //!< The schemes for which the server provides content.

    TextDocumentContentOptions& schemes(const QStringList & v) { _schemes = v; return *this; }
    TextDocumentContentOptions& addScheme(const QString & v) { _schemes.append(v); return *this; }

    const QStringList& schemes() const { return _schemes; }

    bool operator==(const TextDocumentContentOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentContentOptions> fromJson<TextDocumentContentOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentContentOptions &data);

using WorkspaceFoldersServerCapabilitiesChangeNotifications = std::variant<QString, bool>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceFoldersServerCapabilitiesChangeNotifications> fromJson<WorkspaceFoldersServerCapabilitiesChangeNotifications>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const WorkspaceFoldersServerCapabilitiesChangeNotifications &val);

struct WorkspaceFoldersServerCapabilities {
    std::optional<bool> _supported{};  //!< The server has support for workspace folders
    /**
     * Whether the server wants to receive workspace folder
     * change notifications.
     *
     * If a string is provided the string is treated as an ID
     * under which the notification is registered on the client
     * side. The ID can be used to unregister for these events
     * using the `client/unregisterCapability` request.
     */
    std::optional<WorkspaceFoldersServerCapabilitiesChangeNotifications> _changeNotifications{};

    WorkspaceFoldersServerCapabilities& supported(std::optional<bool> v) { _supported = v; return *this; }
    WorkspaceFoldersServerCapabilities& changeNotifications(const std::optional<WorkspaceFoldersServerCapabilitiesChangeNotifications> & v) { _changeNotifications = v; return *this; }

    const std::optional<bool>& supported() const { return _supported; }
    const std::optional<WorkspaceFoldersServerCapabilitiesChangeNotifications>& changeNotifications() const { return _changeNotifications; }

    bool operator==(const WorkspaceFoldersServerCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceFoldersServerCapabilities> fromJson<WorkspaceFoldersServerCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceFoldersServerCapabilities &data);

using WorkspaceOptionsTextDocumentContent = std::variant<TextDocumentContentOptions, TextDocumentContentRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceOptionsTextDocumentContent> fromJson<WorkspaceOptionsTextDocumentContent>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const WorkspaceOptionsTextDocumentContent &val);

/**
 * Defines workspace specific capabilities of the server.
 *
 * @since 3.18.0
 */
struct WorkspaceOptions {
    /**
     * The server supports workspace folder.
     *
     * @since 3.6.0
     */
    std::optional<WorkspaceFoldersServerCapabilities> _workspaceFolders{};
    /**
     * The server is interested in notifications/requests for operations on files.
     *
     * @since 3.16.0
     */
    std::optional<FileOperationOptions> _fileOperations{};
    /**
     * The server supports the `workspace/textDocumentContent` request.
     *
     * @since 3.18.0
     */
    std::optional<WorkspaceOptionsTextDocumentContent> _textDocumentContent{};

    WorkspaceOptions& workspaceFolders(const std::optional<WorkspaceFoldersServerCapabilities> & v) { _workspaceFolders = v; return *this; }
    WorkspaceOptions& fileOperations(const std::optional<FileOperationOptions> & v) { _fileOperations = v; return *this; }
    WorkspaceOptions& textDocumentContent(const std::optional<WorkspaceOptionsTextDocumentContent> & v) { _textDocumentContent = v; return *this; }

    const std::optional<WorkspaceFoldersServerCapabilities>& workspaceFolders() const { return _workspaceFolders; }
    const std::optional<FileOperationOptions>& fileOperations() const { return _fileOperations; }
    const std::optional<WorkspaceOptionsTextDocumentContent>& textDocumentContent() const { return _textDocumentContent; }

    bool operator==(const WorkspaceOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceOptions> fromJson<WorkspaceOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceOptions &data);

/** Server capabilities for a {@link WorkspaceSymbolRequest}. */
struct WorkspaceSymbolOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * The server provides support to resolve additional
     * information for a workspace symbol.
     *
     * @since 3.17.0
     */
    std::optional<bool> _resolveProvider{};

    WorkspaceSymbolOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    WorkspaceSymbolOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }

    bool operator==(const WorkspaceSymbolOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceSymbolOptions> fromJson<WorkspaceSymbolOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceSymbolOptions &data);

using ServerCapabilitiesTextDocumentSync = std::variant<TextDocumentSyncOptions, int>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesTextDocumentSync> fromJson<ServerCapabilitiesTextDocumentSync>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesTextDocumentSync &val);

using ServerCapabilitiesNotebookDocumentSync = std::variant<NotebookDocumentSyncOptions, NotebookDocumentSyncRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesNotebookDocumentSync> fromJson<ServerCapabilitiesNotebookDocumentSync>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesNotebookDocumentSync &val);

using ServerCapabilitiesHoverProvider = std::variant<bool, HoverOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesHoverProvider> fromJson<ServerCapabilitiesHoverProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesHoverProvider &val);

using ServerCapabilitiesDeclarationProvider = std::variant<bool, DeclarationOptions, DeclarationRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesDeclarationProvider> fromJson<ServerCapabilitiesDeclarationProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesDeclarationProvider &val);

using ServerCapabilitiesDefinitionProvider = std::variant<bool, DefinitionOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesDefinitionProvider> fromJson<ServerCapabilitiesDefinitionProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesDefinitionProvider &val);

using ServerCapabilitiesTypeDefinitionProvider = std::variant<bool, TypeDefinitionOptions, TypeDefinitionRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesTypeDefinitionProvider> fromJson<ServerCapabilitiesTypeDefinitionProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesTypeDefinitionProvider &val);

using ServerCapabilitiesImplementationProvider = std::variant<bool, ImplementationOptions, ImplementationRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesImplementationProvider> fromJson<ServerCapabilitiesImplementationProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesImplementationProvider &val);

using ServerCapabilitiesReferencesProvider = std::variant<bool, ReferenceOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesReferencesProvider> fromJson<ServerCapabilitiesReferencesProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesReferencesProvider &val);

using ServerCapabilitiesDocumentHighlightProvider = std::variant<bool, DocumentHighlightOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesDocumentHighlightProvider> fromJson<ServerCapabilitiesDocumentHighlightProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesDocumentHighlightProvider &val);

using ServerCapabilitiesDocumentSymbolProvider = std::variant<bool, DocumentSymbolOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesDocumentSymbolProvider> fromJson<ServerCapabilitiesDocumentSymbolProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesDocumentSymbolProvider &val);

using ServerCapabilitiesCodeActionProvider = std::variant<bool, CodeActionOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesCodeActionProvider> fromJson<ServerCapabilitiesCodeActionProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesCodeActionProvider &val);

using ServerCapabilitiesColorProvider = std::variant<bool, DocumentColorOptions, DocumentColorRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesColorProvider> fromJson<ServerCapabilitiesColorProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesColorProvider &val);

using ServerCapabilitiesWorkspaceSymbolProvider = std::variant<bool, WorkspaceSymbolOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesWorkspaceSymbolProvider> fromJson<ServerCapabilitiesWorkspaceSymbolProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesWorkspaceSymbolProvider &val);

using ServerCapabilitiesDocumentFormattingProvider = std::variant<bool, DocumentFormattingOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesDocumentFormattingProvider> fromJson<ServerCapabilitiesDocumentFormattingProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesDocumentFormattingProvider &val);

using ServerCapabilitiesDocumentRangeFormattingProvider = std::variant<bool, DocumentRangeFormattingOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesDocumentRangeFormattingProvider> fromJson<ServerCapabilitiesDocumentRangeFormattingProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesDocumentRangeFormattingProvider &val);

using ServerCapabilitiesRenameProvider = std::variant<bool, RenameOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesRenameProvider> fromJson<ServerCapabilitiesRenameProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesRenameProvider &val);

using ServerCapabilitiesFoldingRangeProvider = std::variant<bool, FoldingRangeOptions, FoldingRangeRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesFoldingRangeProvider> fromJson<ServerCapabilitiesFoldingRangeProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesFoldingRangeProvider &val);

using ServerCapabilitiesSelectionRangeProvider = std::variant<bool, SelectionRangeOptions, SelectionRangeRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesSelectionRangeProvider> fromJson<ServerCapabilitiesSelectionRangeProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesSelectionRangeProvider &val);

using ServerCapabilitiesCallHierarchyProvider = std::variant<bool, CallHierarchyOptions, CallHierarchyRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesCallHierarchyProvider> fromJson<ServerCapabilitiesCallHierarchyProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesCallHierarchyProvider &val);

using ServerCapabilitiesLinkedEditingRangeProvider = std::variant<bool, LinkedEditingRangeOptions, LinkedEditingRangeRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesLinkedEditingRangeProvider> fromJson<ServerCapabilitiesLinkedEditingRangeProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesLinkedEditingRangeProvider &val);

using ServerCapabilitiesSemanticTokensProvider = std::variant<SemanticTokensOptions, SemanticTokensRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesSemanticTokensProvider> fromJson<ServerCapabilitiesSemanticTokensProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesSemanticTokensProvider &val);

using ServerCapabilitiesMonikerProvider = std::variant<bool, MonikerOptions, MonikerRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesMonikerProvider> fromJson<ServerCapabilitiesMonikerProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesMonikerProvider &val);

using ServerCapabilitiesTypeHierarchyProvider = std::variant<bool, TypeHierarchyOptions, TypeHierarchyRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesTypeHierarchyProvider> fromJson<ServerCapabilitiesTypeHierarchyProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesTypeHierarchyProvider &val);

using ServerCapabilitiesInlineValueProvider = std::variant<bool, InlineValueOptions, InlineValueRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesInlineValueProvider> fromJson<ServerCapabilitiesInlineValueProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesInlineValueProvider &val);

using ServerCapabilitiesInlayHintProvider = std::variant<bool, InlayHintOptions, InlayHintRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesInlayHintProvider> fromJson<ServerCapabilitiesInlayHintProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesInlayHintProvider &val);

using ServerCapabilitiesDiagnosticProvider = std::variant<DiagnosticOptions, DiagnosticRegistrationOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesDiagnosticProvider> fromJson<ServerCapabilitiesDiagnosticProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesDiagnosticProvider &val);

using ServerCapabilitiesInlineCompletionProvider = std::variant<bool, InlineCompletionOptions>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilitiesInlineCompletionProvider> fromJson<ServerCapabilitiesInlineCompletionProvider>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ServerCapabilitiesInlineCompletionProvider &val);

/**
 * Defines the capabilities provided by a language
 * server.
 */
struct ServerCapabilities {
    /**
     * The position encoding the server picked from the encodings offered
     * by the client via the client capability `general.positionEncodings`.
     *
     * If the client didn't provide any position encodings the only valid
     * value that a server can return is 'utf-16'.
     *
     * If omitted it defaults to 'utf-16'.
     *
     * @since 3.17.0
     */
    std::optional<PositionEncodingKind> _positionEncoding{};
    /**
     * Defines how text documents are synced. Is either a detailed structure
     * defining each notification or for backwards compatibility the
     * TextDocumentSyncKind number.
     */
    std::optional<ServerCapabilitiesTextDocumentSync> _textDocumentSync{};
    /**
     * Defines how notebook documents are synced.
     *
     * @since 3.17.0
     */
    std::optional<ServerCapabilitiesNotebookDocumentSync> _notebookDocumentSync{};
    std::optional<CompletionOptions> _completionProvider{};  //!< The server provides completion support.
    std::optional<ServerCapabilitiesHoverProvider> _hoverProvider{};  //!< The server provides hover support.
    std::optional<SignatureHelpOptions> _signatureHelpProvider{};  //!< The server provides signature help support.
    std::optional<ServerCapabilitiesDeclarationProvider> _declarationProvider{};  //!< The server provides Goto Declaration support.
    std::optional<ServerCapabilitiesDefinitionProvider> _definitionProvider{};  //!< The server provides goto definition support.
    std::optional<ServerCapabilitiesTypeDefinitionProvider> _typeDefinitionProvider{};  //!< The server provides Goto Type Definition support.
    std::optional<ServerCapabilitiesImplementationProvider> _implementationProvider{};  //!< The server provides Goto Implementation support.
    std::optional<ServerCapabilitiesReferencesProvider> _referencesProvider{};  //!< The server provides find references support.
    std::optional<ServerCapabilitiesDocumentHighlightProvider> _documentHighlightProvider{};  //!< The server provides document highlight support.
    std::optional<ServerCapabilitiesDocumentSymbolProvider> _documentSymbolProvider{};  //!< The server provides document symbol support.
    /**
     * The server provides code actions. CodeActionOptions may only be
     * specified if the client states that it supports
     * `codeActionLiteralSupport` in its initial `initialize` request.
     */
    std::optional<ServerCapabilitiesCodeActionProvider> _codeActionProvider{};
    std::optional<CodeLensOptions> _codeLensProvider{};  //!< The server provides code lens.
    std::optional<DocumentLinkOptions> _documentLinkProvider{};  //!< The server provides document link support.
    std::optional<ServerCapabilitiesColorProvider> _colorProvider{};  //!< The server provides color provider support.
    std::optional<ServerCapabilitiesWorkspaceSymbolProvider> _workspaceSymbolProvider{};  //!< The server provides workspace symbol support.
    std::optional<ServerCapabilitiesDocumentFormattingProvider> _documentFormattingProvider{};  //!< The server provides document formatting.
    std::optional<ServerCapabilitiesDocumentRangeFormattingProvider> _documentRangeFormattingProvider{};  //!< The server provides document range formatting.
    std::optional<DocumentOnTypeFormattingOptions> _documentOnTypeFormattingProvider{};  //!< The server provides document formatting on typing.
    /**
     * The server provides rename support. RenameOptions may only be
     * specified if the client states that it supports
     * `prepareSupport` in its initial `initialize` request.
     */
    std::optional<ServerCapabilitiesRenameProvider> _renameProvider{};
    std::optional<ServerCapabilitiesFoldingRangeProvider> _foldingRangeProvider{};  //!< The server provides folding provider support.
    std::optional<ServerCapabilitiesSelectionRangeProvider> _selectionRangeProvider{};  //!< The server provides selection range support.
    std::optional<ExecuteCommandOptions> _executeCommandProvider{};  //!< The server provides execute command support.
    /**
     * The server provides call hierarchy support.
     *
     * @since 3.16.0
     */
    std::optional<ServerCapabilitiesCallHierarchyProvider> _callHierarchyProvider{};
    /**
     * The server provides linked editing range support.
     *
     * @since 3.16.0
     */
    std::optional<ServerCapabilitiesLinkedEditingRangeProvider> _linkedEditingRangeProvider{};
    /**
     * The server provides semantic tokens support.
     *
     * @since 3.16.0
     */
    std::optional<ServerCapabilitiesSemanticTokensProvider> _semanticTokensProvider{};
    /**
     * The server provides moniker support.
     *
     * @since 3.16.0
     */
    std::optional<ServerCapabilitiesMonikerProvider> _monikerProvider{};
    /**
     * The server provides type hierarchy support.
     *
     * @since 3.17.0
     */
    std::optional<ServerCapabilitiesTypeHierarchyProvider> _typeHierarchyProvider{};
    /**
     * The server provides inline values.
     *
     * @since 3.17.0
     */
    std::optional<ServerCapabilitiesInlineValueProvider> _inlineValueProvider{};
    /**
     * The server provides inlay hints.
     *
     * @since 3.17.0
     */
    std::optional<ServerCapabilitiesInlayHintProvider> _inlayHintProvider{};
    /**
     * The server has support for pull model diagnostics.
     *
     * @since 3.17.0
     */
    std::optional<ServerCapabilitiesDiagnosticProvider> _diagnosticProvider{};
    /**
     * Inline completion options used during static registration.
     *
     * @since 3.18.0
     */
    std::optional<ServerCapabilitiesInlineCompletionProvider> _inlineCompletionProvider{};
    std::optional<WorkspaceOptions> _workspace{};  //!< Workspace specific server capabilities.
    std::optional<QJsonValue> _experimental{};  //!< Experimental server capabilities.

    ServerCapabilities& positionEncoding(const std::optional<PositionEncodingKind> & v) { _positionEncoding = v; return *this; }
    ServerCapabilities& textDocumentSync(const std::optional<ServerCapabilitiesTextDocumentSync> & v) { _textDocumentSync = v; return *this; }
    ServerCapabilities& notebookDocumentSync(const std::optional<ServerCapabilitiesNotebookDocumentSync> & v) { _notebookDocumentSync = v; return *this; }
    ServerCapabilities& completionProvider(const std::optional<CompletionOptions> & v) { _completionProvider = v; return *this; }
    ServerCapabilities& hoverProvider(const std::optional<ServerCapabilitiesHoverProvider> & v) { _hoverProvider = v; return *this; }
    ServerCapabilities& signatureHelpProvider(const std::optional<SignatureHelpOptions> & v) { _signatureHelpProvider = v; return *this; }
    ServerCapabilities& declarationProvider(const std::optional<ServerCapabilitiesDeclarationProvider> & v) { _declarationProvider = v; return *this; }
    ServerCapabilities& definitionProvider(const std::optional<ServerCapabilitiesDefinitionProvider> & v) { _definitionProvider = v; return *this; }
    ServerCapabilities& typeDefinitionProvider(const std::optional<ServerCapabilitiesTypeDefinitionProvider> & v) { _typeDefinitionProvider = v; return *this; }
    ServerCapabilities& implementationProvider(const std::optional<ServerCapabilitiesImplementationProvider> & v) { _implementationProvider = v; return *this; }
    ServerCapabilities& referencesProvider(const std::optional<ServerCapabilitiesReferencesProvider> & v) { _referencesProvider = v; return *this; }
    ServerCapabilities& documentHighlightProvider(const std::optional<ServerCapabilitiesDocumentHighlightProvider> & v) { _documentHighlightProvider = v; return *this; }
    ServerCapabilities& documentSymbolProvider(const std::optional<ServerCapabilitiesDocumentSymbolProvider> & v) { _documentSymbolProvider = v; return *this; }
    ServerCapabilities& codeActionProvider(const std::optional<ServerCapabilitiesCodeActionProvider> & v) { _codeActionProvider = v; return *this; }
    ServerCapabilities& codeLensProvider(const std::optional<CodeLensOptions> & v) { _codeLensProvider = v; return *this; }
    ServerCapabilities& documentLinkProvider(const std::optional<DocumentLinkOptions> & v) { _documentLinkProvider = v; return *this; }
    ServerCapabilities& colorProvider(const std::optional<ServerCapabilitiesColorProvider> & v) { _colorProvider = v; return *this; }
    ServerCapabilities& workspaceSymbolProvider(const std::optional<ServerCapabilitiesWorkspaceSymbolProvider> & v) { _workspaceSymbolProvider = v; return *this; }
    ServerCapabilities& documentFormattingProvider(const std::optional<ServerCapabilitiesDocumentFormattingProvider> & v) { _documentFormattingProvider = v; return *this; }
    ServerCapabilities& documentRangeFormattingProvider(const std::optional<ServerCapabilitiesDocumentRangeFormattingProvider> & v) { _documentRangeFormattingProvider = v; return *this; }
    ServerCapabilities& documentOnTypeFormattingProvider(const std::optional<DocumentOnTypeFormattingOptions> & v) { _documentOnTypeFormattingProvider = v; return *this; }
    ServerCapabilities& renameProvider(const std::optional<ServerCapabilitiesRenameProvider> & v) { _renameProvider = v; return *this; }
    ServerCapabilities& foldingRangeProvider(const std::optional<ServerCapabilitiesFoldingRangeProvider> & v) { _foldingRangeProvider = v; return *this; }
    ServerCapabilities& selectionRangeProvider(const std::optional<ServerCapabilitiesSelectionRangeProvider> & v) { _selectionRangeProvider = v; return *this; }
    ServerCapabilities& executeCommandProvider(const std::optional<ExecuteCommandOptions> & v) { _executeCommandProvider = v; return *this; }
    ServerCapabilities& callHierarchyProvider(const std::optional<ServerCapabilitiesCallHierarchyProvider> & v) { _callHierarchyProvider = v; return *this; }
    ServerCapabilities& linkedEditingRangeProvider(const std::optional<ServerCapabilitiesLinkedEditingRangeProvider> & v) { _linkedEditingRangeProvider = v; return *this; }
    ServerCapabilities& semanticTokensProvider(const std::optional<ServerCapabilitiesSemanticTokensProvider> & v) { _semanticTokensProvider = v; return *this; }
    ServerCapabilities& monikerProvider(const std::optional<ServerCapabilitiesMonikerProvider> & v) { _monikerProvider = v; return *this; }
    ServerCapabilities& typeHierarchyProvider(const std::optional<ServerCapabilitiesTypeHierarchyProvider> & v) { _typeHierarchyProvider = v; return *this; }
    ServerCapabilities& inlineValueProvider(const std::optional<ServerCapabilitiesInlineValueProvider> & v) { _inlineValueProvider = v; return *this; }
    ServerCapabilities& inlayHintProvider(const std::optional<ServerCapabilitiesInlayHintProvider> & v) { _inlayHintProvider = v; return *this; }
    ServerCapabilities& diagnosticProvider(const std::optional<ServerCapabilitiesDiagnosticProvider> & v) { _diagnosticProvider = v; return *this; }
    ServerCapabilities& inlineCompletionProvider(const std::optional<ServerCapabilitiesInlineCompletionProvider> & v) { _inlineCompletionProvider = v; return *this; }
    ServerCapabilities& workspace(const std::optional<WorkspaceOptions> & v) { _workspace = v; return *this; }
    ServerCapabilities& experimental(const std::optional<QJsonValue> & v) { _experimental = v; return *this; }

    const std::optional<PositionEncodingKind>& positionEncoding() const { return _positionEncoding; }
    const std::optional<ServerCapabilitiesTextDocumentSync>& textDocumentSync() const { return _textDocumentSync; }
    const std::optional<ServerCapabilitiesNotebookDocumentSync>& notebookDocumentSync() const { return _notebookDocumentSync; }
    const std::optional<CompletionOptions>& completionProvider() const { return _completionProvider; }
    const std::optional<ServerCapabilitiesHoverProvider>& hoverProvider() const { return _hoverProvider; }
    const std::optional<SignatureHelpOptions>& signatureHelpProvider() const { return _signatureHelpProvider; }
    const std::optional<ServerCapabilitiesDeclarationProvider>& declarationProvider() const { return _declarationProvider; }
    const std::optional<ServerCapabilitiesDefinitionProvider>& definitionProvider() const { return _definitionProvider; }
    const std::optional<ServerCapabilitiesTypeDefinitionProvider>& typeDefinitionProvider() const { return _typeDefinitionProvider; }
    const std::optional<ServerCapabilitiesImplementationProvider>& implementationProvider() const { return _implementationProvider; }
    const std::optional<ServerCapabilitiesReferencesProvider>& referencesProvider() const { return _referencesProvider; }
    const std::optional<ServerCapabilitiesDocumentHighlightProvider>& documentHighlightProvider() const { return _documentHighlightProvider; }
    const std::optional<ServerCapabilitiesDocumentSymbolProvider>& documentSymbolProvider() const { return _documentSymbolProvider; }
    const std::optional<ServerCapabilitiesCodeActionProvider>& codeActionProvider() const { return _codeActionProvider; }
    const std::optional<CodeLensOptions>& codeLensProvider() const { return _codeLensProvider; }
    const std::optional<DocumentLinkOptions>& documentLinkProvider() const { return _documentLinkProvider; }
    const std::optional<ServerCapabilitiesColorProvider>& colorProvider() const { return _colorProvider; }
    const std::optional<ServerCapabilitiesWorkspaceSymbolProvider>& workspaceSymbolProvider() const { return _workspaceSymbolProvider; }
    const std::optional<ServerCapabilitiesDocumentFormattingProvider>& documentFormattingProvider() const { return _documentFormattingProvider; }
    const std::optional<ServerCapabilitiesDocumentRangeFormattingProvider>& documentRangeFormattingProvider() const { return _documentRangeFormattingProvider; }
    const std::optional<DocumentOnTypeFormattingOptions>& documentOnTypeFormattingProvider() const { return _documentOnTypeFormattingProvider; }
    const std::optional<ServerCapabilitiesRenameProvider>& renameProvider() const { return _renameProvider; }
    const std::optional<ServerCapabilitiesFoldingRangeProvider>& foldingRangeProvider() const { return _foldingRangeProvider; }
    const std::optional<ServerCapabilitiesSelectionRangeProvider>& selectionRangeProvider() const { return _selectionRangeProvider; }
    const std::optional<ExecuteCommandOptions>& executeCommandProvider() const { return _executeCommandProvider; }
    const std::optional<ServerCapabilitiesCallHierarchyProvider>& callHierarchyProvider() const { return _callHierarchyProvider; }
    const std::optional<ServerCapabilitiesLinkedEditingRangeProvider>& linkedEditingRangeProvider() const { return _linkedEditingRangeProvider; }
    const std::optional<ServerCapabilitiesSemanticTokensProvider>& semanticTokensProvider() const { return _semanticTokensProvider; }
    const std::optional<ServerCapabilitiesMonikerProvider>& monikerProvider() const { return _monikerProvider; }
    const std::optional<ServerCapabilitiesTypeHierarchyProvider>& typeHierarchyProvider() const { return _typeHierarchyProvider; }
    const std::optional<ServerCapabilitiesInlineValueProvider>& inlineValueProvider() const { return _inlineValueProvider; }
    const std::optional<ServerCapabilitiesInlayHintProvider>& inlayHintProvider() const { return _inlayHintProvider; }
    const std::optional<ServerCapabilitiesDiagnosticProvider>& diagnosticProvider() const { return _diagnosticProvider; }
    const std::optional<ServerCapabilitiesInlineCompletionProvider>& inlineCompletionProvider() const { return _inlineCompletionProvider; }
    const std::optional<WorkspaceOptions>& workspace() const { return _workspace; }
    const std::optional<QJsonValue>& experimental() const { return _experimental; }

    bool operator==(const ServerCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerCapabilities> fromJson<ServerCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ServerCapabilities &data);

/**
 * Information about the server
 *
 * @since 3.15.0
 * @since 3.18.0 ServerInfo type name added.
 */
struct ServerInfo {
    QString _name{};  //!< The name of the server as defined by the server.
    std::optional<QString> _version{};  //!< The server's version as defined by the server.

    ServerInfo& name(const QString & v) { _name = v; return *this; }
    ServerInfo& version(const std::optional<QString> & v) { _version = v; return *this; }

    const QString& name() const { return _name; }
    const std::optional<QString>& version() const { return _version; }

    bool operator==(const ServerInfo &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ServerInfo> fromJson<ServerInfo>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ServerInfo &data);

/** The result returned from an initialize request. */
struct InitializeResult {
    ServerCapabilities _capabilities{};  //!< The capabilities the language server provides.
    /**
     * Information about the server.
     *
     * @since 3.15.0
     */
    std::optional<ServerInfo> _serverInfo{};

    InitializeResult& capabilities(const ServerCapabilities & v) { _capabilities = v; return *this; }
    InitializeResult& serverInfo(const std::optional<ServerInfo> & v) { _serverInfo = v; return *this; }

    const ServerCapabilities& capabilities() const { return _capabilities; }
    const std::optional<ServerInfo>& serverInfo() const { return _serverInfo; }

    bool operator==(const InitializeResult &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InitializeResult> fromJson<InitializeResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InitializeResult &data);

/**
 * The data type of the ResponseError if the
 * initialize request fails.
 */
struct InitializeError {
    /**
     * Indicates whether the client execute the following retry logic:
     * (1) show the message provided by the ResponseError to the user
     * (2) user selects retry or cancel
     * (3) if user selected retry the initialize method is sent again.
     */
    bool _retry{};

    InitializeError& retry(bool v) { _retry = v; return *this; }

    const bool& retry() const { return _retry; }

    bool operator==(const InitializeError &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InitializeError> fromJson<InitializeError>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InitializeError &data);

struct InitializedParams {

    bool operator==(const InitializedParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InitializedParams> fromJson<InitializedParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InitializedParams &data);

/** The parameters of a change configuration notification. */
struct DidChangeConfigurationParams {
    QJsonValue _settings{};  //!< The actual changed settings

    DidChangeConfigurationParams& settings(const QJsonValue & v) { _settings = v; return *this; }

    const QJsonValue& settings() const { return _settings; }

    bool operator==(const DidChangeConfigurationParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidChangeConfigurationParams> fromJson<DidChangeConfigurationParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidChangeConfigurationParams &data);

struct DidChangeConfigurationRegistrationOptions {
    std::optional<QString> _section{};

    DidChangeConfigurationRegistrationOptions& section(const std::optional<QString> & v) { _section = v; return *this; }

    const std::optional<QString>& section() const { return _section; }

    bool operator==(const DidChangeConfigurationRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidChangeConfigurationRegistrationOptions> fromJson<DidChangeConfigurationRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidChangeConfigurationRegistrationOptions &data);

/** The message type */
namespace MessageType {
    constexpr int Error = 1;
    constexpr int Warning = 2;
    constexpr int Info = 3;
    constexpr int Log = 4;
    constexpr int Debug = 5;
} // namespace MessageType
/** The parameters of a notification message. */
struct ShowMessageParams {
    int _type{};  //!< The message type. See {@link MessageType}
    QString _message{};  //!< The actual message.

    ShowMessageParams& type(int v) { _type = v; return *this; }
    ShowMessageParams& message(const QString & v) { _message = v; return *this; }

    const int& type() const { return _type; }
    const QString& message() const { return _message; }

    bool operator==(const ShowMessageParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ShowMessageParams> fromJson<ShowMessageParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ShowMessageParams &data);

struct MessageActionItem {
    QString _title{};  //!< A short title like 'Retry', 'Open Log' etc.

    MessageActionItem& title(const QString & v) { _title = v; return *this; }

    const QString& title() const { return _title; }

    bool operator==(const MessageActionItem &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MessageActionItem> fromJson<MessageActionItem>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const MessageActionItem &data);

struct ShowMessageRequestParams {
    int _type{};  //!< The message type. See {@link MessageType}
    QString _message{};  //!< The actual message.
    std::optional<QList<MessageActionItem>> _actions{};  //!< The message action items to present.

    ShowMessageRequestParams& type(int v) { _type = v; return *this; }
    ShowMessageRequestParams& message(const QString & v) { _message = v; return *this; }
    ShowMessageRequestParams& actions(const std::optional<QList<MessageActionItem>> & v) { _actions = v; return *this; }
    ShowMessageRequestParams& addAction(const MessageActionItem & v) { if (!_actions) _actions = QList<MessageActionItem>{}; (*_actions).append(v); return *this; }

    const int& type() const { return _type; }
    const QString& message() const { return _message; }
    const std::optional<QList<MessageActionItem>>& actions() const { return _actions; }

    bool operator==(const ShowMessageRequestParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ShowMessageRequestParams> fromJson<ShowMessageRequestParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ShowMessageRequestParams &data);

/** The log message parameters. */
struct LogMessageParams {
    int _type{};  //!< The message type. See {@link MessageType}
    QString _message{};  //!< The actual message.

    LogMessageParams& type(int v) { _type = v; return *this; }
    LogMessageParams& message(const QString & v) { _message = v; return *this; }

    const int& type() const { return _type; }
    const QString& message() const { return _message; }

    bool operator==(const LogMessageParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<LogMessageParams> fromJson<LogMessageParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const LogMessageParams &data);

/** The parameters sent in an open text document notification */
struct DidOpenTextDocumentParams {
    TextDocumentItem _textDocument{};  //!< The document that was opened.

    DidOpenTextDocumentParams& textDocument(const TextDocumentItem & v) { _textDocument = v; return *this; }

    const TextDocumentItem& textDocument() const { return _textDocument; }

    bool operator==(const DidOpenTextDocumentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidOpenTextDocumentParams> fromJson<DidOpenTextDocumentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidOpenTextDocumentParams &data);

/** General text document registration options. */
struct TextDocumentRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};

    TextDocumentRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }

    bool operator==(const TextDocumentRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentRegistrationOptions> fromJson<TextDocumentRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentRegistrationOptions &data);

/** The change text document notification's parameters. */
struct DidChangeTextDocumentParams {
    /**
     * The document that did change. The version number points
     * to the version after all provided content changes have
     * been applied.
     */
    VersionedTextDocumentIdentifier _textDocument{};
    /**
     * The actual content changes. The content changes describe single state changes
     * to the document. So if there are two content changes c1 (at array index 0) and
     * c2 (at array index 1) for a document in state S then c1 moves the document from
     * S to S' and c2 from S' to S''. So c1 is computed on the state S and c2 is computed
     * on the state S'.
     *
     * To mirror the content of a document using change events use the following approach:
     * - start with the same initial content
     * - apply the 'textDocument/didChange' notifications in the order you receive them.
     * - apply the `TextDocumentContentChangeEvent`s in a single notification in the order
     * you receive them.
     */
    QList<TextDocumentContentChangeEvent> _contentChanges{};

    DidChangeTextDocumentParams& textDocument(const VersionedTextDocumentIdentifier & v) { _textDocument = v; return *this; }
    DidChangeTextDocumentParams& contentChanges(const QList<TextDocumentContentChangeEvent> & v) { _contentChanges = v; return *this; }
    DidChangeTextDocumentParams& addContentChange(const TextDocumentContentChangeEvent & v) { _contentChanges.append(v); return *this; }

    const VersionedTextDocumentIdentifier& textDocument() const { return _textDocument; }
    const QList<TextDocumentContentChangeEvent>& contentChanges() const { return _contentChanges; }

    bool operator==(const DidChangeTextDocumentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidChangeTextDocumentParams> fromJson<DidChangeTextDocumentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidChangeTextDocumentParams &data);

/** Describe options to be used when registered for text document change events. */
struct TextDocumentChangeRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    int _syncKind{};  //!< How documents are synced to the server.

    TextDocumentChangeRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    TextDocumentChangeRegistrationOptions& syncKind(int v) { _syncKind = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const int& syncKind() const { return _syncKind; }

    bool operator==(const TextDocumentChangeRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentChangeRegistrationOptions> fromJson<TextDocumentChangeRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentChangeRegistrationOptions &data);

/** The parameters sent in a close text document notification */
struct DidCloseTextDocumentParams {
    TextDocumentIdentifier _textDocument{};  //!< The document that was closed.

    DidCloseTextDocumentParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }

    bool operator==(const DidCloseTextDocumentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidCloseTextDocumentParams> fromJson<DidCloseTextDocumentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidCloseTextDocumentParams &data);

/** The parameters sent in a save text document notification */
struct DidSaveTextDocumentParams {
    TextDocumentIdentifier _textDocument{};  //!< The document that was saved.
    /**
     * Optional the content when saved. Depends on the includeText value
     * when the save notification was requested.
     */
    std::optional<QString> _text{};

    DidSaveTextDocumentParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    DidSaveTextDocumentParams& text(const std::optional<QString> & v) { _text = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const std::optional<QString>& text() const { return _text; }

    bool operator==(const DidSaveTextDocumentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidSaveTextDocumentParams> fromJson<DidSaveTextDocumentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidSaveTextDocumentParams &data);

/** Save registration options. */
struct TextDocumentSaveRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _includeText{};  //!< The client is supposed to include the content on save.

    TextDocumentSaveRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    TextDocumentSaveRegistrationOptions& includeText(std::optional<bool> v) { _includeText = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& includeText() const { return _includeText; }

    bool operator==(const TextDocumentSaveRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentSaveRegistrationOptions> fromJson<TextDocumentSaveRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentSaveRegistrationOptions &data);

/** Represents reasons why a text document is saved. */
namespace TextDocumentSaveReason {
    constexpr int Manual = 1;
    constexpr int AfterDelay = 2;
    constexpr int FocusOut = 3;
} // namespace TextDocumentSaveReason
/** The parameters sent in a will save text document notification. */
struct WillSaveTextDocumentParams {
    TextDocumentIdentifier _textDocument{};  //!< The document that will be saved.
    int _reason{};  //!< The 'TextDocumentSaveReason'.

    WillSaveTextDocumentParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    WillSaveTextDocumentParams& reason(int v) { _reason = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const int& reason() const { return _reason; }

    bool operator==(const WillSaveTextDocumentParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WillSaveTextDocumentParams> fromJson<WillSaveTextDocumentParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WillSaveTextDocumentParams &data);

/** The file event type */
namespace FileChangeType {
    constexpr int Created = 1;
    constexpr int Changed = 2;
    constexpr int Deleted = 3;
} // namespace FileChangeType
/** An event describing a file change. */
struct FileEvent {
    QString _uri{};  //!< The file's uri.
    int _type{};  //!< The change type.

    FileEvent& uri(const QString & v) { _uri = v; return *this; }
    FileEvent& type(int v) { _type = v; return *this; }

    const QString& uri() const { return _uri; }
    const int& type() const { return _type; }

    bool operator==(const FileEvent &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileEvent> fromJson<FileEvent>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileEvent &data);

/** The watched files change notification's parameters. */
struct DidChangeWatchedFilesParams {
    QList<FileEvent> _changes{};  //!< The actual file events.

    DidChangeWatchedFilesParams& changes(const QList<FileEvent> & v) { _changes = v; return *this; }
    DidChangeWatchedFilesParams& addChange(const FileEvent & v) { _changes.append(v); return *this; }

    const QList<FileEvent>& changes() const { return _changes; }

    bool operator==(const DidChangeWatchedFilesParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidChangeWatchedFilesParams> fromJson<DidChangeWatchedFilesParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidChangeWatchedFilesParams &data);

using WatchKind = int;

namespace WatchKinds {
    constexpr int Create = 1;
    constexpr int Change = 2;
    constexpr int Delete = 4;
} // namespace WatchKinds
struct FileSystemWatcher {
    /**
     * The glob pattern to watch. See {@link GlobPattern glob pattern} for more detail.
     *
     * @since 3.17.0 support for relative patterns.
     */
    GlobPattern _globPattern{};
    /**
     * The kind of events of interest. If omitted it defaults
     * to WatchKind.Create | WatchKind.Change | WatchKind.Delete
     * which is 7.
     */
    std::optional<WatchKind> _kind{};

    FileSystemWatcher& globPattern(const GlobPattern & v) { _globPattern = v; return *this; }
    FileSystemWatcher& kind(const std::optional<WatchKind> & v) { _kind = v; return *this; }

    const GlobPattern& globPattern() const { return _globPattern; }
    const std::optional<WatchKind>& kind() const { return _kind; }

    bool operator==(const FileSystemWatcher &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FileSystemWatcher> fromJson<FileSystemWatcher>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FileSystemWatcher &data);

/** Describe options to be used when registered for text document change events. */
struct DidChangeWatchedFilesRegistrationOptions {
    QList<FileSystemWatcher> _watchers{};  //!< The watchers to register.

    DidChangeWatchedFilesRegistrationOptions& watchers(const QList<FileSystemWatcher> & v) { _watchers = v; return *this; }
    DidChangeWatchedFilesRegistrationOptions& addWatcher(const FileSystemWatcher & v) { _watchers.append(v); return *this; }

    const QList<FileSystemWatcher>& watchers() const { return _watchers; }

    bool operator==(const DidChangeWatchedFilesRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DidChangeWatchedFilesRegistrationOptions> fromJson<DidChangeWatchedFilesRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DidChangeWatchedFilesRegistrationOptions &data);

/** The publish diagnostic notification's parameters. */
struct PublishDiagnosticsParams {
    QString _uri{};  //!< The URI for which diagnostic information is reported.
    /**
     * Optional the version number of the document the diagnostics are published for.
     *
     * @since 3.15.0
     */
    std::optional<int> _version{};
    QList<Diagnostic> _diagnostics{};  //!< An array of diagnostic information items.

    PublishDiagnosticsParams& uri(const QString & v) { _uri = v; return *this; }
    PublishDiagnosticsParams& version(std::optional<int> v) { _version = v; return *this; }
    PublishDiagnosticsParams& diagnostics(const QList<Diagnostic> & v) { _diagnostics = v; return *this; }
    PublishDiagnosticsParams& addDiagnostic(const Diagnostic & v) { _diagnostics.append(v); return *this; }

    const QString& uri() const { return _uri; }
    const std::optional<int>& version() const { return _version; }
    const QList<Diagnostic>& diagnostics() const { return _diagnostics; }

    bool operator==(const PublishDiagnosticsParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<PublishDiagnosticsParams> fromJson<PublishDiagnosticsParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const PublishDiagnosticsParams &data);

/** How a completion was triggered */
namespace CompletionTriggerKind {
    constexpr int Invoked = 1;
    constexpr int TriggerCharacter = 2;
    constexpr int TriggerForIncompleteCompletions = 3;
} // namespace CompletionTriggerKind
/** Contains additional information about the context in which a completion request is triggered. */
struct CompletionContext {
    int _triggerKind{};  //!< How the completion was triggered.
    /**
     * The trigger character (a single character) that has trigger code complete.
     * Is undefined if `triggerKind !== CompletionTriggerKind.TriggerCharacter`
     */
    std::optional<QString> _triggerCharacter{};

    CompletionContext& triggerKind(int v) { _triggerKind = v; return *this; }
    CompletionContext& triggerCharacter(const std::optional<QString> & v) { _triggerCharacter = v; return *this; }

    const int& triggerKind() const { return _triggerKind; }
    const std::optional<QString>& triggerCharacter() const { return _triggerCharacter; }

    bool operator==(const CompletionContext &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionContext> fromJson<CompletionContext>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionContext &data);

/** Completion parameters */
struct CompletionParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    /**
     * The completion context. This is only available it the client specifies
     * to send this using the client capability `textDocument.completion.contextSupport === true`
     */
    std::optional<CompletionContext> _context{};

    CompletionParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    CompletionParams& position(const Position & v) { _position = v; return *this; }
    CompletionParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    CompletionParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    CompletionParams& context(const std::optional<CompletionContext> & v) { _context = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const std::optional<CompletionContext>& context() const { return _context; }

    bool operator==(const CompletionParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionParams> fromJson<CompletionParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionParams &data);

/**
 * Additional details for a completion item label.
 *
 * @since 3.17.0
 */
struct CompletionItemLabelDetails {
    /**
     * An optional string which is rendered less prominently directly after {@link CompletionItem.label label},
     * without any spacing. Should be used for function signatures and type annotations.
     */
    std::optional<QString> _detail{};
    /**
     * An optional string which is rendered less prominently after {@link CompletionItem.detail}. Should be used
     * for fully qualified names and file paths.
     */
    std::optional<QString> _description{};

    CompletionItemLabelDetails& detail(const std::optional<QString> & v) { _detail = v; return *this; }
    CompletionItemLabelDetails& description(const std::optional<QString> & v) { _description = v; return *this; }

    const std::optional<QString>& detail() const { return _detail; }
    const std::optional<QString>& description() const { return _description; }

    bool operator==(const CompletionItemLabelDetails &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionItemLabelDetails> fromJson<CompletionItemLabelDetails>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionItemLabelDetails &data);

/**
 * A special text edit to provide an insert and a replace operation.
 *
 * @since 3.16.0
 */
struct InsertReplaceEdit {
    QString _newText{};  //!< The string to be inserted.
    Range _insert{};  //!< The range if the insert is requested
    Range _replace{};  //!< The range if the replace is requested.

    InsertReplaceEdit& newText(const QString & v) { _newText = v; return *this; }
    InsertReplaceEdit& insert(const Range & v) { _insert = v; return *this; }
    InsertReplaceEdit& replace(const Range & v) { _replace = v; return *this; }

    const QString& newText() const { return _newText; }
    const Range& insert() const { return _insert; }
    const Range& replace() const { return _replace; }

    bool operator==(const InsertReplaceEdit &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InsertReplaceEdit> fromJson<InsertReplaceEdit>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InsertReplaceEdit &data);

/**
 * Defines whether the insert text in a completion item should be interpreted as
 * plain text or a snippet.
 */
namespace InsertTextFormat {
    constexpr int PlainText = 1;
    constexpr int Snippet = 2;
} // namespace InsertTextFormat
using CompletionItemTextEdit = std::variant<TextEdit, InsertReplaceEdit>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionItemTextEdit> fromJson<CompletionItemTextEdit>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const CompletionItemTextEdit &val);

/**
 * A completion item represents a text snippet that is
 * proposed to complete text that is being typed.
 */
struct CompletionItem {
    /**
     * The label of this completion item.
     *
     * The label property is also by default the text that
     * is inserted when selecting this completion.
     *
     * If label details are provided the label itself should
     * be an unqualified name of the completion item.
     */
    QString _label{};
    /**
     * Additional details for the label
     *
     * @since 3.17.0
     */
    std::optional<CompletionItemLabelDetails> _labelDetails{};
    /**
     * The kind of this completion item. Based of the kind
     * an icon is chosen by the editor.
     */
    std::optional<int> _kind{};
    /**
     * Tags for this completion item.
     *
     * @since 3.15.0
     */
    std::optional<QList<int>> _tags{};
    /**
     * A human-readable string with additional information
     * about this item, like type or symbol information.
     */
    std::optional<QString> _detail{};
    std::optional<InlayHintLabelPartTooltip> _documentation{};  //!< A human-readable string that represents a doc-comment.
    /**
     * Indicates if this item is deprecated.
     * @deprecated Use `tags` instead.
     */
    std::optional<bool> _deprecated{};
    /**
     * Select this item when showing.
     *
     * *Note* that only one completion item can be selected and that the
     * tool / client decides which item that is. The rule is that the *first*
     * item of those that match best is selected.
     */
    std::optional<bool> _preselect{};
    /**
     * A string that should be used when comparing this item
     * with other items. When `falsy` the {@link CompletionItem.label label}
     * is used.
     */
    std::optional<QString> _sortText{};
    /**
     * A string that should be used when filtering a set of
     * completion items. When `falsy` the {@link CompletionItem.label label}
     * is used.
     */
    std::optional<QString> _filterText{};
    /**
     * A string that should be inserted into a document when selecting
     * this completion. When `falsy` the {@link CompletionItem.label label}
     * is used.
     *
     * The `insertText` is subject to interpretation by the client side.
     * Some tools might not take the string literally. For example
     * VS Code when code complete is requested in this example
     * `con<cursor position>` and a completion item with an `insertText` of
     * `console` is provided it will only insert `sole`. Therefore it is
     * recommended to use `textEdit` instead since it avoids additional client
     * side interpretation.
     */
    std::optional<QString> _insertText{};
    /**
     * The format of the insert text. The format applies to both the
     * `insertText` property and the `newText` property of a provided
     * `textEdit`. If omitted defaults to `InsertTextFormat.PlainText`.
     *
     * Please note that the insertTextFormat doesn't apply to
     * `additionalTextEdits`.
     */
    std::optional<int> _insertTextFormat{};
    /**
     * How whitespace and indentation is handled during completion
     * item insertion. If not provided the clients default value depends on
     * the `textDocument.completion.insertTextMode` client capability.
     *
     * @since 3.16.0
     */
    std::optional<int> _insertTextMode{};
    /**
     * An {@link TextEdit edit} which is applied to a document when selecting
     * this completion. When an edit is provided the value of
     * {@link CompletionItem.insertText insertText} is ignored.
     *
     * Most editors support two different operations when accepting a completion
     * item. One is to insert a completion text and the other is to replace an
     * existing text with a completion text. Since this can usually not be
     * predetermined by a server it can report both ranges. Clients need to
     * signal support for `InsertReplaceEdits` via the
     * `textDocument.completion.insertReplaceSupport` client capability
     * property.
     *
     * *Note 1:* The text edit's range as well as both ranges from an insert
     * replace edit must be a [single line] and they must contain the position
     * at which completion has been requested.
     * *Note 2:* If an `InsertReplaceEdit` is returned the edit's insert range
     * must be a prefix of the edit's replace range, that means it must be
     * contained and starting at the same position.
     *
     * @since 3.16.0 additional type `InsertReplaceEdit`
     */
    std::optional<CompletionItemTextEdit> _textEdit{};
    /**
     * The edit text used if the completion item is part of a CompletionList and
     * CompletionList defines an item default for the text edit range.
     *
     * Clients will only honor this property if they opt into completion list
     * item defaults using the capability `completionList.itemDefaults`.
     *
     * If not provided and a list's default range is provided the label
     * property is used as a text.
     *
     * @since 3.17.0
     */
    std::optional<QString> _textEditText{};
    /**
     * An optional array of additional {@link TextEdit text edits} that are applied when
     * selecting this completion. Edits must not overlap (including the same insert position)
     * with the main {@link CompletionItem.textEdit edit} nor with themselves.
     *
     * Additional text edits should be used to change text unrelated to the current cursor position
     * (for example adding an import statement at the top of the file if the completion item will
     * insert an unqualified type).
     */
    std::optional<QList<TextEdit>> _additionalTextEdits{};
    /**
     * An optional set of characters that when pressed while this completion is active will accept it first and
     * then type that character. *Note* that all commit characters should have `length=1` and that superfluous
     * characters will be ignored.
     */
    std::optional<QStringList> _commitCharacters{};
    /**
     * An optional {@link Command command} that is executed *after* inserting this completion. *Note* that
     * additional modifications to the current document should be described with the
     * {@link CompletionItem.additionalTextEdits additionalTextEdits}-property.
     */
    std::optional<Command> _command{};
    /**
     * A data entry field that is preserved on a completion item between a
     * {@link CompletionRequest} and a {@link CompletionResolveRequest}.
     */
    std::optional<QJsonValue> _data{};

    CompletionItem& label(const QString & v) { _label = v; return *this; }
    CompletionItem& labelDetails(const std::optional<CompletionItemLabelDetails> & v) { _labelDetails = v; return *this; }
    CompletionItem& kind(std::optional<int> v) { _kind = v; return *this; }
    CompletionItem& tags(const std::optional<QList<int>> & v) { _tags = v; return *this; }
    CompletionItem& addTag(int v) { if (!_tags) _tags = QList<int>{}; (*_tags).append(v); return *this; }
    CompletionItem& detail(const std::optional<QString> & v) { _detail = v; return *this; }
    CompletionItem& documentation(const std::optional<InlayHintLabelPartTooltip> & v) { _documentation = v; return *this; }
    CompletionItem& deprecated(std::optional<bool> v) { _deprecated = v; return *this; }
    CompletionItem& preselect(std::optional<bool> v) { _preselect = v; return *this; }
    CompletionItem& sortText(const std::optional<QString> & v) { _sortText = v; return *this; }
    CompletionItem& filterText(const std::optional<QString> & v) { _filterText = v; return *this; }
    CompletionItem& insertText(const std::optional<QString> & v) { _insertText = v; return *this; }
    CompletionItem& insertTextFormat(std::optional<int> v) { _insertTextFormat = v; return *this; }
    CompletionItem& insertTextMode(std::optional<int> v) { _insertTextMode = v; return *this; }
    CompletionItem& textEdit(const std::optional<CompletionItemTextEdit> & v) { _textEdit = v; return *this; }
    CompletionItem& textEditText(const std::optional<QString> & v) { _textEditText = v; return *this; }
    CompletionItem& additionalTextEdits(const std::optional<QList<TextEdit>> & v) { _additionalTextEdits = v; return *this; }
    CompletionItem& addAdditionalTextEdit(const TextEdit & v) { if (!_additionalTextEdits) _additionalTextEdits = QList<TextEdit>{}; (*_additionalTextEdits).append(v); return *this; }
    CompletionItem& commitCharacters(const std::optional<QStringList> & v) { _commitCharacters = v; return *this; }
    CompletionItem& addCommitCharacter(const QString & v) { if (!_commitCharacters) _commitCharacters = QStringList{}; (*_commitCharacters).append(v); return *this; }
    CompletionItem& command(const std::optional<Command> & v) { _command = v; return *this; }
    CompletionItem& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const QString& label() const { return _label; }
    const std::optional<CompletionItemLabelDetails>& labelDetails() const { return _labelDetails; }
    const std::optional<int>& kind() const { return _kind; }
    const std::optional<QList<int>>& tags() const { return _tags; }
    const std::optional<QString>& detail() const { return _detail; }
    const std::optional<InlayHintLabelPartTooltip>& documentation() const { return _documentation; }
    const std::optional<bool>& deprecated() const { return _deprecated; }
    const std::optional<bool>& preselect() const { return _preselect; }
    const std::optional<QString>& sortText() const { return _sortText; }
    const std::optional<QString>& filterText() const { return _filterText; }
    const std::optional<QString>& insertText() const { return _insertText; }
    const std::optional<int>& insertTextFormat() const { return _insertTextFormat; }
    const std::optional<int>& insertTextMode() const { return _insertTextMode; }
    const std::optional<CompletionItemTextEdit>& textEdit() const { return _textEdit; }
    const std::optional<QString>& textEditText() const { return _textEditText; }
    const std::optional<QList<TextEdit>>& additionalTextEdits() const { return _additionalTextEdits; }
    const std::optional<QStringList>& commitCharacters() const { return _commitCharacters; }
    const std::optional<Command>& command() const { return _command; }
    const std::optional<QJsonValue>& data() const { return _data; }

    bool operator==(const CompletionItem &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionItem> fromJson<CompletionItem>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionItem &data);

/**
 * Defines how values from a set of defaults and an individual item will be
 * merged.
 *
 * @since 3.18.0
 */
namespace ApplyKind {
    constexpr int Replace = 1;
    constexpr int Merge = 2;
} // namespace ApplyKind
/**
 * Specifies how fields from a completion item should be combined with those
 * from `completionList.itemDefaults`.
 *
 * If unspecified, all fields will be treated as ApplyKind.Replace.
 *
 * If a field's value is ApplyKind.Replace, the value from a completion item (if
 * provided and not `null`) will always be used instead of the value from
 * `completionItem.itemDefaults`.
 *
 * If a field's value is ApplyKind.Merge, the values will be merged using the rules
 * defined against each field below.
 *
 * Servers are only allowed to return `applyKind` if the client
 * signals support for this via the `completionList.applyKindSupport`
 * capability.
 *
 * @since 3.18.0
 */
struct CompletionItemApplyKinds {
    /**
     * Specifies whether commitCharacters on a completion will replace or be
     * merged with those in `completionList.itemDefaults.commitCharacters`.
     *
     * If ApplyKind.Replace, the commit characters from the completion item will
     * always be used unless not provided, in which case those from
     * `completionList.itemDefaults.commitCharacters` will be used. An
     * empty list can be used if a completion item does not have any commit
     * characters and also should not use those from
     * `completionList.itemDefaults.commitCharacters`.
     *
     * If ApplyKind.Merge the commitCharacters for the completion will be the
     * union of all values in both `completionList.itemDefaults.commitCharacters`
     * and the completion's own `commitCharacters`.
     *
     * @since 3.18.0
     */
    std::optional<int> _commitCharacters{};
    /**
     * Specifies whether the `data` field on a completion will replace or
     * be merged with data from `completionList.itemDefaults.data`.
     *
     * If ApplyKind.Replace, the data from the completion item will be used if
     * provided (and not `null`), otherwise
     * `completionList.itemDefaults.data` will be used. An empty object can
     * be used if a completion item does not have any data but also should
     * not use the value from `completionList.itemDefaults.data`.
     *
     * If ApplyKind.Merge, a shallow merge will be performed between
     * `completionList.itemDefaults.data` and the completion's own data
     * using the following rules:
     *
     * - If a completion's `data` field is not provided (or `null`), the
     * entire `data` field from `completionList.itemDefaults.data` will be
     * used as-is.
     * - If a completion's `data` field is provided, each field will
     * overwrite the field of the same name in
     * `completionList.itemDefaults.data` but no merging of nested fields
     * within that value will occur.
     *
     * @since 3.18.0
     */
    std::optional<int> _data{};

    CompletionItemApplyKinds& commitCharacters(std::optional<int> v) { _commitCharacters = v; return *this; }
    CompletionItemApplyKinds& data(std::optional<int> v) { _data = v; return *this; }

    const std::optional<int>& commitCharacters() const { return _commitCharacters; }
    const std::optional<int>& data() const { return _data; }

    bool operator==(const CompletionItemApplyKinds &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionItemApplyKinds> fromJson<CompletionItemApplyKinds>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionItemApplyKinds &data);

/**
 * Edit range variant that includes ranges for insert and replace operations.
 *
 * @since 3.18.0
 */
struct EditRangeWithInsertReplace {
    Range _insert{};
    Range _replace{};

    EditRangeWithInsertReplace& insert(const Range & v) { _insert = v; return *this; }
    EditRangeWithInsertReplace& replace(const Range & v) { _replace = v; return *this; }

    const Range& insert() const { return _insert; }
    const Range& replace() const { return _replace; }

    bool operator==(const EditRangeWithInsertReplace &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<EditRangeWithInsertReplace> fromJson<EditRangeWithInsertReplace>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const EditRangeWithInsertReplace &data);

using CompletionItemDefaultsEditRange = std::variant<Range, EditRangeWithInsertReplace>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionItemDefaultsEditRange> fromJson<CompletionItemDefaultsEditRange>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const CompletionItemDefaultsEditRange &val);

/**
 * In many cases the items of an actual completion result share the same
 * value for properties like `commitCharacters` or the range of a text
 * edit. A completion list can therefore define item defaults which will
 * be used if a completion item itself doesn't specify the value.
 *
 * If a completion list specifies a default value and a completion item
 * also specifies a corresponding value, the rules for combining these are
 * defined by `applyKinds` (if the client supports it), defaulting to
 * ApplyKind.Replace.
 *
 * Servers are only allowed to return default values if the client
 * signals support for this via the `completionList.itemDefaults`
 * capability.
 *
 * @since 3.17.0
 */
struct CompletionItemDefaults {
    /**
     * A default commit character set.
     *
     * @since 3.17.0
     */
    std::optional<QStringList> _commitCharacters{};
    /**
     * A default edit range.
     *
     * @since 3.17.0
     */
    std::optional<CompletionItemDefaultsEditRange> _editRange{};
    /**
     * A default insert text format.
     *
     * @since 3.17.0
     */
    std::optional<int> _insertTextFormat{};
    /**
     * A default insert text mode.
     *
     * @since 3.17.0
     */
    std::optional<int> _insertTextMode{};
    /**
     * A default data value.
     *
     * @since 3.17.0
     */
    std::optional<QJsonValue> _data{};

    CompletionItemDefaults& commitCharacters(const std::optional<QStringList> & v) { _commitCharacters = v; return *this; }
    CompletionItemDefaults& addCommitCharacter(const QString & v) { if (!_commitCharacters) _commitCharacters = QStringList{}; (*_commitCharacters).append(v); return *this; }
    CompletionItemDefaults& editRange(const std::optional<CompletionItemDefaultsEditRange> & v) { _editRange = v; return *this; }
    CompletionItemDefaults& insertTextFormat(std::optional<int> v) { _insertTextFormat = v; return *this; }
    CompletionItemDefaults& insertTextMode(std::optional<int> v) { _insertTextMode = v; return *this; }
    CompletionItemDefaults& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const std::optional<QStringList>& commitCharacters() const { return _commitCharacters; }
    const std::optional<CompletionItemDefaultsEditRange>& editRange() const { return _editRange; }
    const std::optional<int>& insertTextFormat() const { return _insertTextFormat; }
    const std::optional<int>& insertTextMode() const { return _insertTextMode; }
    const std::optional<QJsonValue>& data() const { return _data; }

    bool operator==(const CompletionItemDefaults &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionItemDefaults> fromJson<CompletionItemDefaults>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionItemDefaults &data);

/**
 * Represents a collection of {@link CompletionItem completion items} to be presented
 * in the editor.
 */
struct CompletionList {
    /**
     * This list it not complete. Further typing results in recomputing this list.
     *
     * Recomputed lists have all their items replaced (not appended) in the
     * incomplete completion sessions.
     */
    bool _isIncomplete{};
    /**
     * In many cases the items of an actual completion result share the same
     * value for properties like `commitCharacters` or the range of a text
     * edit. A completion list can therefore define item defaults which will
     * be used if a completion item itself doesn't specify the value.
     *
     * If a completion list specifies a default value and a completion item
     * also specifies a corresponding value, the rules for combining these are
     * defined by `applyKinds` (if the client supports it), defaulting to
     * ApplyKind.Replace.
     *
     * Servers are only allowed to return default values if the client
     * signals support for this via the `completionList.itemDefaults`
     * capability.
     *
     * @since 3.17.0
     */
    std::optional<CompletionItemDefaults> _itemDefaults{};
    /**
     * Specifies how fields from a completion item should be combined with those
     * from `completionList.itemDefaults`.
     *
     * If unspecified, all fields will be treated as ApplyKind.Replace.
     *
     * If a field's value is ApplyKind.Replace, the value from a completion item
     * (if provided and not `null`) will always be used instead of the value
     * from `completionItem.itemDefaults`.
     *
     * If a field's value is ApplyKind.Merge, the values will be merged using
     * the rules defined against each field below.
     *
     * Servers are only allowed to return `applyKind` if the client
     * signals support for this via the `completionList.applyKindSupport`
     * capability.
     *
     * @since 3.18.0
     */
    std::optional<CompletionItemApplyKinds> _applyKind{};
    QList<CompletionItem> _items{};  //!< The completion items.

    CompletionList& isIncomplete(bool v) { _isIncomplete = v; return *this; }
    CompletionList& itemDefaults(const std::optional<CompletionItemDefaults> & v) { _itemDefaults = v; return *this; }
    CompletionList& applyKind(const std::optional<CompletionItemApplyKinds> & v) { _applyKind = v; return *this; }
    CompletionList& items(const QList<CompletionItem> & v) { _items = v; return *this; }
    CompletionList& addItem(const CompletionItem & v) { _items.append(v); return *this; }

    const bool& isIncomplete() const { return _isIncomplete; }
    const std::optional<CompletionItemDefaults>& itemDefaults() const { return _itemDefaults; }
    const std::optional<CompletionItemApplyKinds>& applyKind() const { return _applyKind; }
    const QList<CompletionItem>& items() const { return _items; }

    bool operator==(const CompletionList &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionList> fromJson<CompletionList>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionList &data);

/** Registration options for a {@link CompletionRequest}. */
struct CompletionRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * Most tools trigger completion request automatically without explicitly requesting
     * it using a keyboard shortcut (e.g. Ctrl+Space). Typically they do so when the user
     * starts to type an identifier. For example if the user types `c` in a JavaScript file
     * code complete will automatically pop up present `console` besides others as a
     * completion item. Characters that make up identifiers don't need to be listed here.
     *
     * If code complete should automatically be trigger on characters not being valid inside
     * an identifier (for example `.` in JavaScript) list them in `triggerCharacters`.
     */
    std::optional<QStringList> _triggerCharacters{};
    /**
     * The list of all possible characters that commit a completion. This field can be used
     * if clients don't support individual commit characters per completion item. See
     * `ClientCapabilities.textDocument.completion.completionItem.commitCharactersSupport`
     *
     * If a server provides both `allCommitCharacters` and commit characters on an individual
     * completion item the ones on the completion item win.
     *
     * @since 3.2.0
     */
    std::optional<QStringList> _allCommitCharacters{};
    /**
     * The server provides support to resolve additional
     * information for a completion item.
     */
    std::optional<bool> _resolveProvider{};
    /**
     * The server supports the following `CompletionItem` specific
     * capabilities.
     *
     * @since 3.17.0
     */
    std::optional<ServerCompletionItemOptions> _completionItem{};

    CompletionRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    CompletionRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    CompletionRegistrationOptions& triggerCharacters(const std::optional<QStringList> & v) { _triggerCharacters = v; return *this; }
    CompletionRegistrationOptions& addTriggerCharacter(const QString & v) { if (!_triggerCharacters) _triggerCharacters = QStringList{}; (*_triggerCharacters).append(v); return *this; }
    CompletionRegistrationOptions& allCommitCharacters(const std::optional<QStringList> & v) { _allCommitCharacters = v; return *this; }
    CompletionRegistrationOptions& addAllCommitCharacter(const QString & v) { if (!_allCommitCharacters) _allCommitCharacters = QStringList{}; (*_allCommitCharacters).append(v); return *this; }
    CompletionRegistrationOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }
    CompletionRegistrationOptions& completionItem(const std::optional<ServerCompletionItemOptions> & v) { _completionItem = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QStringList>& triggerCharacters() const { return _triggerCharacters; }
    const std::optional<QStringList>& allCommitCharacters() const { return _allCommitCharacters; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }
    const std::optional<ServerCompletionItemOptions>& completionItem() const { return _completionItem; }

    bool operator==(const CompletionRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionRegistrationOptions> fromJson<CompletionRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CompletionRegistrationOptions &data);

/** Parameters for a {@link HoverRequest}. */
struct HoverParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.

    HoverParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    HoverParams& position(const Position & v) { _position = v; return *this; }
    HoverParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }

    bool operator==(const HoverParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<HoverParams> fromJson<HoverParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const HoverParams &data);

/**
 * @since 3.18.0
 * @deprecated use MarkupContent instead.
 */
struct MarkedStringWithLanguage {
    QString _language{};
    QString _value{};

    MarkedStringWithLanguage& language(const QString & v) { _language = v; return *this; }
    MarkedStringWithLanguage& value(const QString & v) { _value = v; return *this; }

    const QString& language() const { return _language; }
    const QString& value() const { return _value; }

    bool operator==(const MarkedStringWithLanguage &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MarkedStringWithLanguage> fromJson<MarkedStringWithLanguage>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const MarkedStringWithLanguage &data);

/**
 * MarkedString can be used to render human readable text. It is either a markdown string
 * or a code-block that provides a language and a code snippet. The language identifier
 * is semantically equal to the optional language identifier in fenced code blocks in GitHub
 * issues. See https://help.github.com/articles/creating-and-highlighting-code-blocks/#syntax-highlighting
 *
 * The pair of a language and a value is an equivalent to markdown:
 * ```${language}
 * ${value}
 * ```
 *
 * Note that markdown strings will be sanitized - that means html will be escaped.
 * @deprecated use MarkupContent instead.
 */
using MarkedString = std::variant<QString, MarkedStringWithLanguage>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MarkedString> fromJson<MarkedString>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const MarkedString &val);

using HoverContents = std::variant<MarkupContent, MarkedString, QList<MarkedString>>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<HoverContents> fromJson<HoverContents>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const HoverContents &val);

/** The result of a hover request. */
struct Hover {
    HoverContents _contents{};  //!< The hover's content
    /**
     * An optional range inside the text document that is used to
     * visualize the hover, e.g. by changing the background color.
     */
    std::optional<Range> _range{};

    Hover& contents(const HoverContents & v) { _contents = v; return *this; }
    Hover& range(const std::optional<Range> & v) { _range = v; return *this; }

    const HoverContents& contents() const { return _contents; }
    const std::optional<Range>& range() const { return _range; }

    bool operator==(const Hover &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Hover> fromJson<Hover>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const Hover &data);

/** Registration options for a {@link HoverRequest}. */
struct HoverRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};

    HoverRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    HoverRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const HoverRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<HoverRegistrationOptions> fromJson<HoverRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const HoverRegistrationOptions &data);

/**
 * Represents a parameter of a callable-signature. A parameter can
 * have a label and a doc-comment.
 */
struct ParameterInformation {
    /**
     * The label of this parameter information.
     *
     * Either a string or an inclusive start and exclusive end offsets within its containing
     * signature label. (see SignatureInformation.label). The offsets are based on a UTF-16
     * string representation as `Position` and `Range` does.
     *
     * To avoid ambiguities a server should use the [start, end] offset value instead of using
     * a substring. Whether a client support this is controlled via `labelOffsetSupport` client
     * capability.
     *
     * *Note*: a label of type string should be a substring of its containing signature label.
     * Its intended use case is to highlight the parameter label part in the `SignatureInformation.label`.
     */
    QString _label{};
    /**
     * The human-readable doc-comment of this parameter. Will be shown
     * in the UI but can be omitted.
     */
    std::optional<InlayHintLabelPartTooltip> _documentation{};

    ParameterInformation& label(const QString & v) { _label = v; return *this; }
    ParameterInformation& documentation(const std::optional<InlayHintLabelPartTooltip> & v) { _documentation = v; return *this; }

    const QString& label() const { return _label; }
    const std::optional<InlayHintLabelPartTooltip>& documentation() const { return _documentation; }

    bool operator==(const ParameterInformation &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ParameterInformation> fromJson<ParameterInformation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ParameterInformation &data);

/**
 * Represents the signature of something callable. A signature
 * can have a label, like a function-name, a doc-comment, and
 * a set of parameters.
 */
struct SignatureInformation {
    /**
     * The label of this signature. Will be shown in
     * the UI.
     */
    QString _label{};
    /**
     * The human-readable doc-comment of this signature. Will be shown
     * in the UI but can be omitted.
     */
    std::optional<InlayHintLabelPartTooltip> _documentation{};
    std::optional<QList<ParameterInformation>> _parameters{};  //!< The parameters of this signature.
    /**
     * The index of the active parameter.
     *
     * If `null`, no parameter of the signature is active (for example a named
     * argument that does not match any declared parameters). This is only valid
     * if the client specifies the client capability
     * `textDocument.signatureHelp.noActiveParameterSupport === true`
     *
     * If provided (or `null`), this is used in place of
     * `SignatureHelp.activeParameter`.
     *
     * @since 3.16.0
     */
    std::optional<int> _activeParameter{};

    SignatureInformation& label(const QString & v) { _label = v; return *this; }
    SignatureInformation& documentation(const std::optional<InlayHintLabelPartTooltip> & v) { _documentation = v; return *this; }
    SignatureInformation& parameters(const std::optional<QList<ParameterInformation>> & v) { _parameters = v; return *this; }
    SignatureInformation& addParameter(const ParameterInformation & v) { if (!_parameters) _parameters = QList<ParameterInformation>{}; (*_parameters).append(v); return *this; }
    SignatureInformation& activeParameter(std::optional<int> v) { _activeParameter = v; return *this; }

    const QString& label() const { return _label; }
    const std::optional<InlayHintLabelPartTooltip>& documentation() const { return _documentation; }
    const std::optional<QList<ParameterInformation>>& parameters() const { return _parameters; }
    const std::optional<int>& activeParameter() const { return _activeParameter; }

    bool operator==(const SignatureInformation &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SignatureInformation> fromJson<SignatureInformation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SignatureInformation &data);

/**
 * Signature help represents the signature of something
 * callable. There can be multiple signature but only one
 * active and only one active parameter.
 */
struct SignatureHelp {
    QList<SignatureInformation> _signatures{};  //!< One or more signatures.
    /**
     * The active signature. If omitted or the value lies outside the
     * range of `signatures` the value defaults to zero or is ignored if
     * the `SignatureHelp` has no signatures.
     *
     * Whenever possible implementors should make an active decision about
     * the active signature and shouldn't rely on a default value.
     *
     * In future version of the protocol this property might become
     * mandatory to better express this.
     */
    std::optional<int> _activeSignature{};
    /**
     * The active parameter of the active signature.
     *
     * If `null`, no parameter of the signature is active (for example a named
     * argument that does not match any declared parameters). This is only valid
     * if the client specifies the client capability
     * `textDocument.signatureHelp.noActiveParameterSupport === true`
     *
     * If omitted or the value lies outside the range of
     * `signatures[activeSignature].parameters` defaults to 0 if the active
     * signature has parameters.
     *
     * If the active signature has no parameters it is ignored.
     *
     * In future version of the protocol this property might become
     * mandatory (but still nullable) to better express the active parameter if
     * the active signature does have any.
     *
     * Since version 3.16.0 the `SignatureInformation` itself provides a
     * `activeParameter` property and it should be used instead of this one.
     */
    std::optional<int> _activeParameter{};

    SignatureHelp& signatures(const QList<SignatureInformation> & v) { _signatures = v; return *this; }
    SignatureHelp& addSignature(const SignatureInformation & v) { _signatures.append(v); return *this; }
    SignatureHelp& activeSignature(std::optional<int> v) { _activeSignature = v; return *this; }
    SignatureHelp& activeParameter(std::optional<int> v) { _activeParameter = v; return *this; }

    const QList<SignatureInformation>& signatures() const { return _signatures; }
    const std::optional<int>& activeSignature() const { return _activeSignature; }
    const std::optional<int>& activeParameter() const { return _activeParameter; }

    bool operator==(const SignatureHelp &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SignatureHelp> fromJson<SignatureHelp>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SignatureHelp &data);

/**
 * How a signature help was triggered.
 *
 * @since 3.15.0
 */
namespace SignatureHelpTriggerKind {
    constexpr int Invoked = 1;
    constexpr int TriggerCharacter = 2;
    constexpr int ContentChange = 3;
} // namespace SignatureHelpTriggerKind
/**
 * Additional information about the context in which a signature help request was triggered.
 *
 * @since 3.15.0
 */
struct SignatureHelpContext {
    int _triggerKind{};  //!< Action that caused signature help to be triggered.
    /**
     * Character that caused signature help to be triggered.
     *
     * This is undefined when `triggerKind !== SignatureHelpTriggerKind.TriggerCharacter`
     */
    std::optional<QString> _triggerCharacter{};
    /**
     * `true` if signature help was already showing when it was triggered.
     *
     * Retriggers occurs when the signature help is already active and can be caused by actions such as
     * typing a trigger character, a cursor move, or document content changes.
     */
    bool _isRetrigger{};
    /**
     * The currently active `SignatureHelp`.
     *
     * The `activeSignatureHelp` has its `SignatureHelp.activeSignature` field updated based on
     * the user navigating through available signatures.
     */
    std::optional<SignatureHelp> _activeSignatureHelp{};

    SignatureHelpContext& triggerKind(int v) { _triggerKind = v; return *this; }
    SignatureHelpContext& triggerCharacter(const std::optional<QString> & v) { _triggerCharacter = v; return *this; }
    SignatureHelpContext& isRetrigger(bool v) { _isRetrigger = v; return *this; }
    SignatureHelpContext& activeSignatureHelp(const std::optional<SignatureHelp> & v) { _activeSignatureHelp = v; return *this; }

    const int& triggerKind() const { return _triggerKind; }
    const std::optional<QString>& triggerCharacter() const { return _triggerCharacter; }
    const bool& isRetrigger() const { return _isRetrigger; }
    const std::optional<SignatureHelp>& activeSignatureHelp() const { return _activeSignatureHelp; }

    bool operator==(const SignatureHelpContext &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SignatureHelpContext> fromJson<SignatureHelpContext>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SignatureHelpContext &data);

/** Parameters for a {@link SignatureHelpRequest}. */
struct SignatureHelpParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * The signature help context. This is only available if the client specifies
     * to send this using the client capability `textDocument.signatureHelp.contextSupport === true`
     *
     * @since 3.15.0
     */
    std::optional<SignatureHelpContext> _context{};

    SignatureHelpParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    SignatureHelpParams& position(const Position & v) { _position = v; return *this; }
    SignatureHelpParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    SignatureHelpParams& context(const std::optional<SignatureHelpContext> & v) { _context = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<SignatureHelpContext>& context() const { return _context; }

    bool operator==(const SignatureHelpParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SignatureHelpParams> fromJson<SignatureHelpParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SignatureHelpParams &data);

/** Registration options for a {@link SignatureHelpRequest}. */
struct SignatureHelpRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    std::optional<QStringList> _triggerCharacters{};  //!< List of characters that trigger signature help automatically.
    /**
     * List of characters that re-trigger signature help.
     *
     * These trigger characters are only active when signature help is already showing. All trigger characters
     * are also counted as re-trigger characters.
     *
     * @since 3.15.0
     */
    std::optional<QStringList> _retriggerCharacters{};

    SignatureHelpRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    SignatureHelpRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    SignatureHelpRegistrationOptions& triggerCharacters(const std::optional<QStringList> & v) { _triggerCharacters = v; return *this; }
    SignatureHelpRegistrationOptions& addTriggerCharacter(const QString & v) { if (!_triggerCharacters) _triggerCharacters = QStringList{}; (*_triggerCharacters).append(v); return *this; }
    SignatureHelpRegistrationOptions& retriggerCharacters(const std::optional<QStringList> & v) { _retriggerCharacters = v; return *this; }
    SignatureHelpRegistrationOptions& addRetriggerCharacter(const QString & v) { if (!_retriggerCharacters) _retriggerCharacters = QStringList{}; (*_retriggerCharacters).append(v); return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QStringList>& triggerCharacters() const { return _triggerCharacters; }
    const std::optional<QStringList>& retriggerCharacters() const { return _retriggerCharacters; }

    bool operator==(const SignatureHelpRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SignatureHelpRegistrationOptions> fromJson<SignatureHelpRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SignatureHelpRegistrationOptions &data);

/** Parameters for a {@link DefinitionRequest}. */
struct DefinitionParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};

    DefinitionParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    DefinitionParams& position(const Position & v) { _position = v; return *this; }
    DefinitionParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    DefinitionParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }

    bool operator==(const DefinitionParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DefinitionParams> fromJson<DefinitionParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DefinitionParams &data);

/** Registration options for a {@link DefinitionRequest}. */
struct DefinitionRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};

    DefinitionRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    DefinitionRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const DefinitionRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DefinitionRegistrationOptions> fromJson<DefinitionRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DefinitionRegistrationOptions &data);

/**
 * Value-object that contains additional information when
 * requesting references.
 */
struct ReferenceContext {
    bool _includeDeclaration{};  //!< Include the declaration of the current symbol.

    ReferenceContext& includeDeclaration(bool v) { _includeDeclaration = v; return *this; }

    const bool& includeDeclaration() const { return _includeDeclaration; }

    bool operator==(const ReferenceContext &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ReferenceContext> fromJson<ReferenceContext>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ReferenceContext &data);

/** Parameters for a {@link ReferencesRequest}. */
struct ReferenceParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    ReferenceContext _context{};

    ReferenceParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    ReferenceParams& position(const Position & v) { _position = v; return *this; }
    ReferenceParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    ReferenceParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    ReferenceParams& context(const ReferenceContext & v) { _context = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const ReferenceContext& context() const { return _context; }

    bool operator==(const ReferenceParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ReferenceParams> fromJson<ReferenceParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ReferenceParams &data);

/** Registration options for a {@link ReferencesRequest}. */
struct ReferenceRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};

    ReferenceRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    ReferenceRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const ReferenceRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ReferenceRegistrationOptions> fromJson<ReferenceRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ReferenceRegistrationOptions &data);

/** Parameters for a {@link DocumentHighlightRequest}. */
struct DocumentHighlightParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};

    DocumentHighlightParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    DocumentHighlightParams& position(const Position & v) { _position = v; return *this; }
    DocumentHighlightParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    DocumentHighlightParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }

    bool operator==(const DocumentHighlightParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentHighlightParams> fromJson<DocumentHighlightParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentHighlightParams &data);

/** A document highlight kind. */
namespace DocumentHighlightKind {
    constexpr int Text = 1;
    constexpr int Read = 2;
    constexpr int Write = 3;
} // namespace DocumentHighlightKind
/**
 * A document highlight is a range inside a text document which deserves
 * special attention. Usually a document highlight is visualized by changing
 * the background color of its range.
 */
struct DocumentHighlight {
    Range _range{};  //!< The range this highlight applies to.
    std::optional<int> _kind{};  //!< The highlight kind, default is {@link DocumentHighlightKind.Text text}.

    DocumentHighlight& range(const Range & v) { _range = v; return *this; }
    DocumentHighlight& kind(std::optional<int> v) { _kind = v; return *this; }

    const Range& range() const { return _range; }
    const std::optional<int>& kind() const { return _kind; }

    bool operator==(const DocumentHighlight &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentHighlight> fromJson<DocumentHighlight>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentHighlight &data);

/** Registration options for a {@link DocumentHighlightRequest}. */
struct DocumentHighlightRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};

    DocumentHighlightRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    DocumentHighlightRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const DocumentHighlightRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentHighlightRegistrationOptions> fromJson<DocumentHighlightRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentHighlightRegistrationOptions &data);

/** Parameters for a {@link DocumentSymbolRequest}. */
struct DocumentSymbolParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The text document.

    DocumentSymbolParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    DocumentSymbolParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    DocumentSymbolParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }

    bool operator==(const DocumentSymbolParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentSymbolParams> fromJson<DocumentSymbolParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentSymbolParams &data);

/**
 * Represents information about programming constructs like variables, classes,
 * interfaces etc.
 */
struct SymbolInformation {
    QString _name{};  //!< The name of this symbol.
    int _kind{};  //!< The kind of this symbol.
    /**
     * Tags for this symbol.
     *
     * @since 3.16.0
     */
    std::optional<QList<int>> _tags{};
    /**
     * The name of the symbol containing this symbol. This information is for
     * user interface purposes (e.g. to render a qualifier in the user interface
     * if necessary). It can't be used to re-infer a hierarchy for the document
     * symbols.
     */
    std::optional<QString> _containerName{};
    /**
     * Indicates if this symbol is deprecated.
     *
     * @deprecated Use tags instead
     */
    std::optional<bool> _deprecated{};
    /**
     * The location of this symbol. The location's range is used by a tool
     * to reveal the location in the editor. If the symbol is selected in the
     * tool the range's start information is used to position the cursor. So
     * the range usually spans more than the actual symbol's name and does
     * normally include things like visibility modifiers.
     *
     * The range doesn't have to denote a node range in the sense of an abstract
     * syntax tree. It can therefore not be used to re-construct a hierarchy of
     * the symbols.
     */
    Location _location{};

    SymbolInformation& name(const QString & v) { _name = v; return *this; }
    SymbolInformation& kind(int v) { _kind = v; return *this; }
    SymbolInformation& tags(const std::optional<QList<int>> & v) { _tags = v; return *this; }
    SymbolInformation& addTag(int v) { if (!_tags) _tags = QList<int>{}; (*_tags).append(v); return *this; }
    SymbolInformation& containerName(const std::optional<QString> & v) { _containerName = v; return *this; }
    SymbolInformation& deprecated(std::optional<bool> v) { _deprecated = v; return *this; }
    SymbolInformation& location(const Location & v) { _location = v; return *this; }

    const QString& name() const { return _name; }
    const int& kind() const { return _kind; }
    const std::optional<QList<int>>& tags() const { return _tags; }
    const std::optional<QString>& containerName() const { return _containerName; }
    const std::optional<bool>& deprecated() const { return _deprecated; }
    const Location& location() const { return _location; }

    bool operator==(const SymbolInformation &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SymbolInformation> fromJson<SymbolInformation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SymbolInformation &data);

/**
 * Represents programming constructs like variables, classes, interfaces etc.
 * that appear in a document. Document symbols can be hierarchical and they
 * have two ranges: one that encloses its definition and one that points to
 * its most interesting range, e.g. the range of an identifier.
 */
struct DocumentSymbol {
    /**
     * The name of this symbol. Will be displayed in the user interface and therefore must not be
     * an empty string or a string only consisting of white spaces.
     */
    QString _name{};
    std::optional<QString> _detail{};  //!< More detail for this symbol, e.g the signature of a function.
    int _kind{};  //!< The kind of this symbol.
    /**
     * Tags for this document symbol.
     *
     * @since 3.16.0
     */
    std::optional<QList<int>> _tags{};
    /**
     * Indicates if this symbol is deprecated.
     *
     * @deprecated Use tags instead
     */
    std::optional<bool> _deprecated{};
    /**
     * The range enclosing this symbol not including leading/trailing whitespace but everything else
     * like comments. This information is typically used to determine if the clients cursor is
     * inside the symbol to reveal in the symbol in the UI.
     */
    Range _range{};
    /**
     * The range that should be selected and revealed when this symbol is being picked, e.g the name of a function.
     * Must be contained by the `range`.
     */
    Range _selectionRange{};
    std::optional<QList<DocumentSymbol>> _children{};  //!< Children of this symbol, e.g. properties of a class.

    DocumentSymbol& name(const QString & v) { _name = v; return *this; }
    DocumentSymbol& detail(const std::optional<QString> & v) { _detail = v; return *this; }
    DocumentSymbol& kind(int v) { _kind = v; return *this; }
    DocumentSymbol& tags(const std::optional<QList<int>> & v) { _tags = v; return *this; }
    DocumentSymbol& addTag(int v) { if (!_tags) _tags = QList<int>{}; (*_tags).append(v); return *this; }
    DocumentSymbol& deprecated(std::optional<bool> v) { _deprecated = v; return *this; }
    DocumentSymbol& range(const Range & v) { _range = v; return *this; }
    DocumentSymbol& selectionRange(const Range & v) { _selectionRange = v; return *this; }
    DocumentSymbol& children(const std::optional<QList<DocumentSymbol>> & v) { _children = v; return *this; }
    DocumentSymbol& addChildren(const DocumentSymbol & v) { if (!_children) _children = QList<DocumentSymbol>{}; (*_children).append(v); return *this; }

    const QString& name() const { return _name; }
    const std::optional<QString>& detail() const { return _detail; }
    const int& kind() const { return _kind; }
    const std::optional<QList<int>>& tags() const { return _tags; }
    const std::optional<bool>& deprecated() const { return _deprecated; }
    const Range& range() const { return _range; }
    const Range& selectionRange() const { return _selectionRange; }
    const std::optional<QList<DocumentSymbol>>& children() const { return _children; }

    LANGUAGESERVERPROTOCOL_EXPORT bool operator==(const DocumentSymbol &other) const;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentSymbol> fromJson<DocumentSymbol>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentSymbol &data);

/** Registration options for a {@link DocumentSymbolRequest}. */
struct DocumentSymbolRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * A human-readable string that is shown when multiple outlines trees
     * are shown for the same document.
     *
     * @since 3.16.0
     */
    std::optional<QString> _label{};

    DocumentSymbolRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    DocumentSymbolRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    DocumentSymbolRegistrationOptions& label(const std::optional<QString> & v) { _label = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QString>& label() const { return _label; }

    bool operator==(const DocumentSymbolRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentSymbolRegistrationOptions> fromJson<DocumentSymbolRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentSymbolRegistrationOptions &data);

/**
 * The reason why code actions were requested.
 *
 * @since 3.17.0
 */
namespace CodeActionTriggerKind {
    constexpr int Invoked = 1;
    constexpr int Automatic = 2;
} // namespace CodeActionTriggerKind
/**
 * Contains additional diagnostic information about the context in which
 * a {@link CodeActionProvider.provideCodeActions code action} is run.
 */
struct CodeActionContext {
    /**
     * An array of diagnostics known on the client side overlapping the range provided to the
     * `textDocument/codeAction` request. They are provided so that the server knows which
     * errors are currently presented to the user for the given range. There is no guarantee
     * that these accurately reflect the error state of the resource. The primary parameter
     * to compute code actions is the provided range.
     */
    QList<Diagnostic> _diagnostics{};
    /**
     * Requested kind of actions to return.
     *
     * Actions not of this kind are filtered out by the client before being shown. So servers
     * can omit computing them.
     */
    std::optional<QList<CodeActionKind>> _only{};
    /**
     * The reason why code actions were requested.
     *
     * @since 3.17.0
     */
    std::optional<int> _triggerKind{};

    CodeActionContext& diagnostics(const QList<Diagnostic> & v) { _diagnostics = v; return *this; }
    CodeActionContext& addDiagnostic(const Diagnostic & v) { _diagnostics.append(v); return *this; }
    CodeActionContext& only(const std::optional<QList<CodeActionKind>> & v) { _only = v; return *this; }
    CodeActionContext& addOnly(const CodeActionKind & v) { if (!_only) _only = QList<CodeActionKind>{}; (*_only).append(v); return *this; }
    CodeActionContext& triggerKind(std::optional<int> v) { _triggerKind = v; return *this; }

    const QList<Diagnostic>& diagnostics() const { return _diagnostics; }
    const std::optional<QList<CodeActionKind>>& only() const { return _only; }
    const std::optional<int>& triggerKind() const { return _triggerKind; }

    bool operator==(const CodeActionContext &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeActionContext> fromJson<CodeActionContext>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeActionContext &data);

/** The parameters of a {@link CodeActionRequest}. */
struct CodeActionParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The document in which the command was invoked.
    Range _range{};  //!< The range for which the command was invoked.
    CodeActionContext _context{};  //!< Context carrying additional information.

    CodeActionParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    CodeActionParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    CodeActionParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    CodeActionParams& range(const Range & v) { _range = v; return *this; }
    CodeActionParams& context(const CodeActionContext & v) { _context = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Range& range() const { return _range; }
    const CodeActionContext& context() const { return _context; }

    bool operator==(const CodeActionParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeActionParams> fromJson<CodeActionParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeActionParams &data);

/**
 * Captures why the code action is currently disabled.
 *
 * @since 3.18.0
 */
struct CodeActionDisabled {
    /**
     * Human readable description of why the code action is currently disabled.
     *
     * This is displayed in the code actions UI.
     */
    QString _reason{};

    CodeActionDisabled& reason(const QString & v) { _reason = v; return *this; }

    const QString& reason() const { return _reason; }

    bool operator==(const CodeActionDisabled &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeActionDisabled> fromJson<CodeActionDisabled>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeActionDisabled &data);

/**
 * A code action represents a change that can be performed in code, e.g. to fix a problem or
 * to refactor code.
 *
 * A CodeAction must set either `edit` and/or a `command`. If both are supplied, the `edit` is applied first, then the `command` is executed.
 */
struct CodeAction {
    QString _title{};  //!< A short, human-readable, title for this code action.
    /**
     * The kind of the code action.
     *
     * Used to filter code actions.
     */
    std::optional<CodeActionKind> _kind{};
    std::optional<QList<Diagnostic>> _diagnostics{};  //!< The diagnostics that this code action resolves.
    /**
     * Marks this as a preferred action. Preferred actions are used by the `auto fix` command and can be targeted
     * by keybindings.
     *
     * A quick fix should be marked preferred if it properly addresses the underlying error.
     * A refactoring should be marked preferred if it is the most reasonable choice of actions to take.
     *
     * @since 3.15.0
     */
    std::optional<bool> _isPreferred{};
    /**
     * Marks that the code action cannot currently be applied.
     *
     * Clients should follow the following guidelines regarding disabled code actions:
     *
     * - Disabled code actions are not shown in automatic [lightbulbs](https://code.visualstudio.com/docs/editor/editingevolved#_code-action)
     * code action menus.
     *
     * - Disabled actions are shown as faded out in the code action menu when the user requests a more specific type
     * of code action, such as refactorings.
     *
     * - If the user has a [keybinding](https://code.visualstudio.com/docs/editor/refactoring#_keybindings-for-code-actions)
     * that auto applies a code action and only disabled code actions are returned, the client should show the user an
     * error message with `reason` in the editor.
     *
     * @since 3.16.0
     */
    std::optional<CodeActionDisabled> _disabled{};
    std::optional<WorkspaceEdit> _edit{};  //!< The workspace edit this code action performs.
    /**
     * A command this code action executes. If a code action
     * provides an edit and a command, first the edit is
     * executed and then the command.
     */
    std::optional<Command> _command{};
    /**
     * A data entry field that is preserved on a code action between
     * a `textDocument/codeAction` and a `codeAction/resolve` request.
     *
     * @since 3.16.0
     */
    std::optional<QJsonValue> _data{};
    /**
     * Tags for this code action.
     *
     * @since 3.18.0
     */
    std::optional<QList<int>> _tags{};

    CodeAction& title(const QString & v) { _title = v; return *this; }
    CodeAction& kind(const std::optional<CodeActionKind> & v) { _kind = v; return *this; }
    CodeAction& diagnostics(const std::optional<QList<Diagnostic>> & v) { _diagnostics = v; return *this; }
    CodeAction& addDiagnostic(const Diagnostic & v) { if (!_diagnostics) _diagnostics = QList<Diagnostic>{}; (*_diagnostics).append(v); return *this; }
    CodeAction& isPreferred(std::optional<bool> v) { _isPreferred = v; return *this; }
    CodeAction& disabled(const std::optional<CodeActionDisabled> & v) { _disabled = v; return *this; }
    CodeAction& edit(const std::optional<WorkspaceEdit> & v) { _edit = v; return *this; }
    CodeAction& command(const std::optional<Command> & v) { _command = v; return *this; }
    CodeAction& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }
    CodeAction& tags(const std::optional<QList<int>> & v) { _tags = v; return *this; }
    CodeAction& addTag(int v) { if (!_tags) _tags = QList<int>{}; (*_tags).append(v); return *this; }

    const QString& title() const { return _title; }
    const std::optional<CodeActionKind>& kind() const { return _kind; }
    const std::optional<QList<Diagnostic>>& diagnostics() const { return _diagnostics; }
    const std::optional<bool>& isPreferred() const { return _isPreferred; }
    const std::optional<CodeActionDisabled>& disabled() const { return _disabled; }
    const std::optional<WorkspaceEdit>& edit() const { return _edit; }
    const std::optional<Command>& command() const { return _command; }
    const std::optional<QJsonValue>& data() const { return _data; }
    const std::optional<QList<int>>& tags() const { return _tags; }

    bool operator==(const CodeAction &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeAction> fromJson<CodeAction>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeAction &data);

/** Registration options for a {@link CodeActionRequest}. */
struct CodeActionRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * CodeActionKinds that this server may return.
     *
     * The list of kinds may be generic, such as `CodeActionKind.Refactor`, or the server
     * may list out every specific kind they provide.
     */
    std::optional<QList<CodeActionKind>> _codeActionKinds{};
    /**
     * Static documentation for a class of code actions.
     *
     * Documentation from the provider should be shown in the code actions menu if either:
     *
     * - Code actions of `kind` are requested by the editor. In this case, the editor will show the documentation that
     * most closely matches the requested code action kind. For example, if a provider has documentation for
     * both `Refactor` and `RefactorExtract`, when the user requests code actions for `RefactorExtract`,
     * the editor will use the documentation for `RefactorExtract` instead of the documentation for `Refactor`.
     *
     * - Any code actions of `kind` are returned by the provider.
     *
     * At most one documentation entry should be shown per provider.
     *
     * @since 3.18.0
     */
    std::optional<QList<CodeActionKindDocumentation>> _documentation{};
    /**
     * The server provides support to resolve additional
     * information for a code action.
     *
     * @since 3.16.0
     */
    std::optional<bool> _resolveProvider{};

    CodeActionRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    CodeActionRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    CodeActionRegistrationOptions& codeActionKinds(const std::optional<QList<CodeActionKind>> & v) { _codeActionKinds = v; return *this; }
    CodeActionRegistrationOptions& addCodeActionKind(const CodeActionKind & v) { if (!_codeActionKinds) _codeActionKinds = QList<CodeActionKind>{}; (*_codeActionKinds).append(v); return *this; }
    CodeActionRegistrationOptions& documentation(const std::optional<QList<CodeActionKindDocumentation>> & v) { _documentation = v; return *this; }
    CodeActionRegistrationOptions& addDocumentation(const CodeActionKindDocumentation & v) { if (!_documentation) _documentation = QList<CodeActionKindDocumentation>{}; (*_documentation).append(v); return *this; }
    CodeActionRegistrationOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<QList<CodeActionKind>>& codeActionKinds() const { return _codeActionKinds; }
    const std::optional<QList<CodeActionKindDocumentation>>& documentation() const { return _documentation; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }

    bool operator==(const CodeActionRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeActionRegistrationOptions> fromJson<CodeActionRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeActionRegistrationOptions &data);

/** The parameters of a {@link WorkspaceSymbolRequest}. */
struct WorkspaceSymbolParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    /**
     * A query string to filter symbols by. Clients may send an empty
     * string here to request all symbols.
     *
     * The `query`-parameter should be interpreted in a *relaxed way* as editors
     * will apply their own highlighting and scoring on the results. A good rule
     * of thumb is to match case-insensitive and to simply check that the
     * characters of *query* appear in their order in a candidate symbol.
     * Servers shouldn't use prefix, substring, or similar strict matching.
     */
    QString _query{};

    WorkspaceSymbolParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    WorkspaceSymbolParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    WorkspaceSymbolParams& query(const QString & v) { _query = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const QString& query() const { return _query; }

    bool operator==(const WorkspaceSymbolParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceSymbolParams> fromJson<WorkspaceSymbolParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceSymbolParams &data);

/**
 * Location with only uri and does not include range.
 *
 * @since 3.18.0
 */
struct LocationUriOnly {
    QString _uri{};

    LocationUriOnly& uri(const QString & v) { _uri = v; return *this; }

    const QString& uri() const { return _uri; }

    bool operator==(const LocationUriOnly &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<LocationUriOnly> fromJson<LocationUriOnly>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const LocationUriOnly &data);

using WorkspaceSymbolLocation = std::variant<Location, LocationUriOnly>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceSymbolLocation> fromJson<WorkspaceSymbolLocation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const WorkspaceSymbolLocation &val);

/**
 * A special workspace symbol that supports locations without a range.
 *
 * See also SymbolInformation.
 *
 * @since 3.17.0
 */
struct WorkspaceSymbol {
    QString _name{};  //!< The name of this symbol.
    int _kind{};  //!< The kind of this symbol.
    /**
     * Tags for this symbol.
     *
     * @since 3.16.0
     */
    std::optional<QList<int>> _tags{};
    /**
     * The name of the symbol containing this symbol. This information is for
     * user interface purposes (e.g. to render a qualifier in the user interface
     * if necessary). It can't be used to re-infer a hierarchy for the document
     * symbols.
     */
    std::optional<QString> _containerName{};
    /**
     * The location of the symbol. Whether a server is allowed to
     * return a location without a range depends on the client
     * capability `workspace.symbol.resolveSupport`.
     *
     * See SymbolInformation#location for more details.
     */
    WorkspaceSymbolLocation _location{};
    /**
     * A data entry field that is preserved on a workspace symbol between a
     * workspace symbol request and a workspace symbol resolve request.
     */
    std::optional<QJsonValue> _data{};

    WorkspaceSymbol& name(const QString & v) { _name = v; return *this; }
    WorkspaceSymbol& kind(int v) { _kind = v; return *this; }
    WorkspaceSymbol& tags(const std::optional<QList<int>> & v) { _tags = v; return *this; }
    WorkspaceSymbol& addTag(int v) { if (!_tags) _tags = QList<int>{}; (*_tags).append(v); return *this; }
    WorkspaceSymbol& containerName(const std::optional<QString> & v) { _containerName = v; return *this; }
    WorkspaceSymbol& location(const WorkspaceSymbolLocation & v) { _location = v; return *this; }
    WorkspaceSymbol& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const QString& name() const { return _name; }
    const int& kind() const { return _kind; }
    const std::optional<QList<int>>& tags() const { return _tags; }
    const std::optional<QString>& containerName() const { return _containerName; }
    const WorkspaceSymbolLocation& location() const { return _location; }
    const std::optional<QJsonValue>& data() const { return _data; }

    bool operator==(const WorkspaceSymbol &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceSymbol> fromJson<WorkspaceSymbol>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceSymbol &data);

/** Registration options for a {@link WorkspaceSymbolRequest}. */
struct WorkspaceSymbolRegistrationOptions {
    std::optional<bool> _workDoneProgress{};
    /**
     * The server provides support to resolve additional
     * information for a workspace symbol.
     *
     * @since 3.17.0
     */
    std::optional<bool> _resolveProvider{};

    WorkspaceSymbolRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    WorkspaceSymbolRegistrationOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }

    bool operator==(const WorkspaceSymbolRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceSymbolRegistrationOptions> fromJson<WorkspaceSymbolRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceSymbolRegistrationOptions &data);

/** The parameters of a {@link CodeLensRequest}. */
struct CodeLensParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The document to request code lens for.

    CodeLensParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    CodeLensParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    CodeLensParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }

    bool operator==(const CodeLensParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeLensParams> fromJson<CodeLensParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeLensParams &data);

/**
 * A code lens represents a {@link Command command} that should be shown along with
 * source text, like the number of references, a way to run tests, etc.
 *
 * A code lens is _unresolved_ when no command is associated to it. For performance
 * reasons the creation of a code lens and resolving should be done in two stages.
 */
struct CodeLens {
    Range _range{};  //!< The range in which this code lens is valid. Should only span a single line.
    std::optional<Command> _command{};  //!< The command this code lens represents.
    /**
     * A data entry field that is preserved on a code lens item between
     * a {@link CodeLensRequest} and a {@link CodeLensResolveRequest}
     */
    std::optional<QJsonValue> _data{};

    CodeLens& range(const Range & v) { _range = v; return *this; }
    CodeLens& command(const std::optional<Command> & v) { _command = v; return *this; }
    CodeLens& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const Range& range() const { return _range; }
    const std::optional<Command>& command() const { return _command; }
    const std::optional<QJsonValue>& data() const { return _data; }

    bool operator==(const CodeLens &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeLens> fromJson<CodeLens>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeLens &data);

/** Registration options for a {@link CodeLensRequest}. */
struct CodeLensRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    std::optional<bool> _resolveProvider{};  //!< Code lens has a resolve provider as well.

    CodeLensRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    CodeLensRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    CodeLensRegistrationOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }

    bool operator==(const CodeLensRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeLensRegistrationOptions> fromJson<CodeLensRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CodeLensRegistrationOptions &data);

/** The parameters of a {@link DocumentLinkRequest}. */
struct DocumentLinkParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};
    TextDocumentIdentifier _textDocument{};  //!< The document to provide document links for.

    DocumentLinkParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    DocumentLinkParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }
    DocumentLinkParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }

    bool operator==(const DocumentLinkParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentLinkParams> fromJson<DocumentLinkParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentLinkParams &data);

/**
 * A document link is a range in a text document that links to an internal or external resource, like another
 * text document or a web site.
 */
struct DocumentLink {
    Range _range{};  //!< The range this link applies to.
    std::optional<QString> _target{};  //!< The uri this link points to. If missing a resolve request is sent later.
    /**
     * The tooltip text when you hover over this link.
     *
     * If a tooltip is provided, is will be displayed in a string that includes instructions on how to
     * trigger the link, such as `{0} (ctrl + click)`. The specific instructions vary depending on OS,
     * user settings, and localization.
     *
     * @since 3.15.0
     */
    std::optional<QString> _tooltip{};
    /**
     * A data entry field that is preserved on a document link between a
     * DocumentLinkRequest and a DocumentLinkResolveRequest.
     */
    std::optional<QJsonValue> _data{};

    DocumentLink& range(const Range & v) { _range = v; return *this; }
    DocumentLink& target(const std::optional<QString> & v) { _target = v; return *this; }
    DocumentLink& tooltip(const std::optional<QString> & v) { _tooltip = v; return *this; }
    DocumentLink& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const Range& range() const { return _range; }
    const std::optional<QString>& target() const { return _target; }
    const std::optional<QString>& tooltip() const { return _tooltip; }
    const std::optional<QJsonValue>& data() const { return _data; }

    bool operator==(const DocumentLink &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentLink> fromJson<DocumentLink>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentLink &data);

/** Registration options for a {@link DocumentLinkRequest}. */
struct DocumentLinkRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    std::optional<bool> _resolveProvider{};  //!< Document links have a resolve provider as well.

    DocumentLinkRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    DocumentLinkRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    DocumentLinkRegistrationOptions& resolveProvider(std::optional<bool> v) { _resolveProvider = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& resolveProvider() const { return _resolveProvider; }

    bool operator==(const DocumentLinkRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentLinkRegistrationOptions> fromJson<DocumentLinkRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentLinkRegistrationOptions &data);

/** Value-object describing what options formatting should use. */
struct FormattingOptions {
    int _tabSize{};  //!< Size of a tab in spaces.
    bool _insertSpaces{};  //!< Prefer spaces over tabs.
    /**
     * Trim trailing whitespace on a line.
     *
     * @since 3.15.0
     */
    std::optional<bool> _trimTrailingWhitespace{};
    /**
     * Insert a newline character at the end of the file if one does not exist.
     *
     * @since 3.15.0
     */
    std::optional<bool> _insertFinalNewline{};
    /**
     * Trim all newlines after the final newline at the end of the file.
     *
     * @since 3.15.0
     */
    std::optional<bool> _trimFinalNewlines{};

    FormattingOptions& tabSize(int v) { _tabSize = v; return *this; }
    FormattingOptions& insertSpaces(bool v) { _insertSpaces = v; return *this; }
    FormattingOptions& trimTrailingWhitespace(std::optional<bool> v) { _trimTrailingWhitespace = v; return *this; }
    FormattingOptions& insertFinalNewline(std::optional<bool> v) { _insertFinalNewline = v; return *this; }
    FormattingOptions& trimFinalNewlines(std::optional<bool> v) { _trimFinalNewlines = v; return *this; }

    const int& tabSize() const { return _tabSize; }
    const bool& insertSpaces() const { return _insertSpaces; }
    const std::optional<bool>& trimTrailingWhitespace() const { return _trimTrailingWhitespace; }
    const std::optional<bool>& insertFinalNewline() const { return _insertFinalNewline; }
    const std::optional<bool>& trimFinalNewlines() const { return _trimFinalNewlines; }

    bool operator==(const FormattingOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FormattingOptions> fromJson<FormattingOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FormattingOptions &data);

/** The parameters of a {@link DocumentFormattingRequest}. */
struct DocumentFormattingParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    TextDocumentIdentifier _textDocument{};  //!< The document to format.
    FormattingOptions _options{};  //!< The format options.

    DocumentFormattingParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    DocumentFormattingParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    DocumentFormattingParams& options(const FormattingOptions & v) { _options = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const FormattingOptions& options() const { return _options; }

    bool operator==(const DocumentFormattingParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentFormattingParams> fromJson<DocumentFormattingParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentFormattingParams &data);

/** Registration options for a {@link DocumentFormattingRequest}. */
struct DocumentFormattingRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};

    DocumentFormattingRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    DocumentFormattingRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const DocumentFormattingRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentFormattingRegistrationOptions> fromJson<DocumentFormattingRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentFormattingRegistrationOptions &data);

/** The parameters of a {@link DocumentRangeFormattingRequest}. */
struct DocumentRangeFormattingParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    TextDocumentIdentifier _textDocument{};  //!< The document to format.
    Range _range{};  //!< The range to format
    FormattingOptions _options{};  //!< The format options

    DocumentRangeFormattingParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    DocumentRangeFormattingParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    DocumentRangeFormattingParams& range(const Range & v) { _range = v; return *this; }
    DocumentRangeFormattingParams& options(const FormattingOptions & v) { _options = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Range& range() const { return _range; }
    const FormattingOptions& options() const { return _options; }

    bool operator==(const DocumentRangeFormattingParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentRangeFormattingParams> fromJson<DocumentRangeFormattingParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentRangeFormattingParams &data);

/** Registration options for a {@link DocumentRangeFormattingRequest}. */
struct DocumentRangeFormattingRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * Whether the server supports formatting multiple ranges at once.
     *
     * @since 3.18.0
     */
    std::optional<bool> _rangesSupport{};

    DocumentRangeFormattingRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    DocumentRangeFormattingRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    DocumentRangeFormattingRegistrationOptions& rangesSupport(std::optional<bool> v) { _rangesSupport = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& rangesSupport() const { return _rangesSupport; }

    bool operator==(const DocumentRangeFormattingRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentRangeFormattingRegistrationOptions> fromJson<DocumentRangeFormattingRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentRangeFormattingRegistrationOptions &data);

/**
 * The parameters of a {@link DocumentRangesFormattingRequest}.
 *
 * @since 3.18.0
 */
struct DocumentRangesFormattingParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    TextDocumentIdentifier _textDocument{};  //!< The document to format.
    QList<Range> _ranges{};  //!< The ranges to format
    FormattingOptions _options{};  //!< The format options

    DocumentRangesFormattingParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    DocumentRangesFormattingParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    DocumentRangesFormattingParams& ranges(const QList<Range> & v) { _ranges = v; return *this; }
    DocumentRangesFormattingParams& addRange(const Range & v) { _ranges.append(v); return *this; }
    DocumentRangesFormattingParams& options(const FormattingOptions & v) { _options = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const QList<Range>& ranges() const { return _ranges; }
    const FormattingOptions& options() const { return _options; }

    bool operator==(const DocumentRangesFormattingParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentRangesFormattingParams> fromJson<DocumentRangesFormattingParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentRangesFormattingParams &data);

/** The parameters of a {@link DocumentOnTypeFormattingRequest}. */
struct DocumentOnTypeFormattingParams {
    TextDocumentIdentifier _textDocument{};  //!< The document to format.
    /**
     * The position around which the on type formatting should happen.
     * This is not necessarily the exact position where the character denoted
     * by the property `ch` got typed.
     */
    Position _position{};
    /**
     * The character that has been typed that triggered the formatting
     * on type request. That is not necessarily the last character that
     * got inserted into the document since the client could auto insert
     * characters as well (e.g. like automatic brace completion).
     */
    QString _ch{};
    FormattingOptions _options{};  //!< The formatting options.

    DocumentOnTypeFormattingParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    DocumentOnTypeFormattingParams& position(const Position & v) { _position = v; return *this; }
    DocumentOnTypeFormattingParams& ch(const QString & v) { _ch = v; return *this; }
    DocumentOnTypeFormattingParams& options(const FormattingOptions & v) { _options = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const QString& ch() const { return _ch; }
    const FormattingOptions& options() const { return _options; }

    bool operator==(const DocumentOnTypeFormattingParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentOnTypeFormattingParams> fromJson<DocumentOnTypeFormattingParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentOnTypeFormattingParams &data);

/** Registration options for a {@link DocumentOnTypeFormattingRequest}. */
struct DocumentOnTypeFormattingRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    QString _firstTriggerCharacter{};  //!< A character on which formatting should be triggered, like `{`.
    std::optional<QStringList> _moreTriggerCharacter{};  //!< More trigger characters.

    DocumentOnTypeFormattingRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    DocumentOnTypeFormattingRegistrationOptions& firstTriggerCharacter(const QString & v) { _firstTriggerCharacter = v; return *this; }
    DocumentOnTypeFormattingRegistrationOptions& moreTriggerCharacter(const std::optional<QStringList> & v) { _moreTriggerCharacter = v; return *this; }
    DocumentOnTypeFormattingRegistrationOptions& addMoreTriggerCharacter(const QString & v) { if (!_moreTriggerCharacter) _moreTriggerCharacter = QStringList{}; (*_moreTriggerCharacter).append(v); return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const QString& firstTriggerCharacter() const { return _firstTriggerCharacter; }
    const std::optional<QStringList>& moreTriggerCharacter() const { return _moreTriggerCharacter; }

    bool operator==(const DocumentOnTypeFormattingRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentOnTypeFormattingRegistrationOptions> fromJson<DocumentOnTypeFormattingRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentOnTypeFormattingRegistrationOptions &data);

/** The parameters of a {@link RenameRequest}. */
struct RenameParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * The new name of the symbol. If the given name is not valid the
     * request must return a {@link ResponseError} with an
     * appropriate message set.
     */
    QString _newName{};

    RenameParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    RenameParams& position(const Position & v) { _position = v; return *this; }
    RenameParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    RenameParams& newName(const QString & v) { _newName = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const QString& newName() const { return _newName; }

    bool operator==(const RenameParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RenameParams> fromJson<RenameParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RenameParams &data);

/** Registration options for a {@link RenameRequest}. */
struct RenameRegistrationOptions {
    /**
     * A document selector to identify the scope of the registration. If set to null
     * the document selector provided on the client side will be used.
     */
    std::optional<DocumentSelector> _documentSelector{};
    std::optional<bool> _workDoneProgress{};
    /**
     * Renames should be checked and tested before being executed.
     *
     * @since version 3.12.0
     */
    std::optional<bool> _prepareProvider{};

    RenameRegistrationOptions& documentSelector(const DocumentSelector & v) { _documentSelector = v; return *this; }
    RenameRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    RenameRegistrationOptions& prepareProvider(std::optional<bool> v) { _prepareProvider = v; return *this; }

    const std::optional<DocumentSelector>& documentSelector() const { return _documentSelector; }
    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const std::optional<bool>& prepareProvider() const { return _prepareProvider; }

    bool operator==(const RenameRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RenameRegistrationOptions> fromJson<RenameRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RenameRegistrationOptions &data);

struct PrepareRenameParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.

    PrepareRenameParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    PrepareRenameParams& position(const Position & v) { _position = v; return *this; }
    PrepareRenameParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }
    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }

    bool operator==(const PrepareRenameParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<PrepareRenameParams> fromJson<PrepareRenameParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const PrepareRenameParams &data);

/** The parameters of a {@link ExecuteCommandRequest}. */
struct ExecuteCommandParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    QString _command{};  //!< The identifier of the actual command handler.
    std::optional<QList<QJsonValue>> _arguments{};  //!< Arguments that the command should be invoked with.

    ExecuteCommandParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    ExecuteCommandParams& command(const QString & v) { _command = v; return *this; }
    ExecuteCommandParams& arguments(const std::optional<QList<QJsonValue>> & v) { _arguments = v; return *this; }
    ExecuteCommandParams& addArgument(const QJsonValue & v) { if (!_arguments) _arguments = QList<QJsonValue>{}; (*_arguments).append(v); return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const QString& command() const { return _command; }
    const std::optional<QList<QJsonValue>>& arguments() const { return _arguments; }

    bool operator==(const ExecuteCommandParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ExecuteCommandParams> fromJson<ExecuteCommandParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ExecuteCommandParams &data);

/** Registration options for a {@link ExecuteCommandRequest}. */
struct ExecuteCommandRegistrationOptions {
    std::optional<bool> _workDoneProgress{};
    QStringList _commands{};  //!< The commands to be executed on the server

    ExecuteCommandRegistrationOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }
    ExecuteCommandRegistrationOptions& commands(const QStringList & v) { _commands = v; return *this; }
    ExecuteCommandRegistrationOptions& addCommand(const QString & v) { _commands.append(v); return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }
    const QStringList& commands() const { return _commands; }

    bool operator==(const ExecuteCommandRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ExecuteCommandRegistrationOptions> fromJson<ExecuteCommandRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ExecuteCommandRegistrationOptions &data);

/**
 * Additional data about a workspace edit.
 *
 * @since 3.18.0
 */
struct WorkspaceEditMetadata {
    std::optional<bool> _isRefactoring{};  //!< Signal to the editor that this edit is a refactoring.

    WorkspaceEditMetadata& isRefactoring(std::optional<bool> v) { _isRefactoring = v; return *this; }

    const std::optional<bool>& isRefactoring() const { return _isRefactoring; }

    bool operator==(const WorkspaceEditMetadata &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceEditMetadata> fromJson<WorkspaceEditMetadata>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceEditMetadata &data);

/** The parameters passed via an apply workspace edit request. */
struct ApplyWorkspaceEditParams {
    /**
     * An optional label of the workspace edit. This label is
     * presented in the user interface for example on an undo
     * stack to undo the workspace edit.
     */
    std::optional<QString> _label{};
    WorkspaceEdit _edit{};  //!< The edits to apply.
    /**
     * Additional data about the edit.
     *
     * @since 3.18.0
     */
    std::optional<WorkspaceEditMetadata> _metadata{};

    ApplyWorkspaceEditParams& label(const std::optional<QString> & v) { _label = v; return *this; }
    ApplyWorkspaceEditParams& edit(const WorkspaceEdit & v) { _edit = v; return *this; }
    ApplyWorkspaceEditParams& metadata(const std::optional<WorkspaceEditMetadata> & v) { _metadata = v; return *this; }

    const std::optional<QString>& label() const { return _label; }
    const WorkspaceEdit& edit() const { return _edit; }
    const std::optional<WorkspaceEditMetadata>& metadata() const { return _metadata; }

    bool operator==(const ApplyWorkspaceEditParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ApplyWorkspaceEditParams> fromJson<ApplyWorkspaceEditParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ApplyWorkspaceEditParams &data);

/**
 * The result returned from the apply workspace edit request.
 *
 * @since 3.17 renamed from ApplyWorkspaceEditResponse
 */
struct ApplyWorkspaceEditResult {
    bool _applied{};  //!< Indicates whether the edit was applied or not.
    /**
     * An optional textual description for why the edit was not applied.
     * This may be used by the server for diagnostic logging or to provide
     * a suitable error for a request that triggered the edit.
     */
    std::optional<QString> _failureReason{};
    /**
     * Depending on the client's failure handling strategy `failedChange` might
     * contain the index of the change that failed. This property is only available
     * if the client signals a `failureHandlingStrategy` in its client capabilities.
     */
    std::optional<int> _failedChange{};

    ApplyWorkspaceEditResult& applied(bool v) { _applied = v; return *this; }
    ApplyWorkspaceEditResult& failureReason(const std::optional<QString> & v) { _failureReason = v; return *this; }
    ApplyWorkspaceEditResult& failedChange(std::optional<int> v) { _failedChange = v; return *this; }

    const bool& applied() const { return _applied; }
    const std::optional<QString>& failureReason() const { return _failureReason; }
    const std::optional<int>& failedChange() const { return _failedChange; }

    bool operator==(const ApplyWorkspaceEditResult &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ApplyWorkspaceEditResult> fromJson<ApplyWorkspaceEditResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ApplyWorkspaceEditResult &data);

struct WorkDoneProgressBegin {
    /**
     * Mandatory title of the progress operation. Used to briefly inform about
     * the kind of operation being performed.
     *
     * Examples: "Indexing" or "Linking dependencies".
     */
    QString _title{};
    /**
     * Controls if a cancel button should show to allow the user to cancel the
     * long running operation. Clients that don't support cancellation are allowed
     * to ignore the setting.
     */
    std::optional<bool> _cancellable{};
    /**
     * Optional, more detailed associated progress message. Contains
     * complementary information to the `title`.
     *
     * Examples: "3/25 files", "project/src/module2", "node_modules/some_dep".
     * If unset, the previous progress message (if any) is still valid.
     */
    std::optional<QString> _message{};
    /**
     * Optional progress percentage to display (value 100 is considered 100%).
     * If not provided infinite progress is assumed and clients are allowed
     * to ignore the `percentage` value in subsequent in report notifications.
     *
     * The value should be steadily rising. Clients are free to ignore values
     * that are not following this rule. The value range is [0, 100].
     */
    std::optional<int> _percentage{};

    WorkDoneProgressBegin& title(const QString & v) { _title = v; return *this; }
    WorkDoneProgressBegin& cancellable(std::optional<bool> v) { _cancellable = v; return *this; }
    WorkDoneProgressBegin& message(const std::optional<QString> & v) { _message = v; return *this; }
    WorkDoneProgressBegin& percentage(std::optional<int> v) { _percentage = v; return *this; }

    const QString& title() const { return _title; }
    const std::optional<bool>& cancellable() const { return _cancellable; }
    const std::optional<QString>& message() const { return _message; }
    const std::optional<int>& percentage() const { return _percentage; }

    bool operator==(const WorkDoneProgressBegin &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkDoneProgressBegin> fromJson<WorkDoneProgressBegin>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkDoneProgressBegin &data);

struct WorkDoneProgressReport {
    /**
     * Controls enablement state of a cancel button.
     *
     * Clients that don't support cancellation or don't support controlling the button's
     * enablement state are allowed to ignore the property.
     */
    std::optional<bool> _cancellable{};
    /**
     * Optional, more detailed associated progress message. Contains
     * complementary information to the `title`.
     *
     * Examples: "3/25 files", "project/src/module2", "node_modules/some_dep".
     * If unset, the previous progress message (if any) is still valid.
     */
    std::optional<QString> _message{};
    /**
     * Optional progress percentage to display (value 100 is considered 100%).
     * If not provided infinite progress is assumed and clients are allowed
     * to ignore the `percentage` value in subsequent in report notifications.
     *
     * The value should be steadily rising. Clients are free to ignore values
     * that are not following this rule. The value range is [0, 100]
     */
    std::optional<int> _percentage{};

    WorkDoneProgressReport& cancellable(std::optional<bool> v) { _cancellable = v; return *this; }
    WorkDoneProgressReport& message(const std::optional<QString> & v) { _message = v; return *this; }
    WorkDoneProgressReport& percentage(std::optional<int> v) { _percentage = v; return *this; }

    const std::optional<bool>& cancellable() const { return _cancellable; }
    const std::optional<QString>& message() const { return _message; }
    const std::optional<int>& percentage() const { return _percentage; }

    bool operator==(const WorkDoneProgressReport &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkDoneProgressReport> fromJson<WorkDoneProgressReport>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkDoneProgressReport &data);

struct WorkDoneProgressEnd {
    /**
     * Optional, a final message indicating to for example indicate the outcome
     * of the operation.
     */
    std::optional<QString> _message{};

    WorkDoneProgressEnd& message(const std::optional<QString> & v) { _message = v; return *this; }

    const std::optional<QString>& message() const { return _message; }

    bool operator==(const WorkDoneProgressEnd &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkDoneProgressEnd> fromJson<WorkDoneProgressEnd>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkDoneProgressEnd &data);

struct SetTraceParams {
    TraceValue _value{};

    SetTraceParams& value(const TraceValue & v) { _value = v; return *this; }

    const TraceValue& value() const { return _value; }

    bool operator==(const SetTraceParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SetTraceParams> fromJson<SetTraceParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SetTraceParams &data);

struct LogTraceParams {
    QString _message{};
    std::optional<QString> _verbose{};

    LogTraceParams& message(const QString & v) { _message = v; return *this; }
    LogTraceParams& verbose(const std::optional<QString> & v) { _verbose = v; return *this; }

    const QString& message() const { return _message; }
    const std::optional<QString>& verbose() const { return _verbose; }

    bool operator==(const LogTraceParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<LogTraceParams> fromJson<LogTraceParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const LogTraceParams &data);

struct CancelParams {
    ProgressToken _id{};  //!< The request id to cancel.

    CancelParams& id(const ProgressToken & v) { _id = v; return *this; }

    const ProgressToken& id() const { return _id; }

    bool operator==(const CancelParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CancelParams> fromJson<CancelParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const CancelParams &data);

struct ProgressParams {
    ProgressToken _token{};  //!< The progress token provided by the client or server.
    QJsonValue _value{};  //!< The progress data.

    ProgressParams& token(const ProgressToken & v) { _token = v; return *this; }
    ProgressParams& value(const QJsonValue & v) { _value = v; return *this; }

    const ProgressToken& token() const { return _token; }
    const QJsonValue& value() const { return _value; }

    bool operator==(const ProgressParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ProgressParams> fromJson<ProgressParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ProgressParams &data);

/**
 * A parameter literal used in requests to pass a text document and a position inside that
 * document.
 */
struct TextDocumentPositionParams {
    TextDocumentIdentifier _textDocument{};  //!< The text document.
    Position _position{};  //!< The position inside the text document.

    TextDocumentPositionParams& textDocument(const TextDocumentIdentifier & v) { _textDocument = v; return *this; }
    TextDocumentPositionParams& position(const Position & v) { _position = v; return *this; }

    const TextDocumentIdentifier& textDocument() const { return _textDocument; }
    const Position& position() const { return _position; }

    bool operator==(const TextDocumentPositionParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TextDocumentPositionParams> fromJson<TextDocumentPositionParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const TextDocumentPositionParams &data);

struct WorkDoneProgressParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.

    WorkDoneProgressParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }

    bool operator==(const WorkDoneProgressParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkDoneProgressParams> fromJson<WorkDoneProgressParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkDoneProgressParams &data);

struct PartialResultParams {
    /**
     * An optional token that a server can use to report partial results (e.g. streaming) to
     * the client.
     */
    std::optional<ProgressToken> _partialResultToken{};

    PartialResultParams& partialResultToken(const std::optional<ProgressToken> & v) { _partialResultToken = v; return *this; }

    const std::optional<ProgressToken>& partialResultToken() const { return _partialResultToken; }

    bool operator==(const PartialResultParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<PartialResultParams> fromJson<PartialResultParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const PartialResultParams &data);

/**
 * Represents the connection of two locations. Provides additional metadata over normal {@link Location locations},
 * including an origin range.
 */
struct LocationLink {
    /**
     * Span of the origin of this link.
     *
     * Used as the underlined span for mouse interaction. Defaults to the word range at
     * the definition position.
     */
    std::optional<Range> _originSelectionRange{};
    QString _targetUri{};  //!< The target resource identifier of this link.
    /**
     * The full target range of this link. If the target for example is a symbol then target range is the
     * range enclosing this symbol not including leading/trailing whitespace but everything else
     * like comments. This information is typically used to highlight the range in the editor.
     */
    Range _targetRange{};
    /**
     * The range that should be selected and revealed when this link is being followed, e.g the name of a function.
     * Must be contained by the `targetRange`. See also `DocumentSymbol#range`
     */
    Range _targetSelectionRange{};

    LocationLink& originSelectionRange(const std::optional<Range> & v) { _originSelectionRange = v; return *this; }
    LocationLink& targetUri(const QString & v) { _targetUri = v; return *this; }
    LocationLink& targetRange(const Range & v) { _targetRange = v; return *this; }
    LocationLink& targetSelectionRange(const Range & v) { _targetSelectionRange = v; return *this; }

    const std::optional<Range>& originSelectionRange() const { return _originSelectionRange; }
    const QString& targetUri() const { return _targetUri; }
    const Range& targetRange() const { return _targetRange; }
    const Range& targetSelectionRange() const { return _targetSelectionRange; }

    bool operator==(const LocationLink &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<LocationLink> fromJson<LocationLink>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const LocationLink &data);

/**
 * Static registration options to be returned in the initialize
 * request.
 */
struct StaticRegistrationOptions {
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again. See also Registration#id.
     */
    std::optional<QString> _id{};

    StaticRegistrationOptions& id(const std::optional<QString> & v) { _id = v; return *this; }

    const std::optional<QString>& id() const { return _id; }

    bool operator==(const StaticRegistrationOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<StaticRegistrationOptions> fromJson<StaticRegistrationOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const StaticRegistrationOptions &data);

/**
 * Returns inline value information as the complete text to be shown.
 *
 * @since 3.17.0
 */
struct InlineValueText {
    Range _range{};  //!< The document range for which the inline value applies.
    QString _text{};  //!< The text of the inline value.

    InlineValueText& range(const Range & v) { _range = v; return *this; }
    InlineValueText& text(const QString & v) { _text = v; return *this; }

    const Range& range() const { return _range; }
    const QString& text() const { return _text; }

    bool operator==(const InlineValueText &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueText> fromJson<InlineValueText>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineValueText &data);

/**
 * To compute inline value through a variable lookup.
 *
 * If only a range is specified, the variable name should
 * be extracted from the underlying document.
 *
 * An optional variable name could be used to lookup instead
 * of the extracted name.
 *
 * @since 3.17.0
 */
struct InlineValueVariableLookup {
    /**
     * The document range for which the inline value applies.
     *
     * The range could be used to extract the variable name
     * from the underlying document.
     */
    Range _range{};
    std::optional<QString> _variableName{};  //!< If specified the name of the variable to look up.
    bool _caseSensitiveLookup{};  //!< How to perform the lookup.

    InlineValueVariableLookup& range(const Range & v) { _range = v; return *this; }
    InlineValueVariableLookup& variableName(const std::optional<QString> & v) { _variableName = v; return *this; }
    InlineValueVariableLookup& caseSensitiveLookup(bool v) { _caseSensitiveLookup = v; return *this; }

    const Range& range() const { return _range; }
    const std::optional<QString>& variableName() const { return _variableName; }
    const bool& caseSensitiveLookup() const { return _caseSensitiveLookup; }

    bool operator==(const InlineValueVariableLookup &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueVariableLookup> fromJson<InlineValueVariableLookup>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineValueVariableLookup &data);

/**
 * To compute an inline value through an expression evaluation.
 *
 * If only a range is specified, the expression should be
 * extracted from the underlying document.
 *
 * An optional expression could be evaluated instead of
 * the extracted expression.
 *
 * @since 3.17.0
 */
struct InlineValueEvaluatableExpression {
    /**
     * The document range for which the inline value applies.
     *
     * The range could be used to extract the evaluatable expression
     * from the underlying document.
     */
    Range _range{};
    std::optional<QString> _expression{};  //!< If specified the expression could be evaluated instead.

    InlineValueEvaluatableExpression& range(const Range & v) { _range = v; return *this; }
    InlineValueEvaluatableExpression& expression(const std::optional<QString> & v) { _expression = v; return *this; }

    const Range& range() const { return _range; }
    const std::optional<QString>& expression() const { return _expression; }

    bool operator==(const InlineValueEvaluatableExpression &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueEvaluatableExpression> fromJson<InlineValueEvaluatableExpression>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineValueEvaluatableExpression &data);

/**
 * A diagnostic report with a full set of problems.
 *
 * @since 3.17.0
 */
struct FullDocumentDiagnosticReport {
    /**
     * An optional result id. If provided it will
     * be sent on the next diagnostic request for the
     * same document.
     */
    std::optional<QString> _resultId{};
    QList<Diagnostic> _items{};  //!< The actual items.

    FullDocumentDiagnosticReport& resultId(const std::optional<QString> & v) { _resultId = v; return *this; }
    FullDocumentDiagnosticReport& items(const QList<Diagnostic> & v) { _items = v; return *this; }
    FullDocumentDiagnosticReport& addItem(const Diagnostic & v) { _items.append(v); return *this; }

    const std::optional<QString>& resultId() const { return _resultId; }
    const QList<Diagnostic>& items() const { return _items; }

    bool operator==(const FullDocumentDiagnosticReport &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FullDocumentDiagnosticReport> fromJson<FullDocumentDiagnosticReport>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const FullDocumentDiagnosticReport &data);

/**
 * A diagnostic report indicating that the last returned
 * report is still accurate.
 *
 * @since 3.17.0
 */
struct UnchangedDocumentDiagnosticReport {
    /**
     * A result id which will be sent on the next
     * diagnostic request for the same document.
     */
    QString _resultId{};

    UnchangedDocumentDiagnosticReport& resultId(const QString & v) { _resultId = v; return *this; }

    const QString& resultId() const { return _resultId; }

    bool operator==(const UnchangedDocumentDiagnosticReport &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<UnchangedDocumentDiagnosticReport> fromJson<UnchangedDocumentDiagnosticReport>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const UnchangedDocumentDiagnosticReport &data);

using RelatedFullDocumentDiagnosticReportRelatedDocumentsValue = std::variant<FullDocumentDiagnosticReport, UnchangedDocumentDiagnosticReport>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RelatedFullDocumentDiagnosticReportRelatedDocumentsValue> fromJson<RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const RelatedFullDocumentDiagnosticReportRelatedDocumentsValue &val);

/**
 * A full diagnostic report with a set of related documents.
 *
 * @since 3.17.0
 */
struct RelatedFullDocumentDiagnosticReport {
    /**
     * An optional result id. If provided it will
     * be sent on the next diagnostic request for the
     * same document.
     */
    std::optional<QString> _resultId{};
    QList<Diagnostic> _items{};  //!< The actual items.
    /**
     * Diagnostics of related documents. This information is useful
     * in programming languages where code in a file A can generate
     * diagnostics in a file B which A depends on. An example of
     * such a language is C/C++ where marco definitions in a file
     * a.cpp and result in errors in a header file b.hpp.
     *
     * @since 3.17.0
     */
    std::optional<QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>> _relatedDocuments{};

    RelatedFullDocumentDiagnosticReport& resultId(const std::optional<QString> & v) { _resultId = v; return *this; }
    RelatedFullDocumentDiagnosticReport& items(const QList<Diagnostic> & v) { _items = v; return *this; }
    RelatedFullDocumentDiagnosticReport& addItem(const Diagnostic & v) { _items.append(v); return *this; }
    RelatedFullDocumentDiagnosticReport& relatedDocuments(const std::optional<QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>> & v) { _relatedDocuments = v; return *this; }
    RelatedFullDocumentDiagnosticReport& addRelatedDocument(const QString &key, const RelatedFullDocumentDiagnosticReportRelatedDocumentsValue & v) { if (!_relatedDocuments) _relatedDocuments = QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>{}; (*_relatedDocuments)[key] = v; return *this; }

    const std::optional<QString>& resultId() const { return _resultId; }
    const QList<Diagnostic>& items() const { return _items; }
    const std::optional<QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>>& relatedDocuments() const { return _relatedDocuments; }

    bool operator==(const RelatedFullDocumentDiagnosticReport &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RelatedFullDocumentDiagnosticReport> fromJson<RelatedFullDocumentDiagnosticReport>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RelatedFullDocumentDiagnosticReport &data);

/**
 * An unchanged diagnostic report with a set of related documents.
 *
 * @since 3.17.0
 */
struct RelatedUnchangedDocumentDiagnosticReport {
    /**
     * A result id which will be sent on the next
     * diagnostic request for the same document.
     */
    QString _resultId{};
    /**
     * Diagnostics of related documents. This information is useful
     * in programming languages where code in a file A can generate
     * diagnostics in a file B which A depends on. An example of
     * such a language is C/C++ where marco definitions in a file
     * a.cpp and result in errors in a header file b.hpp.
     *
     * @since 3.17.0
     */
    std::optional<QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>> _relatedDocuments{};

    RelatedUnchangedDocumentDiagnosticReport& resultId(const QString & v) { _resultId = v; return *this; }
    RelatedUnchangedDocumentDiagnosticReport& relatedDocuments(const std::optional<QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>> & v) { _relatedDocuments = v; return *this; }
    RelatedUnchangedDocumentDiagnosticReport& addRelatedDocument(const QString &key, const RelatedFullDocumentDiagnosticReportRelatedDocumentsValue & v) { if (!_relatedDocuments) _relatedDocuments = QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>{}; (*_relatedDocuments)[key] = v; return *this; }

    const QString& resultId() const { return _resultId; }
    const std::optional<QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>>& relatedDocuments() const { return _relatedDocuments; }

    bool operator==(const RelatedUnchangedDocumentDiagnosticReport &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<RelatedUnchangedDocumentDiagnosticReport> fromJson<RelatedUnchangedDocumentDiagnosticReport>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const RelatedUnchangedDocumentDiagnosticReport &data);

/**
 * A partial result for a document diagnostic report.
 *
 * @since 3.17.0
 */
struct DocumentDiagnosticReportPartialResult {
    QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue> _relatedDocuments{};

    DocumentDiagnosticReportPartialResult& relatedDocuments(const QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue> & v) { _relatedDocuments = v; return *this; }
    DocumentDiagnosticReportPartialResult& addRelatedDocument(const QString &key, const RelatedFullDocumentDiagnosticReportRelatedDocumentsValue & v) { _relatedDocuments[key] = v; return *this; }

    const QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>& relatedDocuments() const { return _relatedDocuments; }

    bool operator==(const DocumentDiagnosticReportPartialResult &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentDiagnosticReportPartialResult> fromJson<DocumentDiagnosticReportPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentDiagnosticReportPartialResult &data);

/** The initialize parameters */
struct _InitializeParams {
    std::optional<ProgressToken> _workDoneToken{};  //!< An optional token that a server can use to report work done progress.
    /**
     * The process Id of the parent process that started
     * the server.
     *
     * Is `null` if the process has not been started by another process.
     * If the parent process is not alive then the server should exit.
     */
    std::optional<int> _processId{};
    /**
     * Information about the client
     *
     * @since 3.15.0
     */
    std::optional<ClientInfo> _clientInfo{};
    /**
     * The locale the client is currently showing the user interface
     * in. This must not necessarily be the locale of the operating
     * system.
     *
     * Uses IETF language tags as the value's syntax
     * (See https://en.wikipedia.org/wiki/IETF_language_tag)
     *
     * @since 3.16.0
     */
    std::optional<QString> _locale{};
    /**
     * The rootPath of the workspace. Is null
     * if no folder is open.
     *
     * @deprecated in favour of rootUri.
     */
    std::optional<QString> _rootPath{};
    /**
     * The rootUri of the workspace. Is null if no
     * folder is open. If both `rootPath` and `rootUri` are set
     * `rootUri` wins.
     *
     * @deprecated in favour of workspaceFolders.
     */
    std::optional<QString> _rootUri{};
    ClientCapabilities _capabilities{};  //!< The capabilities provided by the client (editor or tool)
    std::optional<QJsonValue> _initializationOptions{};  //!< User provided initialization options.
    std::optional<TraceValue> _trace{};  //!< The initial trace setting. If omitted trace is disabled ('off').

    _InitializeParams& workDoneToken(const std::optional<ProgressToken> & v) { _workDoneToken = v; return *this; }
    _InitializeParams& processId(std::optional<int> v) { _processId = v; return *this; }
    _InitializeParams& clientInfo(const std::optional<ClientInfo> & v) { _clientInfo = v; return *this; }
    _InitializeParams& locale(const std::optional<QString> & v) { _locale = v; return *this; }
    _InitializeParams& rootPath(const std::optional<QString> & v) { _rootPath = v; return *this; }
    _InitializeParams& rootUri(const std::optional<QString> & v) { _rootUri = v; return *this; }
    _InitializeParams& capabilities(const ClientCapabilities & v) { _capabilities = v; return *this; }
    _InitializeParams& initializationOptions(const std::optional<QJsonValue> & v) { _initializationOptions = v; return *this; }
    _InitializeParams& trace(const std::optional<TraceValue> & v) { _trace = v; return *this; }

    const std::optional<ProgressToken>& workDoneToken() const { return _workDoneToken; }
    const std::optional<int>& processId() const { return _processId; }
    const std::optional<ClientInfo>& clientInfo() const { return _clientInfo; }
    const std::optional<QString>& locale() const { return _locale; }
    const std::optional<QString>& rootPath() const { return _rootPath; }
    const std::optional<QString>& rootUri() const { return _rootUri; }
    const ClientCapabilities& capabilities() const { return _capabilities; }
    const std::optional<QJsonValue>& initializationOptions() const { return _initializationOptions; }
    const std::optional<TraceValue>& trace() const { return _trace; }

    bool operator==(const _InitializeParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<_InitializeParams> fromJson<_InitializeParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const _InitializeParams &data);

struct WorkspaceFoldersInitializeParams {
    /**
     * The workspace folders configured in the client when the server starts.
     *
     * This property is only available if the client supports workspace folders.
     * It can be `null` if the client supports workspace folders but none are
     * configured.
     *
     * @since 3.6.0
     */
    std::optional<InitializeParamsWorkspaceFolders> _workspaceFolders{};

    WorkspaceFoldersInitializeParams& workspaceFolders(const std::optional<InitializeParamsWorkspaceFolders> & v) { _workspaceFolders = v; return *this; }

    const std::optional<InitializeParamsWorkspaceFolders>& workspaceFolders() const { return _workspaceFolders; }

    bool operator==(const WorkspaceFoldersInitializeParams &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceFoldersInitializeParams> fromJson<WorkspaceFoldersInitializeParams>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkspaceFoldersInitializeParams &data);

/** A base for all symbol information. */
struct BaseSymbolInformation {
    QString _name{};  //!< The name of this symbol.
    int _kind{};  //!< The kind of this symbol.
    /**
     * Tags for this symbol.
     *
     * @since 3.16.0
     */
    std::optional<QList<int>> _tags{};
    /**
     * The name of the symbol containing this symbol. This information is for
     * user interface purposes (e.g. to render a qualifier in the user interface
     * if necessary). It can't be used to re-infer a hierarchy for the document
     * symbols.
     */
    std::optional<QString> _containerName{};

    BaseSymbolInformation& name(const QString & v) { _name = v; return *this; }
    BaseSymbolInformation& kind(int v) { _kind = v; return *this; }
    BaseSymbolInformation& tags(const std::optional<QList<int>> & v) { _tags = v; return *this; }
    BaseSymbolInformation& addTag(int v) { if (!_tags) _tags = QList<int>{}; (*_tags).append(v); return *this; }
    BaseSymbolInformation& containerName(const std::optional<QString> & v) { _containerName = v; return *this; }

    const QString& name() const { return _name; }
    const int& kind() const { return _kind; }
    const std::optional<QList<int>>& tags() const { return _tags; }
    const std::optional<QString>& containerName() const { return _containerName; }

    bool operator==(const BaseSymbolInformation &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<BaseSymbolInformation> fromJson<BaseSymbolInformation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const BaseSymbolInformation &data);

/** @since 3.18.0 */
struct PrepareRenamePlaceholder {
    Range _range{};
    QString _placeholder{};

    PrepareRenamePlaceholder& range(const Range & v) { _range = v; return *this; }
    PrepareRenamePlaceholder& placeholder(const QString & v) { _placeholder = v; return *this; }

    const Range& range() const { return _range; }
    const QString& placeholder() const { return _placeholder; }

    bool operator==(const PrepareRenamePlaceholder &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<PrepareRenamePlaceholder> fromJson<PrepareRenamePlaceholder>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const PrepareRenamePlaceholder &data);

/** @since 3.18.0 */
struct PrepareRenameDefaultBehavior {
    bool _defaultBehavior{};

    PrepareRenameDefaultBehavior& defaultBehavior(bool v) { _defaultBehavior = v; return *this; }

    const bool& defaultBehavior() const { return _defaultBehavior; }

    bool operator==(const PrepareRenameDefaultBehavior &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<PrepareRenameDefaultBehavior> fromJson<PrepareRenameDefaultBehavior>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const PrepareRenameDefaultBehavior &data);

struct WorkDoneProgressOptions {
    std::optional<bool> _workDoneProgress{};

    WorkDoneProgressOptions& workDoneProgress(std::optional<bool> v) { _workDoneProgress = v; return *this; }

    const std::optional<bool>& workDoneProgress() const { return _workDoneProgress; }

    bool operator==(const WorkDoneProgressOptions &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkDoneProgressOptions> fromJson<WorkDoneProgressOptions>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const WorkDoneProgressOptions &data);

/** A generic resource operation. */
struct ResourceOperation {
    QString _kind{};  //!< The resource operation kind.
    /**
     * An optional annotation identifier describing the operation.
     *
     * @since 3.16.0
     */
    std::optional<ChangeAnnotationIdentifier> _annotationId{};

    ResourceOperation& kind(const QString & v) { _kind = v; return *this; }
    ResourceOperation& annotationId(const std::optional<ChangeAnnotationIdentifier> & v) { _annotationId = v; return *this; }

    const QString& kind() const { return _kind; }
    const std::optional<ChangeAnnotationIdentifier>& annotationId() const { return _annotationId; }

    bool operator==(const ResourceOperation &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ResourceOperation> fromJson<ResourceOperation>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const ResourceOperation &data);

/** General diagnostics capabilities for pull and push model. */
struct DiagnosticsCapabilities {
    std::optional<bool> _relatedInformation{};  //!< Whether the clients accepts diagnostics with related information.
    /**
     * Client supports the tag property to provide meta data about a diagnostic.
     * Clients supporting tags have to handle unknown tags gracefully.
     *
     * @since 3.15.0
     */
    std::optional<ClientDiagnosticsTagOptions> _tagSupport{};
    /**
     * Client supports a codeDescription property
     *
     * @since 3.16.0
     */
    std::optional<bool> _codeDescriptionSupport{};
    /**
     * Whether code action supports the `data` property which is
     * preserved between a `textDocument/publishDiagnostics` and
     * `textDocument/codeAction` request.
     *
     * @since 3.16.0
     */
    std::optional<bool> _dataSupport{};

    DiagnosticsCapabilities& relatedInformation(std::optional<bool> v) { _relatedInformation = v; return *this; }
    DiagnosticsCapabilities& tagSupport(const std::optional<ClientDiagnosticsTagOptions> & v) { _tagSupport = v; return *this; }
    DiagnosticsCapabilities& codeDescriptionSupport(std::optional<bool> v) { _codeDescriptionSupport = v; return *this; }
    DiagnosticsCapabilities& dataSupport(std::optional<bool> v) { _dataSupport = v; return *this; }

    const std::optional<bool>& relatedInformation() const { return _relatedInformation; }
    const std::optional<ClientDiagnosticsTagOptions>& tagSupport() const { return _tagSupport; }
    const std::optional<bool>& codeDescriptionSupport() const { return _codeDescriptionSupport; }
    const std::optional<bool>& dataSupport() const { return _dataSupport; }

    bool operator==(const DiagnosticsCapabilities &other) const = default;
};

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DiagnosticsCapabilities> fromJson<DiagnosticsCapabilities>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DiagnosticsCapabilities &data);

/**
 * A set of predefined token types. This set is not fixed
 * an clients can specify additional token types via the
 * corresponding client capabilities.
 *
 * @since 3.16.0
 */
namespace SemanticTokenTypes {
    constexpr char namespace_[] = "namespace";
    constexpr char type[] = "type";
    constexpr char class_[] = "class";
    constexpr char enum_[] = "enum";
    constexpr char interface_[] = "interface";
    constexpr char struct_[] = "struct";
    constexpr char typeParameter[] = "typeParameter";
    constexpr char parameter[] = "parameter";
    constexpr char variable[] = "variable";
    constexpr char property[] = "property";
    constexpr char enumMember[] = "enumMember";
    constexpr char event[] = "event";
    constexpr char function[] = "function";
    constexpr char method[] = "method";
    constexpr char macro[] = "macro";
    constexpr char keyword[] = "keyword";
    constexpr char modifier[] = "modifier";
    constexpr char comment[] = "comment";
    constexpr char string[] = "string";
    constexpr char number[] = "number";
    constexpr char regexp[] = "regexp";
    constexpr char operator_[] = "operator";
    constexpr char decorator[] = "decorator";
    constexpr char label[] = "label";
} // namespace SemanticTokenTypes
/**
 * A set of predefined token modifiers. This set is not fixed
 * an clients can specify additional token types via the
 * corresponding client capabilities.
 *
 * @since 3.16.0
 */
namespace SemanticTokenModifiers {
    constexpr char declaration[] = "declaration";
    constexpr char definition[] = "definition";
    constexpr char readonly[] = "readonly";
    constexpr char static_[] = "static";
    constexpr char deprecated[] = "deprecated";
    constexpr char abstract[] = "abstract";
    constexpr char async[] = "async";
    constexpr char modification[] = "modification";
    constexpr char documentation[] = "documentation";
    constexpr char defaultLibrary[] = "defaultLibrary";
} // namespace SemanticTokenModifiers
/**
 * The document diagnostic report kinds.
 *
 * @since 3.17.0
 */
enum class DocumentDiagnosticReportKind {
    full,
    unchanged
};

LANGUAGESERVERPROTOCOL_EXPORT QString toString(DocumentDiagnosticReportKind v);

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentDiagnosticReportKind> fromJson<DocumentDiagnosticReportKind>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const DocumentDiagnosticReportKind &v);

/** Predefined error codes. */
namespace ErrorCodes {
    constexpr int ParseError = -32700;
    constexpr int InvalidRequest = -32600;
    constexpr int MethodNotFound = -32601;
    constexpr int InvalidParams = -32602;
    constexpr int InternalError = -32603;
    constexpr int ServerNotInitialized = -32002;
    constexpr int UnknownErrorCode = -32001;
} // namespace ErrorCodes
namespace LSPErrorCodes {
    constexpr int RequestFailed = -32803;
    constexpr int ServerCancelled = -32802;
    constexpr int ContentModified = -32801;
    constexpr int RequestCancelled = -32800;
} // namespace LSPErrorCodes
/**
 * The definition of a symbol represented as one or many {@link Location locations}.
 * For most programming languages there is only one location at which a symbol is
 * defined.
 *
 * Servers should prefer returning `DefinitionLink` over `Definition` if supported
 * by the client.
 */
using Definition = std::variant<Location, QList<Location>>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<Definition> fromJson<Definition>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const Definition &val);

using DefinitionLink = LocationLink;

/** The declaration of a symbol representation as one or many {@link Location locations}. */
using Declaration = std::variant<Location, QList<Location>>;

using DeclarationLink = LocationLink;

/**
 * Inline value information can be provided by different means:
 * - directly as a text value (class InlineValueText).
 * - as a name to use for a variable lookup (class InlineValueVariableLookup)
 * - as an evaluatable expression (class InlineValueEvaluatableExpression)
 * The InlineValue types combines all inline value types into one type.
 *
 * @since 3.17.0
 */
using InlineValue = std::variant<InlineValueText, InlineValueVariableLookup, InlineValueEvaluatableExpression>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValue> fromJson<InlineValue>(const QJsonValue &val);

/** Returns the 'range' field from the active variant. */
LANGUAGESERVERPROTOCOL_EXPORT Range range(const InlineValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const InlineValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const InlineValue &val);

/**
 * The result of a document diagnostic pull request. A report can
 * either be a full report containing all diagnostics for the
 * requested document or an unchanged report indicating that nothing
 * has changed in terms of diagnostics in comparison to the last
 * pull request.
 *
 * @since 3.17.0
 */
using DocumentDiagnosticReport = std::variant<RelatedFullDocumentDiagnosticReport, RelatedUnchangedDocumentDiagnosticReport>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentDiagnosticReport> fromJson<DocumentDiagnosticReport>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentDiagnosticReport &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const DocumentDiagnosticReport &val);

/** Returns the 'kind' dispatch field value for the active variant. */
LANGUAGESERVERPROTOCOL_EXPORT QString dispatchValue(const DocumentDiagnosticReport &val);

/**
 * The document diagnostic report used when reporting partial result.
 *
 * When using partial results, the first literal sent needs to be a
 * DocumentDiagnosticReport providing the diagnostics on the document
 * followed by n DocumentDiagnosticReportPartialResult literals providing
 * the diagnostics for related documents.
 *
 * ```
 * DocumentDiagnosticReport
 * DocumentDiagnosticReportPartialResult
 * DocumentDiagnosticReportPartialResult
 * ...
 * ```
 *
 * @since 3.18.1
 */
using DocumentDiagnosticReportProgress = std::variant<DocumentDiagnosticReport, DocumentDiagnosticReportPartialResult>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentDiagnosticReportProgress> fromJson<DocumentDiagnosticReportProgress>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const DocumentDiagnosticReportProgress &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const DocumentDiagnosticReportProgress &val);

using PrepareRenameResult = std::variant<Range, PrepareRenamePlaceholder, PrepareRenameDefaultBehavior>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<PrepareRenameResult> fromJson<PrepareRenameResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const PrepareRenameResult &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const PrepareRenameResult &val);

using ImplementationRequestResult = std::variant<Definition, QList<DefinitionLink>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ImplementationRequestResult> fromJson<ImplementationRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ImplementationRequestResult &val);

using ImplementationRequestPartialResult = std::variant<QList<Location>, QList<DefinitionLink>>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ImplementationRequestPartialResult> fromJson<ImplementationRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ImplementationRequestPartialResult &val);

using TypeDefinitionRequestResult = std::variant<Definition, QList<DefinitionLink>, std::monostate>;

using TypeDefinitionRequestPartialResult = std::variant<QList<Location>, QList<DefinitionLink>>;

using WorkspaceFoldersRequestResult = std::variant<QList<WorkspaceFolder>, std::monostate>;

using ConfigurationRequestResult = QJsonArray;

using DocumentColorRequestResult = QList<ColorInformation>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentColorRequestResult> fromJson<DocumentColorRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const DocumentColorRequestResult &data);

using DocumentColorRequestPartialResult = QList<ColorInformation>;

using ColorPresentationRequestResult = QList<ColorPresentation>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ColorPresentationRequestResult> fromJson<ColorPresentationRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const ColorPresentationRequestResult &data);

using ColorPresentationRequestPartialResult = QList<ColorPresentation>;

using FoldingRangeRequestResult = std::variant<QList<FoldingRange>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FoldingRangeRequestResult> fromJson<FoldingRangeRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const FoldingRangeRequestResult &val);

using FoldingRangeRequestPartialResult = QList<FoldingRange>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<FoldingRangeRequestPartialResult> fromJson<FoldingRangeRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const FoldingRangeRequestPartialResult &data);

using DeclarationRequestResult = std::variant<Declaration, QList<DeclarationLink>, std::monostate>;

using DeclarationRequestPartialResult = std::variant<QList<Location>, QList<DeclarationLink>>;

using SelectionRangeRequestResult = std::variant<QList<SelectionRange>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SelectionRangeRequestResult> fromJson<SelectionRangeRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const SelectionRangeRequestResult &val);

using SelectionRangeRequestPartialResult = QList<SelectionRange>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SelectionRangeRequestPartialResult> fromJson<SelectionRangeRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const SelectionRangeRequestPartialResult &data);

using CallHierarchyPrepareRequestResult = std::variant<QList<CallHierarchyItem>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyPrepareRequestResult> fromJson<CallHierarchyPrepareRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const CallHierarchyPrepareRequestResult &val);

using CallHierarchyIncomingCallsRequestResult = std::variant<QList<CallHierarchyIncomingCall>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyIncomingCallsRequestResult> fromJson<CallHierarchyIncomingCallsRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const CallHierarchyIncomingCallsRequestResult &val);

using CallHierarchyIncomingCallsRequestPartialResult = QList<CallHierarchyIncomingCall>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyIncomingCallsRequestPartialResult> fromJson<CallHierarchyIncomingCallsRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const CallHierarchyIncomingCallsRequestPartialResult &data);

using CallHierarchyOutgoingCallsRequestResult = std::variant<QList<CallHierarchyOutgoingCall>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyOutgoingCallsRequestResult> fromJson<CallHierarchyOutgoingCallsRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const CallHierarchyOutgoingCallsRequestResult &val);

using CallHierarchyOutgoingCallsRequestPartialResult = QList<CallHierarchyOutgoingCall>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CallHierarchyOutgoingCallsRequestPartialResult> fromJson<CallHierarchyOutgoingCallsRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const CallHierarchyOutgoingCallsRequestPartialResult &data);

using SemanticTokensRequestResult = std::variant<SemanticTokens, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensRequestResult> fromJson<SemanticTokensRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const SemanticTokensRequestResult &val);

using SemanticTokensDeltaRequestResult = std::variant<SemanticTokens, SemanticTokensDelta, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensDeltaRequestResult> fromJson<SemanticTokensDeltaRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const SemanticTokensDeltaRequestResult &val);

using SemanticTokensDeltaRequestPartialResult = std::variant<SemanticTokensPartialResult, SemanticTokensDeltaPartialResult>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SemanticTokensDeltaRequestPartialResult> fromJson<SemanticTokensDeltaRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonObject toJson(const SemanticTokensDeltaRequestPartialResult &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const SemanticTokensDeltaRequestPartialResult &val);

using SemanticTokensRangeRequestResult = std::variant<SemanticTokens, std::monostate>;

using LinkedEditingRangeRequestResult = std::variant<LinkedEditingRanges, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<LinkedEditingRangeRequestResult> fromJson<LinkedEditingRangeRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const LinkedEditingRangeRequestResult &val);

using WillCreateFilesRequestResult = std::variant<WorkspaceEdit, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WillCreateFilesRequestResult> fromJson<WillCreateFilesRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const WillCreateFilesRequestResult &val);

using WillRenameFilesRequestResult = std::variant<WorkspaceEdit, std::monostate>;

using WillDeleteFilesRequestResult = std::variant<WorkspaceEdit, std::monostate>;

using MonikerRequestResult = std::variant<QList<Moniker>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MonikerRequestResult> fromJson<MonikerRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const MonikerRequestResult &val);

using MonikerRequestPartialResult = QList<Moniker>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<MonikerRequestPartialResult> fromJson<MonikerRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const MonikerRequestPartialResult &data);

using TypeHierarchyPrepareRequestResult = std::variant<QList<TypeHierarchyItem>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeHierarchyPrepareRequestResult> fromJson<TypeHierarchyPrepareRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const TypeHierarchyPrepareRequestResult &val);

using TypeHierarchySupertypesRequestResult = std::variant<QList<TypeHierarchyItem>, std::monostate>;

using TypeHierarchySupertypesRequestPartialResult = QList<TypeHierarchyItem>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<TypeHierarchySupertypesRequestPartialResult> fromJson<TypeHierarchySupertypesRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const TypeHierarchySupertypesRequestPartialResult &data);

using TypeHierarchySubtypesRequestResult = std::variant<QList<TypeHierarchyItem>, std::monostate>;

using TypeHierarchySubtypesRequestPartialResult = QList<TypeHierarchyItem>;

using InlineValueRequestResult = std::variant<QList<InlineValue>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueRequestResult> fromJson<InlineValueRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const InlineValueRequestResult &val);

using InlineValueRequestPartialResult = QList<InlineValue>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineValueRequestPartialResult> fromJson<InlineValueRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const InlineValueRequestPartialResult &data);

using InlayHintRequestResult = std::variant<QList<InlayHint>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHintRequestResult> fromJson<InlayHintRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const InlayHintRequestResult &val);

using InlayHintRequestPartialResult = QList<InlayHint>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlayHintRequestPartialResult> fromJson<InlayHintRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const InlayHintRequestPartialResult &data);

using InlineCompletionRequestResult = std::variant<InlineCompletionList, QList<InlineCompletionItem>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineCompletionRequestResult> fromJson<InlineCompletionRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const InlineCompletionRequestResult &val);

using InlineCompletionRequestPartialResult = QList<InlineCompletionItem>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<InlineCompletionRequestPartialResult> fromJson<InlineCompletionRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const InlineCompletionRequestPartialResult &data);

using ShowMessageRequestResult = std::variant<MessageActionItem, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ShowMessageRequestResult> fromJson<ShowMessageRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ShowMessageRequestResult &val);

using WillSaveTextDocumentWaitUntilRequestResult = std::variant<QList<TextEdit>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WillSaveTextDocumentWaitUntilRequestResult> fromJson<WillSaveTextDocumentWaitUntilRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const WillSaveTextDocumentWaitUntilRequestResult &val);

using CompletionRequestResult = std::variant<QList<CompletionItem>, CompletionList, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionRequestResult> fromJson<CompletionRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const CompletionRequestResult &val);

using CompletionRequestPartialResult = QList<CompletionItem>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CompletionRequestPartialResult> fromJson<CompletionRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const CompletionRequestPartialResult &data);

using HoverRequestResult = std::variant<Hover, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<HoverRequestResult> fromJson<HoverRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const HoverRequestResult &val);

using SignatureHelpRequestResult = std::variant<SignatureHelp, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<SignatureHelpRequestResult> fromJson<SignatureHelpRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const SignatureHelpRequestResult &val);

using DefinitionRequestResult = std::variant<Definition, QList<DefinitionLink>, std::monostate>;

using DefinitionRequestPartialResult = std::variant<QList<Location>, QList<DefinitionLink>>;

using ReferencesRequestResult = std::variant<QList<Location>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ReferencesRequestResult> fromJson<ReferencesRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ReferencesRequestResult &val);

using ReferencesRequestPartialResult = QList<Location>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ReferencesRequestPartialResult> fromJson<ReferencesRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const ReferencesRequestPartialResult &data);

using DocumentHighlightRequestResult = std::variant<QList<DocumentHighlight>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentHighlightRequestResult> fromJson<DocumentHighlightRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const DocumentHighlightRequestResult &val);

using DocumentHighlightRequestPartialResult = QList<DocumentHighlight>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentHighlightRequestPartialResult> fromJson<DocumentHighlightRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const DocumentHighlightRequestPartialResult &data);

using DocumentSymbolRequestResult = std::variant<QList<SymbolInformation>, QList<DocumentSymbol>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentSymbolRequestResult> fromJson<DocumentSymbolRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const DocumentSymbolRequestResult &val);

using DocumentSymbolRequestPartialResult = std::variant<QList<SymbolInformation>, QList<DocumentSymbol>>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentSymbolRequestPartialResult> fromJson<DocumentSymbolRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const DocumentSymbolRequestPartialResult &val);

using CommandOrCodeAction = std::variant<Command, CodeAction>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CommandOrCodeAction> fromJson<CommandOrCodeAction>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const CommandOrCodeAction &val);

using CodeActionRequestResult = std::variant<QList<CommandOrCodeAction>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeActionRequestResult> fromJson<CodeActionRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const CodeActionRequestResult &val);

using CodeActionRequestPartialResult = QJsonArray;

using WorkspaceSymbolRequestResult = std::variant<QList<SymbolInformation>, QList<WorkspaceSymbol>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceSymbolRequestResult> fromJson<WorkspaceSymbolRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const WorkspaceSymbolRequestResult &val);

using WorkspaceSymbolRequestPartialResult = std::variant<QList<SymbolInformation>, QList<WorkspaceSymbol>>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<WorkspaceSymbolRequestPartialResult> fromJson<WorkspaceSymbolRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const WorkspaceSymbolRequestPartialResult &val);

using CodeLensRequestResult = std::variant<QList<CodeLens>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeLensRequestResult> fromJson<CodeLensRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const CodeLensRequestResult &val);

using CodeLensRequestPartialResult = QList<CodeLens>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<CodeLensRequestPartialResult> fromJson<CodeLensRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const CodeLensRequestPartialResult &data);

using DocumentLinkRequestResult = std::variant<QList<DocumentLink>, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentLinkRequestResult> fromJson<DocumentLinkRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const DocumentLinkRequestResult &val);

using DocumentLinkRequestPartialResult = QList<DocumentLink>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<DocumentLinkRequestPartialResult> fromJson<DocumentLinkRequestPartialResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonArray toJson(const DocumentLinkRequestPartialResult &data);

using DocumentFormattingRequestResult = std::variant<QList<TextEdit>, std::monostate>;

using DocumentRangeFormattingRequestResult = std::variant<QList<TextEdit>, std::monostate>;

using DocumentRangesFormattingRequestResult = std::variant<QList<TextEdit>, std::monostate>;

using DocumentOnTypeFormattingRequestResult = std::variant<QList<TextEdit>, std::monostate>;

using RenameRequestResult = std::variant<WorkspaceEdit, std::monostate>;

using PrepareRenameRequestResult = std::variant<PrepareRenameResult, std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<PrepareRenameRequestResult> fromJson<PrepareRenameRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const PrepareRenameRequestResult &val);

using ExecuteCommandRequestResult = std::variant<std::monostate>;

template<>
LANGUAGESERVERPROTOCOL_EXPORT Utils::Result<ExecuteCommandRequestResult> fromJson<ExecuteCommandRequestResult>(const QJsonValue &val);

LANGUAGESERVERPROTOCOL_EXPORT QJsonValue toJsonValue(const ExecuteCommandRequestResult &val);

} // namespace LanguageServerProtocol
