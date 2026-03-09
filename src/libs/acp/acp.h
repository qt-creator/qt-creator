/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 /opt/homebrew/opt/python@3.14/bin/python3.14 \
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

/** MCP capabilities supported by the agent */
struct McpCapabilities {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<bool> _http;  //!< Agent supports [`McpServer::Http`].
    std::optional<bool> _sse;  //!< Agent supports [`McpServer::Sse`].

    McpCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    McpCapabilities& http(std::optional<bool> v) { _http = v; return *this; }
    McpCapabilities& sse(std::optional<bool> v) { _sse = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<bool>& http() const { return _http; }
    const std::optional<bool>& sse() const { return _sse; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<bool> _audio;  //!< Agent supports [`ContentBlock::Audio`].
    /**
     * Agent supports embedded context in `session/prompt` requests.
     *
     * When enabled, the Client is allowed to include [`ContentBlock::Resource`]
     * in prompt requests for pieces of context that are referenced in the message.
     */
    std::optional<bool> _embeddedContext;
    std::optional<bool> _image;  //!< Agent supports [`ContentBlock::Image`].

    PromptCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    PromptCapabilities& audio(std::optional<bool> v) { _audio = v; return *this; }
    PromptCapabilities& embeddedContext(std::optional<bool> v) { _embeddedContext = v; return *this; }
    PromptCapabilities& image(std::optional<bool> v) { _image = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<bool>& audio() const { return _audio; }
    const std::optional<bool>& embeddedContext() const { return _embeddedContext; }
    const std::optional<bool>& image() const { return _image; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptCapabilities> fromJson<PromptCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptCapabilities &data);

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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    SessionCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<bool> _loadSession;  //!< Whether the agent supports `session/load`.
    std::optional<QString> _mcpCapabilities;  //!< MCP capabilities supported by the agent.
    std::optional<QString> _promptCapabilities;  //!< Prompt capabilities supported by the agent.
    std::optional<QString> _sessionCapabilities;

    AgentCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    AgentCapabilities& loadSession(std::optional<bool> v) { _loadSession = v; return *this; }
    AgentCapabilities& mcpCapabilities(const std::optional<QString> & v) { _mcpCapabilities = v; return *this; }
    AgentCapabilities& promptCapabilities(const std::optional<QString> & v) { _promptCapabilities = v; return *this; }
    AgentCapabilities& sessionCapabilities(const std::optional<QString> & v) { _sessionCapabilities = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<bool>& loadSession() const { return _loadSession; }
    const std::optional<QString>& mcpCapabilities() const { return _mcpCapabilities; }
    const std::optional<QString>& promptCapabilities() const { return _promptCapabilities; }
    const std::optional<QString>& sessionCapabilities() const { return _sessionCapabilities; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentCapabilities> fromJson<AgentCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentCapabilities &data);

// Skipped unknown type alias: ExtNotification

using SessionId = QString;
template<>
ACPLIB_EXPORT Utils::Result<SessionId> fromJson<SessionId>(const QJsonValue &val);

/** All text that was typed after the command name is provided as input. */
struct UnstructuredCommandInput {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _hint;  //!< A hint to display when the input hasn't been provided yet

    UnstructuredCommandInput& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    UnstructuredCommandInput& hint(const QString & v) { _hint = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& hint() const { return _hint; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _description;  //!< Human-readable description of what the command does.
    std::optional<QString> _input;  //!< Input for the command if required
    QString _name;  //!< Command name (e.g., `create_plan`, `research_codebase`).

    AvailableCommand& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    AvailableCommand& description(const QString & v) { _description = v; return *this; }
    AvailableCommand& input(const std::optional<QString> & v) { _input = v; return *this; }
    AvailableCommand& name(const QString & v) { _name = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& description() const { return _description; }
    const std::optional<QString>& input() const { return _input; }
    const QString& name() const { return _name; }
};

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommand> fromJson<AvailableCommand>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommand &data);

/** Available commands are ready or have changed */
struct AvailableCommandsUpdate {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QList<AvailableCommand> _availableCommands;  //!< Commands the agent can execute

    AvailableCommandsUpdate& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    AvailableCommandsUpdate& availableCommands(const QList<AvailableCommand> & v) { _availableCommands = v; return *this; }
    AvailableCommandsUpdate& addAvailableCommand(const AvailableCommand & v) { _availableCommands.append(v); return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QList<AvailableCommand>& availableCommands() const { return _availableCommands; }
};

template<>
ACPLIB_EXPORT Utils::Result<AvailableCommandsUpdate> fromJson<AvailableCommandsUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AvailableCommandsUpdate &data);

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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _description;  //!< Optional description for this option value.
    QString _name;  //!< Human-readable label for this option value.
    QString _value;  //!< Unique identifier for this option value.

    SessionConfigSelectOption& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SessionConfigSelectOption& description(const std::optional<QString> & v) { _description = v; return *this; }
    SessionConfigSelectOption& name(const QString & v) { _name = v; return *this; }
    SessionConfigSelectOption& value(const QString & v) { _value = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& description() const { return _description; }
    const QString& name() const { return _name; }
    const QString& value() const { return _value; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelectOption> fromJson<SessionConfigSelectOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigSelectOption &data);

/** A group of possible values for a session configuration option. */
struct SessionConfigSelectGroup {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _group;  //!< Unique identifier for this group.
    QString _name;  //!< Human-readable label for this group.
    QList<SessionConfigSelectOption> _options;  //!< The set of option values in this group.

    SessionConfigSelectGroup& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SessionConfigSelectGroup& group(const QString & v) { _group = v; return *this; }
    SessionConfigSelectGroup& name(const QString & v) { _name = v; return *this; }
    SessionConfigSelectGroup& options(const QList<SessionConfigSelectOption> & v) { _options = v; return *this; }
    SessionConfigSelectGroup& addOption(const SessionConfigSelectOption & v) { _options.append(v); return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& group() const { return _group; }
    const QString& name() const { return _name; }
    const QList<SessionConfigSelectOption>& options() const { return _options; }
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
    QString _currentValue;  //!< The currently selected value.
    QString _options;  //!< The set of selectable options.

    SessionConfigSelect& currentValue(const QString & v) { _currentValue = v; return *this; }
    SessionConfigSelect& options(const QString & v) { _options = v; return *this; }

    const QString& currentValue() const { return _currentValue; }
    const QString& options() const { return _options; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigSelect> fromJson<SessionConfigSelect>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigSelect &data);

/** A session configuration option selector and its current state. */
struct SessionConfigOption {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _category;  //!< Optional semantic category for this option (UX only).
    std::optional<QString> _description;  //!< Optional description for the Client to display to the user.
    QString _id;  //!< Unique identifier for the configuration option.
    QString _name;  //!< Human-readable label for the option.

    SessionConfigOption& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SessionConfigOption& category(const std::optional<QString> & v) { _category = v; return *this; }
    SessionConfigOption& description(const std::optional<QString> & v) { _description = v; return *this; }
    SessionConfigOption& id(const QString & v) { _id = v; return *this; }
    SessionConfigOption& name(const QString & v) { _name = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& category() const { return _category; }
    const std::optional<QString>& description() const { return _description; }
    const QString& id() const { return _id; }
    const QString& name() const { return _name; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionConfigOption> fromJson<SessionConfigOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionConfigOption &data);

/** Session configuration options have been updated. */
struct ConfigOptionUpdate {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QList<SessionConfigOption> _configOptions;  //!< The full set of configuration options and their current values.

    ConfigOptionUpdate& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ConfigOptionUpdate& configOptions(const QList<SessionConfigOption> & v) { _configOptions = v; return *this; }
    ConfigOptionUpdate& addConfigOption(const SessionConfigOption & v) { _configOptions.append(v); return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QList<SessionConfigOption>& configOptions() const { return _configOptions; }
};

template<>
ACPLIB_EXPORT Utils::Result<ConfigOptionUpdate> fromJson<ConfigOptionUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ConfigOptionUpdate &data);

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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QJsonArray> _audience;
    std::optional<QString> _lastModified;
    std::optional<double> _priority;

    Annotations& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    Annotations& audience(const std::optional<QJsonArray> & v) { _audience = v; return *this; }
    Annotations& lastModified(const std::optional<QString> & v) { _lastModified = v; return *this; }
    Annotations& priority(std::optional<double> v) { _priority = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QJsonArray>& audience() const { return _audience; }
    const std::optional<QString>& lastModified() const { return _lastModified; }
    const std::optional<double>& priority() const { return _priority; }
};

template<>
ACPLIB_EXPORT Utils::Result<Annotations> fromJson<Annotations>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Annotations &data);

/** Audio provided to or from an LLM. */
struct AudioContent {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _annotations;
    QString _data;
    QString _mimeType;

    AudioContent& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    AudioContent& annotations(const std::optional<QString> & v) { _annotations = v; return *this; }
    AudioContent& data(const QString & v) { _data = v; return *this; }
    AudioContent& mimeType(const QString & v) { _mimeType = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& annotations() const { return _annotations; }
    const QString& data() const { return _data; }
    const QString& mimeType() const { return _mimeType; }
};

template<>
ACPLIB_EXPORT Utils::Result<AudioContent> fromJson<AudioContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AudioContent &data);

/** Binary resource contents. */
struct BlobResourceContents {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _blob;
    std::optional<QString> _mimeType;
    QString _uri;

    BlobResourceContents& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    BlobResourceContents& blob(const QString & v) { _blob = v; return *this; }
    BlobResourceContents& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    BlobResourceContents& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& blob() const { return _blob; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& uri() const { return _uri; }
};

template<>
ACPLIB_EXPORT Utils::Result<BlobResourceContents> fromJson<BlobResourceContents>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const BlobResourceContents &data);

/** Text-based resource contents. */
struct TextResourceContents {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _mimeType;
    QString _text;
    QString _uri;

    TextResourceContents& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    TextResourceContents& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    TextResourceContents& text(const QString & v) { _text = v; return *this; }
    TextResourceContents& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& text() const { return _text; }
    const QString& uri() const { return _uri; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _annotations;
    EmbeddedResourceResource _resource;

    EmbeddedResource& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    EmbeddedResource& annotations(const std::optional<QString> & v) { _annotations = v; return *this; }
    EmbeddedResource& resource(const EmbeddedResourceResource & v) { _resource = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& annotations() const { return _annotations; }
    const EmbeddedResourceResource& resource() const { return _resource; }
};

template<>
ACPLIB_EXPORT Utils::Result<EmbeddedResource> fromJson<EmbeddedResource>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const EmbeddedResource &data);

/** An image provided to or from an LLM. */
struct ImageContent {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _annotations;
    QString _data;
    QString _mimeType;
    std::optional<QString> _uri;

    ImageContent& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ImageContent& annotations(const std::optional<QString> & v) { _annotations = v; return *this; }
    ImageContent& data(const QString & v) { _data = v; return *this; }
    ImageContent& mimeType(const QString & v) { _mimeType = v; return *this; }
    ImageContent& uri(const std::optional<QString> & v) { _uri = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& annotations() const { return _annotations; }
    const QString& data() const { return _data; }
    const QString& mimeType() const { return _mimeType; }
    const std::optional<QString>& uri() const { return _uri; }
};

template<>
ACPLIB_EXPORT Utils::Result<ImageContent> fromJson<ImageContent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ImageContent &data);

/** A resource that the server is capable of reading, included in a prompt or tool call result. */
struct ResourceLink {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _annotations;
    std::optional<QString> _description;
    std::optional<QString> _mimeType;
    QString _name;
    std::optional<int> _size;
    std::optional<QString> _title;
    QString _uri;

    ResourceLink& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ResourceLink& annotations(const std::optional<QString> & v) { _annotations = v; return *this; }
    ResourceLink& description(const std::optional<QString> & v) { _description = v; return *this; }
    ResourceLink& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    ResourceLink& name(const QString & v) { _name = v; return *this; }
    ResourceLink& size(std::optional<int> v) { _size = v; return *this; }
    ResourceLink& title(const std::optional<QString> & v) { _title = v; return *this; }
    ResourceLink& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& annotations() const { return _annotations; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& name() const { return _name; }
    const std::optional<int>& size() const { return _size; }
    const std::optional<QString>& title() const { return _title; }
    const QString& uri() const { return _uri; }
};

template<>
ACPLIB_EXPORT Utils::Result<ResourceLink> fromJson<ResourceLink>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ResourceLink &data);

/** Text provided to or from an LLM. */
struct TextContent {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _annotations;
    QString _text;

    TextContent& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    TextContent& annotations(const std::optional<QString> & v) { _annotations = v; return *this; }
    TextContent& text(const QString & v) { _text = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& annotations() const { return _annotations; }
    const QString& text() const { return _text; }
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

ACPLIB_EXPORT QJsonObject toJson(const ContentBlock &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const ContentBlock &val);

/** A streamed item of content */
struct ContentChunk {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _content;  //!< A single item of content

    ContentChunk& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ContentChunk& content(const QString & v) { _content = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& content() const { return _content; }
};

template<>
ACPLIB_EXPORT Utils::Result<ContentChunk> fromJson<ContentChunk>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ContentChunk &data);

using SessionModeId = QString;

/**
 * The current mode of the session has changed
 *
 * See protocol docs: [Session Modes](https://agentclientprotocol.com/protocol/session-modes)
 */
struct CurrentModeUpdate {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _currentModeId;  //!< The ID of the current mode

    CurrentModeUpdate& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    CurrentModeUpdate& currentModeId(const QString & v) { _currentModeId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& currentModeId() const { return _currentModeId; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _content;  //!< Human-readable description of what this task aims to accomplish.
    /**
     * The relative importance of this task.
     * Used to indicate which tasks are most critical to the overall goal.
     */
    QString _priority;
    QString _status;  //!< Current execution status of this task.

    PlanEntry& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    PlanEntry& content(const QString & v) { _content = v; return *this; }
    PlanEntry& priority(const QString & v) { _priority = v; return *this; }
    PlanEntry& status(const QString & v) { _status = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& content() const { return _content; }
    const QString& priority() const { return _priority; }
    const QString& status() const { return _status; }
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
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    /**
     * The list of tasks to be accomplished.
     *
     * When updating a plan, the agent must send a complete list of all entries
     * with their current status. The client replaces the entire plan with each update.
     */
    QList<PlanEntry> _entries;

    Plan& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    Plan& entries(const QList<PlanEntry> & v) { _entries = v; return *this; }
    Plan& addEntry(const PlanEntry & v) { _entries.append(v); return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QList<PlanEntry>& entries() const { return _entries; }
};

template<>
ACPLIB_EXPORT Utils::Result<Plan> fromJson<Plan>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Plan &data);

/** Standard content block (text, images, resources). */
struct Content {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _content;  //!< The actual content block.

    Content& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    Content& content(const QString & v) { _content = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& content() const { return _content; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _newText;  //!< The new content after modification.
    std::optional<QString> _oldText;  //!< The original content (None for new files).
    QString _path;  //!< The file path being modified.

    Diff& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    Diff& newText(const QString & v) { _newText = v; return *this; }
    Diff& oldText(const std::optional<QString> & v) { _oldText = v; return *this; }
    Diff& path(const QString & v) { _path = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& newText() const { return _newText; }
    const std::optional<QString>& oldText() const { return _oldText; }
    const QString& path() const { return _path; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _terminalId;

    Terminal& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    Terminal& terminalId(const QString & v) { _terminalId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& terminalId() const { return _terminalId; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<int> _line;  //!< Optional line number within the file.
    QString _path;  //!< The file path being accessed or modified.

    ToolCallLocation& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallLocation& line(std::optional<int> v) { _line = v; return *this; }
    ToolCallLocation& path(const QString & v) { _path = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<int>& line() const { return _line; }
    const QString& path() const { return _path; }
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
 * Represents a tool call that the language model has requested.
 *
 * Tool calls are actions that the agent executes on behalf of the language model,
 * such as reading files, executing code, or fetching data from external sources.
 *
 * See protocol docs: [Tool Calls](https://agentclientprotocol.com/protocol/tool-calls)
 */
struct ToolCall {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QList<ToolCallContent>> _content;  //!< Content produced by the tool call.
    /**
     * The category of tool being invoked.
     * Helps clients choose appropriate icons and UI treatment.
     */
    std::optional<QString> _kind;
    /**
     * File locations affected by this tool call.
     * Enables "follow-along" features in clients.
     */
    std::optional<QList<ToolCallLocation>> _locations;
    std::optional<QString> _rawInput;  //!< Raw input parameters sent to the tool.
    std::optional<QString> _rawOutput;  //!< Raw output returned by the tool.
    std::optional<QString> _status;  //!< Current execution status of the tool call.
    QString _title;  //!< Human-readable title describing what the tool is doing.
    QString _toolCallId;  //!< Unique identifier for this tool call within the session.

    ToolCall& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ToolCall& content(const std::optional<QList<ToolCallContent>> & v) { _content = v; return *this; }
    ToolCall& addContent(const ToolCallContent & v) { if (!_content) _content = QList<ToolCallContent>{}; (*_content).append(v); return *this; }
    ToolCall& kind(const std::optional<QString> & v) { _kind = v; return *this; }
    ToolCall& locations(const std::optional<QList<ToolCallLocation>> & v) { _locations = v; return *this; }
    ToolCall& addLocation(const ToolCallLocation & v) { if (!_locations) _locations = QList<ToolCallLocation>{}; (*_locations).append(v); return *this; }
    ToolCall& rawInput(const std::optional<QString> & v) { _rawInput = v; return *this; }
    ToolCall& rawOutput(const std::optional<QString> & v) { _rawOutput = v; return *this; }
    ToolCall& status(const std::optional<QString> & v) { _status = v; return *this; }
    ToolCall& title(const QString & v) { _title = v; return *this; }
    ToolCall& toolCallId(const QString & v) { _toolCallId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QList<ToolCallContent>>& content() const { return _content; }
    const std::optional<QString>& kind() const { return _kind; }
    const std::optional<QList<ToolCallLocation>>& locations() const { return _locations; }
    const std::optional<QString>& rawInput() const { return _rawInput; }
    const std::optional<QString>& rawOutput() const { return _rawOutput; }
    const std::optional<QString>& status() const { return _status; }
    const QString& title() const { return _title; }
    const QString& toolCallId() const { return _toolCallId; }
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCall> fromJson<ToolCall>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCall &data);

/**
 * An update to an existing tool call.
 *
 * Used to report progress and results as tools execute. All fields except
 * the tool call ID are optional - only changed fields need to be included.
 *
 * See protocol docs: [Updating](https://agentclientprotocol.com/protocol/tool-calls#updating)
 */
struct ToolCallUpdate {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QJsonArray> _content;  //!< Replace the content collection.
    std::optional<QString> _kind;  //!< Update the tool kind.
    std::optional<QJsonArray> _locations;  //!< Replace the locations collection.
    std::optional<QString> _rawInput;  //!< Update the raw input.
    std::optional<QString> _rawOutput;  //!< Update the raw output.
    std::optional<QString> _status;  //!< Update the execution status.
    std::optional<QString> _title;  //!< Update the human-readable title.
    QString _toolCallId;  //!< The ID of the tool call being updated.

    ToolCallUpdate& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ToolCallUpdate& content(const std::optional<QJsonArray> & v) { _content = v; return *this; }
    ToolCallUpdate& kind(const std::optional<QString> & v) { _kind = v; return *this; }
    ToolCallUpdate& locations(const std::optional<QJsonArray> & v) { _locations = v; return *this; }
    ToolCallUpdate& rawInput(const std::optional<QString> & v) { _rawInput = v; return *this; }
    ToolCallUpdate& rawOutput(const std::optional<QString> & v) { _rawOutput = v; return *this; }
    ToolCallUpdate& status(const std::optional<QString> & v) { _status = v; return *this; }
    ToolCallUpdate& title(const std::optional<QString> & v) { _title = v; return *this; }
    ToolCallUpdate& toolCallId(const QString & v) { _toolCallId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QJsonArray>& content() const { return _content; }
    const std::optional<QString>& kind() const { return _kind; }
    const std::optional<QJsonArray>& locations() const { return _locations; }
    const std::optional<QString>& rawInput() const { return _rawInput; }
    const std::optional<QString>& rawOutput() const { return _rawOutput; }
    const std::optional<QString>& status() const { return _status; }
    const std::optional<QString>& title() const { return _title; }
    const QString& toolCallId() const { return _toolCallId; }
};

template<>
ACPLIB_EXPORT Utils::Result<ToolCallUpdate> fromJson<ToolCallUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ToolCallUpdate &data);

/**
 * Different types of updates that can be sent during session processing.
 *
 * These updates provide real-time feedback about the agent's progress.
 *
 * See protocol docs: [Agent Reports Output](https://agentclientprotocol.com/protocol/prompt-turn#3-agent-reports-output)
 */
using SessionUpdate = std::variant<ContentChunk, ToolCall, ToolCallUpdate, Plan, AvailableCommandsUpdate, CurrentModeUpdate, ConfigOptionUpdate>;

template<>
ACPLIB_EXPORT Utils::Result<SessionUpdate> fromJson<SessionUpdate>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionUpdate &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const SessionUpdate &val);

/**
 * Notification containing a session update from the agent.
 *
 * Used to stream real-time progress and results during prompt processing.
 *
 * See protocol docs: [Agent Reports Output](https://agentclientprotocol.com/protocol/prompt-turn#3-agent-reports-output)
 */
struct SessionNotification {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _sessionId;  //!< The ID of the session this update pertains to.
    QString _update;  //!< The actual update content.

    SessionNotification& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SessionNotification& sessionId(const QString & v) { _sessionId = v; return *this; }
    SessionNotification& update(const QString & v) { _update = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& sessionId() const { return _sessionId; }
    const QString& update() const { return _update; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionNotification> fromJson<SessionNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionNotification &data);

struct AgentNotification {
    QString _method;
    std::optional<QString> _params;

    AgentNotification& method(const QString & v) { _method = v; return *this; }
    AgentNotification& params(const std::optional<QString> & v) { _params = v; return *this; }

    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<AgentNotification> fromJson<AgentNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AgentNotification &data);

/** An environment variable to set when launching an MCP server. */
struct EnvVariable {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _name;  //!< The name of the environment variable.
    QString _value;  //!< The value to set for the environment variable.

    EnvVariable& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    EnvVariable& name(const QString & v) { _name = v; return *this; }
    EnvVariable& value(const QString & v) { _value = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& name() const { return _name; }
    const QString& value() const { return _value; }
};

template<>
ACPLIB_EXPORT Utils::Result<EnvVariable> fromJson<EnvVariable>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const EnvVariable &data);

/** Request to create a new terminal and execute a command. */
struct CreateTerminalRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QStringList> _args;  //!< Array of command arguments.
    QString _command;  //!< The command to execute.
    std::optional<QString> _cwd;  //!< Working directory for the command (absolute path).
    std::optional<QList<EnvVariable>> _env;  //!< Environment variables for the command.
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
    QString _sessionId;  //!< The session ID for this request.

    CreateTerminalRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    CreateTerminalRequest& args(const std::optional<QStringList> & v) { _args = v; return *this; }
    CreateTerminalRequest& addArg(const QString & v) { if (!_args) _args = QStringList{}; (*_args).append(v); return *this; }
    CreateTerminalRequest& command(const QString & v) { _command = v; return *this; }
    CreateTerminalRequest& cwd(const std::optional<QString> & v) { _cwd = v; return *this; }
    CreateTerminalRequest& env(const std::optional<QList<EnvVariable>> & v) { _env = v; return *this; }
    CreateTerminalRequest& addEnv(const EnvVariable & v) { if (!_env) _env = QList<EnvVariable>{}; (*_env).append(v); return *this; }
    CreateTerminalRequest& outputByteLimit(std::optional<int> v) { _outputByteLimit = v; return *this; }
    CreateTerminalRequest& sessionId(const QString & v) { _sessionId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QStringList>& args() const { return _args; }
    const QString& command() const { return _command; }
    const std::optional<QString>& cwd() const { return _cwd; }
    const std::optional<QList<EnvVariable>>& env() const { return _env; }
    const std::optional<int>& outputByteLimit() const { return _outputByteLimit; }
    const QString& sessionId() const { return _sessionId; }
};

template<>
ACPLIB_EXPORT Utils::Result<CreateTerminalRequest> fromJson<CreateTerminalRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CreateTerminalRequest &data);

// Skipped unknown type alias: ExtRequest

/** Request to kill a terminal command without releasing the terminal. */
struct KillTerminalCommandRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _sessionId;  //!< The session ID for this request.
    QString _terminalId;  //!< The ID of the terminal to kill.

    KillTerminalCommandRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    KillTerminalCommandRequest& sessionId(const QString & v) { _sessionId = v; return *this; }
    KillTerminalCommandRequest& terminalId(const QString & v) { _terminalId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& sessionId() const { return _sessionId; }
    const QString& terminalId() const { return _terminalId; }
};

template<>
ACPLIB_EXPORT Utils::Result<KillTerminalCommandRequest> fromJson<KillTerminalCommandRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const KillTerminalCommandRequest &data);

/**
 * Request to read content from a text file.
 *
 * Only available if the client supports the `fs.readTextFile` capability.
 */
struct ReadTextFileRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<int> _limit;  //!< Maximum number of lines to read.
    std::optional<int> _line;  //!< Line number to start reading from (1-based).
    QString _path;  //!< Absolute path to the file to read.
    QString _sessionId;  //!< The session ID for this request.

    ReadTextFileRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ReadTextFileRequest& limit(std::optional<int> v) { _limit = v; return *this; }
    ReadTextFileRequest& line(std::optional<int> v) { _line = v; return *this; }
    ReadTextFileRequest& path(const QString & v) { _path = v; return *this; }
    ReadTextFileRequest& sessionId(const QString & v) { _sessionId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<int>& limit() const { return _limit; }
    const std::optional<int>& line() const { return _line; }
    const QString& path() const { return _path; }
    const QString& sessionId() const { return _sessionId; }
};

template<>
ACPLIB_EXPORT Utils::Result<ReadTextFileRequest> fromJson<ReadTextFileRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ReadTextFileRequest &data);

/** Request to release a terminal and free its resources. */
struct ReleaseTerminalRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _sessionId;  //!< The session ID for this request.
    QString _terminalId;  //!< The ID of the terminal to release.

    ReleaseTerminalRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ReleaseTerminalRequest& sessionId(const QString & v) { _sessionId = v; return *this; }
    ReleaseTerminalRequest& terminalId(const QString & v) { _terminalId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& sessionId() const { return _sessionId; }
    const QString& terminalId() const { return _terminalId; }
};

template<>
ACPLIB_EXPORT Utils::Result<ReleaseTerminalRequest> fromJson<ReleaseTerminalRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ReleaseTerminalRequest &data);

/**
 * JSON RPC Request Id
 *
 * An identifier established by the Client that MUST contain a String, Number, or NULL value if included. If it is not included it is assumed to be a notification. The value SHOULD normally not be Null [1] and Numbers SHOULD NOT contain fractional parts [2]
 *
 * The Server MUST reply with the same value in the Response object if included. This member is used to correlate the context between the two objects.
 *
 * [1] The use of Null as a value for the id member in a Request object is discouraged, because this specification uses a value of Null for Responses with an unknown id. Also, because JSON-RPC 1.0 uses an id value of Null for Notifications this could cause confusion in handling.
 *
 * [2] Fractional parts may be problematic, since many decimal fractions cannot be represented exactly as binary fractions.
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _kind;  //!< Hint about the nature of this permission option.
    QString _name;  //!< Human-readable label to display to the user.
    QString _optionId;  //!< Unique identifier for this permission option.

    PermissionOption& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    PermissionOption& kind(const QString & v) { _kind = v; return *this; }
    PermissionOption& name(const QString & v) { _name = v; return *this; }
    PermissionOption& optionId(const QString & v) { _optionId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& kind() const { return _kind; }
    const QString& name() const { return _name; }
    const QString& optionId() const { return _optionId; }
};

template<>
ACPLIB_EXPORT Utils::Result<PermissionOption> fromJson<PermissionOption>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PermissionOption &data);

/**
 * Request for user permission to execute a tool call.
 *
 * Sent when the agent needs authorization before performing a sensitive operation.
 *
 * See protocol docs: [Requesting Permission](https://agentclientprotocol.com/protocol/tool-calls#requesting-permission)
 */
struct RequestPermissionRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QList<PermissionOption> _options;  //!< Available permission options for the user to choose from.
    QString _sessionId;  //!< The session ID for this request.
    QString _toolCall;  //!< Details about the tool call requiring permission.

    RequestPermissionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    RequestPermissionRequest& options(const QList<PermissionOption> & v) { _options = v; return *this; }
    RequestPermissionRequest& addOption(const PermissionOption & v) { _options.append(v); return *this; }
    RequestPermissionRequest& sessionId(const QString & v) { _sessionId = v; return *this; }
    RequestPermissionRequest& toolCall(const QString & v) { _toolCall = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QList<PermissionOption>& options() const { return _options; }
    const QString& sessionId() const { return _sessionId; }
    const QString& toolCall() const { return _toolCall; }
};

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionRequest> fromJson<RequestPermissionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionRequest &data);

/** Request to get the current output and status of a terminal. */
struct TerminalOutputRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _sessionId;  //!< The session ID for this request.
    QString _terminalId;  //!< The ID of the terminal to get output from.

    TerminalOutputRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    TerminalOutputRequest& sessionId(const QString & v) { _sessionId = v; return *this; }
    TerminalOutputRequest& terminalId(const QString & v) { _terminalId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& sessionId() const { return _sessionId; }
    const QString& terminalId() const { return _terminalId; }
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalOutputRequest> fromJson<TerminalOutputRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalOutputRequest &data);

/** Request to wait for a terminal command to exit. */
struct WaitForTerminalExitRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _sessionId;  //!< The session ID for this request.
    QString _terminalId;  //!< The ID of the terminal to wait for.

    WaitForTerminalExitRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    WaitForTerminalExitRequest& sessionId(const QString & v) { _sessionId = v; return *this; }
    WaitForTerminalExitRequest& terminalId(const QString & v) { _terminalId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& sessionId() const { return _sessionId; }
    const QString& terminalId() const { return _terminalId; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _content;  //!< The text content to write to the file.
    QString _path;  //!< Absolute path to the file to write.
    QString _sessionId;  //!< The session ID for this request.

    WriteTextFileRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    WriteTextFileRequest& content(const QString & v) { _content = v; return *this; }
    WriteTextFileRequest& path(const QString & v) { _path = v; return *this; }
    WriteTextFileRequest& sessionId(const QString & v) { _sessionId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& content() const { return _content; }
    const QString& path() const { return _path; }
    const QString& sessionId() const { return _sessionId; }
};

template<>
ACPLIB_EXPORT Utils::Result<WriteTextFileRequest> fromJson<WriteTextFileRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const WriteTextFileRequest &data);

struct AgentRequest {
    RequestId _id;
    QString _method;
    std::optional<QString> _params;

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

/**
 * Predefined error codes for common JSON-RPC and ACP-specific errors.
 *
 * These codes follow the JSON-RPC 2.0 specification for standard errors
 * and use the reserved range (-32000 to -32099) for protocol-specific errors.
 */
using ErrorCode = int;

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
    QString _code;
    /**
     * Optional primitive or structured value that contains additional information about the error.
     * This may include debugging information or context-specific details.
     */
    std::optional<QString> _data;
    /**
     * A string providing a short description of the error.
     * The message should be limited to a concise single sentence.
     */
    QString _message;

    Error& code(const QString & v) { _code = v; return *this; }
    Error& data(const std::optional<QString> & v) { _data = v; return *this; }
    Error& message(const QString & v) { _message = v; return *this; }

    const QString& code() const { return _code; }
    const std::optional<QString>& data() const { return _data; }
    const QString& message() const { return _message; }
};

template<>
ACPLIB_EXPORT Utils::Result<Error> fromJson<Error>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const Error &data);

// Skipped unknown type alias: ExtResponse

/** Describes an available authentication method. */
struct AuthMethod {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _description;  //!< Optional description providing more details about this authentication method.
    QString _id;  //!< Unique identifier for this authentication method.
    QString _name;  //!< Human-readable name of the authentication method.

    AuthMethod& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    AuthMethod& description(const std::optional<QString> & v) { _description = v; return *this; }
    AuthMethod& id(const QString & v) { _id = v; return *this; }
    AuthMethod& name(const QString & v) { _name = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& description() const { return _description; }
    const QString& id() const { return _id; }
    const QString& name() const { return _name; }
};

template<>
ACPLIB_EXPORT Utils::Result<AuthMethod> fromJson<AuthMethod>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AuthMethod &data);

/**
 * Metadata about the implementation of the client or agent.
 * Describes the name and version of an MCP implementation, with an optional
 * title for UI representation.
 */
struct Implementation {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
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

    Implementation& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    Implementation& name(const QString & v) { _name = v; return *this; }
    Implementation& title(const std::optional<QString> & v) { _title = v; return *this; }
    Implementation& version(const QString & v) { _version = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& name() const { return _name; }
    const std::optional<QString>& title() const { return _title; }
    const QString& version() const { return _version; }
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
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _agentCapabilities;  //!< Capabilities supported by the agent.
    /**
     * Information about the Agent name and version sent to the Client.
     *
     * Note: in future versions of the protocol, this will be required.
     */
    std::optional<QString> _agentInfo;
    std::optional<QList<AuthMethod>> _authMethods;  //!< Authentication methods supported by the agent.
    /**
     * The protocol version the client specified if supported by the agent,
     * or the latest protocol version supported by the agent.
     *
     * The client should disconnect, if it doesn't support this version.
     */
    QString _protocolVersion;

    InitializeResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    InitializeResponse& agentCapabilities(const std::optional<QString> & v) { _agentCapabilities = v; return *this; }
    InitializeResponse& agentInfo(const std::optional<QString> & v) { _agentInfo = v; return *this; }
    InitializeResponse& authMethods(const std::optional<QList<AuthMethod>> & v) { _authMethods = v; return *this; }
    InitializeResponse& addAuthMethod(const AuthMethod & v) { if (!_authMethods) _authMethods = QList<AuthMethod>{}; (*_authMethods).append(v); return *this; }
    InitializeResponse& protocolVersion(const QString & v) { _protocolVersion = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& agentCapabilities() const { return _agentCapabilities; }
    const std::optional<QString>& agentInfo() const { return _agentInfo; }
    const std::optional<QList<AuthMethod>>& authMethods() const { return _authMethods; }
    const QString& protocolVersion() const { return _protocolVersion; }
};

template<>
ACPLIB_EXPORT Utils::Result<InitializeResponse> fromJson<InitializeResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const InitializeResponse &data);

/**
 * A mode the agent can operate in.
 *
 * See protocol docs: [Session Modes](https://agentclientprotocol.com/protocol/session-modes)
 */
struct SessionMode {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _description;
    SessionModeId _id;
    QString _name;

    SessionMode& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SessionMode& description(const std::optional<QString> & v) { _description = v; return *this; }
    SessionMode& id(const SessionModeId & v) { _id = v; return *this; }
    SessionMode& name(const QString & v) { _name = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& description() const { return _description; }
    const SessionModeId& id() const { return _id; }
    const QString& name() const { return _name; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionMode> fromJson<SessionMode>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionMode &data);

/** The set of modes and the one currently active. */
struct SessionModeState {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QList<SessionMode> _availableModes;  //!< The set of modes that the Agent can operate in
    QString _currentModeId;  //!< The current mode the Agent is in.

    SessionModeState& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SessionModeState& availableModes(const QList<SessionMode> & v) { _availableModes = v; return *this; }
    SessionModeState& addAvailableMode(const SessionMode & v) { _availableModes.append(v); return *this; }
    SessionModeState& currentModeId(const QString & v) { _currentModeId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QList<SessionMode>& availableModes() const { return _availableModes; }
    const QString& currentModeId() const { return _currentModeId; }
};

template<>
ACPLIB_EXPORT Utils::Result<SessionModeState> fromJson<SessionModeState>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SessionModeState &data);

/** Response from loading an existing session. */
struct LoadSessionResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QJsonArray> _configOptions;  //!< Initial session configuration options if supported by the Agent.
    /**
     * Initial mode state if supported by the Agent
     *
     * See protocol docs: [Session Modes](https://agentclientprotocol.com/protocol/session-modes)
     */
    std::optional<QString> _modes;

    LoadSessionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    LoadSessionResponse& configOptions(const std::optional<QJsonArray> & v) { _configOptions = v; return *this; }
    LoadSessionResponse& modes(const std::optional<QString> & v) { _modes = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QJsonArray>& configOptions() const { return _configOptions; }
    const std::optional<QString>& modes() const { return _modes; }
};

template<>
ACPLIB_EXPORT Utils::Result<LoadSessionResponse> fromJson<LoadSessionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LoadSessionResponse &data);

/**
 * Response from creating a new session.
 *
 * See protocol docs: [Creating a Session](https://agentclientprotocol.com/protocol/session-setup#creating-a-session)
 */
struct NewSessionResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QJsonArray> _configOptions;  //!< Initial session configuration options if supported by the Agent.
    /**
     * Initial mode state if supported by the Agent
     *
     * See protocol docs: [Session Modes](https://agentclientprotocol.com/protocol/session-modes)
     */
    std::optional<QString> _modes;
    /**
     * Unique identifier for the created session.
     *
     * Used in all subsequent requests for this conversation.
     */
    QString _sessionId;

    NewSessionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    NewSessionResponse& configOptions(const std::optional<QJsonArray> & v) { _configOptions = v; return *this; }
    NewSessionResponse& modes(const std::optional<QString> & v) { _modes = v; return *this; }
    NewSessionResponse& sessionId(const QString & v) { _sessionId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QJsonArray>& configOptions() const { return _configOptions; }
    const std::optional<QString>& modes() const { return _modes; }
    const QString& sessionId() const { return _sessionId; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _stopReason;  //!< Indicates why the agent stopped processing the turn.

    PromptResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    PromptResponse& stopReason(const QString & v) { _stopReason = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& stopReason() const { return _stopReason; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptResponse> fromJson<PromptResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptResponse &data);

/** Response to `session/set_config_option` method. */
struct SetSessionConfigOptionResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QList<SessionConfigOption> _configOptions;  //!< The full set of configuration options and their current values.

    SetSessionConfigOptionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SetSessionConfigOptionResponse& configOptions(const QList<SessionConfigOption> & v) { _configOptions = v; return *this; }
    SetSessionConfigOptionResponse& addConfigOption(const SessionConfigOption & v) { _configOptions.append(v); return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QList<SessionConfigOption>& configOptions() const { return _configOptions; }
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionConfigOptionResponse> fromJson<SetSessionConfigOptionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionConfigOptionResponse &data);

/** Response to `session/set_mode` method. */
struct SetSessionModeResponse {
    std::optional<QJsonObject> __meta;

    SetSessionModeResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionModeResponse> fromJson<SetSessionModeResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionModeResponse &data);

using AgentResponse = std::variant<QJsonObject>;

template<>
ACPLIB_EXPORT Utils::Result<AgentResponse> fromJson<AgentResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const AgentResponse &val);

/**
 * Request parameters for the authenticate method.
 *
 * Specifies which authentication method to use.
 */
struct AuthenticateRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    /**
     * The ID of the authentication method to use.
     * Must be one of the methods advertised in the initialize response.
     */
    QString _methodId;

    AuthenticateRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    AuthenticateRequest& methodId(const QString & v) { _methodId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& methodId() const { return _methodId; }
};

template<>
ACPLIB_EXPORT Utils::Result<AuthenticateRequest> fromJson<AuthenticateRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const AuthenticateRequest &data);

/**
 * Notification to cancel ongoing operations for a session.
 *
 * See protocol docs: [Cancellation](https://agentclientprotocol.com/protocol/prompt-turn#cancellation)
 */
struct CancelNotification {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _sessionId;  //!< The ID of the session to cancel operations for.

    CancelNotification& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    CancelNotification& sessionId(const QString & v) { _sessionId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& sessionId() const { return _sessionId; }
};

template<>
ACPLIB_EXPORT Utils::Result<CancelNotification> fromJson<CancelNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CancelNotification &data);

/**
 * Filesystem capabilities supported by the client.
 * File system capabilities that a client may support.
 *
 * See protocol docs: [FileSystem](https://agentclientprotocol.com/protocol/initialization#filesystem)
 */
struct FileSystemCapability {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<bool> _readTextFile;  //!< Whether the Client supports `fs/read_text_file` requests.
    std::optional<bool> _writeTextFile;  //!< Whether the Client supports `fs/write_text_file` requests.

    FileSystemCapability& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    FileSystemCapability& readTextFile(std::optional<bool> v) { _readTextFile = v; return *this; }
    FileSystemCapability& writeTextFile(std::optional<bool> v) { _writeTextFile = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<bool>& readTextFile() const { return _readTextFile; }
    const std::optional<bool>& writeTextFile() const { return _writeTextFile; }
};

template<>
ACPLIB_EXPORT Utils::Result<FileSystemCapability> fromJson<FileSystemCapability>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const FileSystemCapability &data);

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
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    /**
     * File system capabilities supported by the client.
     * Determines which file operations the agent can request.
     */
    std::optional<QString> _fs;
    std::optional<bool> _terminal;  //!< Whether the Client support all `terminal/*` methods.

    ClientCapabilities& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ClientCapabilities& fs(const std::optional<QString> & v) { _fs = v; return *this; }
    ClientCapabilities& terminal(std::optional<bool> v) { _terminal = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& fs() const { return _fs; }
    const std::optional<bool>& terminal() const { return _terminal; }
};

template<>
ACPLIB_EXPORT Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ClientCapabilities &data);

struct ClientNotification {
    QString _method;
    std::optional<QString> _params;

    ClientNotification& method(const QString & v) { _method = v; return *this; }
    ClientNotification& params(const std::optional<QString> & v) { _params = v; return *this; }

    const QString& method() const { return _method; }
    const std::optional<QString>& params() const { return _params; }
};

template<>
ACPLIB_EXPORT Utils::Result<ClientNotification> fromJson<ClientNotification>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ClientNotification &data);

/**
 * Request parameters for the initialize method.
 *
 * Sent by the client to establish connection and negotiate capabilities.
 *
 * See protocol docs: [Initialization](https://agentclientprotocol.com/protocol/initialization)
 */
struct InitializeRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _clientCapabilities;  //!< Capabilities supported by the client.
    /**
     * Information about the Client name and version sent to the Agent.
     *
     * Note: in future versions of the protocol, this will be required.
     */
    std::optional<QString> _clientInfo;
    QString _protocolVersion;  //!< The latest protocol version supported by the client.

    InitializeRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    InitializeRequest& clientCapabilities(const std::optional<QString> & v) { _clientCapabilities = v; return *this; }
    InitializeRequest& clientInfo(const std::optional<QString> & v) { _clientInfo = v; return *this; }
    InitializeRequest& protocolVersion(const QString & v) { _protocolVersion = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& clientCapabilities() const { return _clientCapabilities; }
    const std::optional<QString>& clientInfo() const { return _clientInfo; }
    const QString& protocolVersion() const { return _protocolVersion; }
};

template<>
ACPLIB_EXPORT Utils::Result<InitializeRequest> fromJson<InitializeRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const InitializeRequest &data);

/** An HTTP header to set when making requests to the MCP server. */
struct HttpHeader {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _name;  //!< The name of the HTTP header.
    QString _value;  //!< The value to set for the HTTP header.

    HttpHeader& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    HttpHeader& name(const QString & v) { _name = v; return *this; }
    HttpHeader& value(const QString & v) { _value = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& name() const { return _name; }
    const QString& value() const { return _value; }
};

template<>
ACPLIB_EXPORT Utils::Result<HttpHeader> fromJson<HttpHeader>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const HttpHeader &data);

/** HTTP transport configuration for MCP. */
struct McpServerHttp {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QList<HttpHeader> _headers;  //!< HTTP headers to set when making requests to the MCP server.
    QString _name;  //!< Human-readable name identifying this MCP server.
    QString _url;  //!< URL to the MCP server.

    McpServerHttp& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    McpServerHttp& headers(const QList<HttpHeader> & v) { _headers = v; return *this; }
    McpServerHttp& addHeader(const HttpHeader & v) { _headers.append(v); return *this; }
    McpServerHttp& name(const QString & v) { _name = v; return *this; }
    McpServerHttp& url(const QString & v) { _url = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QList<HttpHeader>& headers() const { return _headers; }
    const QString& name() const { return _name; }
    const QString& url() const { return _url; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpServerHttp> fromJson<McpServerHttp>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServerHttp &data);

/** SSE transport configuration for MCP. */
struct McpServerSse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QList<HttpHeader> _headers;  //!< HTTP headers to set when making requests to the MCP server.
    QString _name;  //!< Human-readable name identifying this MCP server.
    QString _url;  //!< URL to the MCP server.

    McpServerSse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    McpServerSse& headers(const QList<HttpHeader> & v) { _headers = v; return *this; }
    McpServerSse& addHeader(const HttpHeader & v) { _headers.append(v); return *this; }
    McpServerSse& name(const QString & v) { _name = v; return *this; }
    McpServerSse& url(const QString & v) { _url = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QList<HttpHeader>& headers() const { return _headers; }
    const QString& name() const { return _name; }
    const QString& url() const { return _url; }
};

template<>
ACPLIB_EXPORT Utils::Result<McpServerSse> fromJson<McpServerSse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServerSse &data);

/** Stdio transport configuration for MCP. */
struct McpServerStdio {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QStringList _args;  //!< Command-line arguments to pass to the MCP server.
    QString _command;  //!< Path to the MCP server executable.
    QList<EnvVariable> _env;  //!< Environment variables to set when launching the MCP server.
    QString _name;  //!< Human-readable name identifying this MCP server.

    McpServerStdio& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    McpServerStdio& args(const QStringList & v) { _args = v; return *this; }
    McpServerStdio& addArg(const QString & v) { _args.append(v); return *this; }
    McpServerStdio& command(const QString & v) { _command = v; return *this; }
    McpServerStdio& env(const QList<EnvVariable> & v) { _env = v; return *this; }
    McpServerStdio& addEnv(const EnvVariable & v) { _env.append(v); return *this; }
    McpServerStdio& name(const QString & v) { _name = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QStringList& args() const { return _args; }
    const QString& command() const { return _command; }
    const QList<EnvVariable>& env() const { return _env; }
    const QString& name() const { return _name; }
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

/** Returns the 'name' field from the active variant. */
ACPLIB_EXPORT QString name(const McpServer &val);

ACPLIB_EXPORT QJsonObject toJson(const McpServer &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const McpServer &val);

/**
 * Request parameters for loading an existing session.
 *
 * Only available if the Agent supports the `loadSession` capability.
 *
 * See protocol docs: [Loading Sessions](https://agentclientprotocol.com/protocol/session-setup#loading-sessions)
 */
struct LoadSessionRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _cwd;  //!< The working directory for this session.
    QList<McpServer> _mcpServers;  //!< List of MCP servers to connect to for this session.
    QString _sessionId;  //!< The ID of the session to load.

    LoadSessionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    LoadSessionRequest& cwd(const QString & v) { _cwd = v; return *this; }
    LoadSessionRequest& mcpServers(const QList<McpServer> & v) { _mcpServers = v; return *this; }
    LoadSessionRequest& addMcpServer(const McpServer & v) { _mcpServers.append(v); return *this; }
    LoadSessionRequest& sessionId(const QString & v) { _sessionId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& cwd() const { return _cwd; }
    const QList<McpServer>& mcpServers() const { return _mcpServers; }
    const QString& sessionId() const { return _sessionId; }
};

template<>
ACPLIB_EXPORT Utils::Result<LoadSessionRequest> fromJson<LoadSessionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const LoadSessionRequest &data);

/**
 * Request parameters for creating a new session.
 *
 * See protocol docs: [Creating a Session](https://agentclientprotocol.com/protocol/session-setup#creating-a-session)
 */
struct NewSessionRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _cwd;  //!< The working directory for this session. Must be an absolute path.
    QList<McpServer> _mcpServers;  //!< List of MCP (Model Context Protocol) servers the agent should connect to.

    NewSessionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    NewSessionRequest& cwd(const QString & v) { _cwd = v; return *this; }
    NewSessionRequest& mcpServers(const QList<McpServer> & v) { _mcpServers = v; return *this; }
    NewSessionRequest& addMcpServer(const McpServer & v) { _mcpServers.append(v); return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& cwd() const { return _cwd; }
    const QList<McpServer>& mcpServers() const { return _mcpServers; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
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
    QString _sessionId;  //!< The ID of the session to send this user message to

    PromptRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    PromptRequest& prompt(const QList<ContentBlock> & v) { _prompt = v; return *this; }
    PromptRequest& addPrompt(const ContentBlock & v) { _prompt.append(v); return *this; }
    PromptRequest& sessionId(const QString & v) { _sessionId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QList<ContentBlock>& prompt() const { return _prompt; }
    const QString& sessionId() const { return _sessionId; }
};

template<>
ACPLIB_EXPORT Utils::Result<PromptRequest> fromJson<PromptRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const PromptRequest &data);

/** Request parameters for setting a session configuration option. */
struct SetSessionConfigOptionRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _configId;  //!< The ID of the configuration option to set.
    QString _sessionId;  //!< The ID of the session to set the configuration option for.
    QString _value;  //!< The ID of the configuration option value to set.

    SetSessionConfigOptionRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SetSessionConfigOptionRequest& configId(const QString & v) { _configId = v; return *this; }
    SetSessionConfigOptionRequest& sessionId(const QString & v) { _sessionId = v; return *this; }
    SetSessionConfigOptionRequest& value(const QString & v) { _value = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& configId() const { return _configId; }
    const QString& sessionId() const { return _sessionId; }
    const QString& value() const { return _value; }
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionConfigOptionRequest> fromJson<SetSessionConfigOptionRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionConfigOptionRequest &data);

/** Request parameters for setting a session mode. */
struct SetSessionModeRequest {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _modeId;  //!< The ID of the mode to set.
    QString _sessionId;  //!< The ID of the session to set the mode for.

    SetSessionModeRequest& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SetSessionModeRequest& modeId(const QString & v) { _modeId = v; return *this; }
    SetSessionModeRequest& sessionId(const QString & v) { _sessionId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& modeId() const { return _modeId; }
    const QString& sessionId() const { return _sessionId; }
};

template<>
ACPLIB_EXPORT Utils::Result<SetSessionModeRequest> fromJson<SetSessionModeRequest>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SetSessionModeRequest &data);

struct ClientRequest {
    RequestId _id;
    QString _method;
    std::optional<QString> _params;

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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _terminalId;  //!< The unique identifier for the created terminal.

    CreateTerminalResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    CreateTerminalResponse& terminalId(const QString & v) { _terminalId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& terminalId() const { return _terminalId; }
};

template<>
ACPLIB_EXPORT Utils::Result<CreateTerminalResponse> fromJson<CreateTerminalResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const CreateTerminalResponse &data);

/** Response to terminal/kill command method */
struct KillTerminalCommandResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;

    KillTerminalCommandResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
};

template<>
ACPLIB_EXPORT Utils::Result<KillTerminalCommandResponse> fromJson<KillTerminalCommandResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const KillTerminalCommandResponse &data);

/** Response containing the contents of a text file. */
struct ReadTextFileResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _content;

    ReadTextFileResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    ReadTextFileResponse& content(const QString & v) { _content = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& content() const { return _content; }
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
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _optionId;  //!< The ID of the option the user selected.

    SelectedPermissionOutcome& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    SelectedPermissionOutcome& optionId(const QString & v) { _optionId = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& optionId() const { return _optionId; }
};

template<>
ACPLIB_EXPORT Utils::Result<SelectedPermissionOutcome> fromJson<SelectedPermissionOutcome>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const SelectedPermissionOutcome &data);

/** The outcome of a permission request. */
using RequestPermissionOutcome = std::variant<QJsonObject, SelectedPermissionOutcome>;

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionOutcome> fromJson<RequestPermissionOutcome>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionOutcome &val);

ACPLIB_EXPORT QJsonValue toJsonValue(const RequestPermissionOutcome &val);

/** Response to a permission request. */
struct RequestPermissionResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    QString _outcome;  //!< The user's decision on the permission request.

    RequestPermissionResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    RequestPermissionResponse& outcome(const QString & v) { _outcome = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const QString& outcome() const { return _outcome; }
};

template<>
ACPLIB_EXPORT Utils::Result<RequestPermissionResponse> fromJson<RequestPermissionResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const RequestPermissionResponse &data);

/** Exit status of a terminal command. */
struct TerminalExitStatus {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<int> _exitCode;  //!< The process exit code (may be null if terminated by signal).
    std::optional<QString> _signal;  //!< The signal that terminated the process (may be null if exited normally).

    TerminalExitStatus& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    TerminalExitStatus& exitCode(std::optional<int> v) { _exitCode = v; return *this; }
    TerminalExitStatus& signal(const std::optional<QString> & v) { _signal = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<int>& exitCode() const { return _exitCode; }
    const std::optional<QString>& signal() const { return _signal; }
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalExitStatus> fromJson<TerminalExitStatus>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalExitStatus &data);

/** Response containing the terminal output and exit status. */
struct TerminalOutputResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<QString> _exitStatus;  //!< Exit status if the command has completed.
    QString _output;  //!< The terminal output captured so far.
    bool _truncated;  //!< Whether the output was truncated due to byte limits.

    TerminalOutputResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    TerminalOutputResponse& exitStatus(const std::optional<QString> & v) { _exitStatus = v; return *this; }
    TerminalOutputResponse& output(const QString & v) { _output = v; return *this; }
    TerminalOutputResponse& truncated(bool v) { _truncated = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<QString>& exitStatus() const { return _exitStatus; }
    const QString& output() const { return _output; }
    const bool& truncated() const { return _truncated; }
};

template<>
ACPLIB_EXPORT Utils::Result<TerminalOutputResponse> fromJson<TerminalOutputResponse>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const TerminalOutputResponse &data);

/** Response containing the exit status of a terminal command. */
struct WaitForTerminalExitResponse {
    /**
     * The _meta property is reserved by ACP to allow clients and agents to attach additional
     * metadata to their interactions. Implementations MUST NOT make assumptions about values at
     * these keys.
     *
     * See protocol docs: [Extensibility](https://agentclientprotocol.com/protocol/extensibility)
     */
    std::optional<QJsonObject> __meta;
    std::optional<int> _exitCode;  //!< The process exit code (may be null if terminated by signal).
    std::optional<QString> _signal;  //!< The signal that terminated the process (may be null if exited normally).

    WaitForTerminalExitResponse& _meta(const std::optional<QJsonObject> & v) { __meta = v; return *this; }
    WaitForTerminalExitResponse& exitCode(std::optional<int> v) { _exitCode = v; return *this; }
    WaitForTerminalExitResponse& signal(const std::optional<QString> & v) { _signal = v; return *this; }

    const std::optional<QJsonObject>& _meta() const { return __meta; }
    const std::optional<int>& exitCode() const { return _exitCode; }
    const std::optional<QString>& signal() const { return _signal; }
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

using ClientResponse = std::variant<QJsonObject>;

} // namespace Acp
