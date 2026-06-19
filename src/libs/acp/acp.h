/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 C:\dev\bin\Python\313\python.exe \
  scripts/generate_cpp_from_schema.py \
  src/libs/acp/schema/schema.json src/libs/acp/acp.h --namespace Acp --cpp-output src/libs/acp/acp.cpp --export-macro ACPLIB_EXPORT --export-header acp_global.h
*/
#pragma once

#include "acp_global.h"

#include <utils/result.h>
#include <utils/co_result.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariant>

#include <variant>

namespace Acp {

template<typename T> Utils::Result<T> fromJson(const QJsonValue &val) = delete;

/** An environment variable to set when launching an MCP server. */
struct EnvVariable {
    QString _name;  //!< The name of the environment variable.
    QString _value;  //!< The value to set for the environment variable.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    EnvVariable& name(const QString & v) { _name = v; return *this; }
    EnvVariable& value(const QString & v) { _value = v; return *this; }
    EnvVariable& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& value() const { return _value; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<EnvVariable> fromJson<EnvVariable>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const EnvVariable &data);

using SessionId = QString;
template<>
ACPLIB_EXPORT Utils::Result<SessionId> fromJson<SessionId>(const QJsonValue &val);

/** Request to create a new terminal and execute a command. */
struct CreateTerminalRequest {
    SessionId _sessionId;  //!< The session ID for this request.
    QString _command;  //!< The command to execute.
    std::optional<QStringList> _args;  //!< Array of command arguments.
    std::optional<QList<EnvVariable>> _env;  //!< Environment variables for the command.
    std::optional<QString> _cwd;  //!< Working directory for the command (absolute path).
    /**
     * Maximum number of output bytes to retain.
     *
     * When the limit is exceeded, the Client truncates from the beginning of the output
     * to stay within the limit.
     *
     * The Client MUST ensure truncation happens at a character boundary to maintain valid
     * string output, even if this means the retained output is slightly less than the
     * specified limit.
     */
    std::optional<int> _outputByteLimit;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    CreateTerminalRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    CreateTerminalRequest& command(const QString & v) { _command = v; return *this; }
    CreateTerminalRequest& args(const std::optional<QStringList> & v) { _args = v; return *this; }
    CreateTerminalRequest& addArg(const QString & v) { if (!_args) _args = QStringList{}; (*_args).append(v); return *this; }
    CreateTerminalRequest& env(const std::optional<QList<EnvVariable>> & v) { _env = v; return *this; }
    CreateTerminalRequest& addEnv(const EnvVariable & v) { if (!_env) _env = QList<EnvVariable>{}; (*_env).append(v); return *this; }
    CreateTerminalRequest& cwd(const std::optional<QString> & v) { _cwd = v; return *this; }
    CreateTerminalRequest& outputByteLimit(std::optional<int> v) { _outputByteLimit = v; return *this; }
    CreateTerminalRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const QString& command() const { return _command; }
    const std::optional<QStringList>& args() const { return _args; }
    const std::optional<QList<EnvVariable>>& env() const { return _env; }
    const std::optional<QString>& cwd() const { return _cwd; }
    const std::optional<int>& outputByteLimit() const { return _outputByteLimit; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CreateTerminalRequest> fromJson<CreateTerminalRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CreateTerminalRequest &data);

// Skipped unknown type alias: ExtRequest

using TerminalId = QString;

/** Request to kill a terminal without releasing it. */
struct KillTerminalRequest {
    SessionId _sessionId;  //!< The session ID for this request.
    TerminalId _terminalId;  //!< The ID of the terminal to kill.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    KillTerminalRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    KillTerminalRequest& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    KillTerminalRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const TerminalId& terminalId() const { return _terminalId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<KillTerminalRequest> fromJson<KillTerminalRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const KillTerminalRequest &data);

/**
 * Request to read content from a text file.
 *
 * Only available if the client supports the `fs.readTextFile` capability.
 */
struct ReadTextFileRequest {
    SessionId _sessionId;  //!< The session ID for this request.
    QString _path;  //!< Absolute path to the file to read.
    std::optional<int> _line;  //!< Line number to start reading from (1-based).
    std::optional<int> _limit;  //!< Maximum number of lines to read.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ReadTextFileRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    ReadTextFileRequest& path(const QString & v) { _path = v; return *this; }
    ReadTextFileRequest& line(std::optional<int> v) { _line = v; return *this; }
    ReadTextFileRequest& limit(std::optional<int> v) { _limit = v; return *this; }
    ReadTextFileRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const QString& path() const { return _path; }
    const std::optional<int>& line() const { return _line; }
    const std::optional<int>& limit() const { return _limit; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ReadTextFileRequest> fromJson<ReadTextFileRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ReadTextFileRequest &data);

/** Request to release a terminal and free its resources. */
struct ReleaseTerminalRequest {
    SessionId _sessionId;  //!< The session ID for this request.
    TerminalId _terminalId;  //!< The ID of the terminal to release.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ReleaseTerminalRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    ReleaseTerminalRequest& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    ReleaseTerminalRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const TerminalId& terminalId() const { return _terminalId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ReleaseTerminalRequest> fromJson<ReleaseTerminalRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ReleaseTerminalRequest &data);

/**
 * JSON RPC Request Id
 *
 * An identifier established by the Client that MUST contain a String, Number, or NULL value if included. If it is not included it is assumed to be a notification. The value SHOULD normally not be Null \[1\] and Numbers SHOULD NOT contain fractional parts \[2\]
 *
 * The Server MUST reply with the same value in the Response object if included. This member is used to correlate the context between the two objects.
 *
 * \[1\] The use of Null as a value for the id member in a Request object is discouraged, because this specification uses a value of Null for Responses with an unknown id. Also, because JSON-RPC 1.0 uses an id value of Null for Notifications this could cause confusion in handling.
 *
 * \[2\] Fractional parts may be problematic, since many decimal fractions cannot be represented exactly as binary fractions.
 */
using RequestId = std::variant<std::monostate, int, QString>;

template<>
ACPLIB_EXPORT Utils::Result<RequestId> fromJson<RequestId>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const RequestId &val);

using PermissionOptionId = QString;

/**
 * The type of permission option being presented to the user.
 *
 * Helps clients choose appropriate icons and UI treatment.
 */
enum class PermissionOptionKind {
    allow_once,
    allow_always,
    reject_once,
    reject_always
};

ACPLIB_EXPORT QString toString(PermissionOptionKind v);

template<>
ACPLIB_EXPORT Utils::Result<PermissionOptionKind> fromJson<PermissionOptionKind>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const PermissionOptionKind &v);

/** An option presented to the user when requesting permission. */
struct PermissionOption {
    PermissionOptionId _optionId;  //!< Unique identifier for this permission option.
    QString _name;  //!< Human-readable label to display to the user.
    PermissionOptionKind _kind;  //!< Hint about the nature of this permission option.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    PermissionOption& optionId(const PermissionOptionId & v) { _optionId = v; return *this; }
    PermissionOption& name(const QString & v) { _name = v; return *this; }
    PermissionOption& kind(const PermissionOptionKind & v) { _kind = v; return *this; }
    PermissionOption& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const PermissionOptionId& optionId() const { return _optionId; }
    const QString& name() const { return _name; }
    const PermissionOptionKind& kind() const { return _kind; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PermissionOption> fromJson<PermissionOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PermissionOption &data);

/** The sender or recipient of messages and data in a conversation. */
enum class Role {
    assistant,
    user
};

ACPLIB_EXPORT QString toString(Role v);

template<>
ACPLIB_EXPORT Utils::Result<Role> fromJson<Role>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const Role &v);

/**
 * Optional annotations for the client. The client can use annotations to inform how objects are used or displayed
 */
struct Annotations {
    std::optional<QJsonArray> _audience;  //!< Intended recipients for this content, such as the user or assistant.
    std::optional<QString> _lastModified;  //!< Timestamp indicating when the underlying resource was last modified.
    std::optional<double> _priority;  //!< Relative importance of this content when clients choose what to surface.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    Annotations& audience(const std::optional<QJsonArray> & v) { _audience = v; return *this; }
    Annotations& lastModified(const std::optional<QString> & v) { _lastModified = v; return *this; }
    Annotations& priority(std::optional<double> v) { _priority = v; return *this; }
    Annotations& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonArray>& audience() const { return _audience; }
    const std::optional<QString>& lastModified() const { return _lastModified; }
    const std::optional<double>& priority() const { return _priority; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Annotations> fromJson<Annotations>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Annotations &data);

/** Audio provided to or from an LLM. */
struct AudioContent {
    std::optional<Annotations> _annotations;  //!< Optional annotations that help clients decide how to display or route this content.
    QString _data;  //!< Base64-encoded media payload.
    QString _mimeType;  //!< MIME type describing the encoded media payload.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    AudioContent& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    AudioContent& data(const QString & v) { _data = v; return *this; }
    AudioContent& mimeType(const QString & v) { _mimeType = v; return *this; }
    AudioContent& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<Annotations>& annotations() const { return _annotations; }
    const QString& data() const { return _data; }
    const QString& mimeType() const { return _mimeType; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AudioContent> fromJson<AudioContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AudioContent &data);

/** Binary resource contents. */
struct BlobResourceContents {
    QString _blob;  //!< Base64-encoded bytes for a binary resource payload.
    std::optional<QString> _mimeType;  //!< MIME type describing the encoded media payload.
    QString _uri;  //!< URI associated with this resource or media payload.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    BlobResourceContents& blob(const QString & v) { _blob = v; return *this; }
    BlobResourceContents& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    BlobResourceContents& uri(const QString & v) { _uri = v; return *this; }
    BlobResourceContents& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& blob() const { return _blob; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& uri() const { return _uri; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<BlobResourceContents> fromJson<BlobResourceContents>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const BlobResourceContents &data);

/** Text-based resource contents. */
struct TextResourceContents {
    std::optional<QString> _mimeType;  //!< MIME type describing the encoded media payload.
    QString _text;  //!< Text payload carried by this content block.
    QString _uri;  //!< URI associated with this resource or media payload.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    TextResourceContents& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    TextResourceContents& text(const QString & v) { _text = v; return *this; }
    TextResourceContents& uri(const QString & v) { _uri = v; return *this; }
    TextResourceContents& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& text() const { return _text; }
    const QString& uri() const { return _uri; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TextResourceContents> fromJson<TextResourceContents>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TextResourceContents &data);

/** Resource content that can be embedded in a message. */
using EmbeddedResourceResource = std::variant<TextResourceContents, BlobResourceContents>;

template<>
ACPLIB_EXPORT Utils::Result<EmbeddedResourceResource> fromJson<EmbeddedResourceResource>(const QJsonValue &val);

/** Returns the 'uri' field from the active variant. */
ACPLIB_EXPORT QString uri(const EmbeddedResourceResource &val);

ACPLIB_EXPORT QJsonObject toJson(const EmbeddedResourceResource &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const EmbeddedResourceResource &val);

/** The contents of a resource, embedded into a prompt or tool call result. */
struct EmbeddedResource {
    std::optional<Annotations> _annotations;  //!< Optional annotations that help clients decide how to display or route this content.
    EmbeddedResourceResource _resource;  //!< Embedded resource payload, either text or binary data.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    EmbeddedResource& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    EmbeddedResource& resource(const EmbeddedResourceResource & v) { _resource = v; return *this; }
    EmbeddedResource& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<Annotations>& annotations() const { return _annotations; }
    const EmbeddedResourceResource& resource() const { return _resource; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<EmbeddedResource> fromJson<EmbeddedResource>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const EmbeddedResource &data);

/** An image provided to or from an LLM. */
struct ImageContent {
    std::optional<Annotations> _annotations;  //!< Optional annotations that help clients decide how to display or route this content.
    QString _data;  //!< Base64-encoded media payload.
    QString _mimeType;  //!< MIME type describing the encoded media payload.
    std::optional<QString> _uri;  //!< URI associated with this resource or media payload.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ImageContent& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    ImageContent& data(const QString & v) { _data = v; return *this; }
    ImageContent& mimeType(const QString & v) { _mimeType = v; return *this; }
    ImageContent& uri(const std::optional<QString> & v) { _uri = v; return *this; }
    ImageContent& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<Annotations>& annotations() const { return _annotations; }
    const QString& data() const { return _data; }
    const QString& mimeType() const { return _mimeType; }
    const std::optional<QString>& uri() const { return _uri; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ImageContent> fromJson<ImageContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ImageContent &data);

/** A resource that the server is capable of reading, included in a prompt or tool call result. */
struct ResourceLink {
    std::optional<Annotations> _annotations;  //!< Optional annotations that help clients decide how to display or route this content.
    std::optional<QString> _description;  //!< Optional human-readable details shown with this protocol object.
    std::optional<QString> _mimeType;  //!< MIME type describing the encoded media payload.
    QString _name;  //!< Human-readable name shown for this protocol object.
    std::optional<int> _size;  //!< Optional size of the linked resource in bytes, if known.
    std::optional<QString> _title;  //!< Optional display title for end-user UI.
    QString _uri;  //!< URI associated with this resource or media payload.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ResourceLink& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    ResourceLink& description(const std::optional<QString> & v) { _description = v; return *this; }
    ResourceLink& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    ResourceLink& name(const QString & v) { _name = v; return *this; }
    ResourceLink& size(std::optional<int> v) { _size = v; return *this; }
    ResourceLink& title(const std::optional<QString> & v) { _title = v; return *this; }
    ResourceLink& uri(const QString & v) { _uri = v; return *this; }
    ResourceLink& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<Annotations>& annotations() const { return _annotations; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& name() const { return _name; }
    const std::optional<int>& size() const { return _size; }
    const std::optional<QString>& title() const { return _title; }
    const QString& uri() const { return _uri; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ResourceLink> fromJson<ResourceLink>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ResourceLink &data);

/** Text provided to or from an LLM. */
struct TextContent {
    std::optional<Annotations> _annotations;  //!< Optional annotations that help clients decide how to display or route this content.
    QString _text;  //!< Text payload carried by this content block.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    TextContent& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    TextContent& text(const QString & v) { _text = v; return *this; }
    TextContent& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<Annotations>& annotations() const { return _annotations; }
    const QString& text() const { return _text; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TextContent> fromJson<TextContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TextContent &data);

/**
 * Content blocks represent displayable information in the Agent Client Protocol.
 *
 * They provide a structured way to handle various types of user-facing content—whether
 * it's text from language models, images for analysis, or embedded resources for context.
 *
 * Content blocks appear in:
 * - User prompts sent via `session/prompt`
 * - Language model output streamed through `session/update` notifications
 * - Progress updates and results from tool calls
 *
 * This structure is compatible with the Model Context Protocol (MCP), enabling
 * agents to seamlessly forward content from MCP tool outputs without transformation.
 *
 * See protocol docs: [Content](https://agentclientprotocol.com/protocol/content)
 */
using ContentBlock = std::variant<TextContent, ImageContent, AudioContent, ResourceLink, EmbeddedResource>;

template<>
ACPLIB_EXPORT Utils::Result<ContentBlock> fromJson<ContentBlock>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const ContentBlock &val);

ACPLIB_EXPORT QJsonObject toJson(const ContentBlock &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ContentBlock &val);

/** Standard content block (text, images, resources). */
struct Content {
    ContentBlock _content;  //!< The actual content block.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    Content& content(const ContentBlock & v) { _content = v; return *this; }
    Content& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const ContentBlock& content() const { return _content; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Content> fromJson<Content>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Content &data);

/**
 * A diff representing file modifications.
 *
 * Shows changes to files in a format suitable for display in the client UI.
 *
 * See protocol docs: [Content](https://agentclientprotocol.com/protocol/tool-calls#content)
 */
struct Diff {
    QString _path;  //!< The file path being modified.
    std::optional<QString> _oldText;  //!< The original content (None for new files).
    QString _newText;  //!< The new content after modification.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    Diff& path(const QString & v) { _path = v; return *this; }
    Diff& oldText(const std::optional<QString> & v) { _oldText = v; return *this; }
    Diff& newText(const QString & v) { _newText = v; return *this; }
    Diff& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& path() const { return _path; }
    const std::optional<QString>& oldText() const { return _oldText; }
    const QString& newText() const { return _newText; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Diff> fromJson<Diff>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Diff &data);

/**
 * Embed a terminal created with `terminal/create` by its id.
 *
 * The terminal must be added before calling `terminal/release`.
 *
 * See protocol docs: [Terminal](https://agentclientprotocol.com/protocol/terminals)
 */
struct Terminal {
    TerminalId _terminalId;  //!< Identifier of the terminal instance to embed in the content stream.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    Terminal& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    Terminal& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const TerminalId& terminalId() const { return _terminalId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Terminal> fromJson<Terminal>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Terminal &data);

/**
 * Content produced by a tool call.
 *
 * Tool calls can produce different types of content including
 * standard content blocks (text, images) or file diffs.
 *
 * See protocol docs: [Content](https://agentclientprotocol.com/protocol/tool-calls#content)
 */
using ToolCallContent = std::variant<Content, Diff, Terminal>;

template<>
ACPLIB_EXPORT Utils::Result<ToolCallContent> fromJson<ToolCallContent>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const ToolCallContent &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallContent &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ToolCallContent &val);

using ToolCallId = QString;

/**
 * A file location being accessed or modified by a tool.
 *
 * Enables clients to implement "follow-along" features that track
 * which files the agent is working with in real-time.
 *
 * See protocol docs: [Following the Agent](https://agentclientprotocol.com/protocol/tool-calls#following-the-agent)
 */
struct ToolCallLocation {
    QString _path;  //!< The file path being accessed or modified.
    std::optional<int> _line;  //!< Optional line number within the file.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ToolCallLocation& path(const QString & v) { _path = v; return *this; }
    ToolCallLocation& line(std::optional<int> v) { _line = v; return *this; }
    ToolCallLocation& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& path() const { return _path; }
    const std::optional<int>& line() const { return _line; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallLocation> fromJson<ToolCallLocation>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallLocation &data);

/**
 * Execution status of a tool call.
 *
 * Tool calls progress through different statuses during their lifecycle.
 *
 * See protocol docs: [Status](https://agentclientprotocol.com/protocol/tool-calls#status)
 */
enum class ToolCallStatus {
    pending,
    in_progress,
    completed,
    failed
};

ACPLIB_EXPORT QString toString(ToolCallStatus v);

template<>
ACPLIB_EXPORT Utils::Result<ToolCallStatus> fromJson<ToolCallStatus>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ToolCallStatus &v);

/**
 * Categories of tools that can be invoked.
 *
 * Tool kinds help clients choose appropriate icons and optimize how they
 * display tool execution progress.
 *
 * See protocol docs: [Creating](https://agentclientprotocol.com/protocol/tool-calls#creating)
 */
enum class ToolKind {
    read,
    edit,
    delete_,
    move,
    search,
    execute,
    think,
    fetch,
    switch_mode,
    other
};

ACPLIB_EXPORT QString toString(ToolKind v);

template<>
ACPLIB_EXPORT Utils::Result<ToolKind> fromJson<ToolKind>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ToolKind &v);

/**
 * An update to an existing tool call.
 *
 * Used to report progress and results as tools execute. All fields except
 * the tool call ID are optional - only changed fields need to be included.
 *
 * See protocol docs: [Updating](https://agentclientprotocol.com/protocol/tool-calls#updating)
 */
struct ToolCallUpdate {
    ToolCallId _toolCallId;  //!< The ID of the tool call being updated.
    std::optional<ToolKind> _kind;  //!< Update the tool kind.
    std::optional<ToolCallStatus> _status;  //!< Update the execution status.
    std::optional<QString> _title;  //!< Update the human-readable title.
    std::optional<QJsonArray> _content;  //!< Replace the content collection.
    std::optional<QJsonArray> _locations;  //!< Replace the locations collection.
    std::optional<QJsonValue> _rawInput;  //!< Update the raw input.
    std::optional<QJsonValue> _rawOutput;  //!< Update the raw output.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ToolCallUpdate& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }
    ToolCallUpdate& kind(const std::optional<ToolKind> & v) { _kind = v; return *this; }
    ToolCallUpdate& status(const std::optional<ToolCallStatus> & v) { _status = v; return *this; }
    ToolCallUpdate& title(const std::optional<QString> & v) { _title = v; return *this; }
    ToolCallUpdate& content(const std::optional<QJsonArray> & v) { _content = v; return *this; }
    ToolCallUpdate& locations(const std::optional<QJsonArray> & v) { _locations = v; return *this; }
    ToolCallUpdate& rawInput(const std::optional<QJsonValue> & v) { _rawInput = v; return *this; }
    ToolCallUpdate& rawOutput(const std::optional<QJsonValue> & v) { _rawOutput = v; return *this; }
    ToolCallUpdate& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const ToolCallId& toolCallId() const { return _toolCallId; }
    const std::optional<ToolKind>& kind() const { return _kind; }
    const std::optional<ToolCallStatus>& status() const { return _status; }
    const std::optional<QString>& title() const { return _title; }
    const std::optional<QJsonArray>& content() const { return _content; }
    const std::optional<QJsonArray>& locations() const { return _locations; }
    const std::optional<QJsonValue>& rawInput() const { return _rawInput; }
    const std::optional<QJsonValue>& rawOutput() const { return _rawOutput; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallUpdate> fromJson<ToolCallUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallUpdate &data);

/**
 * Request for user permission to execute a tool call.
 *
 * Sent when the agent needs authorization before performing a sensitive operation.
 *
 * See protocol docs: [Requesting Permission](https://agentclientprotocol.com/protocol/tool-calls#requesting-permission)
 */
struct RequestPermissionRequest {
    SessionId _sessionId;  //!< The session ID for this request.
    ToolCallUpdate _toolCall;  //!< Details about the tool call requiring permission.
    QList<PermissionOption> _options;  //!< Available permission options for the user to choose from.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    RequestPermissionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    RequestPermissionRequest& toolCall(const ToolCallUpdate & v) { _toolCall = v; return *this; }
    RequestPermissionRequest& options(const QList<PermissionOption> & v) { _options = v; return *this; }
    RequestPermissionRequest& addOption(const PermissionOption & v) { _options.append(v); return *this; }
    RequestPermissionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const ToolCallUpdate& toolCall() const { return _toolCall; }
    const QList<PermissionOption>& options() const { return _options; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionRequest> fromJson<RequestPermissionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionRequest &data);

/** Request to get the current output and status of a terminal. */
struct TerminalOutputRequest {
    SessionId _sessionId;  //!< The session ID for this request.
    TerminalId _terminalId;  //!< The ID of the terminal to get output from.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    TerminalOutputRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    TerminalOutputRequest& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    TerminalOutputRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const TerminalId& terminalId() const { return _terminalId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalOutputRequest> fromJson<TerminalOutputRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalOutputRequest &data);

/** Request to wait for a terminal command to exit. */
struct WaitForTerminalExitRequest {
    SessionId _sessionId;  //!< The session ID for this request.
    TerminalId _terminalId;  //!< The ID of the terminal to wait for.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    WaitForTerminalExitRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    WaitForTerminalExitRequest& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    WaitForTerminalExitRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const TerminalId& terminalId() const { return _terminalId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<WaitForTerminalExitRequest> fromJson<WaitForTerminalExitRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const WaitForTerminalExitRequest &data);

/**
 * Request to write content to a text file.
 *
 * Only available if the client supports the `fs.writeTextFile` capability.
 */
struct WriteTextFileRequest {
    SessionId _sessionId;  //!< The session ID for this request.
    QString _path;  //!< Absolute path to the file to write.
    QString _content;  //!< The text content to write to the file.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    WriteTextFileRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    WriteTextFileRequest& path(const QString & v) { _path = v; return *this; }
    WriteTextFileRequest& content(const QString & v) { _content = v; return *this; }
    WriteTextFileRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const QString& path() const { return _path; }
    const QString& content() const { return _content; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<WriteTextFileRequest> fromJson<WriteTextFileRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const WriteTextFileRequest &data);

/** A JSON-RPC request object. */
struct AgentRequest {
    RequestId _id;  //!< The request id used to correlate the matching response.
    QString _method;  //!< The method name to invoke.
    std::optional<QString> _params;  //!< Method-specific request parameters.

    AgentRequest& id(const RequestId & v) { _id = v; return *this; }
    AgentRequest& method(const QString & v) { _method = v; return *this; }
    AgentRequest& params(const std::optional<QString> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentRequest> fromJson<AgentRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentRequest &data);

/** Response to the `authenticate` method. */
struct AuthenticateResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    AuthenticateResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AuthenticateResponse> fromJson<AuthenticateResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AuthenticateResponse &data);

/** Response from closing a session. */
struct CloseSessionResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    CloseSessionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CloseSessionResponse> fromJson<CloseSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CloseSessionResponse &data);

/** Response from deleting a session. */
struct DeleteSessionResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    DeleteSessionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<DeleteSessionResponse> fromJson<DeleteSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DeleteSessionResponse &data);

/**
 * Predefined error codes for common JSON-RPC and ACP-specific errors.
 *
 * These codes follow the JSON-RPC 2.0 specification for standard errors
 * and use the reserved range (-32000 to -32099) for protocol-specific errors.
 */
namespace ErrorCode {
    constexpr int Parse_error = -32700;
    constexpr int Invalid_request = -32600;
    constexpr int Method_not_found = -32601;
    constexpr int Invalid_params = -32602;
    constexpr int Internal_error = -32603;
    constexpr int Authentication_required = -32000;
    constexpr int Resource_not_found = -32002;
} // namespace ErrorCode
/**
 * JSON-RPC error object.
 *
 * Represents an error that occurred during method execution, following the
 * JSON-RPC 2.0 error object specification with optional additional data.
 *
 * See protocol docs: [JSON-RPC Error Object](https://www.jsonrpc.org/specification#error_object)
 */
struct Error {
    /**
     * A number indicating the error type that occurred.
     * This must be an integer as defined in the JSON-RPC specification.
     */
    int _code;
    /**
     * A string providing a short description of the error.
     * The message should be limited to a concise single sentence.
     */
    QString _message;
    /**
     * Optional primitive or structured value that contains additional information about the error.
     * This may include debugging information or context-specific details.
     */
    std::optional<QJsonValue> _data;

    Error& code(int v) { _code = v; return *this; }
    Error& message(const QString & v) { _message = v; return *this; }
    Error& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }

    const int& code() const { return _code; }
    const QString& message() const { return _message; }
    const std::optional<QJsonValue>& data() const { return _data; }
};

template<>
ACPLIB_EXPORT Utils::Result<Error> fromJson<Error>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Error &data);

// Skipped unknown type alias: ExtResponse

/**
 * Logout capabilities supported by the agent.
 *
 * By supplying `{}` it means that the agent supports the logout method.
 */
struct LogoutCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    LogoutCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<LogoutCapabilities> fromJson<LogoutCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LogoutCapabilities &data);

/** Authentication-related capabilities supported by the agent. */
struct AgentAuthCapabilities {
    /**
     * Whether the agent supports the logout method.
     *
     * By supplying `{}` it means that the agent supports the logout method.
     */
    std::optional<LogoutCapabilities> _logout;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    AgentAuthCapabilities& logout(const std::optional<LogoutCapabilities> & v) { _logout = v; return *this; }
    AgentAuthCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<LogoutCapabilities>& logout() const { return _logout; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentAuthCapabilities> fromJson<AgentAuthCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentAuthCapabilities &data);

/** MCP capabilities supported by the agent */
struct McpCapabilities {
    std::optional<bool> _http;  //!< Agent supports [`McpServer::Http`].
    std::optional<bool> _sse;  //!< Agent supports [`McpServer::Sse`].
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    McpCapabilities& http(std::optional<bool> v) { _http = v; return *this; }
    McpCapabilities& sse(std::optional<bool> v) { _sse = v; return *this; }
    McpCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<bool>& http() const { return _http; }
    const std::optional<bool>& sse() const { return _sse; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpCapabilities> fromJson<McpCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpCapabilities &data);

/**
 * Prompt capabilities supported by the agent in `session/prompt` requests.
 *
 * Baseline agent functionality requires support for [`ContentBlock::Text`]
 * and [`ContentBlock::ResourceLink`] in prompt requests.
 *
 * Other variants must be explicitly opted in to.
 * Capabilities for different types of content in prompt requests.
 *
 * Indicates which content types beyond the baseline (text and resource links)
 * the agent can process.
 *
 * See protocol docs: [Prompt Capabilities](https://agentclientprotocol.com/protocol/initialization#prompt-capabilities)
 */
struct PromptCapabilities {
    std::optional<bool> _image;  //!< Agent supports [`ContentBlock::Image`].
    std::optional<bool> _audio;  //!< Agent supports [`ContentBlock::Audio`].
    /**
     * Agent supports embedded context in `session/prompt` requests.
     *
     * When enabled, the Client is allowed to include [`ContentBlock::Resource`]
     * in prompt requests for pieces of context that are referenced in the message.
     */
    std::optional<bool> _embeddedContext;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    PromptCapabilities& image(std::optional<bool> v) { _image = v; return *this; }
    PromptCapabilities& audio(std::optional<bool> v) { _audio = v; return *this; }
    PromptCapabilities& embeddedContext(std::optional<bool> v) { _embeddedContext = v; return *this; }
    PromptCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<bool>& image() const { return _image; }
    const std::optional<bool>& audio() const { return _audio; }
    const std::optional<bool>& embeddedContext() const { return _embeddedContext; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptCapabilities> fromJson<PromptCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptCapabilities &data);

/**
 * Capabilities for additional session directories support.
 *
 * By supplying `{}` it means that the agent supports the `additionalDirectories`
 * field on supported session lifecycle requests. Agents that also support
 * `session/list` may return `SessionInfo.additionalDirectories` to report the
 * complete ordered additional-root list associated with a listed session.
 */
struct SessionAdditionalDirectoriesCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionAdditionalDirectoriesCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionAdditionalDirectoriesCapabilities> fromJson<SessionAdditionalDirectoriesCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionAdditionalDirectoriesCapabilities &data);

/**
 * Capabilities for the `session/close` method.
 *
 * By supplying `{}` it means that the agent supports closing of sessions.
 */
struct SessionCloseCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionCloseCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionCloseCapabilities> fromJson<SessionCloseCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionCloseCapabilities &data);

/**
 * Capabilities for the `session/delete` method.
 *
 * Supplying `{}` means the agent supports deleting sessions from `session/list`.
 */
struct SessionDeleteCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionDeleteCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionDeleteCapabilities> fromJson<SessionDeleteCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionDeleteCapabilities &data);

/**
 * Capabilities for the `session/list` method.
 *
 * By supplying `{}` it means that the agent supports listing of sessions.
 */
struct SessionListCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionListCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionListCapabilities> fromJson<SessionListCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionListCapabilities &data);

/**
 * Capabilities for the `session/resume` method.
 *
 * By supplying `{}` it means that the agent supports resuming of sessions.
 */
struct SessionResumeCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionResumeCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionResumeCapabilities> fromJson<SessionResumeCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionResumeCapabilities &data);

/**
 * Session capabilities supported by the agent.
 *
 * As a baseline, all Agents **MUST** support `session/new`, `session/prompt`, `session/cancel`, and `session/update`.
 *
 * Optionally, they **MAY** support other session methods and notifications by specifying additional capabilities.
 *
 * Note: `session/load` is still handled by the top-level `load_session` capability. This will be unified in future versions of the protocol.
 *
 * See protocol docs: [Session Capabilities](https://agentclientprotocol.com/protocol/initialization#session-capabilities)
 */
struct SessionCapabilities {
    std::optional<SessionListCapabilities> _list;  //!< Whether the agent supports `session/list`.
    /**
     * Whether the agent supports `session/delete`.
     *
     * Optional. Omitted or `null` both mean the agent does not advertise support.
     * Supplying `{}` means the agent supports deleting sessions from `session/list`.
     */
    std::optional<SessionDeleteCapabilities> _delete_;
    /**
     * Whether the agent supports `additionalDirectories` on supported session lifecycle requests.
     *
     * Agents that also support `session/list` may return
     * `SessionInfo.additionalDirectories` to report the complete ordered
     * additional-root list associated with a listed session.
     */
    std::optional<SessionAdditionalDirectoriesCapabilities> _additionalDirectories;
    std::optional<SessionResumeCapabilities> _resume;  //!< Whether the agent supports `session/resume`.
    std::optional<SessionCloseCapabilities> _close;  //!< Whether the agent supports `session/close`.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionCapabilities& list(const std::optional<SessionListCapabilities> & v) { _list = v; return *this; }
    SessionCapabilities& delete_(const std::optional<SessionDeleteCapabilities> & v) { _delete_ = v; return *this; }
    SessionCapabilities& additionalDirectories(const std::optional<SessionAdditionalDirectoriesCapabilities> & v) { _additionalDirectories = v; return *this; }
    SessionCapabilities& resume(const std::optional<SessionResumeCapabilities> & v) { _resume = v; return *this; }
    SessionCapabilities& close(const std::optional<SessionCloseCapabilities> & v) { _close = v; return *this; }
    SessionCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<SessionListCapabilities>& list() const { return _list; }
    const std::optional<SessionDeleteCapabilities>& delete_() const { return _delete_; }
    const std::optional<SessionAdditionalDirectoriesCapabilities>& additionalDirectories() const { return _additionalDirectories; }
    const std::optional<SessionResumeCapabilities>& resume() const { return _resume; }
    const std::optional<SessionCloseCapabilities>& close() const { return _close; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionCapabilities> fromJson<SessionCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionCapabilities &data);

/**
 * Capabilities supported by the agent.
 *
 * Advertised during initialization to inform the client about
 * available features and content types.
 *
 * See protocol docs: [Agent Capabilities](https://agentclientprotocol.com/protocol/initialization#agent-capabilities)
 */
struct AgentCapabilities {
    std::optional<bool> _loadSession;  //!< Whether the agent supports `session/load`.
    std::optional<PromptCapabilities> _promptCapabilities;  //!< Prompt capabilities supported by the agent.
    std::optional<McpCapabilities> _mcpCapabilities;  //!< MCP capabilities supported by the agent.
    std::optional<SessionCapabilities> _sessionCapabilities;  //!< Session lifecycle and prompt capabilities advertised by the agent.
    std::optional<AgentAuthCapabilities> _auth;  //!< Authentication-related capabilities supported by the agent.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    AgentCapabilities& loadSession(std::optional<bool> v) { _loadSession = v; return *this; }
    AgentCapabilities& promptCapabilities(const std::optional<PromptCapabilities> & v) { _promptCapabilities = v; return *this; }
    AgentCapabilities& mcpCapabilities(const std::optional<McpCapabilities> & v) { _mcpCapabilities = v; return *this; }
    AgentCapabilities& sessionCapabilities(const std::optional<SessionCapabilities> & v) { _sessionCapabilities = v; return *this; }
    AgentCapabilities& auth(const std::optional<AgentAuthCapabilities> & v) { _auth = v; return *this; }
    AgentCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<bool>& loadSession() const { return _loadSession; }
    const std::optional<PromptCapabilities>& promptCapabilities() const { return _promptCapabilities; }
    const std::optional<McpCapabilities>& mcpCapabilities() const { return _mcpCapabilities; }
    const std::optional<SessionCapabilities>& sessionCapabilities() const { return _sessionCapabilities; }
    const std::optional<AgentAuthCapabilities>& auth() const { return _auth; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentCapabilities> fromJson<AgentCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentCapabilities &data);

using AuthMethodId = QString;

/**
 * Agent handles authentication itself.
 *
 * This is the default authentication method type.
 */
struct AuthMethodAgent {
    AuthMethodId _id;  //!< Unique identifier for this authentication method.
    QString _name;  //!< Human-readable name of the authentication method.
    std::optional<QString> _description;  //!< Optional description providing more details about this authentication method.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    AuthMethodAgent& id(const AuthMethodId & v) { _id = v; return *this; }
    AuthMethodAgent& name(const QString & v) { _name = v; return *this; }
    AuthMethodAgent& description(const std::optional<QString> & v) { _description = v; return *this; }
    AuthMethodAgent& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const AuthMethodId& id() const { return _id; }
    const QString& name() const { return _name; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AuthMethodAgent> fromJson<AuthMethodAgent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AuthMethodAgent &data);

/**
 * Describes an available authentication method.
 *
 * The `type` field acts as the discriminator in the serialized JSON form.
 * When no `type` is present, the method is treated as `agent`.
 */
using AuthMethod = std::variant<AuthMethodAgent>;

template<>
ACPLIB_EXPORT Utils::Result<AuthMethod> fromJson<AuthMethod>(const QJsonValue &val);

/** Returns the 'name' field from the active variant. */
ACPLIB_EXPORT QString name(const AuthMethod &val);

ACPLIB_EXPORT QJsonObject toJson(const AuthMethod &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const AuthMethod &val);

/**
 * Metadata about the implementation of the client or agent.
 * Describes the name and version of an MCP implementation, with an optional
 * title for UI representation.
 */
struct Implementation {
    /**
     * Intended for programmatic or logical use, but can be used as a display
     * name fallback if title isn’t present.
     */
    QString _name;
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable
     * and easily understood.
     *
     * If not provided, the name should be used for display.
     */
    std::optional<QString> _title;
    /**
     * Version of the implementation. Can be displayed to the user or used
     * for debugging or metrics purposes. (e.g. "1.0.0").
     */
    QString _version;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    Implementation& name(const QString & v) { _name = v; return *this; }
    Implementation& title(const std::optional<QString> & v) { _title = v; return *this; }
    Implementation& version(const QString & v) { _version = v; return *this; }
    Implementation& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const std::optional<QString>& title() const { return _title; }
    const QString& version() const { return _version; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Implementation> fromJson<Implementation>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Implementation &data);

using ProtocolVersion = int;
template<>
ACPLIB_EXPORT Utils::Result<ProtocolVersion> fromJson<ProtocolVersion>(const QJsonValue &val);

/**
 * Response to the `initialize` method.
 *
 * Contains the negotiated protocol version and agent capabilities.
 *
 * See protocol docs: [Initialization](https://agentclientprotocol.com/protocol/initialization)
 */
struct InitializeResponse {
    /**
     * The protocol version the client specified if supported by the agent,
     * or the latest protocol version supported by the agent.
     *
     * The client should disconnect, if it doesn't support this version.
     */
    ProtocolVersion _protocolVersion;
    std::optional<AgentCapabilities> _agentCapabilities;  //!< Capabilities supported by the agent.
    std::optional<QList<AuthMethod>> _authMethods;  //!< Authentication methods supported by the agent.
    /**
     * Information about the Agent name and version sent to the Client.
     *
     * Note: in future versions of the protocol, this will be required.
     */
    std::optional<Implementation> _agentInfo;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    InitializeResponse& protocolVersion(const ProtocolVersion & v) { _protocolVersion = v; return *this; }
    InitializeResponse& agentCapabilities(const std::optional<AgentCapabilities> & v) { _agentCapabilities = v; return *this; }
    InitializeResponse& authMethods(const std::optional<QList<AuthMethod>> & v) { _authMethods = v; return *this; }
    InitializeResponse& addAuthMethod(const AuthMethod & v) { if (!_authMethods) _authMethods = QList<AuthMethod>{}; (*_authMethods).append(v); return *this; }
    InitializeResponse& agentInfo(const std::optional<Implementation> & v) { _agentInfo = v; return *this; }
    InitializeResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const ProtocolVersion& protocolVersion() const { return _protocolVersion; }
    const std::optional<AgentCapabilities>& agentCapabilities() const { return _agentCapabilities; }
    const std::optional<QList<AuthMethod>>& authMethods() const { return _authMethods; }
    const std::optional<Implementation>& agentInfo() const { return _agentInfo; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<InitializeResponse> fromJson<InitializeResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const InitializeResponse &data);

/** Information about a session returned by session/list */
struct SessionInfo {
    SessionId _sessionId;  //!< Unique identifier for the session
    QString _cwd;  //!< The working directory for this session. Must be an absolute path.
    /**
     * Additional workspace roots reported for this session. Each path must be absolute.
     *
     * When present, this is the complete ordered additional-root list reported
     * by the Agent. Omitted and empty values are equivalent: the response
     * reports no additional roots.
     */
    std::optional<QStringList> _additionalDirectories;
    std::optional<QString> _title;  //!< Human-readable title for the session
    std::optional<QString> _updatedAt;  //!< ISO 8601 timestamp of last activity
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionInfo& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    SessionInfo& cwd(const QString & v) { _cwd = v; return *this; }
    SessionInfo& additionalDirectories(const std::optional<QStringList> & v) { _additionalDirectories = v; return *this; }
    SessionInfo& addAdditionalDirectory(const QString & v) { if (!_additionalDirectories) _additionalDirectories = QStringList{}; (*_additionalDirectories).append(v); return *this; }
    SessionInfo& title(const std::optional<QString> & v) { _title = v; return *this; }
    SessionInfo& updatedAt(const std::optional<QString> & v) { _updatedAt = v; return *this; }
    SessionInfo& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const QString& cwd() const { return _cwd; }
    const std::optional<QStringList>& additionalDirectories() const { return _additionalDirectories; }
    const std::optional<QString>& title() const { return _title; }
    const std::optional<QString>& updatedAt() const { return _updatedAt; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionInfo> fromJson<SessionInfo>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionInfo &data);

/** Response from listing sessions. */
struct ListSessionsResponse {
    QList<SessionInfo> _sessions;  //!< Array of session information objects
    /**
     * Opaque cursor token. If present, pass this in the next request's cursor parameter
     * to fetch the next page. If absent, there are no more results.
     */
    std::optional<QString> _nextCursor;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ListSessionsResponse& sessions(const QList<SessionInfo> & v) { _sessions = v; return *this; }
    ListSessionsResponse& addSession(const SessionInfo & v) { _sessions.append(v); return *this; }
    ListSessionsResponse& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    ListSessionsResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QList<SessionInfo>& sessions() const { return _sessions; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ListSessionsResponse> fromJson<ListSessionsResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ListSessionsResponse &data);

using SessionConfigId = QString;

/**
 * Semantic category for a session configuration option.
 *
 * This is intended to help Clients distinguish broadly common selectors (e.g. model selector vs
 * session mode selector vs thought/reasoning level) for UX purposes (keyboard shortcuts, icons,
 * placement). It MUST NOT be required for correctness. Clients MUST handle missing or unknown
 * categories gracefully.
 *
 * Category names beginning with `_` are free for custom use, like other ACP extension methods.
 * Category names that do not begin with `_` are reserved for the ACP spec.
 */
enum class SessionConfigOptionCategory {
    mode,
    model,
    thought_level
};

ACPLIB_EXPORT QString toString(SessionConfigOptionCategory v);

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigOptionCategory> fromJson<SessionConfigOptionCategory>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const SessionConfigOptionCategory &v);

using SessionConfigGroupId = QString;

using SessionConfigValueId = QString;

/** A possible value for a session configuration option. */
struct SessionConfigSelectOption {
    SessionConfigValueId _value;  //!< Unique identifier for this option value.
    QString _name;  //!< Human-readable label for this option value.
    std::optional<QString> _description;  //!< Optional description for this option value.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionConfigSelectOption& value(const SessionConfigValueId & v) { _value = v; return *this; }
    SessionConfigSelectOption& name(const QString & v) { _name = v; return *this; }
    SessionConfigSelectOption& description(const std::optional<QString> & v) { _description = v; return *this; }
    SessionConfigSelectOption& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionConfigValueId& value() const { return _value; }
    const QString& name() const { return _name; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelectOption> fromJson<SessionConfigSelectOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigSelectOption &data);

/** A group of possible values for a session configuration option. */
struct SessionConfigSelectGroup {
    SessionConfigGroupId _group;  //!< Unique identifier for this group.
    QString _name;  //!< Human-readable label for this group.
    QList<SessionConfigSelectOption> _options;  //!< The set of option values in this group.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionConfigSelectGroup& group(const SessionConfigGroupId & v) { _group = v; return *this; }
    SessionConfigSelectGroup& name(const QString & v) { _name = v; return *this; }
    SessionConfigSelectGroup& options(const QList<SessionConfigSelectOption> & v) { _options = v; return *this; }
    SessionConfigSelectGroup& addOption(const SessionConfigSelectOption & v) { _options.append(v); return *this; }
    SessionConfigSelectGroup& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionConfigGroupId& group() const { return _group; }
    const QString& name() const { return _name; }
    const QList<SessionConfigSelectOption>& options() const { return _options; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelectGroup> fromJson<SessionConfigSelectGroup>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigSelectGroup &data);

/** Possible values for a session configuration option. */
using SessionConfigSelectOptions = std::variant<QList<SessionConfigSelectOption>, QList<SessionConfigSelectGroup>>;

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelectOptions> fromJson<SessionConfigSelectOptions>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const SessionConfigSelectOptions &val);

/** A single-value selector (dropdown) session configuration option payload. */
struct SessionConfigSelect {
    SessionConfigValueId _currentValue;  //!< The currently selected value.
    SessionConfigSelectOptions _options;  //!< The set of selectable options.

    SessionConfigSelect& currentValue(const SessionConfigValueId & v) { _currentValue = v; return *this; }
    SessionConfigSelect& options(const SessionConfigSelectOptions & v) { _options = v; return *this; }

    const SessionConfigValueId& currentValue() const { return _currentValue; }
    const SessionConfigSelectOptions& options() const { return _options; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelect> fromJson<SessionConfigSelect>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigSelect &data);

/** A session configuration option selector and its current state. */
struct SessionConfigOption {
    SessionConfigId _id;  //!< Unique identifier for the configuration option.
    QString _name;  //!< Human-readable label for the option.
    std::optional<QString> _description;  //!< Optional description for the Client to display to the user.
    std::optional<SessionConfigOptionCategory> _category;  //!< Optional semantic category for this option (UX only).
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QJsonObject _additionalProperties;  //!< additional properties

    SessionConfigOption& id(const SessionConfigId & v) { _id = v; return *this; }
    SessionConfigOption& name(const QString & v) { _name = v; return *this; }
    SessionConfigOption& description(const std::optional<QString> & v) { _description = v; return *this; }
    SessionConfigOption& category(const std::optional<SessionConfigOptionCategory> & v) { _category = v; return *this; }
    SessionConfigOption& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SessionConfigOption& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    SessionConfigOption& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const SessionConfigId& id() const { return _id; }
    const QString& name() const { return _name; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<SessionConfigOptionCategory>& category() const { return _category; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigOption> fromJson<SessionConfigOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigOption &data);

using SessionModeId = QString;

/**
 * A mode the agent can operate in.
 *
 * See protocol docs: [Session Modes](https://agentclientprotocol.com/protocol/session-modes)
 */
struct SessionMode {
    SessionModeId _id;  //!< Stable identifier used to refer to this protocol object in later messages.
    QString _name;  //!< Human-readable name shown for this protocol object.
    std::optional<QString> _description;  //!< Optional human-readable details shown with this protocol object.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionMode& id(const SessionModeId & v) { _id = v; return *this; }
    SessionMode& name(const QString & v) { _name = v; return *this; }
    SessionMode& description(const std::optional<QString> & v) { _description = v; return *this; }
    SessionMode& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionModeId& id() const { return _id; }
    const QString& name() const { return _name; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionMode> fromJson<SessionMode>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionMode &data);

/** The set of modes and the one currently active. */
struct SessionModeState {
    SessionModeId _currentModeId;  //!< The current mode the Agent is in.
    QList<SessionMode> _availableModes;  //!< The set of modes that the Agent can operate in
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionModeState& currentModeId(const SessionModeId & v) { _currentModeId = v; return *this; }
    SessionModeState& availableModes(const QList<SessionMode> & v) { _availableModes = v; return *this; }
    SessionModeState& addAvailableMode(const SessionMode & v) { _availableModes.append(v); return *this; }
    SessionModeState& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionModeId& currentModeId() const { return _currentModeId; }
    const QList<SessionMode>& availableModes() const { return _availableModes; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionModeState> fromJson<SessionModeState>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionModeState &data);

/** Response from loading an existing session. */
struct LoadSessionResponse {
    /**
     * Initial mode state if supported by the Agent
     *
     * See protocol docs: [Session Modes](https://agentclientprotocol.com/protocol/session-modes)
     */
    std::optional<SessionModeState> _modes;
    std::optional<QJsonArray> _configOptions;  //!< Initial session configuration options if supported by the Agent.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    LoadSessionResponse& modes(const std::optional<SessionModeState> & v) { _modes = v; return *this; }
    LoadSessionResponse& configOptions(const std::optional<QJsonArray> & v) { _configOptions = v; return *this; }
    LoadSessionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<SessionModeState>& modes() const { return _modes; }
    const std::optional<QJsonArray>& configOptions() const { return _configOptions; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<LoadSessionResponse> fromJson<LoadSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LoadSessionResponse &data);

/** Response to the `logout` method. */
struct LogoutResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    LogoutResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<LogoutResponse> fromJson<LogoutResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LogoutResponse &data);

/**
 * Response from creating a new session.
 *
 * See protocol docs: [Creating a Session](https://agentclientprotocol.com/protocol/session-setup#creating-a-session)
 */
struct NewSessionResponse {
    /**
     * Unique identifier for the created session.
     *
     * Used in all subsequent requests for this conversation.
     */
    SessionId _sessionId;
    /**
     * Initial mode state if supported by the Agent
     *
     * See protocol docs: [Session Modes](https://agentclientprotocol.com/protocol/session-modes)
     */
    std::optional<SessionModeState> _modes;
    std::optional<QJsonArray> _configOptions;  //!< Initial session configuration options if supported by the Agent.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    NewSessionResponse& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    NewSessionResponse& modes(const std::optional<SessionModeState> & v) { _modes = v; return *this; }
    NewSessionResponse& configOptions(const std::optional<QJsonArray> & v) { _configOptions = v; return *this; }
    NewSessionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const std::optional<SessionModeState>& modes() const { return _modes; }
    const std::optional<QJsonArray>& configOptions() const { return _configOptions; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<NewSessionResponse> fromJson<NewSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const NewSessionResponse &data);

/**
 * Reasons why an agent stops processing a prompt turn.
 *
 * See protocol docs: [Stop Reasons](https://agentclientprotocol.com/protocol/prompt-turn#stop-reasons)
 */
enum class StopReason {
    end_turn,
    max_tokens,
    max_turn_requests,
    refusal,
    cancelled
};

ACPLIB_EXPORT QString toString(StopReason v);

template<>
ACPLIB_EXPORT Utils::Result<StopReason> fromJson<StopReason>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const StopReason &v);

/**
 * Response from processing a user prompt.
 *
 * See protocol docs: [Check for Completion](https://agentclientprotocol.com/protocol/prompt-turn#4-check-for-completion)
 */
struct PromptResponse {
    StopReason _stopReason;  //!< Indicates why the agent stopped processing the turn.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    PromptResponse& stopReason(const StopReason & v) { _stopReason = v; return *this; }
    PromptResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const StopReason& stopReason() const { return _stopReason; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptResponse> fromJson<PromptResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptResponse &data);

/** Response from resuming an existing session. */
struct ResumeSessionResponse {
    /**
     * Initial mode state if supported by the Agent
     *
     * See protocol docs: [Session Modes](https://agentclientprotocol.com/protocol/session-modes)
     */
    std::optional<SessionModeState> _modes;
    std::optional<QJsonArray> _configOptions;  //!< Initial session configuration options if supported by the Agent.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ResumeSessionResponse& modes(const std::optional<SessionModeState> & v) { _modes = v; return *this; }
    ResumeSessionResponse& configOptions(const std::optional<QJsonArray> & v) { _configOptions = v; return *this; }
    ResumeSessionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<SessionModeState>& modes() const { return _modes; }
    const std::optional<QJsonArray>& configOptions() const { return _configOptions; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ResumeSessionResponse> fromJson<ResumeSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ResumeSessionResponse &data);

/** Response to `session/set_config_option` method. */
struct SetSessionConfigOptionResponse {
    QList<SessionConfigOption> _configOptions;  //!< The full set of configuration options and their current values.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SetSessionConfigOptionResponse& configOptions(const QList<SessionConfigOption> & v) { _configOptions = v; return *this; }
    SetSessionConfigOptionResponse& addConfigOption(const SessionConfigOption & v) { _configOptions.append(v); return *this; }
    SetSessionConfigOptionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QList<SessionConfigOption>& configOptions() const { return _configOptions; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionConfigOptionResponse> fromJson<SetSessionConfigOptionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionConfigOptionResponse &data);

/** Response to `session/set_mode` method. */
struct SetSessionModeResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SetSessionModeResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionModeResponse> fromJson<SetSessionModeResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionModeResponse &data);

/** A JSON-RPC response object. */
using AgentResponse = std::variant<QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<AgentResponse> fromJson<AgentResponse>(const QJsonValue & /*val*/);

ACPLIB_EXPORT QJsonValue toJsonValue(const AgentResponse &val);

// Skipped unknown type alias: ExtNotification

/** All text that was typed after the command name is provided as input. */
struct UnstructuredCommandInput {
    QString _hint;  //!< A hint to display when the input hasn't been provided yet
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    UnstructuredCommandInput& hint(const QString & v) { _hint = v; return *this; }
    UnstructuredCommandInput& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& hint() const { return _hint; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<UnstructuredCommandInput> fromJson<UnstructuredCommandInput>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const UnstructuredCommandInput &data);

/** The input specification for a command. */
using AvailableCommandInput = std::variant<UnstructuredCommandInput>;

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommandInput> fromJson<AvailableCommandInput>(const QJsonValue &val);

/** Returns the 'hint' field from the active variant. */
ACPLIB_EXPORT QString hint(const AvailableCommandInput &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommandInput &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const AvailableCommandInput &val);

/** Information about a command. */
struct AvailableCommand {
    QString _name;  //!< Command name (e.g., `create_plan`, `research_codebase`).
    QString _description;  //!< Human-readable description of what the command does.
    std::optional<AvailableCommandInput> _input;  //!< Input for the command if required
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    AvailableCommand& name(const QString & v) { _name = v; return *this; }
    AvailableCommand& description(const QString & v) { _description = v; return *this; }
    AvailableCommand& input(const std::optional<AvailableCommandInput> & v) { _input = v; return *this; }
    AvailableCommand& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& description() const { return _description; }
    const std::optional<AvailableCommandInput>& input() const { return _input; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommand> fromJson<AvailableCommand>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommand &data);

/** Available commands are ready or have changed */
struct AvailableCommandsUpdate {
    QList<AvailableCommand> _availableCommands;  //!< Commands the agent can execute
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    AvailableCommandsUpdate& availableCommands(const QList<AvailableCommand> & v) { _availableCommands = v; return *this; }
    AvailableCommandsUpdate& addAvailableCommand(const AvailableCommand & v) { _availableCommands.append(v); return *this; }
    AvailableCommandsUpdate& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QList<AvailableCommand>& availableCommands() const { return _availableCommands; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommandsUpdate> fromJson<AvailableCommandsUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommandsUpdate &data);

/** Session configuration options have been updated. */
struct ConfigOptionUpdate {
    QList<SessionConfigOption> _configOptions;  //!< The full set of configuration options and their current values.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ConfigOptionUpdate& configOptions(const QList<SessionConfigOption> & v) { _configOptions = v; return *this; }
    ConfigOptionUpdate& addConfigOption(const SessionConfigOption & v) { _configOptions.append(v); return *this; }
    ConfigOptionUpdate& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QList<SessionConfigOption>& configOptions() const { return _configOptions; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ConfigOptionUpdate> fromJson<ConfigOptionUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ConfigOptionUpdate &data);

using MessageId = QString;

/** A streamed item of content */
struct ContentChunk {
    ContentBlock _content;  //!< A single item of content
    /**
     * A unique identifier for the message this chunk belongs to.
     *
     * All chunks belonging to the same message share the same `messageId`.
     * A change in `messageId` indicates a new message has started.
     */
    std::optional<MessageId> _messageId;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ContentChunk& content(const ContentBlock & v) { _content = v; return *this; }
    ContentChunk& messageId(const std::optional<MessageId> & v) { _messageId = v; return *this; }
    ContentChunk& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const ContentBlock& content() const { return _content; }
    const std::optional<MessageId>& messageId() const { return _messageId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ContentChunk> fromJson<ContentChunk>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ContentChunk &data);

/**
 * The current mode of the session has changed
 *
 * See protocol docs: [Session Modes](https://agentclientprotocol.com/protocol/session-modes)
 */
struct CurrentModeUpdate {
    SessionModeId _currentModeId;  //!< The ID of the current mode
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    CurrentModeUpdate& currentModeId(const SessionModeId & v) { _currentModeId = v; return *this; }
    CurrentModeUpdate& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionModeId& currentModeId() const { return _currentModeId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CurrentModeUpdate> fromJson<CurrentModeUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CurrentModeUpdate &data);

/**
 * Priority levels for plan entries.
 *
 * Used to indicate the relative importance or urgency of different
 * tasks in the execution plan.
 * See protocol docs: [Plan Entries](https://agentclientprotocol.com/protocol/agent-plan#plan-entries)
 */
enum class PlanEntryPriority {
    high,
    medium,
    low
};

ACPLIB_EXPORT QString toString(PlanEntryPriority v);

template<>
ACPLIB_EXPORT Utils::Result<PlanEntryPriority> fromJson<PlanEntryPriority>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const PlanEntryPriority &v);

/**
 * Status of a plan entry in the execution flow.
 *
 * Tracks the lifecycle of each task from planning through completion.
 * See protocol docs: [Plan Entries](https://agentclientprotocol.com/protocol/agent-plan#plan-entries)
 */
enum class PlanEntryStatus {
    pending,
    in_progress,
    completed
};

ACPLIB_EXPORT QString toString(PlanEntryStatus v);

template<>
ACPLIB_EXPORT Utils::Result<PlanEntryStatus> fromJson<PlanEntryStatus>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const PlanEntryStatus &v);

/**
 * A single entry in the execution plan.
 *
 * Represents a task or goal that the assistant intends to accomplish
 * as part of fulfilling the user's request.
 * See protocol docs: [Plan Entries](https://agentclientprotocol.com/protocol/agent-plan#plan-entries)
 */
struct PlanEntry {
    QString _content;  //!< Human-readable description of what this task aims to accomplish.
    /**
     * The relative importance of this task.
     * Used to indicate which tasks are most critical to the overall goal.
     */
    PlanEntryPriority _priority;
    PlanEntryStatus _status;  //!< Current execution status of this task.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    PlanEntry& content(const QString & v) { _content = v; return *this; }
    PlanEntry& priority(const PlanEntryPriority & v) { _priority = v; return *this; }
    PlanEntry& status(const PlanEntryStatus & v) { _status = v; return *this; }
    PlanEntry& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& content() const { return _content; }
    const PlanEntryPriority& priority() const { return _priority; }
    const PlanEntryStatus& status() const { return _status; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PlanEntry> fromJson<PlanEntry>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PlanEntry &data);

/**
 * An execution plan for accomplishing complex tasks.
 *
 * Plans consist of multiple entries representing individual tasks or goals.
 * Agents report plans to clients to provide visibility into their execution strategy.
 * Plans can evolve during execution as the agent discovers new requirements or completes tasks.
 *
 * See protocol docs: [Agent Plan](https://agentclientprotocol.com/protocol/agent-plan)
 */
struct Plan {
    /**
     * The list of tasks to be accomplished.
     *
     * When updating a plan, the agent must send a complete list of all entries
     * with their current status. The client replaces the entire plan with each update.
     */
    QList<PlanEntry> _entries;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    Plan& entries(const QList<PlanEntry> & v) { _entries = v; return *this; }
    Plan& addEntry(const PlanEntry & v) { _entries.append(v); return *this; }
    Plan& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QList<PlanEntry>& entries() const { return _entries; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Plan> fromJson<Plan>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Plan &data);

/**
 * Update to session metadata. All fields are optional to support partial updates.
 *
 * Agents send this notification to update session information like title or custom metadata.
 * This allows clients to display dynamic session names and track session state changes.
 */
struct SessionInfoUpdate {
    std::optional<QString> _title;  //!< Human-readable title for the session. Set to null to clear.
    std::optional<QString> _updatedAt;  //!< ISO 8601 timestamp of last activity. Set to null to clear.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionInfoUpdate& title(const std::optional<QString> & v) { _title = v; return *this; }
    SessionInfoUpdate& updatedAt(const std::optional<QString> & v) { _updatedAt = v; return *this; }
    SessionInfoUpdate& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QString>& title() const { return _title; }
    const std::optional<QString>& updatedAt() const { return _updatedAt; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionInfoUpdate> fromJson<SessionInfoUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionInfoUpdate &data);

/**
 * Represents a tool call that the language model has requested.
 *
 * Tool calls are actions that the agent executes on behalf of the language model,
 * such as reading files, executing code, or fetching data from external sources.
 *
 * See protocol docs: [Tool Calls](https://agentclientprotocol.com/protocol/tool-calls)
 */
struct ToolCall {
    ToolCallId _toolCallId;  //!< Unique identifier for this tool call within the session.
    QString _title;  //!< Human-readable title describing what the tool is doing.
    /**
     * The category of tool being invoked.
     * Helps clients choose appropriate icons and UI treatment.
     */
    std::optional<ToolKind> _kind;
    std::optional<ToolCallStatus> _status;  //!< Current execution status of the tool call.
    std::optional<QList<ToolCallContent>> _content;  //!< Content produced by the tool call.
    /**
     * File locations affected by this tool call.
     * Enables "follow-along" features in clients.
     */
    std::optional<QList<ToolCallLocation>> _locations;
    std::optional<QJsonValue> _rawInput;  //!< Raw input parameters sent to the tool.
    std::optional<QJsonValue> _rawOutput;  //!< Raw output returned by the tool.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ToolCall& toolCallId(const ToolCallId & v) { _toolCallId = v; return *this; }
    ToolCall& title(const QString & v) { _title = v; return *this; }
    ToolCall& kind(const std::optional<ToolKind> & v) { _kind = v; return *this; }
    ToolCall& status(const std::optional<ToolCallStatus> & v) { _status = v; return *this; }
    ToolCall& content(const std::optional<QList<ToolCallContent>> & v) { _content = v; return *this; }
    ToolCall& addContent(const ToolCallContent & v) { if (!_content) _content = QList<ToolCallContent>{}; (*_content).append(v); return *this; }
    ToolCall& locations(const std::optional<QList<ToolCallLocation>> & v) { _locations = v; return *this; }
    ToolCall& addLocation(const ToolCallLocation & v) { if (!_locations) _locations = QList<ToolCallLocation>{}; (*_locations).append(v); return *this; }
    ToolCall& rawInput(const std::optional<QJsonValue> & v) { _rawInput = v; return *this; }
    ToolCall& rawOutput(const std::optional<QJsonValue> & v) { _rawOutput = v; return *this; }
    ToolCall& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const ToolCallId& toolCallId() const { return _toolCallId; }
    const QString& title() const { return _title; }
    const std::optional<ToolKind>& kind() const { return _kind; }
    const std::optional<ToolCallStatus>& status() const { return _status; }
    const std::optional<QList<ToolCallContent>>& content() const { return _content; }
    const std::optional<QList<ToolCallLocation>>& locations() const { return _locations; }
    const std::optional<QJsonValue>& rawInput() const { return _rawInput; }
    const std::optional<QJsonValue>& rawOutput() const { return _rawOutput; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCall> fromJson<ToolCall>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCall &data);

/** Cost information for a session. */
struct Cost {
    double _amount;  //!< Total cumulative cost for session.
    QString _currency;  //!< ISO 4217 currency code (e.g., "USD", "EUR").
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    Cost& amount(double v) { _amount = v; return *this; }
    Cost& currency(const QString & v) { _currency = v; return *this; }
    Cost& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const double& amount() const { return _amount; }
    const QString& currency() const { return _currency; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<Cost> fromJson<Cost>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Cost &data);

/** Context window and cost update for a session. */
struct UsageUpdate {
    int _used;  //!< Tokens currently in context.
    int _size;  //!< Total context window size in tokens.
    std::optional<Cost> _cost;  //!< Cumulative session cost (optional).
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    UsageUpdate& used(int v) { _used = v; return *this; }
    UsageUpdate& size(int v) { _size = v; return *this; }
    UsageUpdate& cost(const std::optional<Cost> & v) { _cost = v; return *this; }
    UsageUpdate& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const int& used() const { return _used; }
    const int& size() const { return _size; }
    const std::optional<Cost>& cost() const { return _cost; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<UsageUpdate> fromJson<UsageUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const UsageUpdate &data);

/**
 * Different types of updates that can be sent during session processing.
 *
 * These updates provide real-time feedback about the agent's progress.
 *
 * See protocol docs: [Agent Reports Output](https://agentclientprotocol.com/protocol/prompt-turn#3-agent-reports-output)
 */
struct SessionUpdate {
    using Variant = std::variant<ContentChunk, ToolCall, ToolCallUpdate, Plan, AvailableCommandsUpdate, CurrentModeUpdate, ConfigOptionUpdate, SessionInfoUpdate, UsageUpdate>;
    Variant _value;
    QString _kind;  //!< discriminator value (sessionUpdate)

    template<typename T> const T* get() const { return std::get_if<T>(&_value); }
    const QString& kind() const { return _kind; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionUpdate> fromJson<SessionUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionUpdate &data);

ACPLIB_EXPORT QJsonValue toJsonValue(const SessionUpdate &val);

/**
 * Notification containing a session update from the agent.
 *
 * Used to stream real-time progress and results during prompt processing.
 *
 * See protocol docs: [Agent Reports Output](https://agentclientprotocol.com/protocol/prompt-turn#3-agent-reports-output)
 */
struct SessionNotification {
    SessionId _sessionId;  //!< The ID of the session this update pertains to.
    SessionUpdate _update;  //!< The actual update content.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionNotification& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    SessionNotification& update(const SessionUpdate & v) { _update = v; return *this; }
    SessionNotification& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const SessionUpdate& update() const { return _update; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionNotification> fromJson<SessionNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionNotification &data);

/** A JSON-RPC notification object. */
struct AgentNotification {
    QString _method;  //!< The notification method name.
    std::optional<QString> _params;  //!< Method-specific notification parameters.

    AgentNotification& method(const QString & v) { _method = v; return *this; }
    AgentNotification& params(const std::optional<QString> & v) { _params = v; return *this; }

    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentNotification> fromJson<AgentNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentNotification &data);

/**
 * Request parameters for the authenticate method.
 *
 * Specifies which authentication method to use.
 */
struct AuthenticateRequest {
    /**
     * The ID of the authentication method to use.
     * Must be one of the methods advertised in the initialize response.
     */
    AuthMethodId _methodId;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    AuthenticateRequest& methodId(const AuthMethodId & v) { _methodId = v; return *this; }
    AuthenticateRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const AuthMethodId& methodId() const { return _methodId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<AuthenticateRequest> fromJson<AuthenticateRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AuthenticateRequest &data);

/**
 * Request parameters for closing an active session.
 *
 * If supported, the agent **must** cancel any ongoing work related to the session
 * (treat it as if `session/cancel` was called) and then free up any resources
 * associated with the session.
 *
 * Only available if the Agent supports the `sessionCapabilities.close` capability.
 */
struct CloseSessionRequest {
    SessionId _sessionId;  //!< The ID of the session to close.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    CloseSessionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    CloseSessionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CloseSessionRequest> fromJson<CloseSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CloseSessionRequest &data);

/**
 * Request parameters for deleting an existing session from `session/list`.
 *
 * Only available if the Agent supports the `sessionCapabilities.delete` capability.
 */
struct DeleteSessionRequest {
    SessionId _sessionId;  //!< The ID of the session to delete.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    DeleteSessionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    DeleteSessionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<DeleteSessionRequest> fromJson<DeleteSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const DeleteSessionRequest &data);

/**
 * File system capabilities that a client may support.
 *
 * See protocol docs: [FileSystem](https://agentclientprotocol.com/protocol/initialization#filesystem)
 */
struct FileSystemCapabilities {
    std::optional<bool> _readTextFile;  //!< Whether the Client supports `fs/read_text_file` requests.
    std::optional<bool> _writeTextFile;  //!< Whether the Client supports `fs/write_text_file` requests.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    FileSystemCapabilities& readTextFile(std::optional<bool> v) { _readTextFile = v; return *this; }
    FileSystemCapabilities& writeTextFile(std::optional<bool> v) { _writeTextFile = v; return *this; }
    FileSystemCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<bool>& readTextFile() const { return _readTextFile; }
    const std::optional<bool>& writeTextFile() const { return _writeTextFile; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<FileSystemCapabilities> fromJson<FileSystemCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const FileSystemCapabilities &data);

/**
 * Capabilities supported by the client.
 *
 * Advertised during initialization to inform the agent about
 * available features and methods.
 *
 * See protocol docs: [Client Capabilities](https://agentclientprotocol.com/protocol/initialization#client-capabilities)
 */
struct ClientCapabilities {
    /**
     * File system capabilities supported by the client.
     * Determines which file operations the agent can request.
     */
    std::optional<FileSystemCapabilities> _fs;
    std::optional<bool> _terminal;  //!< Whether the Client support all `terminal/*` methods.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ClientCapabilities& fs(const std::optional<FileSystemCapabilities> & v) { _fs = v; return *this; }
    ClientCapabilities& terminal(std::optional<bool> v) { _terminal = v; return *this; }
    ClientCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<FileSystemCapabilities>& fs() const { return _fs; }
    const std::optional<bool>& terminal() const { return _terminal; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ClientCapabilities &data);

/**
 * Request parameters for the initialize method.
 *
 * Sent by the client to establish connection and negotiate capabilities.
 *
 * See protocol docs: [Initialization](https://agentclientprotocol.com/protocol/initialization)
 */
struct InitializeRequest {
    ProtocolVersion _protocolVersion;  //!< The latest protocol version supported by the client.
    std::optional<ClientCapabilities> _clientCapabilities;  //!< Capabilities supported by the client.
    /**
     * Information about the Client name and version sent to the Agent.
     *
     * Note: in future versions of the protocol, this will be required.
     */
    std::optional<Implementation> _clientInfo;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    InitializeRequest& protocolVersion(const ProtocolVersion & v) { _protocolVersion = v; return *this; }
    InitializeRequest& clientCapabilities(const std::optional<ClientCapabilities> & v) { _clientCapabilities = v; return *this; }
    InitializeRequest& clientInfo(const std::optional<Implementation> & v) { _clientInfo = v; return *this; }
    InitializeRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const ProtocolVersion& protocolVersion() const { return _protocolVersion; }
    const std::optional<ClientCapabilities>& clientCapabilities() const { return _clientCapabilities; }
    const std::optional<Implementation>& clientInfo() const { return _clientInfo; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<InitializeRequest> fromJson<InitializeRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const InitializeRequest &data);

/**
 * Request parameters for listing existing sessions.
 *
 * Only available if the Agent supports the `sessionCapabilities.list` capability.
 */
struct ListSessionsRequest {
    std::optional<QString> _cwd;  //!< Filter sessions by working directory. Must be an absolute path.
    std::optional<QString> _cursor;  //!< Opaque cursor token from a previous response's nextCursor field for cursor-based pagination
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ListSessionsRequest& cwd(const std::optional<QString> & v) { _cwd = v; return *this; }
    ListSessionsRequest& cursor(const std::optional<QString> & v) { _cursor = v; return *this; }
    ListSessionsRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QString>& cwd() const { return _cwd; }
    const std::optional<QString>& cursor() const { return _cursor; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ListSessionsRequest> fromJson<ListSessionsRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ListSessionsRequest &data);

/** An HTTP header to set when making requests to the MCP server. */
struct HttpHeader {
    QString _name;  //!< The name of the HTTP header.
    QString _value;  //!< The value to set for the HTTP header.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    HttpHeader& name(const QString & v) { _name = v; return *this; }
    HttpHeader& value(const QString & v) { _value = v; return *this; }
    HttpHeader& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& value() const { return _value; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<HttpHeader> fromJson<HttpHeader>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const HttpHeader &data);

/** HTTP transport configuration for MCP. */
struct McpServerHttp {
    QString _name;  //!< Human-readable name identifying this MCP server.
    QString _url;  //!< URL to the MCP server.
    QList<HttpHeader> _headers;  //!< HTTP headers to set when making requests to the MCP server.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    McpServerHttp& name(const QString & v) { _name = v; return *this; }
    McpServerHttp& url(const QString & v) { _url = v; return *this; }
    McpServerHttp& headers(const QList<HttpHeader> & v) { _headers = v; return *this; }
    McpServerHttp& addHeader(const HttpHeader & v) { _headers.append(v); return *this; }
    McpServerHttp& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& url() const { return _url; }
    const QList<HttpHeader>& headers() const { return _headers; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpServerHttp> fromJson<McpServerHttp>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServerHttp &data);

/** SSE transport configuration for MCP. */
struct McpServerSse {
    QString _name;  //!< Human-readable name identifying this MCP server.
    QString _url;  //!< URL to the MCP server.
    QList<HttpHeader> _headers;  //!< HTTP headers to set when making requests to the MCP server.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    McpServerSse& name(const QString & v) { _name = v; return *this; }
    McpServerSse& url(const QString & v) { _url = v; return *this; }
    McpServerSse& headers(const QList<HttpHeader> & v) { _headers = v; return *this; }
    McpServerSse& addHeader(const HttpHeader & v) { _headers.append(v); return *this; }
    McpServerSse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& url() const { return _url; }
    const QList<HttpHeader>& headers() const { return _headers; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpServerSse> fromJson<McpServerSse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServerSse &data);

/** Stdio transport configuration for MCP. */
struct McpServerStdio {
    QString _name;  //!< Human-readable name identifying this MCP server.
    QString _command;  //!< Path to the MCP server executable.
    QStringList _args;  //!< Command-line arguments to pass to the MCP server.
    QList<EnvVariable> _env;  //!< Environment variables to set when launching the MCP server.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    McpServerStdio& name(const QString & v) { _name = v; return *this; }
    McpServerStdio& command(const QString & v) { _command = v; return *this; }
    McpServerStdio& args(const QStringList & v) { _args = v; return *this; }
    McpServerStdio& addArg(const QString & v) { _args.append(v); return *this; }
    McpServerStdio& env(const QList<EnvVariable> & v) { _env = v; return *this; }
    McpServerStdio& addEnv(const EnvVariable & v) { _env.append(v); return *this; }
    McpServerStdio& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& name() const { return _name; }
    const QString& command() const { return _command; }
    const QStringList& args() const { return _args; }
    const QList<EnvVariable>& env() const { return _env; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpServerStdio> fromJson<McpServerStdio>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServerStdio &data);

/**
 * Configuration for connecting to an MCP (Model Context Protocol) server.
 *
 * MCP servers provide tools and context that the agent can use when
 * processing prompts.
 *
 * See protocol docs: [MCP Servers](https://agentclientprotocol.com/protocol/session-setup#mcp-servers)
 */
using McpServer = std::variant<McpServerHttp, McpServerSse, McpServerStdio>;

template<>
ACPLIB_EXPORT Utils::Result<McpServer> fromJson<McpServer>(const QJsonValue &val);

/** Returns the 'type' dispatch field value for the active variant. */
ACPLIB_EXPORT QString dispatchValue(const McpServer &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServer &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const McpServer &val);

/** Returns the 'name' field from the active variant. */
ACPLIB_EXPORT QString name(const McpServer &val);

/**
 * Request parameters for loading an existing session.
 *
 * Only available if the Agent supports the `loadSession` capability.
 *
 * See protocol docs: [Loading Sessions](https://agentclientprotocol.com/protocol/session-setup#loading-sessions)
 */
struct LoadSessionRequest {
    QList<McpServer> _mcpServers;  //!< List of MCP servers to connect to for this session.
    QString _cwd;  //!< The working directory for this session.
    /**
     * Additional workspace roots to activate for this session. Each path must be absolute.
     *
     * When omitted or empty, no additional roots are activated. When non-empty,
     * this is the complete resulting additional-root list for the loaded
     * session. It may differ from any previously used or reported list as long as
     * the request `cwd` matches the session's `cwd`.
     */
    std::optional<QStringList> _additionalDirectories;
    SessionId _sessionId;  //!< The ID of the session to load.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    LoadSessionRequest& mcpServers(const QList<McpServer> & v) { _mcpServers = v; return *this; }
    LoadSessionRequest& addMcpServer(const McpServer & v) { _mcpServers.append(v); return *this; }
    LoadSessionRequest& cwd(const QString & v) { _cwd = v; return *this; }
    LoadSessionRequest& additionalDirectories(const std::optional<QStringList> & v) { _additionalDirectories = v; return *this; }
    LoadSessionRequest& addAdditionalDirectory(const QString & v) { if (!_additionalDirectories) _additionalDirectories = QStringList{}; (*_additionalDirectories).append(v); return *this; }
    LoadSessionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    LoadSessionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QList<McpServer>& mcpServers() const { return _mcpServers; }
    const QString& cwd() const { return _cwd; }
    const std::optional<QStringList>& additionalDirectories() const { return _additionalDirectories; }
    const SessionId& sessionId() const { return _sessionId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<LoadSessionRequest> fromJson<LoadSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LoadSessionRequest &data);

/**
 * Request parameters for the logout method.
 *
 * Terminates the current authenticated session.
 */
struct LogoutRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    LogoutRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<LogoutRequest> fromJson<LogoutRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LogoutRequest &data);

/**
 * Request parameters for creating a new session.
 *
 * See protocol docs: [Creating a Session](https://agentclientprotocol.com/protocol/session-setup#creating-a-session)
 */
struct NewSessionRequest {
    QString _cwd;  //!< The working directory for this session. Must be an absolute path.
    /**
     * Additional workspace roots for this session. Each path must be absolute.
     *
     * These expand the session's filesystem scope without changing `cwd`, which
     * remains the base for relative paths. When omitted or empty, no
     * additional roots are activated for the new session.
     */
    std::optional<QStringList> _additionalDirectories;
    QList<McpServer> _mcpServers;  //!< List of MCP (Model Context Protocol) servers the agent should connect to.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    NewSessionRequest& cwd(const QString & v) { _cwd = v; return *this; }
    NewSessionRequest& additionalDirectories(const std::optional<QStringList> & v) { _additionalDirectories = v; return *this; }
    NewSessionRequest& addAdditionalDirectory(const QString & v) { if (!_additionalDirectories) _additionalDirectories = QStringList{}; (*_additionalDirectories).append(v); return *this; }
    NewSessionRequest& mcpServers(const QList<McpServer> & v) { _mcpServers = v; return *this; }
    NewSessionRequest& addMcpServer(const McpServer & v) { _mcpServers.append(v); return *this; }
    NewSessionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& cwd() const { return _cwd; }
    const std::optional<QStringList>& additionalDirectories() const { return _additionalDirectories; }
    const QList<McpServer>& mcpServers() const { return _mcpServers; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<NewSessionRequest> fromJson<NewSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const NewSessionRequest &data);

/**
 * Request parameters for sending a user prompt to the agent.
 *
 * Contains the user's message and any additional context.
 *
 * See protocol docs: [User Message](https://agentclientprotocol.com/protocol/prompt-turn#1-user-message)
 */
struct PromptRequest {
    SessionId _sessionId;  //!< The ID of the session to send this user message to
    /**
     * The blocks of content that compose the user's message.
     *
     * As a baseline, the Agent MUST support [`ContentBlock::Text`] and [`ContentBlock::ResourceLink`],
     * while other variants are optionally enabled via [`PromptCapabilities`].
     *
     * The Client MUST adapt its interface according to [`PromptCapabilities`].
     *
     * The client MAY include referenced pieces of context as either
     * [`ContentBlock::Resource`] or [`ContentBlock::ResourceLink`].
     *
     * When available, [`ContentBlock::Resource`] is preferred
     * as it avoids extra round-trips and allows the message to include
     * pieces of context from sources the agent may not have access to.
     */
    QList<ContentBlock> _prompt;
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    PromptRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    PromptRequest& prompt(const QList<ContentBlock> & v) { _prompt = v; return *this; }
    PromptRequest& addPrompt(const ContentBlock & v) { _prompt.append(v); return *this; }
    PromptRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const QList<ContentBlock>& prompt() const { return _prompt; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptRequest> fromJson<PromptRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptRequest &data);

/**
 * Request parameters for resuming an existing session.
 *
 * Resumes an existing session without returning previous messages (unlike `session/load`).
 * This is useful for agents that can resume sessions but don't implement full session loading.
 *
 * Only available if the Agent supports the `sessionCapabilities.resume` capability.
 */
struct ResumeSessionRequest {
    SessionId _sessionId;  //!< The ID of the session to resume.
    QString _cwd;  //!< The working directory for this session.
    /**
     * Additional workspace roots to activate for this session. Each path must be absolute.
     *
     * When omitted or empty, no additional roots are activated. When non-empty,
     * this is the complete resulting additional-root list for the resumed
     * session. It may differ from any previously used or reported list as long as
     * the request `cwd` matches the session's `cwd`.
     */
    std::optional<QStringList> _additionalDirectories;
    std::optional<QList<McpServer>> _mcpServers;  //!< List of MCP servers to connect to for this session.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ResumeSessionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    ResumeSessionRequest& cwd(const QString & v) { _cwd = v; return *this; }
    ResumeSessionRequest& additionalDirectories(const std::optional<QStringList> & v) { _additionalDirectories = v; return *this; }
    ResumeSessionRequest& addAdditionalDirectory(const QString & v) { if (!_additionalDirectories) _additionalDirectories = QStringList{}; (*_additionalDirectories).append(v); return *this; }
    ResumeSessionRequest& mcpServers(const std::optional<QList<McpServer>> & v) { _mcpServers = v; return *this; }
    ResumeSessionRequest& addMcpServer(const McpServer & v) { if (!_mcpServers) _mcpServers = QList<McpServer>{}; (*_mcpServers).append(v); return *this; }
    ResumeSessionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const QString& cwd() const { return _cwd; }
    const std::optional<QStringList>& additionalDirectories() const { return _additionalDirectories; }
    const std::optional<QList<McpServer>>& mcpServers() const { return _mcpServers; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ResumeSessionRequest> fromJson<ResumeSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ResumeSessionRequest &data);

/** Request parameters for setting a session configuration option. */
struct SetSessionConfigOptionRequest {
    SessionId _sessionId;  //!< The ID of the session to set the configuration option for.
    SessionConfigId _configId;  //!< The ID of the configuration option to set.
    SessionConfigValueId _value;  //!< The ID of the configuration option value to set.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SetSessionConfigOptionRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    SetSessionConfigOptionRequest& configId(const SessionConfigId & v) { _configId = v; return *this; }
    SetSessionConfigOptionRequest& value(const SessionConfigValueId & v) { _value = v; return *this; }
    SetSessionConfigOptionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const SessionConfigId& configId() const { return _configId; }
    const SessionConfigValueId& value() const { return _value; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionConfigOptionRequest> fromJson<SetSessionConfigOptionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionConfigOptionRequest &data);

/** Request parameters for setting a session mode. */
struct SetSessionModeRequest {
    SessionId _sessionId;  //!< The ID of the session to set the mode for.
    SessionModeId _modeId;  //!< The ID of the mode to set.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SetSessionModeRequest& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    SetSessionModeRequest& modeId(const SessionModeId & v) { _modeId = v; return *this; }
    SetSessionModeRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const SessionModeId& modeId() const { return _modeId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionModeRequest> fromJson<SetSessionModeRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionModeRequest &data);

/** A JSON-RPC request object. */
struct ClientRequest {
    RequestId _id;  //!< The request id used to correlate the matching response.
    QString _method;  //!< The method name to invoke.
    std::optional<QString> _params;  //!< Method-specific request parameters.

    ClientRequest& id(const RequestId & v) { _id = v; return *this; }
    ClientRequest& method(const QString & v) { _method = v; return *this; }
    ClientRequest& params(const std::optional<QString> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<ClientRequest> fromJson<ClientRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ClientRequest &data);

/** Response containing the ID of the created terminal. */
struct CreateTerminalResponse {
    TerminalId _terminalId;  //!< The unique identifier for the created terminal.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    CreateTerminalResponse& terminalId(const TerminalId & v) { _terminalId = v; return *this; }
    CreateTerminalResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const TerminalId& terminalId() const { return _terminalId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CreateTerminalResponse> fromJson<CreateTerminalResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CreateTerminalResponse &data);

/** Response to `terminal/kill` method */
struct KillTerminalResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    KillTerminalResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<KillTerminalResponse> fromJson<KillTerminalResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const KillTerminalResponse &data);

/** Response containing the contents of a text file. */
struct ReadTextFileResponse {
    QString _content;  //!< Content payload returned by this response.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ReadTextFileResponse& content(const QString & v) { _content = v; return *this; }
    ReadTextFileResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& content() const { return _content; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ReadTextFileResponse> fromJson<ReadTextFileResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ReadTextFileResponse &data);

/** Response to terminal/release method */
struct ReleaseTerminalResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    ReleaseTerminalResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<ReleaseTerminalResponse> fromJson<ReleaseTerminalResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ReleaseTerminalResponse &data);

/** The user selected one of the provided options. */
struct SelectedPermissionOutcome {
    PermissionOptionId _optionId;  //!< The ID of the option the user selected.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SelectedPermissionOutcome& optionId(const PermissionOptionId & v) { _optionId = v; return *this; }
    SelectedPermissionOutcome& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const PermissionOptionId& optionId() const { return _optionId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SelectedPermissionOutcome> fromJson<SelectedPermissionOutcome>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SelectedPermissionOutcome &data);

/** The outcome of a permission request. */
struct RequestPermissionOutcome {
    using Variant = std::variant<std::monostate, SelectedPermissionOutcome>;
    Variant _value;
    QString _kind;  //!< discriminator value (outcome)

    template<typename T> const T* get() const { return std::get_if<T>(&_value); }
    const QString& kind() const { return _kind; }
};

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionOutcome> fromJson<RequestPermissionOutcome>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionOutcome &data);

ACPLIB_EXPORT QJsonValue toJsonValue(const RequestPermissionOutcome &val);

/** Response to a permission request. */
struct RequestPermissionResponse {
    RequestPermissionOutcome _outcome;  //!< The user's decision on the permission request.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    RequestPermissionResponse& outcome(const RequestPermissionOutcome & v) { _outcome = v; return *this; }
    RequestPermissionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const RequestPermissionOutcome& outcome() const { return _outcome; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionResponse> fromJson<RequestPermissionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionResponse &data);

/** Exit status of a terminal command. */
struct TerminalExitStatus {
    std::optional<int> _exitCode;  //!< The process exit code (may be null if terminated by signal).
    std::optional<QString> _signal;  //!< The signal that terminated the process (may be null if exited normally).
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    TerminalExitStatus& exitCode(std::optional<int> v) { _exitCode = v; return *this; }
    TerminalExitStatus& signal(const std::optional<QString> & v) { _signal = v; return *this; }
    TerminalExitStatus& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<int>& exitCode() const { return _exitCode; }
    const std::optional<QString>& signal() const { return _signal; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalExitStatus> fromJson<TerminalExitStatus>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalExitStatus &data);

/** Response containing the terminal output and exit status. */
struct TerminalOutputResponse {
    QString _output;  //!< The terminal output captured so far.
    bool _truncated;  //!< Whether the output was truncated due to byte limits.
    std::optional<TerminalExitStatus> _exitStatus;  //!< Exit status if the command has completed.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    TerminalOutputResponse& output(const QString & v) { _output = v; return *this; }
    TerminalOutputResponse& truncated(bool v) { _truncated = v; return *this; }
    TerminalOutputResponse& exitStatus(const std::optional<TerminalExitStatus> & v) { _exitStatus = v; return *this; }
    TerminalOutputResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const QString& output() const { return _output; }
    const bool& truncated() const { return _truncated; }
    const std::optional<TerminalExitStatus>& exitStatus() const { return _exitStatus; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalOutputResponse> fromJson<TerminalOutputResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalOutputResponse &data);

/** Response containing the exit status of a terminal command. */
struct WaitForTerminalExitResponse {
    std::optional<int> _exitCode;  //!< The process exit code (may be null if terminated by signal).
    std::optional<QString> _signal;  //!< The signal that terminated the process (may be null if exited normally).
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    WaitForTerminalExitResponse& exitCode(std::optional<int> v) { _exitCode = v; return *this; }
    WaitForTerminalExitResponse& signal(const std::optional<QString> & v) { _signal = v; return *this; }
    WaitForTerminalExitResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<int>& exitCode() const { return _exitCode; }
    const std::optional<QString>& signal() const { return _signal; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<WaitForTerminalExitResponse> fromJson<WaitForTerminalExitResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const WaitForTerminalExitResponse &data);

/** Response to `fs/write_text_file` */
struct WriteTextFileResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    WriteTextFileResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<WriteTextFileResponse> fromJson<WriteTextFileResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const WriteTextFileResponse &data);

/** A JSON-RPC response object. */
using ClientResponse = std::variant<QJsonObject>;

/**
 * Notification to cancel ongoing operations for a session.
 *
 * See protocol docs: [Cancellation](https://agentclientprotocol.com/protocol/prompt-turn#cancellation)
 */
struct CancelNotification {
    SessionId _sessionId;  //!< The ID of the session to cancel operations for.
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    CancelNotification& sessionId(const SessionId & v) { _sessionId = v; return *this; }
    CancelNotification& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const SessionId& sessionId() const { return _sessionId; }
    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<CancelNotification> fromJson<CancelNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelNotification &data);

/** A JSON-RPC notification object. */
struct ClientNotification {
    QString _method;  //!< The notification method name.
    std::optional<QString> _params;  //!< Method-specific notification parameters.

    ClientNotification& method(const QString & v) { _method = v; return *this; }
    ClientNotification& params(const std::optional<QString> & v) { _params = v; return *this; }

    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<ClientNotification> fromJson<ClientNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ClientNotification &data);

} // namespace Acp
