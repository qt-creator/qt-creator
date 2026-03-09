/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 /opt/homebrew/opt/python@3.14/bin/python3.14 \
  scripts/generate_cpp_from_schema.py \
  --namespace Mcp::Generated::Schema::_2025_11_25 --cpp-output src/libs/mcp/schemas/schema_2025_11_25.cpp --export-macro MCPSERVER_EXPORT --export-header ../server/mcpserver_global.h src/libs/mcp/schemas/schema-2025-11-25.json src/libs/mcp/schemas/schema_2025_11_25.h
*/
#pragma once

#include "../server/mcpserver_global.h"

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

namespace Mcp::Generated::Schema::_2025_11_25 {

template<typename T> Utils::Result<T> fromJson(const QJsonValue &val) = delete;

/** The sender or recipient of messages and data in a conversation. */
enum class Role {
    assistant,
    user
};

MCPSERVER_EXPORT QString toString(Role v);

template<>
MCPSERVER_EXPORT Utils::Result<Role> fromJson<Role>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const Role &v);

/**
 * Optional annotations for the client. The client can use annotations to inform how objects are used or displayed
 */
struct Annotations {
    /**
     * Describes who the intended audience of this object or data is.
     *
     * It can include multiple entries to indicate content useful for multiple audiences (e.g., `["user", "assistant"]`).
     */
    std::optional<QList<Role>> _audience;
    /**
     * The moment the resource was last modified, as an ISO 8601 formatted string.
     *
     * Should be an ISO 8601 formatted string (e.g., "2025-01-12T15:00:58Z").
     *
     * Examples: last activity timestamp in an open file, timestamp when the resource
     * was attached, etc.
     */
    std::optional<QString> _lastModified;
    /**
     * Describes how important this data is for operating the server.
     *
     * A value of 1 means "most important," and indicates that the data is
     * effectively required, while 0 means "least important," and indicates that
     * the data is entirely optional.
     */
    std::optional<double> _priority;

    Annotations& audience(const std::optional<QList<Role>> & v) { _audience = v; return *this; }
    Annotations& addAudience(const Role & v) { if (!_audience) _audience = QList<Role>{}; (*_audience).append(v); return *this; }
    Annotations& lastModified(const std::optional<QString> & v) { _lastModified = v; return *this; }
    Annotations& priority(std::optional<double> v) { _priority = v; return *this; }

    const std::optional<QList<Role>>& audience() const { return _audience; }
    const std::optional<QString>& lastModified() const { return _lastModified; }
    const std::optional<double>& priority() const { return _priority; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Annotations> fromJson<Annotations>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Annotations &data);

/** Audio provided to or from an LLM. */
struct AudioContent {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<Annotations> _annotations;  //!< Optional annotations for the client.
    QString _data;  //!< The base64-encoded audio data.
    QString _mimeType;  //!< The MIME type of the audio. Different providers may support different audio types.

    AudioContent& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    AudioContent& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    AudioContent& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    AudioContent& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    AudioContent& data(const QString & v) { _data = v; return *this; }
    AudioContent& mimeType(const QString & v) { _mimeType = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const QString& data() const { return _data; }
    const QString& mimeType() const { return _mimeType; }
};

template<>
MCPSERVER_EXPORT Utils::Result<AudioContent> fromJson<AudioContent>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const AudioContent &data);

/** Base interface for metadata with name (identifier) and title (display name) properties. */
struct BaseMetadata {
    QString _name;  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for Tool,
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title;

    BaseMetadata& name(const QString & v) { _name = v; return *this; }
    BaseMetadata& title(const std::optional<QString> & v) { _title = v; return *this; }

    const QString& name() const { return _name; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<BaseMetadata> fromJson<BaseMetadata>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const BaseMetadata &data);

struct BlobResourceContents {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _blob;  //!< A base64-encoded string representing the binary data of the item.
    std::optional<QString> _mimeType;  //!< The MIME type of this resource, if known.
    QString _uri;  //!< The URI of this resource.

    BlobResourceContents& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    BlobResourceContents& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    BlobResourceContents& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    BlobResourceContents& blob(const QString & v) { _blob = v; return *this; }
    BlobResourceContents& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    BlobResourceContents& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QString& blob() const { return _blob; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<BlobResourceContents> fromJson<BlobResourceContents>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const BlobResourceContents &data);

struct BooleanSchema {
    std::optional<bool> _default_;
    std::optional<QString> _description;
    std::optional<QString> _title;

    BooleanSchema& default_(std::optional<bool> v) { _default_ = v; return *this; }
    BooleanSchema& description(const std::optional<QString> & v) { _description = v; return *this; }
    BooleanSchema& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<bool>& default_() const { return _default_; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<BooleanSchema> fromJson<BooleanSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const BooleanSchema &data);

/** A progress token, used to associate progress notifications with the original request. */
using ProgressToken = std::variant<QString, int>;

template<>
MCPSERVER_EXPORT Utils::Result<ProgressToken> fromJson<ProgressToken>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ProgressToken &val);

/**
 * Metadata for augmenting a request with task execution.
 * Include this in the `task` field of the request parameters.
 */
struct TaskMetadata {
    std::optional<int> _ttl;  //!< Requested duration in milliseconds to retain task from creation.

    TaskMetadata& ttl(std::optional<int> v) { _ttl = v; return *this; }

    const std::optional<int>& ttl() const { return _ttl; }
};

template<>
MCPSERVER_EXPORT Utils::Result<TaskMetadata> fromJson<TaskMetadata>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TaskMetadata &data);

/** Parameters for a `tools/call` request. */
struct CallToolRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<QMap<QString, QJsonValue>> _arguments;  //!< Arguments to use for the tool call.
    QString _name;  //!< The name of the tool.
    /**
     * If specified, the caller is requesting task-augmented execution for this request.
     * The request will return a CreateTaskResult immediately, and the actual result can be
     * retrieved later via tasks/result.
     *
     * Task augmentation is subject to capability negotiation - receivers MUST declare support
     * for task augmentation of specific request types in their capabilities.
     */
    std::optional<TaskMetadata> _task;

    CallToolRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    CallToolRequestParams& arguments(const std::optional<QMap<QString, QJsonValue>> & v) { _arguments = v; return *this; }
    CallToolRequestParams& addArgument(const QString &key, const QJsonValue &v) { if (!_arguments) _arguments = QMap<QString, QJsonValue>{}; (*_arguments)[key] = v; return *this; }
    CallToolRequestParams& arguments(const QJsonObject &obj) { if (!_arguments) _arguments = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_arguments)[it.key()] = it.value(); return *this; }
    CallToolRequestParams& name(const QString & v) { _name = v; return *this; }
    CallToolRequestParams& task(const std::optional<TaskMetadata> & v) { _task = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const std::optional<QMap<QString, QJsonValue>>& arguments() const { return _arguments; }
    QJsonObject argumentsAsObject() const { if (!_arguments) return {}; QJsonObject o; for (auto it = _arguments->constBegin(); it != _arguments->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QString& name() const { return _name; }
    const std::optional<TaskMetadata>& task() const { return _task; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CallToolRequestParams::Meta> fromJson<CallToolRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CallToolRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<CallToolRequestParams> fromJson<CallToolRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CallToolRequestParams &data);

/** A uniquely identifying ID for a request in JSON-RPC. */
using RequestId = std::variant<QString, int>;

/** Used by the client to invoke a tool provided by the server. */
struct CallToolRequest {
    RequestId _id;
    CallToolRequestParams _params;

    CallToolRequest& id(const RequestId & v) { _id = v; return *this; }
    CallToolRequest& params(const CallToolRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const CallToolRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CallToolRequest> fromJson<CallToolRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CallToolRequest &data);

struct TextResourceContents {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<QString> _mimeType;  //!< The MIME type of this resource, if known.
    QString _text;  //!< The text of the item. This must only be set if the item can actually be represented as text (not binary data).
    QString _uri;  //!< The URI of this resource.

    TextResourceContents& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    TextResourceContents& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    TextResourceContents& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    TextResourceContents& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    TextResourceContents& text(const QString & v) { _text = v; return *this; }
    TextResourceContents& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& text() const { return _text; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<TextResourceContents> fromJson<TextResourceContents>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TextResourceContents &data);

using EmbeddedResourceResource = std::variant<TextResourceContents, BlobResourceContents>;

template<>
MCPSERVER_EXPORT Utils::Result<EmbeddedResourceResource> fromJson<EmbeddedResourceResource>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const EmbeddedResourceResource &val);

/**
 * The contents of a resource, embedded into a prompt or tool call result.
 *
 * It is up to the client how best to render embedded resources for the benefit
 * of the LLM and/or the user.
 */
struct EmbeddedResource {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<Annotations> _annotations;  //!< Optional annotations for the client.
    EmbeddedResourceResource _resource;

    EmbeddedResource& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    EmbeddedResource& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    EmbeddedResource& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    EmbeddedResource& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    EmbeddedResource& resource(const EmbeddedResourceResource & v) { _resource = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const EmbeddedResourceResource& resource() const { return _resource; }
};

template<>
MCPSERVER_EXPORT Utils::Result<EmbeddedResource> fromJson<EmbeddedResource>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const EmbeddedResource &data);

/** An image provided to or from an LLM. */
struct ImageContent {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<Annotations> _annotations;  //!< Optional annotations for the client.
    QString _data;  //!< The base64-encoded image data.
    QString _mimeType;  //!< The MIME type of the image. Different providers may support different image types.

    ImageContent& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ImageContent& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ImageContent& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ImageContent& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    ImageContent& data(const QString & v) { _data = v; return *this; }
    ImageContent& mimeType(const QString & v) { _mimeType = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const QString& data() const { return _data; }
    const QString& mimeType() const { return _mimeType; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ImageContent> fromJson<ImageContent>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ImageContent &data);

/** An optionally-sized icon that can be displayed in a user interface. */
struct Icon {
    /**
     * Optional specifier for the theme this icon is designed for. `light` indicates
     * the icon is designed to be used with a light background, and `dark` indicates
     * the icon is designed to be used with a dark background.
     *
     * If not provided, the client should assume the icon can be used with any theme.
     */
    enum class Theme {
        dark,
        light
    };

    /**
     * Optional MIME type override if the source MIME type is missing or generic.
     * For example: `"image/png"`, `"image/jpeg"`, or `"image/svg+xml"`.
     */
    std::optional<QString> _mimeType;
    /**
     * Optional array of strings that specify sizes at which the icon can be used.
     * Each string should be in WxH format (e.g., `"48x48"`, `"96x96"`) or `"any"` for scalable formats like SVG.
     *
     * If not provided, the client should assume that the icon can be used at any size.
     */
    std::optional<QStringList> _sizes;
    /**
     * A standard URI pointing to an icon resource. May be an HTTP/HTTPS URL or a
     * `data:` URI with Base64-encoded image data.
     *
     * Consumers SHOULD takes steps to ensure URLs serving icons are from the
     * same domain as the client/server or a trusted domain.
     *
     * Consumers SHOULD take appropriate precautions when consuming SVGs as they can contain
     * executable JavaScript.
     */
    QString _src;
    std::optional<Theme> _theme;

    Icon& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    Icon& sizes(const std::optional<QStringList> & v) { _sizes = v; return *this; }
    Icon& addSize(const QString & v) { if (!_sizes) _sizes = QStringList{}; (*_sizes).append(v); return *this; }
    Icon& src(const QString & v) { _src = v; return *this; }
    Icon& theme(const std::optional<Theme> & v) { _theme = v; return *this; }

    const std::optional<QString>& mimeType() const { return _mimeType; }
    const std::optional<QStringList>& sizes() const { return _sizes; }
    const QString& src() const { return _src; }
    const std::optional<Theme>& theme() const { return _theme; }
};

MCPSERVER_EXPORT QString toString(const Icon::Theme &v);

template<>
MCPSERVER_EXPORT Utils::Result<Icon::Theme> fromJson<Icon::Theme>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const Icon::Theme &v);

template<>
MCPSERVER_EXPORT Utils::Result<Icon> fromJson<Icon>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Icon &data);

/**
 * A resource that the server is capable of reading, included in a prompt or tool call result.
 *
 * Note: resource links returned by tools are not guaranteed to appear in the results of `resources/list` requests.
 */
struct ResourceLink {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<Annotations> _annotations;  //!< Optional annotations for the client.
    /**
     * A description of what this resource represents.
     *
     * This can be used by clients to improve the LLM's understanding of available resources. It can be thought of like a "hint" to the model.
     */
    std::optional<QString> _description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::optional<QList<Icon>> _icons;
    std::optional<QString> _mimeType;  //!< The MIME type of this resource, if known.
    QString _name;  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * The size of the raw resource content, in bytes (i.e., before base64 encoding or any tokenization), if known.
     *
     * This can be used by Hosts to display file sizes and estimate context window usage.
     */
    std::optional<int> _size;
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for Tool,
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title;
    QString _uri;  //!< The URI of this resource.

    ResourceLink& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ResourceLink& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ResourceLink& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ResourceLink& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    ResourceLink& description(const std::optional<QString> & v) { _description = v; return *this; }
    ResourceLink& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    ResourceLink& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    ResourceLink& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    ResourceLink& name(const QString & v) { _name = v; return *this; }
    ResourceLink& size(std::optional<int> v) { _size = v; return *this; }
    ResourceLink& title(const std::optional<QString> & v) { _title = v; return *this; }
    ResourceLink& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QList<Icon>>& icons() const { return _icons; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& name() const { return _name; }
    const std::optional<int>& size() const { return _size; }
    const std::optional<QString>& title() const { return _title; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceLink> fromJson<ResourceLink>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceLink &data);

/** Text provided to or from an LLM. */
struct TextContent {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<Annotations> _annotations;  //!< Optional annotations for the client.
    QString _text;  //!< The text content of the message.

    TextContent& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    TextContent& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    TextContent& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    TextContent& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    TextContent& text(const QString & v) { _text = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const QString& text() const { return _text; }
};

template<>
MCPSERVER_EXPORT Utils::Result<TextContent> fromJson<TextContent>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TextContent &data);

using ContentBlock = std::variant<TextContent, ImageContent, AudioContent, ResourceLink, EmbeddedResource>;

template<>
MCPSERVER_EXPORT Utils::Result<ContentBlock> fromJson<ContentBlock>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ContentBlock &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ContentBlock &val);

/** Returns the 'type' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const ContentBlock &val);

/** The server's response to a tool call. */
struct CallToolResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QList<ContentBlock> _content;  //!< A list of content objects that represent the unstructured result of the tool call.
    /**
     * Whether the tool call ended in an error.
     *
     * If not set, this is assumed to be false (the call was successful).
     *
     * Any errors that originate from the tool SHOULD be reported inside the result
     * object, with `isError` set to true, _not_ as an MCP protocol-level error
     * response. Otherwise, the LLM would not be able to see that an error occurred
     * and self-correct.
     *
     * However, any errors in _finding_ the tool, an error indicating that the
     * server does not support tool calls, or any other exceptional conditions,
     * should be reported as an MCP error response.
     */
    std::optional<bool> _isError;
    std::optional<QMap<QString, QJsonValue>> _structuredContent;  //!< An optional JSON object that represents the structured result of the tool call.

    CallToolResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    CallToolResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    CallToolResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    CallToolResult& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    CallToolResult& addContent(const ContentBlock & v) { _content.append(v); return *this; }
    CallToolResult& isError(std::optional<bool> v) { _isError = v; return *this; }
    CallToolResult& structuredContent(const std::optional<QMap<QString, QJsonValue>> & v) { _structuredContent = v; return *this; }
    CallToolResult& addStructuredContent(const QString &key, const QJsonValue &v) { if (!_structuredContent) _structuredContent = QMap<QString, QJsonValue>{}; (*_structuredContent)[key] = v; return *this; }
    CallToolResult& structuredContent(const QJsonObject &obj) { if (!_structuredContent) _structuredContent = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_structuredContent)[it.key()] = it.value(); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QList<ContentBlock>& content() const { return _content; }
    const std::optional<bool>& isError() const { return _isError; }
    const std::optional<QMap<QString, QJsonValue>>& structuredContent() const { return _structuredContent; }
    QJsonObject structuredContentAsObject() const { if (!_structuredContent) return {}; QJsonObject o; for (auto it = _structuredContent->constBegin(); it != _structuredContent->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CallToolResult> fromJson<CallToolResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CallToolResult &data);

/** A request to cancel a task. */
struct CancelTaskRequest {
    struct Params {
        QString _taskId;  //!< The task identifier to cancel.

        Params& taskId(const QString & v) { _taskId = v; return *this; }

        const QString& taskId() const { return _taskId; }
    };

    RequestId _id;
    Params _params;

    CancelTaskRequest& id(const RequestId & v) { _id = v; return *this; }
    CancelTaskRequest& params(const Params & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const Params& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CancelTaskRequest::Params> fromJson<CancelTaskRequest::Params>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CancelTaskRequest::Params &data);

template<>
MCPSERVER_EXPORT Utils::Result<CancelTaskRequest> fromJson<CancelTaskRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CancelTaskRequest &data);

struct Result {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QJsonObject _additionalProperties;  //!< additional properties

    Result& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    Result& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    Result& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    Result& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    Result& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Result> fromJson<Result>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Result &data);

/** The status of a task. */
enum class TaskStatus {
    cancelled,
    completed,
    failed,
    input_required,
    working
};

MCPSERVER_EXPORT QString toString(TaskStatus v);

template<>
MCPSERVER_EXPORT Utils::Result<TaskStatus> fromJson<TaskStatus>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const TaskStatus &v);

/** Data associated with a task. */
struct Task {
    QString _createdAt;  //!< ISO 8601 timestamp when the task was created.
    QString _lastUpdatedAt;  //!< ISO 8601 timestamp when the task was last updated.
    std::optional<int> _pollInterval;  //!< Suggested polling interval in milliseconds.
    TaskStatus _status;  //!< Current task state.
    /**
     * Optional human-readable message describing the current task state.
     * This can provide context for any status, including:
     * - Reasons for "cancelled" status
     * - Summaries for "completed" status
     * - Diagnostic information for "failed" status (e.g., error details, what went wrong)
     */
    std::optional<QString> _statusMessage;
    QString _taskId;  //!< The task identifier.
    std::optional<int> _ttl;  //!< Actual retention duration from creation in milliseconds, null for unlimited.

    Task& createdAt(const QString & v) { _createdAt = v; return *this; }
    Task& lastUpdatedAt(const QString & v) { _lastUpdatedAt = v; return *this; }
    Task& pollInterval(std::optional<int> v) { _pollInterval = v; return *this; }
    Task& status(const TaskStatus & v) { _status = v; return *this; }
    Task& statusMessage(const std::optional<QString> & v) { _statusMessage = v; return *this; }
    Task& taskId(const QString & v) { _taskId = v; return *this; }
    Task& ttl(std::optional<int> v) { _ttl = v; return *this; }

    const QString& createdAt() const { return _createdAt; }
    const QString& lastUpdatedAt() const { return _lastUpdatedAt; }
    const std::optional<int>& pollInterval() const { return _pollInterval; }
    const TaskStatus& status() const { return _status; }
    const std::optional<QString>& statusMessage() const { return _statusMessage; }
    const QString& taskId() const { return _taskId; }
    const std::optional<int>& ttl() const { return _ttl; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Task> fromJson<Task>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Task &data);

/** The response to a tasks/cancel request. */
struct CancelTaskResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _createdAt;  //!< ISO 8601 timestamp when the task was created.
    QString _lastUpdatedAt;  //!< ISO 8601 timestamp when the task was last updated.
    std::optional<int> _pollInterval;  //!< Suggested polling interval in milliseconds.
    TaskStatus _status;  //!< Current task state.
    /**
     * Optional human-readable message describing the current task state.
     * This can provide context for any status, including:
     * - Reasons for "cancelled" status
     * - Summaries for "completed" status
     * - Diagnostic information for "failed" status (e.g., error details, what went wrong)
     */
    std::optional<QString> _statusMessage;
    QString _taskId;  //!< The task identifier.
    std::optional<int> _ttl;  //!< Actual retention duration from creation in milliseconds, null for unlimited.

    CancelTaskResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    CancelTaskResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    CancelTaskResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    CancelTaskResult& createdAt(const QString & v) { _createdAt = v; return *this; }
    CancelTaskResult& lastUpdatedAt(const QString & v) { _lastUpdatedAt = v; return *this; }
    CancelTaskResult& pollInterval(std::optional<int> v) { _pollInterval = v; return *this; }
    CancelTaskResult& status(const TaskStatus & v) { _status = v; return *this; }
    CancelTaskResult& statusMessage(const std::optional<QString> & v) { _statusMessage = v; return *this; }
    CancelTaskResult& taskId(const QString & v) { _taskId = v; return *this; }
    CancelTaskResult& ttl(std::optional<int> v) { _ttl = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QString& createdAt() const { return _createdAt; }
    const QString& lastUpdatedAt() const { return _lastUpdatedAt; }
    const std::optional<int>& pollInterval() const { return _pollInterval; }
    const TaskStatus& status() const { return _status; }
    const std::optional<QString>& statusMessage() const { return _statusMessage; }
    const QString& taskId() const { return _taskId; }
    const std::optional<int>& ttl() const { return _ttl; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CancelTaskResult> fromJson<CancelTaskResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CancelTaskResult &data);

/** Parameters for a `notifications/cancelled` notification. */
struct CancelledNotificationParams {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<QString> _reason;  //!< An optional string describing the reason for the cancellation. This MAY be logged or presented to the user.
    /**
     * The ID of the request to cancel.
     *
     * This MUST correspond to the ID of a request previously issued in the same direction.
     * This MUST be provided for cancelling non-task requests.
     * This MUST NOT be used for cancelling tasks (use the `tasks/cancel` request instead).
     */
    std::optional<RequestId> _requestId;

    CancelledNotificationParams& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    CancelledNotificationParams& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    CancelledNotificationParams& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    CancelledNotificationParams& reason(const std::optional<QString> & v) { _reason = v; return *this; }
    CancelledNotificationParams& requestId(const std::optional<RequestId> & v) { _requestId = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& reason() const { return _reason; }
    const std::optional<RequestId>& requestId() const { return _requestId; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CancelledNotificationParams> fromJson<CancelledNotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CancelledNotificationParams &data);

/**
 * This notification can be sent by either side to indicate that it is cancelling a previously-issued request.
 *
 * The request SHOULD still be in-flight, but due to communication latency, it is always possible that this notification MAY arrive after the request has already finished.
 *
 * This notification indicates that the result will be unused, so any associated processing SHOULD cease.
 *
 * A client MUST NOT attempt to cancel its `initialize` request.
 *
 * For task cancellation, use the `tasks/cancel` request instead of this notification.
 */
struct CancelledNotification {
    CancelledNotificationParams _params;

    CancelledNotification& params(const CancelledNotificationParams & v) { _params = v; return *this; }

    const CancelledNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CancelledNotification> fromJson<CancelledNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CancelledNotification &data);

/**
 * Capabilities a client may support. Known capabilities are defined here, in this schema, but this is not a closed set: any client can define its own, additional capabilities.
 */
struct ClientCapabilities {
    /** Present if the client supports elicitation from the server. */
    struct Elicitation {
        std::optional<QMap<QString, QJsonValue>> _form;
        std::optional<QMap<QString, QJsonValue>> _url;

        Elicitation& form(const std::optional<QMap<QString, QJsonValue>> & v) { _form = v; return *this; }
        Elicitation& addForm(const QString &key, const QJsonValue &v) { if (!_form) _form = QMap<QString, QJsonValue>{}; (*_form)[key] = v; return *this; }
        Elicitation& form(const QJsonObject &obj) { if (!_form) _form = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_form)[it.key()] = it.value(); return *this; }
        Elicitation& url(const std::optional<QMap<QString, QJsonValue>> & v) { _url = v; return *this; }
        Elicitation& addUrl(const QString &key, const QJsonValue &v) { if (!_url) _url = QMap<QString, QJsonValue>{}; (*_url)[key] = v; return *this; }
        Elicitation& url(const QJsonObject &obj) { if (!_url) _url = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_url)[it.key()] = it.value(); return *this; }

        const std::optional<QMap<QString, QJsonValue>>& form() const { return _form; }
        QJsonObject formAsObject() const { if (!_form) return {}; QJsonObject o; for (auto it = _form->constBegin(); it != _form->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
        const std::optional<QMap<QString, QJsonValue>>& url() const { return _url; }
        QJsonObject urlAsObject() const { if (!_url) return {}; QJsonObject o; for (auto it = _url->constBegin(); it != _url->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    };

    /** Present if the client supports listing roots. */
    struct Roots {
        std::optional<bool> _listChanged;  //!< Whether the client supports notifications for changes to the roots list.

        Roots& listChanged(std::optional<bool> v) { _listChanged = v; return *this; }

        const std::optional<bool>& listChanged() const { return _listChanged; }
    };

    /** Present if the client supports sampling from an LLM. */
    struct Sampling {
        /**
         * Whether the client supports context inclusion via includeContext parameter.
         * If not declared, servers SHOULD only use `includeContext: "none"` (or omit it).
         */
        std::optional<QMap<QString, QJsonValue>> _context;
        std::optional<QMap<QString, QJsonValue>> _tools;  //!< Whether the client supports tool use via tools and toolChoice parameters.

        Sampling& context(const std::optional<QMap<QString, QJsonValue>> & v) { _context = v; return *this; }
        Sampling& addContext(const QString &key, const QJsonValue &v) { if (!_context) _context = QMap<QString, QJsonValue>{}; (*_context)[key] = v; return *this; }
        Sampling& context(const QJsonObject &obj) { if (!_context) _context = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_context)[it.key()] = it.value(); return *this; }
        Sampling& tools(const std::optional<QMap<QString, QJsonValue>> & v) { _tools = v; return *this; }
        Sampling& addTool(const QString &key, const QJsonValue &v) { if (!_tools) _tools = QMap<QString, QJsonValue>{}; (*_tools)[key] = v; return *this; }
        Sampling& tools(const QJsonObject &obj) { if (!_tools) _tools = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_tools)[it.key()] = it.value(); return *this; }

        const std::optional<QMap<QString, QJsonValue>>& context() const { return _context; }
        QJsonObject contextAsObject() const { if (!_context) return {}; QJsonObject o; for (auto it = _context->constBegin(); it != _context->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
        const std::optional<QMap<QString, QJsonValue>>& tools() const { return _tools; }
        QJsonObject toolsAsObject() const { if (!_tools) return {}; QJsonObject o; for (auto it = _tools->constBegin(); it != _tools->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    };

    /** Present if the client supports task-augmented requests. */
    struct Tasks {
        /** Specifies which request types can be augmented with tasks. */
        struct Requests {
            /** Task support for elicitation-related requests. */
            struct Elicitation {
                std::optional<QMap<QString, QJsonValue>> _create;  //!< Whether the client supports task-augmented elicitation/create requests.

                Elicitation& create(const std::optional<QMap<QString, QJsonValue>> & v) { _create = v; return *this; }
                Elicitation& addCreate(const QString &key, const QJsonValue &v) { if (!_create) _create = QMap<QString, QJsonValue>{}; (*_create)[key] = v; return *this; }
                Elicitation& create(const QJsonObject &obj) { if (!_create) _create = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_create)[it.key()] = it.value(); return *this; }

                const std::optional<QMap<QString, QJsonValue>>& create() const { return _create; }
                QJsonObject createAsObject() const { if (!_create) return {}; QJsonObject o; for (auto it = _create->constBegin(); it != _create->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
            };

            /** Task support for sampling-related requests. */
            struct Sampling {
                std::optional<QMap<QString, QJsonValue>> _createMessage;  //!< Whether the client supports task-augmented sampling/createMessage requests.

                Sampling& createMessage(const std::optional<QMap<QString, QJsonValue>> & v) { _createMessage = v; return *this; }
                Sampling& addCreateMessage(const QString &key, const QJsonValue &v) { if (!_createMessage) _createMessage = QMap<QString, QJsonValue>{}; (*_createMessage)[key] = v; return *this; }
                Sampling& createMessage(const QJsonObject &obj) { if (!_createMessage) _createMessage = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_createMessage)[it.key()] = it.value(); return *this; }

                const std::optional<QMap<QString, QJsonValue>>& createMessage() const { return _createMessage; }
                QJsonObject createMessageAsObject() const { if (!_createMessage) return {}; QJsonObject o; for (auto it = _createMessage->constBegin(); it != _createMessage->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
            };

            std::optional<Elicitation> _elicitation;  //!< Task support for elicitation-related requests.
            std::optional<Sampling> _sampling;  //!< Task support for sampling-related requests.

            Requests& elicitation(const std::optional<Elicitation> & v) { _elicitation = v; return *this; }
            Requests& sampling(const std::optional<Sampling> & v) { _sampling = v; return *this; }

            const std::optional<Elicitation>& elicitation() const { return _elicitation; }
            const std::optional<Sampling>& sampling() const { return _sampling; }
        };

        std::optional<QMap<QString, QJsonValue>> _cancel;  //!< Whether this client supports tasks/cancel.
        std::optional<QMap<QString, QJsonValue>> _list;  //!< Whether this client supports tasks/list.
        std::optional<Requests> _requests;  //!< Specifies which request types can be augmented with tasks.

        Tasks& cancel(const std::optional<QMap<QString, QJsonValue>> & v) { _cancel = v; return *this; }
        Tasks& addCancel(const QString &key, const QJsonValue &v) { if (!_cancel) _cancel = QMap<QString, QJsonValue>{}; (*_cancel)[key] = v; return *this; }
        Tasks& cancel(const QJsonObject &obj) { if (!_cancel) _cancel = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_cancel)[it.key()] = it.value(); return *this; }
        Tasks& list(const std::optional<QMap<QString, QJsonValue>> & v) { _list = v; return *this; }
        Tasks& addList(const QString &key, const QJsonValue &v) { if (!_list) _list = QMap<QString, QJsonValue>{}; (*_list)[key] = v; return *this; }
        Tasks& list(const QJsonObject &obj) { if (!_list) _list = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_list)[it.key()] = it.value(); return *this; }
        Tasks& requests(const std::optional<Requests> & v) { _requests = v; return *this; }

        const std::optional<QMap<QString, QJsonValue>>& cancel() const { return _cancel; }
        QJsonObject cancelAsObject() const { if (!_cancel) return {}; QJsonObject o; for (auto it = _cancel->constBegin(); it != _cancel->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
        const std::optional<QMap<QString, QJsonValue>>& list() const { return _list; }
        QJsonObject listAsObject() const { if (!_list) return {}; QJsonObject o; for (auto it = _list->constBegin(); it != _list->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
        const std::optional<Requests>& requests() const { return _requests; }
    };

    std::optional<Elicitation> _elicitation;  //!< Present if the client supports elicitation from the server.
    std::optional<QMap<QString, QJsonObject>> _experimental;  //!< Experimental, non-standard capabilities that the client supports.
    std::optional<Roots> _roots;  //!< Present if the client supports listing roots.
    std::optional<Sampling> _sampling;  //!< Present if the client supports sampling from an LLM.
    std::optional<Tasks> _tasks;  //!< Present if the client supports task-augmented requests.

    ClientCapabilities& elicitation(const std::optional<Elicitation> & v) { _elicitation = v; return *this; }
    ClientCapabilities& experimental(const std::optional<QMap<QString, QJsonObject>> & v) { _experimental = v; return *this; }
    ClientCapabilities& addExperimental(const QString &key, const QJsonObject & v) { if (!_experimental) _experimental = QMap<QString, QJsonObject>{}; (*_experimental)[key] = v; return *this; }
    ClientCapabilities& roots(const std::optional<Roots> & v) { _roots = v; return *this; }
    ClientCapabilities& sampling(const std::optional<Sampling> & v) { _sampling = v; return *this; }
    ClientCapabilities& tasks(const std::optional<Tasks> & v) { _tasks = v; return *this; }

    const std::optional<Elicitation>& elicitation() const { return _elicitation; }
    const std::optional<QMap<QString, QJsonObject>>& experimental() const { return _experimental; }
    const std::optional<Roots>& roots() const { return _roots; }
    const std::optional<Sampling>& sampling() const { return _sampling; }
    const std::optional<Tasks>& tasks() const { return _tasks; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities::Elicitation> fromJson<ClientCapabilities::Elicitation>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities::Elicitation &data);

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities::Roots> fromJson<ClientCapabilities::Roots>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities::Roots &data);

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities::Sampling> fromJson<ClientCapabilities::Sampling>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities::Sampling &data);

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities::Tasks::Requests::Elicitation> fromJson<ClientCapabilities::Tasks::Requests::Elicitation>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities::Tasks::Requests::Elicitation &data);

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities::Tasks::Requests::Sampling> fromJson<ClientCapabilities::Tasks::Requests::Sampling>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities::Tasks::Requests::Sampling &data);

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities::Tasks::Requests> fromJson<ClientCapabilities::Tasks::Requests>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities::Tasks::Requests &data);

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities::Tasks> fromJson<ClientCapabilities::Tasks>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities::Tasks &data);

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities &data);

struct NotificationParams {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.

    NotificationParams& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    NotificationParams& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    NotificationParams& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
};

template<>
MCPSERVER_EXPORT Utils::Result<NotificationParams> fromJson<NotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const NotificationParams &data);

/** This notification is sent from the client to the server after initialization has finished. */
struct InitializedNotification {
    std::optional<NotificationParams> _params;

    InitializedNotification& params(const std::optional<NotificationParams> & v) { _params = v; return *this; }

    const std::optional<NotificationParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<InitializedNotification> fromJson<InitializedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InitializedNotification &data);

/** Parameters for a `notifications/progress` notification. */
struct ProgressNotificationParams {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<QString> _message;  //!< An optional message describing the current progress.
    double _progress;  //!< The progress thus far. This should increase every time progress is made, even if the total is unknown.
    ProgressToken _progressToken;  //!< The progress token which was given in the initial request, used to associate this notification with the request that is proceeding.
    std::optional<double> _total;  //!< Total number of items to process (or total progress required), if known.

    ProgressNotificationParams& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ProgressNotificationParams& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ProgressNotificationParams& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ProgressNotificationParams& message(const std::optional<QString> & v) { _message = v; return *this; }
    ProgressNotificationParams& progress(double v) { _progress = v; return *this; }
    ProgressNotificationParams& progressToken(const ProgressToken & v) { _progressToken = v; return *this; }
    ProgressNotificationParams& total(std::optional<double> v) { _total = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& message() const { return _message; }
    const double& progress() const { return _progress; }
    const ProgressToken& progressToken() const { return _progressToken; }
    const std::optional<double>& total() const { return _total; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ProgressNotificationParams> fromJson<ProgressNotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ProgressNotificationParams &data);

/**
 * An out-of-band notification used to inform the receiver of a progress update for a long-running request.
 */
struct ProgressNotification {
    ProgressNotificationParams _params;

    ProgressNotification& params(const ProgressNotificationParams & v) { _params = v; return *this; }

    const ProgressNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ProgressNotification> fromJson<ProgressNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ProgressNotification &data);

/**
 * A notification from the client to the server, informing it that the list of roots has changed.
 * This notification should be sent whenever the client adds, removes, or modifies any root.
 * The server should then request an updated list of roots using the ListRootsRequest.
 */
struct RootsListChangedNotification {
    std::optional<NotificationParams> _params;

    RootsListChangedNotification& params(const std::optional<NotificationParams> & v) { _params = v; return *this; }

    const std::optional<NotificationParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<RootsListChangedNotification> fromJson<RootsListChangedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const RootsListChangedNotification &data);

/** Parameters for a `notifications/tasks/status` notification. */
struct TaskStatusNotificationParams {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _createdAt;  //!< ISO 8601 timestamp when the task was created.
    QString _lastUpdatedAt;  //!< ISO 8601 timestamp when the task was last updated.
    std::optional<int> _pollInterval;  //!< Suggested polling interval in milliseconds.
    TaskStatus _status;  //!< Current task state.
    /**
     * Optional human-readable message describing the current task state.
     * This can provide context for any status, including:
     * - Reasons for "cancelled" status
     * - Summaries for "completed" status
     * - Diagnostic information for "failed" status (e.g., error details, what went wrong)
     */
    std::optional<QString> _statusMessage;
    QString _taskId;  //!< The task identifier.
    std::optional<int> _ttl;  //!< Actual retention duration from creation in milliseconds, null for unlimited.

    TaskStatusNotificationParams& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    TaskStatusNotificationParams& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    TaskStatusNotificationParams& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    TaskStatusNotificationParams& createdAt(const QString & v) { _createdAt = v; return *this; }
    TaskStatusNotificationParams& lastUpdatedAt(const QString & v) { _lastUpdatedAt = v; return *this; }
    TaskStatusNotificationParams& pollInterval(std::optional<int> v) { _pollInterval = v; return *this; }
    TaskStatusNotificationParams& status(const TaskStatus & v) { _status = v; return *this; }
    TaskStatusNotificationParams& statusMessage(const std::optional<QString> & v) { _statusMessage = v; return *this; }
    TaskStatusNotificationParams& taskId(const QString & v) { _taskId = v; return *this; }
    TaskStatusNotificationParams& ttl(std::optional<int> v) { _ttl = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QString& createdAt() const { return _createdAt; }
    const QString& lastUpdatedAt() const { return _lastUpdatedAt; }
    const std::optional<int>& pollInterval() const { return _pollInterval; }
    const TaskStatus& status() const { return _status; }
    const std::optional<QString>& statusMessage() const { return _statusMessage; }
    const QString& taskId() const { return _taskId; }
    const std::optional<int>& ttl() const { return _ttl; }
};

template<>
MCPSERVER_EXPORT Utils::Result<TaskStatusNotificationParams> fromJson<TaskStatusNotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TaskStatusNotificationParams &data);

/**
 * An optional notification from the receiver to the requestor, informing them that a task's status has changed. Receivers are not required to send these notifications.
 */
struct TaskStatusNotification {
    TaskStatusNotificationParams _params;

    TaskStatusNotification& params(const TaskStatusNotificationParams & v) { _params = v; return *this; }

    const TaskStatusNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<TaskStatusNotification> fromJson<TaskStatusNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TaskStatusNotification &data);

using ClientNotification = std::variant<CancelledNotification, InitializedNotification, ProgressNotification, TaskStatusNotification, RootsListChangedNotification>;

template<>
MCPSERVER_EXPORT Utils::Result<ClientNotification> fromJson<ClientNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientNotification &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ClientNotification &val);

/** Returns the 'method' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const ClientNotification &val);

/** Identifies a prompt. */
struct PromptReference {
    QString _name;  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for Tool,
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title;

    PromptReference& name(const QString & v) { _name = v; return *this; }
    PromptReference& title(const std::optional<QString> & v) { _title = v; return *this; }

    const QString& name() const { return _name; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PromptReference> fromJson<PromptReference>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PromptReference &data);

/** A reference to a resource or resource template definition. */
struct ResourceTemplateReference {
    QString _uri;  //!< The URI or URI template of the resource.

    ResourceTemplateReference& uri(const QString & v) { _uri = v; return *this; }

    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceTemplateReference> fromJson<ResourceTemplateReference>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceTemplateReference &data);

using CompleteRequestParamsRef = std::variant<PromptReference, ResourceTemplateReference>;

template<>
MCPSERVER_EXPORT Utils::Result<CompleteRequestParamsRef> fromJson<CompleteRequestParamsRef>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const CompleteRequestParamsRef &val);

/** Parameters for a `completion/complete` request. */
struct CompleteRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    /** The argument's information */
    struct Argument {
        QString _name;  //!< The name of the argument
        QString _value;  //!< The value of the argument to use for completion matching.

        Argument& name(const QString & v) { _name = v; return *this; }
        Argument& value(const QString & v) { _value = v; return *this; }

        const QString& name() const { return _name; }
        const QString& value() const { return _value; }
    };

    /** Additional, optional context for completions */
    struct Context {
        std::optional<QMap<QString, QString>> _arguments;  //!< Previously-resolved variables in a URI template or prompt.

        Context& arguments(const std::optional<QMap<QString, QString>> & v) { _arguments = v; return *this; }
        Context& addArgument(const QString &key, const QString & v) { if (!_arguments) _arguments = QMap<QString, QString>{}; (*_arguments)[key] = v; return *this; }

        const std::optional<QMap<QString, QString>>& arguments() const { return _arguments; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    Argument _argument;  //!< The argument's information
    std::optional<Context> _context;  //!< Additional, optional context for completions
    CompleteRequestParamsRef _ref;

    CompleteRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    CompleteRequestParams& argument(const Argument & v) { _argument = v; return *this; }
    CompleteRequestParams& context(const std::optional<Context> & v) { _context = v; return *this; }
    CompleteRequestParams& ref(const CompleteRequestParamsRef & v) { _ref = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const Argument& argument() const { return _argument; }
    const std::optional<Context>& context() const { return _context; }
    const CompleteRequestParamsRef& ref() const { return _ref; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CompleteRequestParams::Meta> fromJson<CompleteRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<CompleteRequestParams::Argument> fromJson<CompleteRequestParams::Argument>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteRequestParams::Argument &data);

template<>
MCPSERVER_EXPORT Utils::Result<CompleteRequestParams::Context> fromJson<CompleteRequestParams::Context>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteRequestParams::Context &data);

template<>
MCPSERVER_EXPORT Utils::Result<CompleteRequestParams> fromJson<CompleteRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteRequestParams &data);

/** A request from the client to the server, to ask for completion options. */
struct CompleteRequest {
    RequestId _id;
    CompleteRequestParams _params;

    CompleteRequest& id(const RequestId & v) { _id = v; return *this; }
    CompleteRequest& params(const CompleteRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const CompleteRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CompleteRequest> fromJson<CompleteRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteRequest &data);

/** Parameters for a `prompts/get` request. */
struct GetPromptRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<QMap<QString, QString>> _arguments;  //!< Arguments to use for templating the prompt.
    QString _name;  //!< The name of the prompt or prompt template.

    GetPromptRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    GetPromptRequestParams& arguments(const std::optional<QMap<QString, QString>> & v) { _arguments = v; return *this; }
    GetPromptRequestParams& addArgument(const QString &key, const QString & v) { if (!_arguments) _arguments = QMap<QString, QString>{}; (*_arguments)[key] = v; return *this; }
    GetPromptRequestParams& name(const QString & v) { _name = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const std::optional<QMap<QString, QString>>& arguments() const { return _arguments; }
    const QString& name() const { return _name; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetPromptRequestParams::Meta> fromJson<GetPromptRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetPromptRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<GetPromptRequestParams> fromJson<GetPromptRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetPromptRequestParams &data);

/** Used by the client to get a prompt provided by the server. */
struct GetPromptRequest {
    RequestId _id;
    GetPromptRequestParams _params;

    GetPromptRequest& id(const RequestId & v) { _id = v; return *this; }
    GetPromptRequest& params(const GetPromptRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const GetPromptRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetPromptRequest> fromJson<GetPromptRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetPromptRequest &data);

/** A request to retrieve the result of a completed task. */
struct GetTaskPayloadRequest {
    struct Params {
        QString _taskId;  //!< The task identifier to retrieve results for.

        Params& taskId(const QString & v) { _taskId = v; return *this; }

        const QString& taskId() const { return _taskId; }
    };

    RequestId _id;
    Params _params;

    GetTaskPayloadRequest& id(const RequestId & v) { _id = v; return *this; }
    GetTaskPayloadRequest& params(const Params & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const Params& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetTaskPayloadRequest::Params> fromJson<GetTaskPayloadRequest::Params>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetTaskPayloadRequest::Params &data);

template<>
MCPSERVER_EXPORT Utils::Result<GetTaskPayloadRequest> fromJson<GetTaskPayloadRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetTaskPayloadRequest &data);

/** A request to retrieve the state of a task. */
struct GetTaskRequest {
    struct Params {
        QString _taskId;  //!< The task identifier to query.

        Params& taskId(const QString & v) { _taskId = v; return *this; }

        const QString& taskId() const { return _taskId; }
    };

    RequestId _id;
    Params _params;

    GetTaskRequest& id(const RequestId & v) { _id = v; return *this; }
    GetTaskRequest& params(const Params & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const Params& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetTaskRequest::Params> fromJson<GetTaskRequest::Params>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetTaskRequest::Params &data);

template<>
MCPSERVER_EXPORT Utils::Result<GetTaskRequest> fromJson<GetTaskRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetTaskRequest &data);

/** Describes the MCP implementation. */
struct Implementation {
    /**
     * An optional human-readable description of what this implementation does.
     *
     * This can be used by clients or servers to provide context about their purpose
     * and capabilities. For example, a server might describe the types of resources
     * or tools it provides, while a client might describe its intended use case.
     */
    std::optional<QString> _description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::optional<QList<Icon>> _icons;
    QString _name;  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for Tool,
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title;
    QString _version;
    std::optional<QString> _websiteUrl;  //!< An optional URL of the website for this implementation.

    Implementation& description(const std::optional<QString> & v) { _description = v; return *this; }
    Implementation& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    Implementation& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    Implementation& name(const QString & v) { _name = v; return *this; }
    Implementation& title(const std::optional<QString> & v) { _title = v; return *this; }
    Implementation& version(const QString & v) { _version = v; return *this; }
    Implementation& websiteUrl(const std::optional<QString> & v) { _websiteUrl = v; return *this; }

    const std::optional<QString>& description() const { return _description; }
    const std::optional<QList<Icon>>& icons() const { return _icons; }
    const QString& name() const { return _name; }
    const std::optional<QString>& title() const { return _title; }
    const QString& version() const { return _version; }
    const std::optional<QString>& websiteUrl() const { return _websiteUrl; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Implementation> fromJson<Implementation>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Implementation &data);

/** Parameters for an `initialize` request. */
struct InitializeRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    ClientCapabilities _capabilities;
    Implementation _clientInfo;
    QString _protocolVersion;  //!< The latest version of the Model Context Protocol that the client supports. The client MAY decide to support older versions as well.

    InitializeRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    InitializeRequestParams& capabilities(const ClientCapabilities & v) { _capabilities = v; return *this; }
    InitializeRequestParams& clientInfo(const Implementation & v) { _clientInfo = v; return *this; }
    InitializeRequestParams& protocolVersion(const QString & v) { _protocolVersion = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const ClientCapabilities& capabilities() const { return _capabilities; }
    const Implementation& clientInfo() const { return _clientInfo; }
    const QString& protocolVersion() const { return _protocolVersion; }
};

template<>
MCPSERVER_EXPORT Utils::Result<InitializeRequestParams::Meta> fromJson<InitializeRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InitializeRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<InitializeRequestParams> fromJson<InitializeRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InitializeRequestParams &data);

/**
 * This request is sent from the client to the server when it first connects, asking it to begin initialization.
 */
struct InitializeRequest {
    RequestId _id;
    InitializeRequestParams _params;

    InitializeRequest& id(const RequestId & v) { _id = v; return *this; }
    InitializeRequest& params(const InitializeRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const InitializeRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<InitializeRequest> fromJson<InitializeRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InitializeRequest &data);

/** Common parameters for paginated requests. */
struct PaginatedRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * An opaque token representing the current pagination position.
     * If provided, the server should return results starting after this cursor.
     */
    std::optional<QString> _cursor;

    PaginatedRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    PaginatedRequestParams& cursor(const std::optional<QString> & v) { _cursor = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const std::optional<QString>& cursor() const { return _cursor; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PaginatedRequestParams::Meta> fromJson<PaginatedRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PaginatedRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<PaginatedRequestParams> fromJson<PaginatedRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PaginatedRequestParams &data);

/** Sent from the client to request a list of prompts and prompt templates the server has. */
struct ListPromptsRequest {
    RequestId _id;
    std::optional<PaginatedRequestParams> _params;

    ListPromptsRequest& id(const RequestId & v) { _id = v; return *this; }
    ListPromptsRequest& params(const std::optional<PaginatedRequestParams> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const std::optional<PaginatedRequestParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListPromptsRequest> fromJson<ListPromptsRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListPromptsRequest &data);

/** Sent from the client to request a list of resource templates the server has. */
struct ListResourceTemplatesRequest {
    RequestId _id;
    std::optional<PaginatedRequestParams> _params;

    ListResourceTemplatesRequest& id(const RequestId & v) { _id = v; return *this; }
    ListResourceTemplatesRequest& params(const std::optional<PaginatedRequestParams> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const std::optional<PaginatedRequestParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListResourceTemplatesRequest> fromJson<ListResourceTemplatesRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListResourceTemplatesRequest &data);

/** Sent from the client to request a list of resources the server has. */
struct ListResourcesRequest {
    RequestId _id;
    std::optional<PaginatedRequestParams> _params;

    ListResourcesRequest& id(const RequestId & v) { _id = v; return *this; }
    ListResourcesRequest& params(const std::optional<PaginatedRequestParams> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const std::optional<PaginatedRequestParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListResourcesRequest> fromJson<ListResourcesRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListResourcesRequest &data);

/** A request to retrieve a list of tasks. */
struct ListTasksRequest {
    RequestId _id;
    std::optional<PaginatedRequestParams> _params;

    ListTasksRequest& id(const RequestId & v) { _id = v; return *this; }
    ListTasksRequest& params(const std::optional<PaginatedRequestParams> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const std::optional<PaginatedRequestParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListTasksRequest> fromJson<ListTasksRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListTasksRequest &data);

/** Sent from the client to request a list of tools the server has. */
struct ListToolsRequest {
    RequestId _id;
    std::optional<PaginatedRequestParams> _params;

    ListToolsRequest& id(const RequestId & v) { _id = v; return *this; }
    ListToolsRequest& params(const std::optional<PaginatedRequestParams> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const std::optional<PaginatedRequestParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListToolsRequest> fromJson<ListToolsRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListToolsRequest &data);

/** Common params for any request. */
struct RequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.

    RequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
};

template<>
MCPSERVER_EXPORT Utils::Result<RequestParams::Meta> fromJson<RequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const RequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<RequestParams> fromJson<RequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const RequestParams &data);

/**
 * A ping, issued by either the server or the client, to check that the other party is still alive. The receiver must promptly respond, or else may be disconnected.
 */
struct PingRequest {
    RequestId _id;
    std::optional<RequestParams> _params;

    PingRequest& id(const RequestId & v) { _id = v; return *this; }
    PingRequest& params(const std::optional<RequestParams> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const std::optional<RequestParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PingRequest> fromJson<PingRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PingRequest &data);

/** Parameters for a `resources/read` request. */
struct ReadResourceRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _uri;  //!< The URI of the resource. The URI can use any protocol; it is up to the server how to interpret it.

    ReadResourceRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    ReadResourceRequestParams& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ReadResourceRequestParams::Meta> fromJson<ReadResourceRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ReadResourceRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<ReadResourceRequestParams> fromJson<ReadResourceRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ReadResourceRequestParams &data);

/** Sent from the client to the server, to read a specific resource URI. */
struct ReadResourceRequest {
    RequestId _id;
    ReadResourceRequestParams _params;

    ReadResourceRequest& id(const RequestId & v) { _id = v; return *this; }
    ReadResourceRequest& params(const ReadResourceRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const ReadResourceRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ReadResourceRequest> fromJson<ReadResourceRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ReadResourceRequest &data);

/**
 * The severity of a log message.
 *
 * These map to syslog message severities, as specified in RFC-5424:
 * https://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1
 */
enum class LoggingLevel {
    alert,
    critical,
    debug,
    emergency,
    error,
    info,
    notice,
    warning
};

MCPSERVER_EXPORT QString toString(LoggingLevel v);

template<>
MCPSERVER_EXPORT Utils::Result<LoggingLevel> fromJson<LoggingLevel>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const LoggingLevel &v);

/** Parameters for a `logging/setLevel` request. */
struct SetLevelRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    LoggingLevel _level;  //!< The level of logging that the client wants to receive from the server. The server should send all logs at this level and higher (i.e., more severe) to the client as notifications/message.

    SetLevelRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    SetLevelRequestParams& level(const LoggingLevel & v) { _level = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const LoggingLevel& level() const { return _level; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SetLevelRequestParams::Meta> fromJson<SetLevelRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SetLevelRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<SetLevelRequestParams> fromJson<SetLevelRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SetLevelRequestParams &data);

/** A request from the client to the server, to enable or adjust logging. */
struct SetLevelRequest {
    RequestId _id;
    SetLevelRequestParams _params;

    SetLevelRequest& id(const RequestId & v) { _id = v; return *this; }
    SetLevelRequest& params(const SetLevelRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const SetLevelRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SetLevelRequest> fromJson<SetLevelRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SetLevelRequest &data);

/** Parameters for a `resources/subscribe` request. */
struct SubscribeRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _uri;  //!< The URI of the resource. The URI can use any protocol; it is up to the server how to interpret it.

    SubscribeRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    SubscribeRequestParams& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SubscribeRequestParams::Meta> fromJson<SubscribeRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscribeRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<SubscribeRequestParams> fromJson<SubscribeRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscribeRequestParams &data);

/**
 * Sent from the client to request resources/updated notifications from the server whenever a particular resource changes.
 */
struct SubscribeRequest {
    RequestId _id;
    SubscribeRequestParams _params;

    SubscribeRequest& id(const RequestId & v) { _id = v; return *this; }
    SubscribeRequest& params(const SubscribeRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const SubscribeRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SubscribeRequest> fromJson<SubscribeRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscribeRequest &data);

/** Parameters for a `resources/unsubscribe` request. */
struct UnsubscribeRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _uri;  //!< The URI of the resource. The URI can use any protocol; it is up to the server how to interpret it.

    UnsubscribeRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    UnsubscribeRequestParams& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<UnsubscribeRequestParams::Meta> fromJson<UnsubscribeRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const UnsubscribeRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<UnsubscribeRequestParams> fromJson<UnsubscribeRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const UnsubscribeRequestParams &data);

/**
 * Sent from the client to request cancellation of resources/updated notifications from the server. This should follow a previous resources/subscribe request.
 */
struct UnsubscribeRequest {
    RequestId _id;
    UnsubscribeRequestParams _params;

    UnsubscribeRequest& id(const RequestId & v) { _id = v; return *this; }
    UnsubscribeRequest& params(const UnsubscribeRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const UnsubscribeRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<UnsubscribeRequest> fromJson<UnsubscribeRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const UnsubscribeRequest &data);

using ClientRequest = std::variant<InitializeRequest, PingRequest, ListResourcesRequest, ListResourceTemplatesRequest, ReadResourceRequest, SubscribeRequest, UnsubscribeRequest, ListPromptsRequest, GetPromptRequest, ListToolsRequest, CallToolRequest, GetTaskRequest, GetTaskPayloadRequest, CancelTaskRequest, ListTasksRequest, SetLevelRequest, CompleteRequest>;

template<>
MCPSERVER_EXPORT Utils::Result<ClientRequest> fromJson<ClientRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientRequest &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ClientRequest &val);

/** Returns the 'method' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const ClientRequest &val);

/** Returns the 'id' field from the active variant. */
MCPSERVER_EXPORT RequestId id(const ClientRequest &val);

/** The result of a tool use, provided by the user back to the assistant. */
struct ToolResultContent {
    /**
     * Optional metadata about the tool result. Clients SHOULD preserve this field when
     * including tool results in subsequent sampling requests to enable caching optimizations.
     *
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    std::optional<QMap<QString, QJsonValue>> __meta;
    /**
     * The unstructured result content of the tool use.
     *
     * This has the same format as CallToolResult.content and can include text, images,
     * audio, resource links, and embedded resources.
     */
    QList<ContentBlock> _content;
    /**
     * Whether the tool use resulted in an error.
     *
     * If true, the content typically describes the error that occurred.
     * Default: false
     */
    std::optional<bool> _isError;
    /**
     * An optional structured result object.
     *
     * If the tool defined an outputSchema, this SHOULD conform to that schema.
     */
    std::optional<QMap<QString, QJsonValue>> _structuredContent;
    /**
     * The ID of the tool use this result corresponds to.
     *
     * This MUST match the ID from a previous ToolUseContent.
     */
    QString _toolUseId;

    ToolResultContent& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ToolResultContent& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ToolResultContent& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ToolResultContent& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    ToolResultContent& addContent(const ContentBlock & v) { _content.append(v); return *this; }
    ToolResultContent& isError(std::optional<bool> v) { _isError = v; return *this; }
    ToolResultContent& structuredContent(const std::optional<QMap<QString, QJsonValue>> & v) { _structuredContent = v; return *this; }
    ToolResultContent& addStructuredContent(const QString &key, const QJsonValue &v) { if (!_structuredContent) _structuredContent = QMap<QString, QJsonValue>{}; (*_structuredContent)[key] = v; return *this; }
    ToolResultContent& structuredContent(const QJsonObject &obj) { if (!_structuredContent) _structuredContent = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_structuredContent)[it.key()] = it.value(); return *this; }
    ToolResultContent& toolUseId(const QString & v) { _toolUseId = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QList<ContentBlock>& content() const { return _content; }
    const std::optional<bool>& isError() const { return _isError; }
    const std::optional<QMap<QString, QJsonValue>>& structuredContent() const { return _structuredContent; }
    QJsonObject structuredContentAsObject() const { if (!_structuredContent) return {}; QJsonObject o; for (auto it = _structuredContent->constBegin(); it != _structuredContent->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QString& toolUseId() const { return _toolUseId; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ToolResultContent> fromJson<ToolResultContent>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ToolResultContent &data);

/** A request from the assistant to call a tool. */
struct ToolUseContent {
    /**
     * Optional metadata about the tool use. Clients SHOULD preserve this field when
     * including tool uses in subsequent sampling requests to enable caching optimizations.
     *
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    std::optional<QMap<QString, QJsonValue>> __meta;
    /**
     * A unique identifier for this tool use.
     *
     * This ID is used to match tool results to their corresponding tool uses.
     */
    QString _id;
    QMap<QString, QJsonValue> _input;  //!< The arguments to pass to the tool, conforming to the tool's input schema.
    QString _name;  //!< The name of the tool to call.

    ToolUseContent& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ToolUseContent& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ToolUseContent& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ToolUseContent& id(const QString & v) { _id = v; return *this; }
    ToolUseContent& input(const QMap<QString, QJsonValue> & v) { _input = v; return *this; }
    ToolUseContent& addInput(const QString &key, const QJsonValue &v) { _input[key] = v; return *this; }
    ToolUseContent& input(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _input[it.key()] = it.value(); return *this; }
    ToolUseContent& name(const QString & v) { _name = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QString& id() const { return _id; }
    const QMap<QString, QJsonValue>& input() const { return _input; }
    QJsonObject inputAsObject() const { QJsonObject o; for (auto it = _input.constBegin(); it != _input.constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QString& name() const { return _name; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ToolUseContent> fromJson<ToolUseContent>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ToolUseContent &data);

using SamplingMessageContentBlock = std::variant<TextContent, ImageContent, AudioContent, ToolUseContent, ToolResultContent>;

template<>
MCPSERVER_EXPORT Utils::Result<SamplingMessageContentBlock> fromJson<SamplingMessageContentBlock>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SamplingMessageContentBlock &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const SamplingMessageContentBlock &val);

/** Returns the 'type' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const SamplingMessageContentBlock &val);

using CreateMessageResultContent = std::variant<TextContent, ImageContent, AudioContent, ToolUseContent, ToolResultContent, QList<SamplingMessageContentBlock>>;

template<>
MCPSERVER_EXPORT Utils::Result<CreateMessageResultContent> fromJson<CreateMessageResultContent>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const CreateMessageResultContent &val);

/**
 * The client's response to a sampling/createMessage request from the server.
 * The client should inform the user before returning the sampled message, to allow them
 * to inspect the response (human in the loop) and decide whether to allow the server to see it.
 */
struct CreateMessageResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    CreateMessageResultContent _content;
    QString _model;  //!< The name of the model that generated the message.
    Role _role;
    /**
     * The reason why sampling stopped, if known.
     *
     * Standard values:
     * - "endTurn": Natural end of the assistant's turn
     * - "stopSequence": A stop sequence was encountered
     * - "maxTokens": Maximum token limit was reached
     * - "toolUse": The model wants to use one or more tools
     *
     * This field is an open string to allow for provider-specific stop reasons.
     */
    std::optional<QString> _stopReason;

    CreateMessageResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    CreateMessageResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    CreateMessageResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    CreateMessageResult& content(const CreateMessageResultContent & v) { _content = v; return *this; }
    CreateMessageResult& model(const QString & v) { _model = v; return *this; }
    CreateMessageResult& role(const Role & v) { _role = v; return *this; }
    CreateMessageResult& stopReason(const std::optional<QString> & v) { _stopReason = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const CreateMessageResultContent& content() const { return _content; }
    const QString& model() const { return _model; }
    const Role& role() const { return _role; }
    const std::optional<QString>& stopReason() const { return _stopReason; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CreateMessageResult> fromJson<CreateMessageResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CreateMessageResult &data);

using ElicitResultContentValue = std::variant<QStringList, QString, int, bool>;

template<>
MCPSERVER_EXPORT Utils::Result<ElicitResultContentValue> fromJson<ElicitResultContentValue>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ElicitResultContentValue &val);

/** The client's response to an elicitation request. */
struct ElicitResult {
    /**
     * The user action in response to the elicitation.
     * - "accept": User submitted the form/confirmed the action
     * - "decline": User explicitly decline the action
     * - "cancel": User dismissed without making an explicit choice
     */
    enum class Action {
        accept,
        cancel,
        decline
    };

    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    Action _action;
    /**
     * The submitted form data, only present when action is "accept" and mode was "form".
     * Contains values matching the requested schema.
     * Omitted for out-of-band mode responses.
     */
    std::optional<QMap<QString, ElicitResultContentValue>> _content;

    ElicitResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ElicitResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ElicitResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ElicitResult& action(const Action & v) { _action = v; return *this; }
    ElicitResult& content(const std::optional<QMap<QString, ElicitResultContentValue>> & v) { _content = v; return *this; }
    ElicitResult& addContent(const QString &key, const ElicitResultContentValue & v) { if (!_content) _content = QMap<QString, ElicitResultContentValue>{}; (*_content)[key] = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const Action& action() const { return _action; }
    const std::optional<QMap<QString, ElicitResultContentValue>>& content() const { return _content; }
};

MCPSERVER_EXPORT QString toString(const ElicitResult::Action &v);

template<>
MCPSERVER_EXPORT Utils::Result<ElicitResult::Action> fromJson<ElicitResult::Action>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ElicitResult::Action &v);

template<>
MCPSERVER_EXPORT Utils::Result<ElicitResult> fromJson<ElicitResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitResult &data);

/**
 * The response to a tasks/result request.
 * The structure matches the result type of the original request.
 * For example, a tools/call task would return the CallToolResult structure.
 */
struct GetTaskPayloadResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QJsonObject _additionalProperties;  //!< additional properties

    GetTaskPayloadResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    GetTaskPayloadResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    GetTaskPayloadResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    GetTaskPayloadResult& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    GetTaskPayloadResult& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetTaskPayloadResult> fromJson<GetTaskPayloadResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetTaskPayloadResult &data);

/** The response to a tasks/get request. */
struct GetTaskResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _createdAt;  //!< ISO 8601 timestamp when the task was created.
    QString _lastUpdatedAt;  //!< ISO 8601 timestamp when the task was last updated.
    std::optional<int> _pollInterval;  //!< Suggested polling interval in milliseconds.
    TaskStatus _status;  //!< Current task state.
    /**
     * Optional human-readable message describing the current task state.
     * This can provide context for any status, including:
     * - Reasons for "cancelled" status
     * - Summaries for "completed" status
     * - Diagnostic information for "failed" status (e.g., error details, what went wrong)
     */
    std::optional<QString> _statusMessage;
    QString _taskId;  //!< The task identifier.
    std::optional<int> _ttl;  //!< Actual retention duration from creation in milliseconds, null for unlimited.

    GetTaskResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    GetTaskResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    GetTaskResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    GetTaskResult& createdAt(const QString & v) { _createdAt = v; return *this; }
    GetTaskResult& lastUpdatedAt(const QString & v) { _lastUpdatedAt = v; return *this; }
    GetTaskResult& pollInterval(std::optional<int> v) { _pollInterval = v; return *this; }
    GetTaskResult& status(const TaskStatus & v) { _status = v; return *this; }
    GetTaskResult& statusMessage(const std::optional<QString> & v) { _statusMessage = v; return *this; }
    GetTaskResult& taskId(const QString & v) { _taskId = v; return *this; }
    GetTaskResult& ttl(std::optional<int> v) { _ttl = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QString& createdAt() const { return _createdAt; }
    const QString& lastUpdatedAt() const { return _lastUpdatedAt; }
    const std::optional<int>& pollInterval() const { return _pollInterval; }
    const TaskStatus& status() const { return _status; }
    const std::optional<QString>& statusMessage() const { return _statusMessage; }
    const QString& taskId() const { return _taskId; }
    const std::optional<int>& ttl() const { return _ttl; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetTaskResult> fromJson<GetTaskResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetTaskResult &data);

/** Represents a root directory or file that the server can operate on. */
struct Root {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * An optional name for the root. This can be used to provide a human-readable
     * identifier for the root, which may be useful for display purposes or for
     * referencing the root in other parts of the application.
     */
    std::optional<QString> _name;
    /**
     * The URI identifying the root. This *must* start with file:// for now.
     * This restriction may be relaxed in future versions of the protocol to allow
     * other URI schemes.
     */
    QString _uri;

    Root& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    Root& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    Root& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    Root& name(const std::optional<QString> & v) { _name = v; return *this; }
    Root& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& name() const { return _name; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Root> fromJson<Root>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Root &data);

/**
 * The client's response to a roots/list request from the server.
 * This result contains an array of Root objects, each representing a root directory
 * or file that the server can operate on.
 */
struct ListRootsResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QList<Root> _roots;

    ListRootsResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ListRootsResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ListRootsResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ListRootsResult& roots(const QList<Root> & v) { _roots = v; return *this; }
    ListRootsResult& addRoot(const Root & v) { _roots.append(v); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QList<Root>& roots() const { return _roots; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListRootsResult> fromJson<ListRootsResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListRootsResult &data);

/** The response to a tasks/list request. */
struct ListTasksResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor;
    QList<Task> _tasks;

    ListTasksResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ListTasksResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ListTasksResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ListTasksResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    ListTasksResult& tasks(const QList<Task> & v) { _tasks = v; return *this; }
    ListTasksResult& addTask(const Task & v) { _tasks.append(v); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const QList<Task>& tasks() const { return _tasks; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListTasksResult> fromJson<ListTasksResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListTasksResult &data);

using ClientResult = std::variant<Result, GetTaskResult, GetTaskPayloadResult, CancelTaskResult, ListTasksResult, CreateMessageResult, ListRootsResult, ElicitResult>;

template<>
MCPSERVER_EXPORT Utils::Result<ClientResult> fromJson<ClientResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientResult &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ClientResult &val);

/** The server's response to a completion/complete request */
struct CompleteResult {
    struct Completion {
        std::optional<bool> _hasMore;  //!< Indicates whether there are additional completion options beyond those provided in the current response, even if the exact total is unknown.
        std::optional<int> _total;  //!< The total number of completion options available. This can exceed the number of values actually sent in the response.
        QStringList _values;  //!< An array of completion values. Must not exceed 100 items.

        Completion& hasMore(std::optional<bool> v) { _hasMore = v; return *this; }
        Completion& total(std::optional<int> v) { _total = v; return *this; }
        Completion& values(const QStringList & v) { _values = v; return *this; }
        Completion& addValue(const QString & v) { _values.append(v); return *this; }

        const std::optional<bool>& hasMore() const { return _hasMore; }
        const std::optional<int>& total() const { return _total; }
        const QStringList& values() const { return _values; }
    };

    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    Completion _completion;

    CompleteResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    CompleteResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    CompleteResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    CompleteResult& completion(const Completion & v) { _completion = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const Completion& completion() const { return _completion; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CompleteResult::Completion> fromJson<CompleteResult::Completion>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteResult::Completion &data);

template<>
MCPSERVER_EXPORT Utils::Result<CompleteResult> fromJson<CompleteResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteResult &data);

/**
 * Hints to use for model selection.
 *
 * Keys not declared here are currently left unspecified by the spec and are up
 * to the client to interpret.
 */
struct ModelHint {
    /**
     * A hint for a model name.
     *
     * The client SHOULD treat this as a substring of a model name; for example:
     * - `claude-3-5-sonnet` should match `claude-3-5-sonnet-20241022`
     * - `sonnet` should match `claude-3-5-sonnet-20241022`, `claude-3-sonnet-20240229`, etc.
     * - `claude` should match any Claude model
     *
     * The client MAY also map the string to a different provider's model name or a different model family, as long as it fills a similar niche; for example:
     * - `gemini-1.5-flash` could match `claude-3-haiku-20240307`
     */
    std::optional<QString> _name;

    ModelHint& name(const std::optional<QString> & v) { _name = v; return *this; }

    const std::optional<QString>& name() const { return _name; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ModelHint> fromJson<ModelHint>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ModelHint &data);

/**
 * The server's preferences for model selection, requested of the client during sampling.
 *
 * Because LLMs can vary along multiple dimensions, choosing the "best" model is
 * rarely straightforward.  Different models excel in different areas—some are
 * faster but less capable, others are more capable but more expensive, and so
 * on. This interface allows servers to express their priorities across multiple
 * dimensions to help clients make an appropriate selection for their use case.
 *
 * These preferences are always advisory. The client MAY ignore them. It is also
 * up to the client to decide how to interpret these preferences and how to
 * balance them against other considerations.
 */
struct ModelPreferences {
    /**
     * How much to prioritize cost when selecting a model. A value of 0 means cost
     * is not important, while a value of 1 means cost is the most important
     * factor.
     */
    std::optional<double> _costPriority;
    /**
     * Optional hints to use for model selection.
     *
     * If multiple hints are specified, the client MUST evaluate them in order
     * (such that the first match is taken).
     *
     * The client SHOULD prioritize these hints over the numeric priorities, but
     * MAY still use the priorities to select from ambiguous matches.
     */
    std::optional<QList<ModelHint>> _hints;
    /**
     * How much to prioritize intelligence and capabilities when selecting a
     * model. A value of 0 means intelligence is not important, while a value of 1
     * means intelligence is the most important factor.
     */
    std::optional<double> _intelligencePriority;
    /**
     * How much to prioritize sampling speed (latency) when selecting a model. A
     * value of 0 means speed is not important, while a value of 1 means speed is
     * the most important factor.
     */
    std::optional<double> _speedPriority;

    ModelPreferences& costPriority(std::optional<double> v) { _costPriority = v; return *this; }
    ModelPreferences& hints(const std::optional<QList<ModelHint>> & v) { _hints = v; return *this; }
    ModelPreferences& addHint(const ModelHint & v) { if (!_hints) _hints = QList<ModelHint>{}; (*_hints).append(v); return *this; }
    ModelPreferences& intelligencePriority(std::optional<double> v) { _intelligencePriority = v; return *this; }
    ModelPreferences& speedPriority(std::optional<double> v) { _speedPriority = v; return *this; }

    const std::optional<double>& costPriority() const { return _costPriority; }
    const std::optional<QList<ModelHint>>& hints() const { return _hints; }
    const std::optional<double>& intelligencePriority() const { return _intelligencePriority; }
    const std::optional<double>& speedPriority() const { return _speedPriority; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ModelPreferences> fromJson<ModelPreferences>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ModelPreferences &data);

/** Describes a message issued to or received from an LLM API. */
struct SamplingMessage {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    CreateMessageResultContent _content;
    Role _role;

    SamplingMessage& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    SamplingMessage& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    SamplingMessage& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    SamplingMessage& content(const CreateMessageResultContent & v) { _content = v; return *this; }
    SamplingMessage& role(const Role & v) { _role = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const CreateMessageResultContent& content() const { return _content; }
    const Role& role() const { return _role; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SamplingMessage> fromJson<SamplingMessage>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SamplingMessage &data);

/**
 * Additional properties describing a Tool to clients.
 *
 * NOTE: all properties in ToolAnnotations are **hints**.
 * They are not guaranteed to provide a faithful description of
 * tool behavior (including descriptive properties like `title`).
 *
 * Clients should never make tool use decisions based on ToolAnnotations
 * received from untrusted servers.
 */
struct ToolAnnotations {
    /**
     * If true, the tool may perform destructive updates to its environment.
     * If false, the tool performs only additive updates.
     *
     * (This property is meaningful only when `readOnlyHint == false`)
     *
     * Default: true
     */
    std::optional<bool> _destructiveHint;
    /**
     * If true, calling the tool repeatedly with the same arguments
     * will have no additional effect on its environment.
     *
     * (This property is meaningful only when `readOnlyHint == false`)
     *
     * Default: false
     */
    std::optional<bool> _idempotentHint;
    /**
     * If true, this tool may interact with an "open world" of external
     * entities. If false, the tool's domain of interaction is closed.
     * For example, the world of a web search tool is open, whereas that
     * of a memory tool is not.
     *
     * Default: true
     */
    std::optional<bool> _openWorldHint;
    /**
     * If true, the tool does not modify its environment.
     *
     * Default: false
     */
    std::optional<bool> _readOnlyHint;
    std::optional<QString> _title;  //!< A human-readable title for the tool.

    ToolAnnotations& destructiveHint(std::optional<bool> v) { _destructiveHint = v; return *this; }
    ToolAnnotations& idempotentHint(std::optional<bool> v) { _idempotentHint = v; return *this; }
    ToolAnnotations& openWorldHint(std::optional<bool> v) { _openWorldHint = v; return *this; }
    ToolAnnotations& readOnlyHint(std::optional<bool> v) { _readOnlyHint = v; return *this; }
    ToolAnnotations& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<bool>& destructiveHint() const { return _destructiveHint; }
    const std::optional<bool>& idempotentHint() const { return _idempotentHint; }
    const std::optional<bool>& openWorldHint() const { return _openWorldHint; }
    const std::optional<bool>& readOnlyHint() const { return _readOnlyHint; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ToolAnnotations> fromJson<ToolAnnotations>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ToolAnnotations &data);

/** Execution-related properties for a tool. */
struct ToolExecution {
    /**
     * Indicates whether this tool supports task-augmented execution.
     * This allows clients to handle long-running operations through polling
     * the task system.
     *
     * - "forbidden": Tool does not support task-augmented execution (default when absent)
     * - "optional": Tool may support task-augmented execution
     * - "required": Tool requires task-augmented execution
     *
     * Default: "forbidden"
     */
    enum class TaskSupport {
        forbidden,
        optional,
        required
    };

    std::optional<TaskSupport> _taskSupport;

    ToolExecution& taskSupport(const std::optional<TaskSupport> & v) { _taskSupport = v; return *this; }

    const std::optional<TaskSupport>& taskSupport() const { return _taskSupport; }
};

MCPSERVER_EXPORT QString toString(const ToolExecution::TaskSupport &v);

template<>
MCPSERVER_EXPORT Utils::Result<ToolExecution::TaskSupport> fromJson<ToolExecution::TaskSupport>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ToolExecution::TaskSupport &v);

template<>
MCPSERVER_EXPORT Utils::Result<ToolExecution> fromJson<ToolExecution>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ToolExecution &data);

/** Definition for a tool the client can call. */
struct Tool {
    /** A JSON Schema object defining the expected parameters for the tool. */
    struct InputSchema {
        std::optional<QString> _dollarschema;
        std::optional<QMap<QString, QJsonObject>> _properties;
        std::optional<QStringList> _required;

        InputSchema& dollarschema(const std::optional<QString> & v) { _dollarschema = v; return *this; }
        InputSchema& properties(const std::optional<QMap<QString, QJsonObject>> & v) { _properties = v; return *this; }
        InputSchema& addProperty(const QString &key, const QJsonObject & v) { if (!_properties) _properties = QMap<QString, QJsonObject>{}; (*_properties)[key] = v; return *this; }
        InputSchema& required(const std::optional<QStringList> & v) { _required = v; return *this; }
        InputSchema& addRequired(const QString & v) { if (!_required) _required = QStringList{}; (*_required).append(v); return *this; }

        const std::optional<QString>& dollarschema() const { return _dollarschema; }
        const std::optional<QMap<QString, QJsonObject>>& properties() const { return _properties; }
        const std::optional<QStringList>& required() const { return _required; }
    };

    /**
     * An optional JSON Schema object defining the structure of the tool's output returned in
     * the structuredContent field of a CallToolResult.
     *
     * Defaults to JSON Schema 2020-12 when no explicit $schema is provided.
     * Currently restricted to type: "object" at the root level.
     */
    struct OutputSchema {
        std::optional<QString> _dollarschema;
        std::optional<QMap<QString, QJsonObject>> _properties;
        std::optional<QStringList> _required;

        OutputSchema& dollarschema(const std::optional<QString> & v) { _dollarschema = v; return *this; }
        OutputSchema& properties(const std::optional<QMap<QString, QJsonObject>> & v) { _properties = v; return *this; }
        OutputSchema& addProperty(const QString &key, const QJsonObject & v) { if (!_properties) _properties = QMap<QString, QJsonObject>{}; (*_properties)[key] = v; return *this; }
        OutputSchema& required(const std::optional<QStringList> & v) { _required = v; return *this; }
        OutputSchema& addRequired(const QString & v) { if (!_required) _required = QStringList{}; (*_required).append(v); return *this; }

        const std::optional<QString>& dollarschema() const { return _dollarschema; }
        const std::optional<QMap<QString, QJsonObject>>& properties() const { return _properties; }
        const std::optional<QStringList>& required() const { return _required; }
    };

    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * Optional additional tool information.
     *
     * Display name precedence order is: title, annotations.title, then name.
     */
    std::optional<ToolAnnotations> _annotations;
    /**
     * A human-readable description of the tool.
     *
     * This can be used by clients to improve the LLM's understanding of available tools. It can be thought of like a "hint" to the model.
     */
    std::optional<QString> _description;
    std::optional<ToolExecution> _execution;  //!< Execution-related properties for this tool.
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::optional<QList<Icon>> _icons;
    InputSchema _inputSchema;  //!< A JSON Schema object defining the expected parameters for the tool.
    QString _name;  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * An optional JSON Schema object defining the structure of the tool's output returned in
     * the structuredContent field of a CallToolResult.
     *
     * Defaults to JSON Schema 2020-12 when no explicit $schema is provided.
     * Currently restricted to type: "object" at the root level.
     */
    std::optional<OutputSchema> _outputSchema;
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for Tool,
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title;

    Tool& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    Tool& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    Tool& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    Tool& annotations(const std::optional<ToolAnnotations> & v) { _annotations = v; return *this; }
    Tool& description(const std::optional<QString> & v) { _description = v; return *this; }
    Tool& execution(const std::optional<ToolExecution> & v) { _execution = v; return *this; }
    Tool& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    Tool& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    Tool& inputSchema(const InputSchema & v) { _inputSchema = v; return *this; }
    Tool& name(const QString & v) { _name = v; return *this; }
    Tool& outputSchema(const std::optional<OutputSchema> & v) { _outputSchema = v; return *this; }
    Tool& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<ToolAnnotations>& annotations() const { return _annotations; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<ToolExecution>& execution() const { return _execution; }
    const std::optional<QList<Icon>>& icons() const { return _icons; }
    const InputSchema& inputSchema() const { return _inputSchema; }
    const QString& name() const { return _name; }
    const std::optional<OutputSchema>& outputSchema() const { return _outputSchema; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Tool::InputSchema> fromJson<Tool::InputSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Tool::InputSchema &data);

template<>
MCPSERVER_EXPORT Utils::Result<Tool::OutputSchema> fromJson<Tool::OutputSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Tool::OutputSchema &data);

template<>
MCPSERVER_EXPORT Utils::Result<Tool> fromJson<Tool>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Tool &data);

/** Controls tool selection behavior for sampling requests. */
struct ToolChoice {
    /**
     * Controls the tool use ability of the model:
     * - "auto": Model decides whether to use tools (default)
     * - "required": Model MUST use at least one tool before completing
     * - "none": Model MUST NOT use any tools
     */
    enum class Mode {
        auto_,
        none,
        required
    };

    std::optional<Mode> _mode;

    ToolChoice& mode(const std::optional<Mode> & v) { _mode = v; return *this; }

    const std::optional<Mode>& mode() const { return _mode; }
};

MCPSERVER_EXPORT QString toString(const ToolChoice::Mode &v);

template<>
MCPSERVER_EXPORT Utils::Result<ToolChoice::Mode> fromJson<ToolChoice::Mode>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ToolChoice::Mode &v);

template<>
MCPSERVER_EXPORT Utils::Result<ToolChoice> fromJson<ToolChoice>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ToolChoice &data);

/** Parameters for a `sampling/createMessage` request. */
struct CreateMessageRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    /**
     * A request to include context from one or more MCP servers (including the caller), to be attached to the prompt.
     * The client MAY ignore this request.
     *
     * Default is "none". Values "thisServer" and "allServers" are soft-deprecated. Servers SHOULD only use these values if the client
     * declares ClientCapabilities.sampling.context. These values may be removed in future spec releases.
     */
    enum class IncludeContext {
        allServers,
        none,
        thisServer
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<IncludeContext> _includeContext;
    /**
     * The requested maximum number of tokens to sample (to prevent runaway completions).
     *
     * The client MAY choose to sample fewer tokens than the requested maximum.
     */
    int _maxTokens;
    QList<SamplingMessage> _messages;
    std::optional<QMap<QString, QJsonValue>> _metadata;  //!< Optional metadata to pass through to the LLM provider. The format of this metadata is provider-specific.
    std::optional<ModelPreferences> _modelPreferences;  //!< The server's preferences for which model to select. The client MAY ignore these preferences.
    std::optional<QStringList> _stopSequences;
    std::optional<QString> _systemPrompt;  //!< An optional system prompt the server wants to use for sampling. The client MAY modify or omit this prompt.
    /**
     * If specified, the caller is requesting task-augmented execution for this request.
     * The request will return a CreateTaskResult immediately, and the actual result can be
     * retrieved later via tasks/result.
     *
     * Task augmentation is subject to capability negotiation - receivers MUST declare support
     * for task augmentation of specific request types in their capabilities.
     */
    std::optional<TaskMetadata> _task;
    std::optional<double> _temperature;
    /**
     * Controls how the model uses tools.
     * The client MUST return an error if this field is provided but ClientCapabilities.sampling.tools is not declared.
     * Default is `{ mode: "auto" }`.
     */
    std::optional<ToolChoice> _toolChoice;
    /**
     * Tools that the model may use during generation.
     * The client MUST return an error if this field is provided but ClientCapabilities.sampling.tools is not declared.
     */
    std::optional<QList<Tool>> _tools;

    CreateMessageRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    CreateMessageRequestParams& includeContext(const std::optional<IncludeContext> & v) { _includeContext = v; return *this; }
    CreateMessageRequestParams& maxTokens(int v) { _maxTokens = v; return *this; }
    CreateMessageRequestParams& messages(const QList<SamplingMessage> & v) { _messages = v; return *this; }
    CreateMessageRequestParams& addMessage(const SamplingMessage & v) { _messages.append(v); return *this; }
    CreateMessageRequestParams& metadata(const std::optional<QMap<QString, QJsonValue>> & v) { _metadata = v; return *this; }
    CreateMessageRequestParams& addMetadata(const QString &key, const QJsonValue &v) { if (!_metadata) _metadata = QMap<QString, QJsonValue>{}; (*_metadata)[key] = v; return *this; }
    CreateMessageRequestParams& metadata(const QJsonObject &obj) { if (!_metadata) _metadata = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_metadata)[it.key()] = it.value(); return *this; }
    CreateMessageRequestParams& modelPreferences(const std::optional<ModelPreferences> & v) { _modelPreferences = v; return *this; }
    CreateMessageRequestParams& stopSequences(const std::optional<QStringList> & v) { _stopSequences = v; return *this; }
    CreateMessageRequestParams& addStopSequence(const QString & v) { if (!_stopSequences) _stopSequences = QStringList{}; (*_stopSequences).append(v); return *this; }
    CreateMessageRequestParams& systemPrompt(const std::optional<QString> & v) { _systemPrompt = v; return *this; }
    CreateMessageRequestParams& task(const std::optional<TaskMetadata> & v) { _task = v; return *this; }
    CreateMessageRequestParams& temperature(std::optional<double> v) { _temperature = v; return *this; }
    CreateMessageRequestParams& toolChoice(const std::optional<ToolChoice> & v) { _toolChoice = v; return *this; }
    CreateMessageRequestParams& tools(const std::optional<QList<Tool>> & v) { _tools = v; return *this; }
    CreateMessageRequestParams& addTool(const Tool & v) { if (!_tools) _tools = QList<Tool>{}; (*_tools).append(v); return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const std::optional<IncludeContext>& includeContext() const { return _includeContext; }
    const int& maxTokens() const { return _maxTokens; }
    const QList<SamplingMessage>& messages() const { return _messages; }
    const std::optional<QMap<QString, QJsonValue>>& metadata() const { return _metadata; }
    QJsonObject metadataAsObject() const { if (!_metadata) return {}; QJsonObject o; for (auto it = _metadata->constBegin(); it != _metadata->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<ModelPreferences>& modelPreferences() const { return _modelPreferences; }
    const std::optional<QStringList>& stopSequences() const { return _stopSequences; }
    const std::optional<QString>& systemPrompt() const { return _systemPrompt; }
    const std::optional<TaskMetadata>& task() const { return _task; }
    const std::optional<double>& temperature() const { return _temperature; }
    const std::optional<ToolChoice>& toolChoice() const { return _toolChoice; }
    const std::optional<QList<Tool>>& tools() const { return _tools; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CreateMessageRequestParams::Meta> fromJson<CreateMessageRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CreateMessageRequestParams::Meta &data);

MCPSERVER_EXPORT QString toString(const CreateMessageRequestParams::IncludeContext &v);

template<>
MCPSERVER_EXPORT Utils::Result<CreateMessageRequestParams::IncludeContext> fromJson<CreateMessageRequestParams::IncludeContext>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const CreateMessageRequestParams::IncludeContext &v);

template<>
MCPSERVER_EXPORT Utils::Result<CreateMessageRequestParams> fromJson<CreateMessageRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CreateMessageRequestParams &data);

/**
 * A request from the server to sample an LLM via the client. The client has full discretion over which model to select. The client should also inform the user before beginning sampling, to allow them to inspect the request (human in the loop) and decide whether to approve it.
 */
struct CreateMessageRequest {
    RequestId _id;
    CreateMessageRequestParams _params;

    CreateMessageRequest& id(const RequestId & v) { _id = v; return *this; }
    CreateMessageRequest& params(const CreateMessageRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const CreateMessageRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CreateMessageRequest> fromJson<CreateMessageRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CreateMessageRequest &data);

/** A response to a task-augmented request. */
struct CreateTaskResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    Task _task;

    CreateTaskResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    CreateTaskResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    CreateTaskResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    CreateTaskResult& task(const Task & v) { _task = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const Task& task() const { return _task; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CreateTaskResult> fromJson<CreateTaskResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CreateTaskResult &data);

using Cursor = QString;
template<>
MCPSERVER_EXPORT Utils::Result<Cursor> fromJson<Cursor>(const QJsonValue &val);

/**
 * Use TitledSingleSelectEnumSchema instead.
 * This interface will be removed in a future version.
 */
struct LegacyTitledEnumSchema {
    std::optional<QString> _default_;
    std::optional<QString> _description;
    QStringList _enum_;
    /**
     * (Legacy) Display names for enum values.
     * Non-standard according to JSON schema 2020-12.
     */
    std::optional<QStringList> _enumNames;
    std::optional<QString> _title;

    LegacyTitledEnumSchema& default_(const std::optional<QString> & v) { _default_ = v; return *this; }
    LegacyTitledEnumSchema& description(const std::optional<QString> & v) { _description = v; return *this; }
    LegacyTitledEnumSchema& enum_(const QStringList & v) { _enum_ = v; return *this; }
    LegacyTitledEnumSchema& addEnum(const QString & v) { _enum_.append(v); return *this; }
    LegacyTitledEnumSchema& enumNames(const std::optional<QStringList> & v) { _enumNames = v; return *this; }
    LegacyTitledEnumSchema& addEnumName(const QString & v) { if (!_enumNames) _enumNames = QStringList{}; (*_enumNames).append(v); return *this; }
    LegacyTitledEnumSchema& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<QString>& default_() const { return _default_; }
    const std::optional<QString>& description() const { return _description; }
    const QStringList& enum_() const { return _enum_; }
    const std::optional<QStringList>& enumNames() const { return _enumNames; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<LegacyTitledEnumSchema> fromJson<LegacyTitledEnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const LegacyTitledEnumSchema &data);

struct NumberSchema {
    enum class Type {
        integer,
        number
    };

    std::optional<int> _default_;
    std::optional<QString> _description;
    std::optional<int> _maximum;
    std::optional<int> _minimum;
    std::optional<QString> _title;
    Type _type;

    NumberSchema& default_(std::optional<int> v) { _default_ = v; return *this; }
    NumberSchema& description(const std::optional<QString> & v) { _description = v; return *this; }
    NumberSchema& maximum(std::optional<int> v) { _maximum = v; return *this; }
    NumberSchema& minimum(std::optional<int> v) { _minimum = v; return *this; }
    NumberSchema& title(const std::optional<QString> & v) { _title = v; return *this; }
    NumberSchema& type(const Type & v) { _type = v; return *this; }

    const std::optional<int>& default_() const { return _default_; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<int>& maximum() const { return _maximum; }
    const std::optional<int>& minimum() const { return _minimum; }
    const std::optional<QString>& title() const { return _title; }
    const Type& type() const { return _type; }
};

MCPSERVER_EXPORT QString toString(const NumberSchema::Type &v);

template<>
MCPSERVER_EXPORT Utils::Result<NumberSchema::Type> fromJson<NumberSchema::Type>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const NumberSchema::Type &v);

template<>
MCPSERVER_EXPORT Utils::Result<NumberSchema> fromJson<NumberSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const NumberSchema &data);

struct StringSchema {
    enum class Format {
        date,
        dateminustime,
        email,
        uri
    };

    std::optional<QString> _default_;
    std::optional<QString> _description;
    std::optional<Format> _format;
    std::optional<int> _maxLength;
    std::optional<int> _minLength;
    std::optional<QString> _title;

    StringSchema& default_(const std::optional<QString> & v) { _default_ = v; return *this; }
    StringSchema& description(const std::optional<QString> & v) { _description = v; return *this; }
    StringSchema& format(const std::optional<Format> & v) { _format = v; return *this; }
    StringSchema& maxLength(std::optional<int> v) { _maxLength = v; return *this; }
    StringSchema& minLength(std::optional<int> v) { _minLength = v; return *this; }
    StringSchema& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<QString>& default_() const { return _default_; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<Format>& format() const { return _format; }
    const std::optional<int>& maxLength() const { return _maxLength; }
    const std::optional<int>& minLength() const { return _minLength; }
    const std::optional<QString>& title() const { return _title; }
};

MCPSERVER_EXPORT QString toString(const StringSchema::Format &v);

template<>
MCPSERVER_EXPORT Utils::Result<StringSchema::Format> fromJson<StringSchema::Format>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const StringSchema::Format &v);

template<>
MCPSERVER_EXPORT Utils::Result<StringSchema> fromJson<StringSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const StringSchema &data);

/** Schema for multiple-selection enumeration with display titles for each option. */
struct TitledMultiSelectEnumSchema {
    /** Schema for array items with enum options and display labels. */
    struct Items {
        struct AnyOfItem {
            QString _const_;  //!< The constant enum value.
            QString _title;  //!< Display title for this option.

            AnyOfItem& const_(const QString & v) { _const_ = v; return *this; }
            AnyOfItem& title(const QString & v) { _title = v; return *this; }

            const QString& const_() const { return _const_; }
            const QString& title() const { return _title; }
        };

        QList<AnyOfItem> _anyOf;  //!< Array of enum options with values and display labels.

        Items& anyOf(const QList<AnyOfItem> & v) { _anyOf = v; return *this; }
        Items& addAnyOf(const AnyOfItem & v) { _anyOf.append(v); return *this; }

        const QList<AnyOfItem>& anyOf() const { return _anyOf; }
    };

    std::optional<QStringList> _default_;  //!< Optional default value.
    std::optional<QString> _description;  //!< Optional description for the enum field.
    Items _items;  //!< Schema for array items with enum options and display labels.
    std::optional<int> _maxItems;  //!< Maximum number of items to select.
    std::optional<int> _minItems;  //!< Minimum number of items to select.
    std::optional<QString> _title;  //!< Optional title for the enum field.

    TitledMultiSelectEnumSchema& default_(const std::optional<QStringList> & v) { _default_ = v; return *this; }
    TitledMultiSelectEnumSchema& addDefault(const QString & v) { if (!_default_) _default_ = QStringList{}; (*_default_).append(v); return *this; }
    TitledMultiSelectEnumSchema& description(const std::optional<QString> & v) { _description = v; return *this; }
    TitledMultiSelectEnumSchema& items(const Items & v) { _items = v; return *this; }
    TitledMultiSelectEnumSchema& maxItems(std::optional<int> v) { _maxItems = v; return *this; }
    TitledMultiSelectEnumSchema& minItems(std::optional<int> v) { _minItems = v; return *this; }
    TitledMultiSelectEnumSchema& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<QStringList>& default_() const { return _default_; }
    const std::optional<QString>& description() const { return _description; }
    const Items& items() const { return _items; }
    const std::optional<int>& maxItems() const { return _maxItems; }
    const std::optional<int>& minItems() const { return _minItems; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<TitledMultiSelectEnumSchema::Items::AnyOfItem> fromJson<TitledMultiSelectEnumSchema::Items::AnyOfItem>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TitledMultiSelectEnumSchema::Items::AnyOfItem &data);

template<>
MCPSERVER_EXPORT Utils::Result<TitledMultiSelectEnumSchema::Items> fromJson<TitledMultiSelectEnumSchema::Items>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TitledMultiSelectEnumSchema::Items &data);

template<>
MCPSERVER_EXPORT Utils::Result<TitledMultiSelectEnumSchema> fromJson<TitledMultiSelectEnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TitledMultiSelectEnumSchema &data);

/** Schema for single-selection enumeration with display titles for each option. */
struct TitledSingleSelectEnumSchema {
    struct OneOfItem {
        QString _const_;  //!< The enum value.
        QString _title;  //!< Display label for this option.

        OneOfItem& const_(const QString & v) { _const_ = v; return *this; }
        OneOfItem& title(const QString & v) { _title = v; return *this; }

        const QString& const_() const { return _const_; }
        const QString& title() const { return _title; }
    };

    std::optional<QString> _default_;  //!< Optional default value.
    std::optional<QString> _description;  //!< Optional description for the enum field.
    QList<OneOfItem> _oneOf;  //!< Array of enum options with values and display labels.
    std::optional<QString> _title;  //!< Optional title for the enum field.

    TitledSingleSelectEnumSchema& default_(const std::optional<QString> & v) { _default_ = v; return *this; }
    TitledSingleSelectEnumSchema& description(const std::optional<QString> & v) { _description = v; return *this; }
    TitledSingleSelectEnumSchema& oneOf(const QList<OneOfItem> & v) { _oneOf = v; return *this; }
    TitledSingleSelectEnumSchema& addOneOf(const OneOfItem & v) { _oneOf.append(v); return *this; }
    TitledSingleSelectEnumSchema& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<QString>& default_() const { return _default_; }
    const std::optional<QString>& description() const { return _description; }
    const QList<OneOfItem>& oneOf() const { return _oneOf; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<TitledSingleSelectEnumSchema::OneOfItem> fromJson<TitledSingleSelectEnumSchema::OneOfItem>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TitledSingleSelectEnumSchema::OneOfItem &data);

template<>
MCPSERVER_EXPORT Utils::Result<TitledSingleSelectEnumSchema> fromJson<TitledSingleSelectEnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TitledSingleSelectEnumSchema &data);

/** Schema for multiple-selection enumeration without display titles for options. */
struct UntitledMultiSelectEnumSchema {
    /** Schema for the array items. */
    struct Items {
        QStringList _enum_;  //!< Array of enum values to choose from.

        Items& enum_(const QStringList & v) { _enum_ = v; return *this; }
        Items& addEnum(const QString & v) { _enum_.append(v); return *this; }

        const QStringList& enum_() const { return _enum_; }
    };

    std::optional<QStringList> _default_;  //!< Optional default value.
    std::optional<QString> _description;  //!< Optional description for the enum field.
    Items _items;  //!< Schema for the array items.
    std::optional<int> _maxItems;  //!< Maximum number of items to select.
    std::optional<int> _minItems;  //!< Minimum number of items to select.
    std::optional<QString> _title;  //!< Optional title for the enum field.

    UntitledMultiSelectEnumSchema& default_(const std::optional<QStringList> & v) { _default_ = v; return *this; }
    UntitledMultiSelectEnumSchema& addDefault(const QString & v) { if (!_default_) _default_ = QStringList{}; (*_default_).append(v); return *this; }
    UntitledMultiSelectEnumSchema& description(const std::optional<QString> & v) { _description = v; return *this; }
    UntitledMultiSelectEnumSchema& items(const Items & v) { _items = v; return *this; }
    UntitledMultiSelectEnumSchema& maxItems(std::optional<int> v) { _maxItems = v; return *this; }
    UntitledMultiSelectEnumSchema& minItems(std::optional<int> v) { _minItems = v; return *this; }
    UntitledMultiSelectEnumSchema& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<QStringList>& default_() const { return _default_; }
    const std::optional<QString>& description() const { return _description; }
    const Items& items() const { return _items; }
    const std::optional<int>& maxItems() const { return _maxItems; }
    const std::optional<int>& minItems() const { return _minItems; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<UntitledMultiSelectEnumSchema::Items> fromJson<UntitledMultiSelectEnumSchema::Items>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const UntitledMultiSelectEnumSchema::Items &data);

template<>
MCPSERVER_EXPORT Utils::Result<UntitledMultiSelectEnumSchema> fromJson<UntitledMultiSelectEnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const UntitledMultiSelectEnumSchema &data);

/** Schema for single-selection enumeration without display titles for options. */
struct UntitledSingleSelectEnumSchema {
    std::optional<QString> _default_;  //!< Optional default value.
    std::optional<QString> _description;  //!< Optional description for the enum field.
    QStringList _enum_;  //!< Array of enum values to choose from.
    std::optional<QString> _title;  //!< Optional title for the enum field.

    UntitledSingleSelectEnumSchema& default_(const std::optional<QString> & v) { _default_ = v; return *this; }
    UntitledSingleSelectEnumSchema& description(const std::optional<QString> & v) { _description = v; return *this; }
    UntitledSingleSelectEnumSchema& enum_(const QStringList & v) { _enum_ = v; return *this; }
    UntitledSingleSelectEnumSchema& addEnum(const QString & v) { _enum_.append(v); return *this; }
    UntitledSingleSelectEnumSchema& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<QString>& default_() const { return _default_; }
    const std::optional<QString>& description() const { return _description; }
    const QStringList& enum_() const { return _enum_; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<UntitledSingleSelectEnumSchema> fromJson<UntitledSingleSelectEnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const UntitledSingleSelectEnumSchema &data);

/**
 * Restricted schema definitions that only allow primitive types
 * without nested objects or arrays.
 */
using PrimitiveSchemaDefinition = std::variant<StringSchema, NumberSchema, BooleanSchema, UntitledSingleSelectEnumSchema, TitledSingleSelectEnumSchema, UntitledMultiSelectEnumSchema, TitledMultiSelectEnumSchema, LegacyTitledEnumSchema>;

template<>
MCPSERVER_EXPORT Utils::Result<PrimitiveSchemaDefinition> fromJson<PrimitiveSchemaDefinition>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PrimitiveSchemaDefinition &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const PrimitiveSchemaDefinition &val);

/**
 * The parameters for a request to elicit non-sensitive information from the user via a form in the client.
 */
struct ElicitRequestFormParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    /**
     * A restricted subset of JSON Schema.
     * Only top-level properties are allowed, without nesting.
     */
    struct RequestedSchema {
        std::optional<QString> _dollarschema;
        QMap<QString, PrimitiveSchemaDefinition> _properties;
        std::optional<QStringList> _required;

        RequestedSchema& dollarschema(const std::optional<QString> & v) { _dollarschema = v; return *this; }
        RequestedSchema& properties(const QMap<QString, PrimitiveSchemaDefinition> & v) { _properties = v; return *this; }
        RequestedSchema& addProperty(const QString &key, const PrimitiveSchemaDefinition & v) { _properties[key] = v; return *this; }
        RequestedSchema& required(const std::optional<QStringList> & v) { _required = v; return *this; }
        RequestedSchema& addRequired(const QString & v) { if (!_required) _required = QStringList{}; (*_required).append(v); return *this; }

        const std::optional<QString>& dollarschema() const { return _dollarschema; }
        const QMap<QString, PrimitiveSchemaDefinition>& properties() const { return _properties; }
        const std::optional<QStringList>& required() const { return _required; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _message;  //!< The message to present to the user describing what information is being requested.
    /**
     * A restricted subset of JSON Schema.
     * Only top-level properties are allowed, without nesting.
     */
    RequestedSchema _requestedSchema;
    /**
     * If specified, the caller is requesting task-augmented execution for this request.
     * The request will return a CreateTaskResult immediately, and the actual result can be
     * retrieved later via tasks/result.
     *
     * Task augmentation is subject to capability negotiation - receivers MUST declare support
     * for task augmentation of specific request types in their capabilities.
     */
    std::optional<TaskMetadata> _task;

    ElicitRequestFormParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    ElicitRequestFormParams& message(const QString & v) { _message = v; return *this; }
    ElicitRequestFormParams& requestedSchema(const RequestedSchema & v) { _requestedSchema = v; return *this; }
    ElicitRequestFormParams& task(const std::optional<TaskMetadata> & v) { _task = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const QString& message() const { return _message; }
    const RequestedSchema& requestedSchema() const { return _requestedSchema; }
    const std::optional<TaskMetadata>& task() const { return _task; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequestFormParams::Meta> fromJson<ElicitRequestFormParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequestFormParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequestFormParams::RequestedSchema> fromJson<ElicitRequestFormParams::RequestedSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequestFormParams::RequestedSchema &data);

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequestFormParams> fromJson<ElicitRequestFormParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequestFormParams &data);

/** The parameters for a request to elicit information from the user via a URL in the client. */
struct ElicitRequestURLParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * The ID of the elicitation, which must be unique within the context of the server.
     * The client MUST treat this ID as an opaque value.
     */
    QString _elicitationId;
    QString _message;  //!< The message to present to the user explaining why the interaction is needed.
    /**
     * If specified, the caller is requesting task-augmented execution for this request.
     * The request will return a CreateTaskResult immediately, and the actual result can be
     * retrieved later via tasks/result.
     *
     * Task augmentation is subject to capability negotiation - receivers MUST declare support
     * for task augmentation of specific request types in their capabilities.
     */
    std::optional<TaskMetadata> _task;
    QString _url;  //!< The URL that the user should navigate to.

    ElicitRequestURLParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    ElicitRequestURLParams& elicitationId(const QString & v) { _elicitationId = v; return *this; }
    ElicitRequestURLParams& message(const QString & v) { _message = v; return *this; }
    ElicitRequestURLParams& task(const std::optional<TaskMetadata> & v) { _task = v; return *this; }
    ElicitRequestURLParams& url(const QString & v) { _url = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const QString& elicitationId() const { return _elicitationId; }
    const QString& message() const { return _message; }
    const std::optional<TaskMetadata>& task() const { return _task; }
    const QString& url() const { return _url; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequestURLParams::Meta> fromJson<ElicitRequestURLParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequestURLParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequestURLParams> fromJson<ElicitRequestURLParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequestURLParams &data);

/** The parameters for a request to elicit additional information from the user via the client. */
using ElicitRequestParams = std::variant<ElicitRequestURLParams, ElicitRequestFormParams>;

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequestParams> fromJson<ElicitRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequestParams &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ElicitRequestParams &val);

/** Returns the 'mode' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const ElicitRequestParams &val);

/** Returns the 'message' field from the active variant. */
MCPSERVER_EXPORT QString message(const ElicitRequestParams &val);

/** A request from the server to elicit additional information from the user via the client. */
struct ElicitRequest {
    RequestId _id;
    ElicitRequestParams _params;

    ElicitRequest& id(const RequestId & v) { _id = v; return *this; }
    ElicitRequest& params(const ElicitRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const ElicitRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequest> fromJson<ElicitRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequest &data);

/**
 * An optional notification from the server to the client, informing it of a completion of a out-of-band elicitation request.
 */
struct ElicitationCompleteNotification {
    struct Params {
        QString _elicitationId;  //!< The ID of the elicitation that completed.

        Params& elicitationId(const QString & v) { _elicitationId = v; return *this; }

        const QString& elicitationId() const { return _elicitationId; }
    };

    Params _params;

    ElicitationCompleteNotification& params(const Params & v) { _params = v; return *this; }

    const Params& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ElicitationCompleteNotification::Params> fromJson<ElicitationCompleteNotification::Params>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitationCompleteNotification::Params &data);

template<>
MCPSERVER_EXPORT Utils::Result<ElicitationCompleteNotification> fromJson<ElicitationCompleteNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitationCompleteNotification &data);

using EmptyResult = Result;

using EnumSchema = std::variant<UntitledSingleSelectEnumSchema, TitledSingleSelectEnumSchema, UntitledMultiSelectEnumSchema, TitledMultiSelectEnumSchema, LegacyTitledEnumSchema>;

template<>
MCPSERVER_EXPORT Utils::Result<EnumSchema> fromJson<EnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const EnumSchema &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const EnumSchema &val);

struct Error {
    int _code;  //!< The error type that occurred.
    std::optional<QString> _data;  //!< Additional information about the error. The value of this member is defined by the sender (e.g. detailed error information, nested errors etc.).
    QString _message;  //!< A short description of the error. The message SHOULD be limited to a concise single sentence.

    Error& code(int v) { _code = v; return *this; }
    Error& data(const std::optional<QString> & v) { _data = v; return *this; }
    Error& message(const QString & v) { _message = v; return *this; }

    const int& code() const { return _code; }
    const std::optional<QString>& data() const { return _data; }
    const QString& message() const { return _message; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Error> fromJson<Error>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Error &data);

/**
 * Describes a message returned as part of a prompt.
 *
 * This is similar to `SamplingMessage`, but also supports the embedding of
 * resources from the MCP server.
 */
struct PromptMessage {
    ContentBlock _content;
    Role _role;

    PromptMessage& content(const ContentBlock & v) { _content = v; return *this; }
    PromptMessage& role(const Role & v) { _role = v; return *this; }

    const ContentBlock& content() const { return _content; }
    const Role& role() const { return _role; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PromptMessage> fromJson<PromptMessage>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PromptMessage &data);

/** The server's response to a prompts/get request from the client. */
struct GetPromptResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<QString> _description;  //!< An optional description for the prompt.
    QList<PromptMessage> _messages;

    GetPromptResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    GetPromptResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    GetPromptResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    GetPromptResult& description(const std::optional<QString> & v) { _description = v; return *this; }
    GetPromptResult& messages(const QList<PromptMessage> & v) { _messages = v; return *this; }
    GetPromptResult& addMessage(const PromptMessage & v) { _messages.append(v); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& description() const { return _description; }
    const QList<PromptMessage>& messages() const { return _messages; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetPromptResult> fromJson<GetPromptResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetPromptResult &data);

/** Base interface to add `icons` property. */
struct Icons {
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::optional<QList<Icon>> _icons;

    Icons& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    Icons& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }

    const std::optional<QList<Icon>>& icons() const { return _icons; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Icons> fromJson<Icons>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Icons &data);

/**
 * Capabilities that a server may support. Known capabilities are defined here, in this schema, but this is not a closed set: any server can define its own, additional capabilities.
 */
struct ServerCapabilities {
    /** Present if the server offers any prompt templates. */
    struct Prompts {
        std::optional<bool> _listChanged;  //!< Whether this server supports notifications for changes to the prompt list.

        Prompts& listChanged(std::optional<bool> v) { _listChanged = v; return *this; }

        const std::optional<bool>& listChanged() const { return _listChanged; }
    };

    /** Present if the server offers any resources to read. */
    struct Resources {
        std::optional<bool> _listChanged;  //!< Whether this server supports notifications for changes to the resource list.
        std::optional<bool> _subscribe;  //!< Whether this server supports subscribing to resource updates.

        Resources& listChanged(std::optional<bool> v) { _listChanged = v; return *this; }
        Resources& subscribe(std::optional<bool> v) { _subscribe = v; return *this; }

        const std::optional<bool>& listChanged() const { return _listChanged; }
        const std::optional<bool>& subscribe() const { return _subscribe; }
    };

    /** Present if the server supports task-augmented requests. */
    struct Tasks {
        /** Specifies which request types can be augmented with tasks. */
        struct Requests {
            /** Task support for tool-related requests. */
            struct Tools {
                std::optional<QMap<QString, QJsonValue>> _call;  //!< Whether the server supports task-augmented tools/call requests.

                Tools& call(const std::optional<QMap<QString, QJsonValue>> & v) { _call = v; return *this; }
                Tools& addCall(const QString &key, const QJsonValue &v) { if (!_call) _call = QMap<QString, QJsonValue>{}; (*_call)[key] = v; return *this; }
                Tools& call(const QJsonObject &obj) { if (!_call) _call = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_call)[it.key()] = it.value(); return *this; }

                const std::optional<QMap<QString, QJsonValue>>& call() const { return _call; }
                QJsonObject callAsObject() const { if (!_call) return {}; QJsonObject o; for (auto it = _call->constBegin(); it != _call->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
            };

            std::optional<Tools> _tools;  //!< Task support for tool-related requests.

            Requests& tools(const std::optional<Tools> & v) { _tools = v; return *this; }

            const std::optional<Tools>& tools() const { return _tools; }
        };

        std::optional<QMap<QString, QJsonValue>> _cancel;  //!< Whether this server supports tasks/cancel.
        std::optional<QMap<QString, QJsonValue>> _list;  //!< Whether this server supports tasks/list.
        std::optional<Requests> _requests;  //!< Specifies which request types can be augmented with tasks.

        Tasks& cancel(const std::optional<QMap<QString, QJsonValue>> & v) { _cancel = v; return *this; }
        Tasks& addCancel(const QString &key, const QJsonValue &v) { if (!_cancel) _cancel = QMap<QString, QJsonValue>{}; (*_cancel)[key] = v; return *this; }
        Tasks& cancel(const QJsonObject &obj) { if (!_cancel) _cancel = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_cancel)[it.key()] = it.value(); return *this; }
        Tasks& list(const std::optional<QMap<QString, QJsonValue>> & v) { _list = v; return *this; }
        Tasks& addList(const QString &key, const QJsonValue &v) { if (!_list) _list = QMap<QString, QJsonValue>{}; (*_list)[key] = v; return *this; }
        Tasks& list(const QJsonObject &obj) { if (!_list) _list = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_list)[it.key()] = it.value(); return *this; }
        Tasks& requests(const std::optional<Requests> & v) { _requests = v; return *this; }

        const std::optional<QMap<QString, QJsonValue>>& cancel() const { return _cancel; }
        QJsonObject cancelAsObject() const { if (!_cancel) return {}; QJsonObject o; for (auto it = _cancel->constBegin(); it != _cancel->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
        const std::optional<QMap<QString, QJsonValue>>& list() const { return _list; }
        QJsonObject listAsObject() const { if (!_list) return {}; QJsonObject o; for (auto it = _list->constBegin(); it != _list->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
        const std::optional<Requests>& requests() const { return _requests; }
    };

    /** Present if the server offers any tools to call. */
    struct Tools {
        std::optional<bool> _listChanged;  //!< Whether this server supports notifications for changes to the tool list.

        Tools& listChanged(std::optional<bool> v) { _listChanged = v; return *this; }

        const std::optional<bool>& listChanged() const { return _listChanged; }
    };

    std::optional<QMap<QString, QJsonValue>> _completions;  //!< Present if the server supports argument autocompletion suggestions.
    std::optional<QMap<QString, QJsonObject>> _experimental;  //!< Experimental, non-standard capabilities that the server supports.
    std::optional<QMap<QString, QJsonValue>> _logging;  //!< Present if the server supports sending log messages to the client.
    std::optional<Prompts> _prompts;  //!< Present if the server offers any prompt templates.
    std::optional<Resources> _resources;  //!< Present if the server offers any resources to read.
    std::optional<Tasks> _tasks;  //!< Present if the server supports task-augmented requests.
    std::optional<Tools> _tools;  //!< Present if the server offers any tools to call.

    ServerCapabilities& completions(const std::optional<QMap<QString, QJsonValue>> & v) { _completions = v; return *this; }
    ServerCapabilities& addCompletion(const QString &key, const QJsonValue &v) { if (!_completions) _completions = QMap<QString, QJsonValue>{}; (*_completions)[key] = v; return *this; }
    ServerCapabilities& completions(const QJsonObject &obj) { if (!_completions) _completions = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_completions)[it.key()] = it.value(); return *this; }
    ServerCapabilities& experimental(const std::optional<QMap<QString, QJsonObject>> & v) { _experimental = v; return *this; }
    ServerCapabilities& addExperimental(const QString &key, const QJsonObject & v) { if (!_experimental) _experimental = QMap<QString, QJsonObject>{}; (*_experimental)[key] = v; return *this; }
    ServerCapabilities& logging(const std::optional<QMap<QString, QJsonValue>> & v) { _logging = v; return *this; }
    ServerCapabilities& addLogging(const QString &key, const QJsonValue &v) { if (!_logging) _logging = QMap<QString, QJsonValue>{}; (*_logging)[key] = v; return *this; }
    ServerCapabilities& logging(const QJsonObject &obj) { if (!_logging) _logging = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_logging)[it.key()] = it.value(); return *this; }
    ServerCapabilities& prompts(const std::optional<Prompts> & v) { _prompts = v; return *this; }
    ServerCapabilities& resources(const std::optional<Resources> & v) { _resources = v; return *this; }
    ServerCapabilities& tasks(const std::optional<Tasks> & v) { _tasks = v; return *this; }
    ServerCapabilities& tools(const std::optional<Tools> & v) { _tools = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& completions() const { return _completions; }
    QJsonObject completionsAsObject() const { if (!_completions) return {}; QJsonObject o; for (auto it = _completions->constBegin(); it != _completions->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QMap<QString, QJsonObject>>& experimental() const { return _experimental; }
    const std::optional<QMap<QString, QJsonValue>>& logging() const { return _logging; }
    QJsonObject loggingAsObject() const { if (!_logging) return {}; QJsonObject o; for (auto it = _logging->constBegin(); it != _logging->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<Prompts>& prompts() const { return _prompts; }
    const std::optional<Resources>& resources() const { return _resources; }
    const std::optional<Tasks>& tasks() const { return _tasks; }
    const std::optional<Tools>& tools() const { return _tools; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities::Prompts> fromJson<ServerCapabilities::Prompts>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities::Prompts &data);

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities::Resources> fromJson<ServerCapabilities::Resources>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities::Resources &data);

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities::Tasks::Requests::Tools> fromJson<ServerCapabilities::Tasks::Requests::Tools>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities::Tasks::Requests::Tools &data);

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities::Tasks::Requests> fromJson<ServerCapabilities::Tasks::Requests>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities::Tasks::Requests &data);

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities::Tasks> fromJson<ServerCapabilities::Tasks>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities::Tasks &data);

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities::Tools> fromJson<ServerCapabilities::Tools>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities::Tools &data);

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities> fromJson<ServerCapabilities>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities &data);

/** After receiving an initialize request from the client, the server sends this response. */
struct InitializeResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    ServerCapabilities _capabilities;
    /**
     * Instructions describing how to use the server and its features.
     *
     * This can be used by clients to improve the LLM's understanding of available tools, resources, etc. It can be thought of like a "hint" to the model. For example, this information MAY be added to the system prompt.
     */
    std::optional<QString> _instructions;
    QString _protocolVersion;  //!< The version of the Model Context Protocol that the server wants to use. This may not match the version that the client requested. If the client cannot support this version, it MUST disconnect.
    Implementation _serverInfo;

    InitializeResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    InitializeResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    InitializeResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    InitializeResult& capabilities(const ServerCapabilities & v) { _capabilities = v; return *this; }
    InitializeResult& instructions(const std::optional<QString> & v) { _instructions = v; return *this; }
    InitializeResult& protocolVersion(const QString & v) { _protocolVersion = v; return *this; }
    InitializeResult& serverInfo(const Implementation & v) { _serverInfo = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const ServerCapabilities& capabilities() const { return _capabilities; }
    const std::optional<QString>& instructions() const { return _instructions; }
    const QString& protocolVersion() const { return _protocolVersion; }
    const Implementation& serverInfo() const { return _serverInfo; }
};

template<>
MCPSERVER_EXPORT Utils::Result<InitializeResult> fromJson<InitializeResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InitializeResult &data);

/** A response to a request that indicates an error occurred. */
struct JSONRPCErrorResponse {
    Error _error;
    std::optional<RequestId> _id;

    JSONRPCErrorResponse& error(const Error & v) { _error = v; return *this; }
    JSONRPCErrorResponse& id(const std::optional<RequestId> & v) { _id = v; return *this; }

    const Error& error() const { return _error; }
    const std::optional<RequestId>& id() const { return _id; }
};

template<>
MCPSERVER_EXPORT Utils::Result<JSONRPCErrorResponse> fromJson<JSONRPCErrorResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const JSONRPCErrorResponse &data);

/** A notification which does not expect a response. */
struct JSONRPCNotification {
    QString _method;
    std::optional<QMap<QString, QJsonValue>> _params;

    JSONRPCNotification& method(const QString & v) { _method = v; return *this; }
    JSONRPCNotification& params(const std::optional<QMap<QString, QJsonValue>> & v) { _params = v; return *this; }
    JSONRPCNotification& addParam(const QString &key, const QJsonValue &v) { if (!_params) _params = QMap<QString, QJsonValue>{}; (*_params)[key] = v; return *this; }
    JSONRPCNotification& params(const QJsonObject &obj) { if (!_params) _params = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_params)[it.key()] = it.value(); return *this; }

    const QString& method() const { return _method; }
    const std::optional<QMap<QString, QJsonValue>>& params() const { return _params; }
    QJsonObject paramsAsObject() const { if (!_params) return {}; QJsonObject o; for (auto it = _params->constBegin(); it != _params->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
};

template<>
MCPSERVER_EXPORT Utils::Result<JSONRPCNotification> fromJson<JSONRPCNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const JSONRPCNotification &data);

/** A request that expects a response. */
struct JSONRPCRequest {
    RequestId _id;
    QString _method;
    std::optional<QMap<QString, QJsonValue>> _params;

    JSONRPCRequest& id(const RequestId & v) { _id = v; return *this; }
    JSONRPCRequest& method(const QString & v) { _method = v; return *this; }
    JSONRPCRequest& params(const std::optional<QMap<QString, QJsonValue>> & v) { _params = v; return *this; }
    JSONRPCRequest& addParam(const QString &key, const QJsonValue &v) { if (!_params) _params = QMap<QString, QJsonValue>{}; (*_params)[key] = v; return *this; }
    JSONRPCRequest& params(const QJsonObject &obj) { if (!_params) _params = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_params)[it.key()] = it.value(); return *this; }

    const RequestId& id() const { return _id; }
    const QString& method() const { return _method; }
    const std::optional<QMap<QString, QJsonValue>>& params() const { return _params; }
    QJsonObject paramsAsObject() const { if (!_params) return {}; QJsonObject o; for (auto it = _params->constBegin(); it != _params->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
};

template<>
MCPSERVER_EXPORT Utils::Result<JSONRPCRequest> fromJson<JSONRPCRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const JSONRPCRequest &data);

/** A successful (non-error) response to a request. */
struct JSONRPCResultResponse {
    RequestId _id;
    Result _result;

    JSONRPCResultResponse& id(const RequestId & v) { _id = v; return *this; }
    JSONRPCResultResponse& result(const Result & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const Result& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<JSONRPCResultResponse> fromJson<JSONRPCResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const JSONRPCResultResponse &data);

/** Refers to any valid JSON-RPC object that can be decoded off the wire, or encoded to be sent. */
using JSONRPCMessage = std::variant<JSONRPCRequest, JSONRPCNotification, JSONRPCResultResponse, JSONRPCErrorResponse>;

template<>
MCPSERVER_EXPORT Utils::Result<JSONRPCMessage> fromJson<JSONRPCMessage>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const JSONRPCMessage &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const JSONRPCMessage &val);

/** A response to a request, containing either the result or error. */
using JSONRPCResponse = std::variant<JSONRPCResultResponse, JSONRPCErrorResponse>;

template<>
MCPSERVER_EXPORT Utils::Result<JSONRPCResponse> fromJson<JSONRPCResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const JSONRPCResponse &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const JSONRPCResponse &val);

/** Describes an argument that a prompt can accept. */
struct PromptArgument {
    std::optional<QString> _description;  //!< A human-readable description of the argument.
    QString _name;  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    std::optional<bool> _required;  //!< Whether this argument must be provided.
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for Tool,
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title;

    PromptArgument& description(const std::optional<QString> & v) { _description = v; return *this; }
    PromptArgument& name(const QString & v) { _name = v; return *this; }
    PromptArgument& required(std::optional<bool> v) { _required = v; return *this; }
    PromptArgument& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<QString>& description() const { return _description; }
    const QString& name() const { return _name; }
    const std::optional<bool>& required() const { return _required; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PromptArgument> fromJson<PromptArgument>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PromptArgument &data);

/** A prompt or prompt template that the server offers. */
struct Prompt {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<QList<PromptArgument>> _arguments;  //!< A list of arguments to use for templating the prompt.
    std::optional<QString> _description;  //!< An optional description of what this prompt provides
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::optional<QList<Icon>> _icons;
    QString _name;  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for Tool,
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title;

    Prompt& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    Prompt& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    Prompt& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    Prompt& arguments(const std::optional<QList<PromptArgument>> & v) { _arguments = v; return *this; }
    Prompt& addArgument(const PromptArgument & v) { if (!_arguments) _arguments = QList<PromptArgument>{}; (*_arguments).append(v); return *this; }
    Prompt& description(const std::optional<QString> & v) { _description = v; return *this; }
    Prompt& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    Prompt& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    Prompt& name(const QString & v) { _name = v; return *this; }
    Prompt& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QList<PromptArgument>>& arguments() const { return _arguments; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QList<Icon>>& icons() const { return _icons; }
    const QString& name() const { return _name; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Prompt> fromJson<Prompt>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Prompt &data);

/** The server's response to a prompts/list request from the client. */
struct ListPromptsResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor;
    QList<Prompt> _prompts;

    ListPromptsResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ListPromptsResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ListPromptsResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ListPromptsResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    ListPromptsResult& prompts(const QList<Prompt> & v) { _prompts = v; return *this; }
    ListPromptsResult& addPrompt(const Prompt & v) { _prompts.append(v); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const QList<Prompt>& prompts() const { return _prompts; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListPromptsResult> fromJson<ListPromptsResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListPromptsResult &data);

/** A template description for resources available on the server. */
struct ResourceTemplate {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<Annotations> _annotations;  //!< Optional annotations for the client.
    /**
     * A description of what this template is for.
     *
     * This can be used by clients to improve the LLM's understanding of available resources. It can be thought of like a "hint" to the model.
     */
    std::optional<QString> _description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::optional<QList<Icon>> _icons;
    std::optional<QString> _mimeType;  //!< The MIME type for all resources that match this template. This should only be included if all resources matching this template have the same type.
    QString _name;  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for Tool,
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title;
    QString _uriTemplate;  //!< A URI template (according to RFC 6570) that can be used to construct resource URIs.

    ResourceTemplate& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ResourceTemplate& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ResourceTemplate& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ResourceTemplate& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    ResourceTemplate& description(const std::optional<QString> & v) { _description = v; return *this; }
    ResourceTemplate& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    ResourceTemplate& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    ResourceTemplate& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    ResourceTemplate& name(const QString & v) { _name = v; return *this; }
    ResourceTemplate& title(const std::optional<QString> & v) { _title = v; return *this; }
    ResourceTemplate& uriTemplate(const QString & v) { _uriTemplate = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QList<Icon>>& icons() const { return _icons; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& name() const { return _name; }
    const std::optional<QString>& title() const { return _title; }
    const QString& uriTemplate() const { return _uriTemplate; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceTemplate> fromJson<ResourceTemplate>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceTemplate &data);

/** The server's response to a resources/templates/list request from the client. */
struct ListResourceTemplatesResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor;
    QList<ResourceTemplate> _resourceTemplates;

    ListResourceTemplatesResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ListResourceTemplatesResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ListResourceTemplatesResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ListResourceTemplatesResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    ListResourceTemplatesResult& resourceTemplates(const QList<ResourceTemplate> & v) { _resourceTemplates = v; return *this; }
    ListResourceTemplatesResult& addResourceTemplate(const ResourceTemplate & v) { _resourceTemplates.append(v); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const QList<ResourceTemplate>& resourceTemplates() const { return _resourceTemplates; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListResourceTemplatesResult> fromJson<ListResourceTemplatesResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListResourceTemplatesResult &data);

/** A known resource that the server is capable of reading. */
struct Resource {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<Annotations> _annotations;  //!< Optional annotations for the client.
    /**
     * A description of what this resource represents.
     *
     * This can be used by clients to improve the LLM's understanding of available resources. It can be thought of like a "hint" to the model.
     */
    std::optional<QString> _description;
    /**
     * Optional set of sized icons that the client can display in a user interface.
     *
     * Clients that support rendering icons MUST support at least the following MIME types:
     * - `image/png` - PNG images (safe, universal compatibility)
     * - `image/jpeg` (and `image/jpg`) - JPEG images (safe, universal compatibility)
     *
     * Clients that support rendering icons SHOULD also support:
     * - `image/svg+xml` - SVG images (scalable but requires security precautions)
     * - `image/webp` - WebP images (modern, efficient format)
     */
    std::optional<QList<Icon>> _icons;
    std::optional<QString> _mimeType;  //!< The MIME type of this resource, if known.
    QString _name;  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * The size of the raw resource content, in bytes (i.e., before base64 encoding or any tokenization), if known.
     *
     * This can be used by Hosts to display file sizes and estimate context window usage.
     */
    std::optional<int> _size;
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for Tool,
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title;
    QString _uri;  //!< The URI of this resource.

    Resource& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    Resource& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    Resource& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    Resource& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    Resource& description(const std::optional<QString> & v) { _description = v; return *this; }
    Resource& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    Resource& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    Resource& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    Resource& name(const QString & v) { _name = v; return *this; }
    Resource& size(std::optional<int> v) { _size = v; return *this; }
    Resource& title(const std::optional<QString> & v) { _title = v; return *this; }
    Resource& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QList<Icon>>& icons() const { return _icons; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& name() const { return _name; }
    const std::optional<int>& size() const { return _size; }
    const std::optional<QString>& title() const { return _title; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Resource> fromJson<Resource>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Resource &data);

/** The server's response to a resources/list request from the client. */
struct ListResourcesResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor;
    QList<Resource> _resources;

    ListResourcesResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ListResourcesResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ListResourcesResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ListResourcesResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    ListResourcesResult& resources(const QList<Resource> & v) { _resources = v; return *this; }
    ListResourcesResult& addResource(const Resource & v) { _resources.append(v); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const QList<Resource>& resources() const { return _resources; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListResourcesResult> fromJson<ListResourcesResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListResourcesResult &data);

/**
 * Sent from the server to request a list of root URIs from the client. Roots allow
 * servers to ask for specific directories or files to operate on. A common example
 * for roots is providing a set of repositories or directories a server should operate
 * on.
 *
 * This request is typically used when the server needs to understand the file system
 * structure or access specific locations that the client has permission to read from.
 */
struct ListRootsRequest {
    RequestId _id;
    std::optional<RequestParams> _params;

    ListRootsRequest& id(const RequestId & v) { _id = v; return *this; }
    ListRootsRequest& params(const std::optional<RequestParams> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const std::optional<RequestParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListRootsRequest> fromJson<ListRootsRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListRootsRequest &data);

/** The server's response to a tools/list request from the client. */
struct ListToolsResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor;
    QList<Tool> _tools;

    ListToolsResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ListToolsResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ListToolsResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ListToolsResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    ListToolsResult& tools(const QList<Tool> & v) { _tools = v; return *this; }
    ListToolsResult& addTool(const Tool & v) { _tools.append(v); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const QList<Tool>& tools() const { return _tools; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListToolsResult> fromJson<ListToolsResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListToolsResult &data);

/** Parameters for a `notifications/message` notification. */
struct LoggingMessageNotificationParams {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _data;  //!< The data to be logged, such as a string message or an object. Any JSON serializable type is allowed here.
    LoggingLevel _level;  //!< The severity of this log message.
    std::optional<QString> _logger;  //!< An optional name of the logger issuing this message.

    LoggingMessageNotificationParams& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    LoggingMessageNotificationParams& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    LoggingMessageNotificationParams& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    LoggingMessageNotificationParams& data(const QString & v) { _data = v; return *this; }
    LoggingMessageNotificationParams& level(const LoggingLevel & v) { _level = v; return *this; }
    LoggingMessageNotificationParams& logger(const std::optional<QString> & v) { _logger = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QString& data() const { return _data; }
    const LoggingLevel& level() const { return _level; }
    const std::optional<QString>& logger() const { return _logger; }
};

template<>
MCPSERVER_EXPORT Utils::Result<LoggingMessageNotificationParams> fromJson<LoggingMessageNotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const LoggingMessageNotificationParams &data);

/**
 * JSONRPCNotification of a log message passed from server to client. If no logging/setLevel request has been sent from the client, the server MAY decide which messages to send automatically.
 */
struct LoggingMessageNotification {
    LoggingMessageNotificationParams _params;

    LoggingMessageNotification& params(const LoggingMessageNotificationParams & v) { _params = v; return *this; }

    const LoggingMessageNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<LoggingMessageNotification> fromJson<LoggingMessageNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const LoggingMessageNotification &data);

using MultiSelectEnumSchema = std::variant<UntitledMultiSelectEnumSchema, TitledMultiSelectEnumSchema>;

template<>
MCPSERVER_EXPORT Utils::Result<MultiSelectEnumSchema> fromJson<MultiSelectEnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const MultiSelectEnumSchema &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const MultiSelectEnumSchema &val);

struct Notification {
    QString _method;
    std::optional<QMap<QString, QJsonValue>> _params;

    Notification& method(const QString & v) { _method = v; return *this; }
    Notification& params(const std::optional<QMap<QString, QJsonValue>> & v) { _params = v; return *this; }
    Notification& addParam(const QString &key, const QJsonValue &v) { if (!_params) _params = QMap<QString, QJsonValue>{}; (*_params)[key] = v; return *this; }
    Notification& params(const QJsonObject &obj) { if (!_params) _params = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_params)[it.key()] = it.value(); return *this; }

    const QString& method() const { return _method; }
    const std::optional<QMap<QString, QJsonValue>>& params() const { return _params; }
    QJsonObject paramsAsObject() const { if (!_params) return {}; QJsonObject o; for (auto it = _params->constBegin(); it != _params->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Notification> fromJson<Notification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Notification &data);

struct PaginatedRequest {
    RequestId _id;
    QString _method;
    std::optional<PaginatedRequestParams> _params;

    PaginatedRequest& id(const RequestId & v) { _id = v; return *this; }
    PaginatedRequest& method(const QString & v) { _method = v; return *this; }
    PaginatedRequest& params(const std::optional<PaginatedRequestParams> & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const QString& method() const { return _method; }
    const std::optional<PaginatedRequestParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PaginatedRequest> fromJson<PaginatedRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PaginatedRequest &data);

struct PaginatedResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor;

    PaginatedResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    PaginatedResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    PaginatedResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    PaginatedResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PaginatedResult> fromJson<PaginatedResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PaginatedResult &data);

/**
 * An optional notification from the server to the client, informing it that the list of prompts it offers has changed. This may be issued by servers without any previous subscription from the client.
 */
struct PromptListChangedNotification {
    std::optional<NotificationParams> _params;

    PromptListChangedNotification& params(const std::optional<NotificationParams> & v) { _params = v; return *this; }

    const std::optional<NotificationParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PromptListChangedNotification> fromJson<PromptListChangedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PromptListChangedNotification &data);

/** The server's response to a resources/read request from the client. */
struct ReadResourceResult {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QList<EmbeddedResourceResource> _contents;

    ReadResourceResult& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ReadResourceResult& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ReadResourceResult& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ReadResourceResult& contents(const QList<EmbeddedResourceResource> & v) { _contents = v; return *this; }
    ReadResourceResult& addContent(const EmbeddedResourceResource & v) { _contents.append(v); return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QList<EmbeddedResourceResource>& contents() const { return _contents; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ReadResourceResult> fromJson<ReadResourceResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ReadResourceResult &data);

/**
 * Metadata for associating messages with a task.
 * Include this in the `_meta` field under the key `io.modelcontextprotocol/related-task`.
 */
struct RelatedTaskMetadata {
    QString _taskId;  //!< The task identifier this message is associated with.

    RelatedTaskMetadata& taskId(const QString & v) { _taskId = v; return *this; }

    const QString& taskId() const { return _taskId; }
};

template<>
MCPSERVER_EXPORT Utils::Result<RelatedTaskMetadata> fromJson<RelatedTaskMetadata>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const RelatedTaskMetadata &data);

struct Request {
    QString _method;
    std::optional<QMap<QString, QJsonValue>> _params;

    Request& method(const QString & v) { _method = v; return *this; }
    Request& params(const std::optional<QMap<QString, QJsonValue>> & v) { _params = v; return *this; }
    Request& addParam(const QString &key, const QJsonValue &v) { if (!_params) _params = QMap<QString, QJsonValue>{}; (*_params)[key] = v; return *this; }
    Request& params(const QJsonObject &obj) { if (!_params) _params = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_params)[it.key()] = it.value(); return *this; }

    const QString& method() const { return _method; }
    const std::optional<QMap<QString, QJsonValue>>& params() const { return _params; }
    QJsonObject paramsAsObject() const { if (!_params) return {}; QJsonObject o; for (auto it = _params->constBegin(); it != _params->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Request> fromJson<Request>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Request &data);

/** The contents of a specific resource or sub-resource. */
struct ResourceContents {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    std::optional<QString> _mimeType;  //!< The MIME type of this resource, if known.
    QString _uri;  //!< The URI of this resource.

    ResourceContents& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ResourceContents& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ResourceContents& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ResourceContents& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    ResourceContents& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceContents> fromJson<ResourceContents>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceContents &data);

/**
 * An optional notification from the server to the client, informing it that the list of resources it can read from has changed. This may be issued by servers without any previous subscription from the client.
 */
struct ResourceListChangedNotification {
    std::optional<NotificationParams> _params;

    ResourceListChangedNotification& params(const std::optional<NotificationParams> & v) { _params = v; return *this; }

    const std::optional<NotificationParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceListChangedNotification> fromJson<ResourceListChangedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceListChangedNotification &data);

/** Common parameters when working with resources. */
struct ResourceRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _uri;  //!< The URI of the resource. The URI can use any protocol; it is up to the server how to interpret it.

    ResourceRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    ResourceRequestParams& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceRequestParams::Meta> fromJson<ResourceRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<ResourceRequestParams> fromJson<ResourceRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceRequestParams &data);

/** Parameters for a `notifications/resources/updated` notification. */
struct ResourceUpdatedNotificationParams {
    std::optional<QMap<QString, QJsonValue>> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    QString _uri;  //!< The URI of the resource that has been updated. This might be a sub-resource of the one that the client actually subscribed to.

    ResourceUpdatedNotificationParams& _meta(const std::optional<QMap<QString, QJsonValue>> & v) { __meta = v; return *this; }
    ResourceUpdatedNotificationParams& add_meta(const QString &key, const QJsonValue &v) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; (*__meta)[key] = v; return *this; }
    ResourceUpdatedNotificationParams& _meta(const QJsonObject &obj) { if (!__meta) __meta = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*__meta)[it.key()] = it.value(); return *this; }
    ResourceUpdatedNotificationParams& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<QMap<QString, QJsonValue>>& _meta() const { return __meta; }
    QJsonObject _metaAsObject() const { if (!__meta) return {}; QJsonObject o; for (auto it = __meta->constBegin(); it != __meta->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceUpdatedNotificationParams> fromJson<ResourceUpdatedNotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceUpdatedNotificationParams &data);

/**
 * A notification from the server to the client, informing it that a resource has changed and may need to be read again. This should only be sent if the client previously sent a resources/subscribe request.
 */
struct ResourceUpdatedNotification {
    ResourceUpdatedNotificationParams _params;

    ResourceUpdatedNotification& params(const ResourceUpdatedNotificationParams & v) { _params = v; return *this; }

    const ResourceUpdatedNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceUpdatedNotification> fromJson<ResourceUpdatedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceUpdatedNotification &data);

/**
 * An optional notification from the server to the client, informing it that the list of tools it offers has changed. This may be issued by servers without any previous subscription from the client.
 */
struct ToolListChangedNotification {
    std::optional<NotificationParams> _params;

    ToolListChangedNotification& params(const std::optional<NotificationParams> & v) { _params = v; return *this; }

    const std::optional<NotificationParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ToolListChangedNotification> fromJson<ToolListChangedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ToolListChangedNotification &data);

using ServerNotification = std::variant<CancelledNotification, ProgressNotification, ResourceListChangedNotification, ResourceUpdatedNotification, PromptListChangedNotification, ToolListChangedNotification, TaskStatusNotification, LoggingMessageNotification, ElicitationCompleteNotification>;

template<>
MCPSERVER_EXPORT Utils::Result<ServerNotification> fromJson<ServerNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerNotification &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ServerNotification &val);

/** Returns the 'method' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const ServerNotification &val);

using ServerRequest = std::variant<PingRequest, GetTaskRequest, GetTaskPayloadRequest, CancelTaskRequest, ListTasksRequest, CreateMessageRequest, ListRootsRequest, ElicitRequest>;

template<>
MCPSERVER_EXPORT Utils::Result<ServerRequest> fromJson<ServerRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerRequest &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ServerRequest &val);

/** Returns the 'method' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const ServerRequest &val);

/** Returns the 'id' field from the active variant. */
MCPSERVER_EXPORT RequestId id(const ServerRequest &val);

using ServerResult = std::variant<Result, InitializeResult, ListResourcesResult, ListResourceTemplatesResult, ReadResourceResult, ListPromptsResult, GetPromptResult, ListToolsResult, CallToolResult, GetTaskResult, GetTaskPayloadResult, CancelTaskResult, ListTasksResult, CompleteResult>;

template<>
MCPSERVER_EXPORT Utils::Result<ServerResult> fromJson<ServerResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerResult &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ServerResult &val);

using SingleSelectEnumSchema = std::variant<UntitledSingleSelectEnumSchema, TitledSingleSelectEnumSchema>;

template<>
MCPSERVER_EXPORT Utils::Result<SingleSelectEnumSchema> fromJson<SingleSelectEnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SingleSelectEnumSchema &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const SingleSelectEnumSchema &val);

/** Common params for any task-augmented request. */
struct TaskAugmentedRequestParams {
    /**
     * See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
     */
    struct Meta {
        std::optional<ProgressToken> _progressToken;  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by notifications/progress). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

        Meta& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

        const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
    };

    std::optional<Meta> __meta;  //!< See [General fields: `_meta`](/specification/2025-11-25/basic/index#meta) for notes on `_meta` usage.
    /**
     * If specified, the caller is requesting task-augmented execution for this request.
     * The request will return a CreateTaskResult immediately, and the actual result can be
     * retrieved later via tasks/result.
     *
     * Task augmentation is subject to capability negotiation - receivers MUST declare support
     * for task augmentation of specific request types in their capabilities.
     */
    std::optional<TaskMetadata> _task;

    TaskAugmentedRequestParams& _meta(const std::optional<Meta> & v) { __meta = v; return *this; }
    TaskAugmentedRequestParams& task(const std::optional<TaskMetadata> & v) { _task = v; return *this; }

    const std::optional<Meta>& _meta() const { return __meta; }
    const std::optional<TaskMetadata>& task() const { return _task; }
};

template<>
MCPSERVER_EXPORT Utils::Result<TaskAugmentedRequestParams::Meta> fromJson<TaskAugmentedRequestParams::Meta>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TaskAugmentedRequestParams::Meta &data);

template<>
MCPSERVER_EXPORT Utils::Result<TaskAugmentedRequestParams> fromJson<TaskAugmentedRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TaskAugmentedRequestParams &data);

/**
 * An error response that indicates that the server requires the client to provide additional information via an elicitation request.
 */
struct URLElicitationRequiredError {
    QString _error;
    std::optional<RequestId> _id;

    URLElicitationRequiredError& error(const QString & v) { _error = v; return *this; }
    URLElicitationRequiredError& id(const std::optional<RequestId> & v) { _id = v; return *this; }

    const QString& error() const { return _error; }
    const std::optional<RequestId>& id() const { return _id; }
};

template<>
MCPSERVER_EXPORT Utils::Result<URLElicitationRequiredError> fromJson<URLElicitationRequiredError>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const URLElicitationRequiredError &data);

} // namespace Mcp::Generated::Schema::_2025_11_25
