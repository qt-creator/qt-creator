/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 python3 \
  scripts/generate_cpp_from_schema.py \
  src/libs/mcp/schemas/schema-2026-07-28.json src/libs/mcp/schemas/schema_2026_07_28.h --namespace Mcp::Generated::Schema::_2026_07_28 --cpp-output src/libs/mcp/schemas/schema_2026_07_28.cpp --export-macro MCPSERVER_EXPORT --export-header ../server/mcpserver_global.h --no-cxx20
*/
#pragma once

#include "../server/mcpserver_global.h"

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
#include <variant>

namespace Mcp::Generated::Schema::_2026_07_28 {

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
    std::optional<QList<Role>> _audience{};
    /**
     * The moment the resource was last modified, as an ISO 8601 formatted string.
     *
     * Should be an ISO 8601 formatted string (e.g., "2025-01-12T15:00:58Z").
     *
     * Examples: last activity timestamp in an open file, timestamp when the resource
     * was attached, etc.
     */
    std::optional<QString> _lastModified{};
    /**
     * Describes how important this data is for operating the server.
     *
     * A value of 1 means "most important," and indicates that the data is
     * effectively required, while 0 means "least important," and indicates that
     * the data is entirely optional.
     */
    std::optional<double> _priority{};

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

using MetaObject = QJsonObject;

/** Audio provided to or from an LLM. */
struct AudioContent {
    std::optional<MetaObject> __meta{};
    std::optional<Annotations> _annotations{};  //!< Optional annotations for the client.
    QString _data{};  //!< The base64-encoded audio data.
    QString _mimeType{};  //!< The MIME type of the audio. Different providers may support different audio types.

    AudioContent& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    AudioContent& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    AudioContent& data(const QString & v) { _data = v; return *this; }
    AudioContent& mimeType(const QString & v) { _mimeType = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const QString& data() const { return _data; }
    const QString& mimeType() const { return _mimeType; }
};

template<>
MCPSERVER_EXPORT Utils::Result<AudioContent> fromJson<AudioContent>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const AudioContent &data);

/** Base interface for metadata with name (identifier) and title (display name) properties. */
struct BaseMetadata {
    QString _name{};  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title{};

    BaseMetadata& name(const QString & v) { _name = v; return *this; }
    BaseMetadata& title(const std::optional<QString> & v) { _title = v; return *this; }

    const QString& name() const { return _name; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<BaseMetadata> fromJson<BaseMetadata>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const BaseMetadata &data);

struct BlobResourceContents {
    std::optional<MetaObject> __meta{};
    QString _blob{};  //!< A base64-encoded string representing the binary data of the item.
    std::optional<QString> _mimeType{};  //!< The MIME type of this resource, if known.
    QString _uri{};  //!< The URI of this resource.

    BlobResourceContents& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    BlobResourceContents& blob(const QString & v) { _blob = v; return *this; }
    BlobResourceContents& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    BlobResourceContents& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const QString& blob() const { return _blob; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<BlobResourceContents> fromJson<BlobResourceContents>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const BlobResourceContents &data);

struct BooleanSchema {
    std::optional<bool> _default_{};
    std::optional<QString> _description{};
    std::optional<QString> _title{};

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

/** An optionally-sized icon that can be displayed in a user interface. */
struct Icon {
    /**
     * Optional specifier for the theme this icon is designed for. `"light"` indicates
     * the icon is designed to be used with a light background, and `"dark"` indicates
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
    std::optional<QString> _mimeType{};
    /**
     * Optional array of strings that specify sizes at which the icon can be used.
     * Each string should be in WxH format (e.g., `"48x48"`, `"96x96"`) or `"any"` for scalable formats like SVG.
     *
     * If not provided, the client should assume that the icon can be used at any size.
     */
    std::optional<QStringList> _sizes{};
    /**
     * A standard URI pointing to an icon resource. May be an HTTP/HTTPS URL or a
     * `data:` URI with Base64-encoded image data.
     *
     * Consumers SHOULD take steps to ensure URLs serving icons are from the
     * same domain as the client/server or a trusted domain.
     *
     * Consumers SHOULD take appropriate precautions when consuming SVGs as they can contain
     * executable JavaScript.
     */
    QString _src{};
    std::optional<Theme> _theme{};

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

/** Describes the MCP implementation. */
struct Implementation {
    /**
     * An optional human-readable description of what this implementation does.
     *
     * This can be used by clients or servers to provide context about their purpose
     * and capabilities. For example, a server might describe the types of resources
     * or tools it provides, while a client might describe its intended use case.
     */
    std::optional<QString> _description{};
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
    std::optional<QList<Icon>> _icons{};
    QString _name{};  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title{};
    QString _version{};  //!< The version of this implementation.
    std::optional<QString> _websiteUrl{};  //!< An optional URL of the website for this implementation.

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

/**
 * Extends {@link MetaObject} with additional result-specific fields. All key naming rules from `MetaObject` apply.
 */
struct ResultMetaObject {
    /**
     * Identifies the server software producing the response. Servers SHOULD
     * include this field on every response unless specifically configured not
     * to do so.
     *
     * The {@link Implementation} schema requires `name` and `version`; other
     * fields are optional.
     *
     * The value is self-reported by the server and is not verified by the
     * protocol. It is intended for display, logging, and debugging. Clients
     * SHOULD NOT use it to change their behavior, and SHOULD NOT rely on it for
     * security decisions.
     */
    std::optional<Implementation> _iodotmodelcontextprotocolslashserverInfo{};

    ResultMetaObject& iodotmodelcontextprotocolslashserverInfo(const std::optional<Implementation> & v) { _iodotmodelcontextprotocolslashserverInfo = v; return *this; }

    const std::optional<Implementation>& iodotmodelcontextprotocolslashserverInfo() const { return _iodotmodelcontextprotocolslashserverInfo; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResultMetaObject> fromJson<ResultMetaObject>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResultMetaObject &data);

/** A result that supports a time-to-live (TTL) hint for client-side caching. */
struct CacheableResult {
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    enum class CacheScope {
        private_,
        public_
    };

    std::optional<ResultMetaObject> __meta{};
    CacheScope _cacheScope{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    int _ttlMs{};

    CacheableResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    CacheableResult& cacheScope(const CacheScope & v) { _cacheScope = v; return *this; }
    CacheableResult& resultType(const QString & v) { _resultType = v; return *this; }
    CacheableResult& ttlMs(int v) { _ttlMs = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const CacheScope& cacheScope() const { return _cacheScope; }
    const QString& resultType() const { return _resultType; }
    const int& ttlMs() const { return _ttlMs; }
};

MCPSERVER_EXPORT QString toString(const CacheableResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<CacheableResult::CacheScope> fromJson<CacheableResult::CacheScope>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const CacheableResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<CacheableResult> fromJson<CacheableResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CacheableResult &data);

/** An image provided to or from an LLM. */
struct ImageContent {
    std::optional<MetaObject> __meta{};
    std::optional<Annotations> _annotations{};  //!< Optional annotations for the client.
    QString _data{};  //!< The base64-encoded image data.
    QString _mimeType{};  //!< The MIME type of the image. Different providers may support different image types.

    ImageContent& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    ImageContent& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    ImageContent& data(const QString & v) { _data = v; return *this; }
    ImageContent& mimeType(const QString & v) { _mimeType = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const QString& data() const { return _data; }
    const QString& mimeType() const { return _mimeType; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ImageContent> fromJson<ImageContent>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ImageContent &data);

/** Text provided to or from an LLM. */
struct TextContent {
    std::optional<MetaObject> __meta{};
    std::optional<Annotations> _annotations{};  //!< Optional annotations for the client.
    QString _text{};  //!< The text content of the message.

    TextContent& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    TextContent& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    TextContent& text(const QString & v) { _text = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const QString& text() const { return _text; }
};

template<>
MCPSERVER_EXPORT Utils::Result<TextContent> fromJson<TextContent>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const TextContent &data);

struct TextResourceContents {
    std::optional<MetaObject> __meta{};
    std::optional<QString> _mimeType{};  //!< The MIME type of this resource, if known.
    QString _text{};  //!< The text of the item. This must only be set if the item can actually be represented as text (not binary data).
    QString _uri{};  //!< The URI of this resource.

    TextResourceContents& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    TextResourceContents& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    TextResourceContents& text(const QString & v) { _text = v; return *this; }
    TextResourceContents& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
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
    std::optional<MetaObject> __meta{};
    std::optional<Annotations> _annotations{};  //!< Optional annotations for the client.
    EmbeddedResourceResource _resource{};

    EmbeddedResource& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    EmbeddedResource& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    EmbeddedResource& resource(const EmbeddedResourceResource & v) { _resource = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const std::optional<Annotations>& annotations() const { return _annotations; }
    const EmbeddedResourceResource& resource() const { return _resource; }
};

template<>
MCPSERVER_EXPORT Utils::Result<EmbeddedResource> fromJson<EmbeddedResource>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const EmbeddedResource &data);

/**
 * A resource that the server is capable of reading, included in a prompt or tool call result.
 *
 * Note: resource links returned by tools are not guaranteed to appear in the results of {@link ListResourcesRequestresources/list} requests.
 */
struct ResourceLink {
    std::optional<MetaObject> __meta{};
    std::optional<Annotations> _annotations{};  //!< Optional annotations for the client.
    /**
     * A description of what this resource represents.
     *
     * This can be used by clients to improve the LLM's understanding of available resources. It can be thought of like a "hint" to the model.
     */
    std::optional<QString> _description{};
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
    std::optional<QList<Icon>> _icons{};
    std::optional<QString> _mimeType{};  //!< The MIME type of this resource, if known.
    QString _name{};  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * The size of the raw resource content, in bytes (i.e., before base64 encoding or any tokenization), if known.
     *
     * This can be used by Hosts to display file sizes and estimate context window usage.
     */
    std::optional<int> _size{};
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title{};
    QString _uri{};  //!< The URI of this resource.

    ResourceLink& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    ResourceLink& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    ResourceLink& description(const std::optional<QString> & v) { _description = v; return *this; }
    ResourceLink& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    ResourceLink& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    ResourceLink& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    ResourceLink& name(const QString & v) { _name = v; return *this; }
    ResourceLink& size(std::optional<int> v) { _size = v; return *this; }
    ResourceLink& title(const std::optional<QString> & v) { _title = v; return *this; }
    ResourceLink& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
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

using ContentBlock = std::variant<TextContent, ImageContent, AudioContent, ResourceLink, EmbeddedResource>;

template<>
MCPSERVER_EXPORT Utils::Result<ContentBlock> fromJson<ContentBlock>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ContentBlock &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ContentBlock &val);

/** Returns the 'type' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const ContentBlock &val);

/** The result of a tool use, provided by the user back to the assistant. */
struct ToolResultContent {
    /**
     * Optional metadata about the tool result. Clients SHOULD preserve this field when
     * including tool results in subsequent sampling requests to enable caching optimizations.
     */
    std::optional<MetaObject> __meta{};
    /**
     * The unstructured result content of the tool use.
     *
     * This has the same format as {@link CallToolResult.content} and can include text, images,
     * audio, resource links, and embedded resources.
     */
    QList<ContentBlock> _content{};
    /**
     * Whether the tool use resulted in an error.
     *
     * If true, the content typically describes the error that occurred.
     * Default: false
     */
    std::optional<bool> _isError{};
    /**
     * An optional structured result value.
     *
     * This can be any JSON value (object, array, string, number, boolean, or null).
     * If the tool defined an {@link Tool.outputSchema}, this SHOULD conform to that schema.
     */
    std::optional<QJsonValue> _structuredContent{};
    /**
     * The ID of the tool use this result corresponds to.
     *
     * This MUST match the ID from a previous {@link ToolUseContent}.
     */
    QString _toolUseId{};

    ToolResultContent& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    ToolResultContent& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    ToolResultContent& addContent(const ContentBlock & v) { _content.append(v); return *this; }
    ToolResultContent& isError(std::optional<bool> v) { _isError = v; return *this; }
    ToolResultContent& structuredContent(const std::optional<QJsonValue> & v) { _structuredContent = v; return *this; }
    ToolResultContent& toolUseId(const QString & v) { _toolUseId = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const QList<ContentBlock>& content() const { return _content; }
    const std::optional<bool>& isError() const { return _isError; }
    const std::optional<QJsonValue>& structuredContent() const { return _structuredContent; }
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
     */
    std::optional<MetaObject> __meta{};
    /**
     * A unique identifier for this tool use.
     *
     * This ID is used to match tool results to their corresponding tool uses.
     */
    QString _id{};
    QMap<QString, QJsonValue> _input{};  //!< The arguments to pass to the tool, conforming to the tool's input schema.
    QString _name{};  //!< The name of the tool to call.

    ToolUseContent& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    ToolUseContent& id(const QString & v) { _id = v; return *this; }
    ToolUseContent& input(const QMap<QString, QJsonValue> & v) { _input = v; return *this; }
    ToolUseContent& addInput(const QString &key, const QJsonValue &v) { _input[key] = v; return *this; }
    ToolUseContent& input(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _input[it.key()] = it.value(); return *this; }
    ToolUseContent& name(const QString & v) { _name = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
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
 * The result returned by the client for a {@link CreateMessageRequestsampling/createMessage} request.
 * The client should inform the user before returning the sampled message, to allow them
 * to inspect the response (human in the loop) and decide whether to allow the server to see it.
 */
struct CreateMessageResult {
    std::optional<MetaObject> __meta{};
    CreateMessageResultContent _content{};
    QString _model{};  //!< The name of the model that generated the message.
    Role _role{};
    /**
     * The reason why sampling stopped, if known.
     *
     * Standard values:
     * - `"endTurn"`: Natural end of the assistant's turn
     * - `"stopSequence"`: A stop sequence was encountered
     * - `"maxTokens"`: Maximum token limit was reached
     * - `"toolUse"`: The model wants to use one or more tools
     *
     * This field is an open string to allow for provider-specific stop reasons.
     */
    std::optional<QString> _stopReason{};

    CreateMessageResult& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    CreateMessageResult& content(const CreateMessageResultContent & v) { _content = v; return *this; }
    CreateMessageResult& model(const QString & v) { _model = v; return *this; }
    CreateMessageResult& role(const Role & v) { _role = v; return *this; }
    CreateMessageResult& stopReason(const std::optional<QString> & v) { _stopReason = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
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

/** The result returned by the client for an {@link ElicitRequestelicitation/create} request. */
struct ElicitResult {
    /**
     * The user action in response to the elicitation.
     * - `"accept"`: User submitted the form/confirmed the action
     * - `"decline"`: User explicitly declined the action
     * - `"cancel"`: User dismissed without making an explicit choice
     */
    enum class Action {
        accept,
        cancel,
        decline
    };

    Action _action{};
    /**
     * The submitted form data, only present when action is `"accept"` and mode was `"form"`.
     * Contains values matching the requested schema.
     * Omitted for out-of-band mode responses.
     */
    std::optional<QMap<QString, ElicitResultContentValue>> _content{};

    ElicitResult& action(const Action & v) { _action = v; return *this; }
    ElicitResult& content(const std::optional<QMap<QString, ElicitResultContentValue>> & v) { _content = v; return *this; }
    ElicitResult& addContent(const QString &key, const ElicitResultContentValue & v) { if (!_content) _content = QMap<QString, ElicitResultContentValue>{}; (*_content)[key] = v; return *this; }

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

/** Represents a root directory or file that the server can operate on. */
struct Root {
    std::optional<MetaObject> __meta{};
    /**
     * An optional name for the root. This can be used to provide a human-readable
     * identifier for the root, which may be useful for display purposes or for
     * referencing the root in other parts of the application.
     */
    std::optional<QString> _name{};
    /**
     * The URI identifying the root. This *must* start with `file://` for now.
     * This restriction may be relaxed in future versions of the protocol to allow
     * other URI schemes.
     */
    QString _uri{};

    Root& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    Root& name(const std::optional<QString> & v) { _name = v; return *this; }
    Root& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const std::optional<QString>& name() const { return _name; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Root> fromJson<Root>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Root &data);

/**
 * The result returned by the client for a {@link ListRootsRequestroots/list} request.
 * This result contains an array of {@link Root} objects, each representing a root directory
 * or file that the server can operate on.
 */
struct ListRootsResult {
    QList<Root> _roots{};

    ListRootsResult& roots(const QList<Root> & v) { _roots = v; return *this; }
    ListRootsResult& addRoot(const Root & v) { _roots.append(v); return *this; }

    const QList<Root>& roots() const { return _roots; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListRootsResult> fromJson<ListRootsResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListRootsResult &data);

using InputResponse = std::variant<CreateMessageResult, ListRootsResult, ElicitResult>;

template<>
MCPSERVER_EXPORT Utils::Result<InputResponse> fromJson<InputResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InputResponse &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const InputResponse &val);

using InputResponses = QMap<QString, InputResponse>;
template<>
MCPSERVER_EXPORT Utils::Result<InputResponses> fromJson<InputResponses>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InputResponses &data);

/**
 * Capabilities a client may support. Known capabilities are defined here, in this schema, but this is not a closed set: any client can define its own, additional capabilities.
 */
struct ClientCapabilities {
    /** Present if the client supports elicitation from the server. */
    struct Elicitation {
        std::optional<QJsonObject> _form{};
        std::optional<QJsonObject> _url{};

        Elicitation& form(const std::optional<QJsonObject> & v) { _form = v; return *this; }
        Elicitation& url(const std::optional<QJsonObject> & v) { _url = v; return *this; }

        const std::optional<QJsonObject>& form() const { return _form; }
        const std::optional<QJsonObject>& url() const { return _url; }
    };

    /** Present if the client supports sampling from an LLM. */
    struct Sampling {
        /**
         * Whether the client supports context inclusion via `includeContext` parameter.
         * If not declared, servers SHOULD only use `includeContext: "none"` (or omit it).
         */
        std::optional<QJsonObject> _context{};
        std::optional<QJsonObject> _tools{};  //!< Whether the client supports tool use via `tools` and `toolChoice` parameters.

        Sampling& context(const std::optional<QJsonObject> & v) { _context = v; return *this; }
        Sampling& tools(const std::optional<QJsonObject> & v) { _tools = v; return *this; }

        const std::optional<QJsonObject>& context() const { return _context; }
        const std::optional<QJsonObject>& tools() const { return _tools; }
    };

    std::optional<Elicitation> _elicitation{};  //!< Present if the client supports elicitation from the server.
    std::optional<QMap<QString, QJsonObject>> _experimental{};  //!< Experimental, non-standard capabilities that the client supports.
    /**
     * Optional MCP extensions that the client supports. Keys are extension identifiers
     * (e.g., "io.modelcontextprotocol/oauth-client-credentials"), and values are
     * per-extension settings objects. An empty object indicates support with no settings.
     *
     * Keys MUST follow the {@link MetaObject`_meta` key naming rules}, with a
     * mandatory prefix.
     */
    std::optional<QMap<QString, QJsonObject>> _extensions{};
    std::optional<QJsonObject> _roots{};  //!< Present if the client supports listing roots.
    std::optional<Sampling> _sampling{};  //!< Present if the client supports sampling from an LLM.

    ClientCapabilities& elicitation(const std::optional<Elicitation> & v) { _elicitation = v; return *this; }
    ClientCapabilities& experimental(const std::optional<QMap<QString, QJsonObject>> & v) { _experimental = v; return *this; }
    ClientCapabilities& addExperimental(const QString &key, const QJsonObject & v) { if (!_experimental) _experimental = QMap<QString, QJsonObject>{}; (*_experimental)[key] = v; return *this; }
    ClientCapabilities& extensions(const std::optional<QMap<QString, QJsonObject>> & v) { _extensions = v; return *this; }
    ClientCapabilities& addExtension(const QString &key, const QJsonObject & v) { if (!_extensions) _extensions = QMap<QString, QJsonObject>{}; (*_extensions)[key] = v; return *this; }
    ClientCapabilities& roots(const std::optional<QJsonObject> & v) { _roots = v; return *this; }
    ClientCapabilities& sampling(const std::optional<Sampling> & v) { _sampling = v; return *this; }

    const std::optional<Elicitation>& elicitation() const { return _elicitation; }
    const std::optional<QMap<QString, QJsonObject>>& experimental() const { return _experimental; }
    const std::optional<QMap<QString, QJsonObject>>& extensions() const { return _extensions; }
    const std::optional<QJsonObject>& roots() const { return _roots; }
    const std::optional<Sampling>& sampling() const { return _sampling; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities::Elicitation> fromJson<ClientCapabilities::Elicitation>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities::Elicitation &data);

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities::Sampling> fromJson<ClientCapabilities::Sampling>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities::Sampling &data);

template<>
MCPSERVER_EXPORT Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientCapabilities &data);

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

/** A progress token, used to associate progress notifications with the original request. */
using ProgressToken = std::variant<QString, int>;

template<>
MCPSERVER_EXPORT Utils::Result<ProgressToken> fromJson<ProgressToken>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ProgressToken &val);

/**
 * Extends {@link MetaObject} with additional request-specific fields. All key naming rules from `MetaObject` apply.
 */
struct RequestMetaObject {
    /**
     * The client's capabilities for this specific request. Required.
     *
     * Capabilities are declared per-request rather than once at initialization;
     * an empty object means the client supports no optional capabilities.
     * Servers MUST NOT infer capabilities from prior requests.
     */
    ClientCapabilities _iodotmodelcontextprotocolslashclientCapabilities{};
    /**
     * Identifies the client software making the request. Clients SHOULD
     * include this field on every request unless specifically configured not
     * to do so.
     *
     * The {@link Implementation} schema requires `name` and `version`; other
     * fields are optional.
     *
     * The value is self-reported by the client and is not verified by the
     * protocol. It is intended for display, logging, and debugging. Servers
     * SHOULD NOT use it to change their behavior, and SHOULD NOT rely on it for
     * security decisions.
     */
    std::optional<Implementation> _iodotmodelcontextprotocolslashclientInfo{};
    /**
     * The desired log level for this request. Optional.
     *
     * If absent, the server MUST NOT send any {@link LoggingMessageNotificationnotifications/message}
     * notifications for this request. The client opts in to log messages by
     * explicitly setting a level. Replaces the former `logging/setLevel` RPC.
     */
    std::optional<LoggingLevel> _iodotmodelcontextprotocolslashlogLevel{};
    /**
     * The MCP Protocol Version being used for this request. Required.
     *
     * For the HTTP transport, this value MUST match the `MCP-Protocol-Version`
     * header; otherwise the server MUST return a `400 Bad Request`. If the
     * server does not support the requested version, it MUST return an
     * {@link UnsupportedProtocolVersionError}.
     */
    QString _iodotmodelcontextprotocolslashprotocolVersion{};
    std::optional<ProgressToken> _progressToken{};  //!< If specified, the caller is requesting out-of-band progress notifications for this request (as represented by {@link ProgressNotificationnotifications/progress}). The value of this parameter is an opaque token that will be attached to any subsequent notifications. The receiver is not obligated to provide these notifications.

    RequestMetaObject& iodotmodelcontextprotocolslashclientCapabilities(const ClientCapabilities & v) { _iodotmodelcontextprotocolslashclientCapabilities = v; return *this; }
    RequestMetaObject& iodotmodelcontextprotocolslashclientInfo(const std::optional<Implementation> & v) { _iodotmodelcontextprotocolslashclientInfo = v; return *this; }
    RequestMetaObject& iodotmodelcontextprotocolslashlogLevel(const std::optional<LoggingLevel> & v) { _iodotmodelcontextprotocolslashlogLevel = v; return *this; }
    RequestMetaObject& iodotmodelcontextprotocolslashprotocolVersion(const QString & v) { _iodotmodelcontextprotocolslashprotocolVersion = v; return *this; }
    RequestMetaObject& progressToken(const std::optional<ProgressToken> & v) { _progressToken = v; return *this; }

    const ClientCapabilities& iodotmodelcontextprotocolslashclientCapabilities() const { return _iodotmodelcontextprotocolslashclientCapabilities; }
    const std::optional<Implementation>& iodotmodelcontextprotocolslashclientInfo() const { return _iodotmodelcontextprotocolslashclientInfo; }
    const std::optional<LoggingLevel>& iodotmodelcontextprotocolslashlogLevel() const { return _iodotmodelcontextprotocolslashlogLevel; }
    const QString& iodotmodelcontextprotocolslashprotocolVersion() const { return _iodotmodelcontextprotocolslashprotocolVersion; }
    const std::optional<ProgressToken>& progressToken() const { return _progressToken; }
};

template<>
MCPSERVER_EXPORT Utils::Result<RequestMetaObject> fromJson<RequestMetaObject>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const RequestMetaObject &data);

/** Parameters for a `tools/call` request. */
struct CallToolRequestParams {
    RequestMetaObject __meta{};
    std::optional<QMap<QString, QJsonValue>> _arguments{};  //!< Arguments to use for the tool call.
    std::optional<InputResponses> _inputResponses{};
    QString _name{};  //!< The name of the tool.
    std::optional<QString> _requestState{};

    CallToolRequestParams& _meta(const RequestMetaObject & v) { __meta = v; return *this; }
    CallToolRequestParams& arguments(const std::optional<QMap<QString, QJsonValue>> & v) { _arguments = v; return *this; }
    CallToolRequestParams& addArgument(const QString &key, const QJsonValue &v) { if (!_arguments) _arguments = QMap<QString, QJsonValue>{}; (*_arguments)[key] = v; return *this; }
    CallToolRequestParams& arguments(const QJsonObject &obj) { if (!_arguments) _arguments = QMap<QString, QJsonValue>{}; for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) (*_arguments)[it.key()] = it.value(); return *this; }
    CallToolRequestParams& inputResponses(const std::optional<InputResponses> & v) { _inputResponses = v; return *this; }
    CallToolRequestParams& name(const QString & v) { _name = v; return *this; }
    CallToolRequestParams& requestState(const std::optional<QString> & v) { _requestState = v; return *this; }

    const RequestMetaObject& _meta() const { return __meta; }
    const std::optional<QMap<QString, QJsonValue>>& arguments() const { return _arguments; }
    QJsonObject argumentsAsObject() const { if (!_arguments) return {}; QJsonObject o; for (auto it = _arguments->constBegin(); it != _arguments->constEnd(); ++it) o.insert(it.key(), it.value()); return o; }
    const std::optional<InputResponses>& inputResponses() const { return _inputResponses; }
    const QString& name() const { return _name; }
    const std::optional<QString>& requestState() const { return _requestState; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CallToolRequestParams> fromJson<CallToolRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CallToolRequestParams &data);

/** A uniquely identifying ID for a request in JSON-RPC. */
using RequestId = std::variant<QString, int>;

/** Used by the client to invoke a tool provided by the server. */
struct CallToolRequest {
    RequestId _id{};
    CallToolRequestParams _params{};

    CallToolRequest& id(const RequestId & v) { _id = v; return *this; }
    CallToolRequest& params(const CallToolRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const CallToolRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CallToolRequest> fromJson<CallToolRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CallToolRequest &data);

/** The result returned by the server for a {@link CallToolRequesttools/call} request. */
struct CallToolResult {
    std::optional<ResultMetaObject> __meta{};
    QList<ContentBlock> _content{};  //!< A list of content objects that represent the unstructured result of the tool call.
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
    std::optional<bool> _isError{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};
    /**
     * An optional JSON value that represents the structured result of the tool call.
     *
     * This can be any JSON value (object, array, string, number, boolean, or null)
     * that conforms to the tool's outputSchema if one is defined.
     */
    std::optional<QJsonValue> _structuredContent{};

    CallToolResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    CallToolResult& content(const QList<ContentBlock> & v) { _content = v; return *this; }
    CallToolResult& addContent(const ContentBlock & v) { _content.append(v); return *this; }
    CallToolResult& isError(std::optional<bool> v) { _isError = v; return *this; }
    CallToolResult& resultType(const QString & v) { _resultType = v; return *this; }
    CallToolResult& structuredContent(const std::optional<QJsonValue> & v) { _structuredContent = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const QList<ContentBlock>& content() const { return _content; }
    const std::optional<bool>& isError() const { return _isError; }
    const QString& resultType() const { return _resultType; }
    const std::optional<QJsonValue>& structuredContent() const { return _structuredContent; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CallToolResult> fromJson<CallToolResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CallToolResult &data);

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
    std::optional<QString> _name{};

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
    std::optional<double> _costPriority{};
    /**
     * Optional hints to use for model selection.
     *
     * If multiple hints are specified, the client MUST evaluate them in order
     * (such that the first match is taken).
     *
     * The client SHOULD prioritize these hints over the numeric priorities, but
     * MAY still use the priorities to select from ambiguous matches.
     */
    std::optional<QList<ModelHint>> _hints{};
    /**
     * How much to prioritize intelligence and capabilities when selecting a
     * model. A value of 0 means intelligence is not important, while a value of 1
     * means intelligence is the most important factor.
     */
    std::optional<double> _intelligencePriority{};
    /**
     * How much to prioritize sampling speed (latency) when selecting a model. A
     * value of 0 means speed is not important, while a value of 1 means speed is
     * the most important factor.
     */
    std::optional<double> _speedPriority{};

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
    std::optional<MetaObject> __meta{};
    CreateMessageResultContent _content{};
    Role _role{};

    SamplingMessage& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    SamplingMessage& content(const CreateMessageResultContent & v) { _content = v; return *this; }
    SamplingMessage& role(const Role & v) { _role = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const CreateMessageResultContent& content() const { return _content; }
    const Role& role() const { return _role; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SamplingMessage> fromJson<SamplingMessage>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SamplingMessage &data);

/**
 * Additional properties describing a {@link Tool} to clients.
 *
 * NOTE: all properties in `ToolAnnotations` are **hints**.
 * They are not guaranteed to provide a faithful description of
 * tool behavior (including descriptive properties like `title`).
 *
 * Clients should never make tool use decisions based on `ToolAnnotations`
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
    std::optional<bool> _destructiveHint{};
    /**
     * If true, calling the tool repeatedly with the same arguments
     * will have no additional effect on its environment.
     *
     * (This property is meaningful only when `readOnlyHint == false`)
     *
     * Default: false
     */
    std::optional<bool> _idempotentHint{};
    /**
     * If true, this tool may interact with an "open world" of external
     * entities. If false, the tool's domain of interaction is closed.
     * For example, the world of a web search tool is open, whereas that
     * of a memory tool is not.
     *
     * Default: true
     */
    std::optional<bool> _openWorldHint{};
    /**
     * If true, the tool does not modify its environment.
     *
     * Default: false
     */
    std::optional<bool> _readOnlyHint{};
    std::optional<QString> _title{};  //!< A human-readable title for the tool.

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

/** Definition for a tool the client can call. */
struct Tool {
    /**
     * A JSON Schema object defining the expected parameters for the tool.
     *
     * Tool arguments are always JSON objects, so `type: "object"` is required at the root.
     * Beyond that, any JSON Schema 2020-12 keyword may appear alongside `type` — including
     * composition keywords (`oneOf`, `anyOf`, `allOf`, `not`), conditional keywords
     * (`if`/`then`/`else`), reference keywords (`$ref`, `$defs`, `$anchor`), and any other
     * standard validation or annotation keywords.
     *
     * Property schemas may carry an `x-mcp-header` annotation to mirror the
     * argument value into an HTTP header on the Streamable HTTP transport. See
     * the Streamable HTTP transport specification for the validity and
     * extraction rules.
     *
     * Defaults to JSON Schema 2020-12 when no explicit `$schema` is provided.
     */
    struct InputSchema {
        std::optional<QString> _dollarschema{};
        QJsonObject _additionalProperties;  //!< additional properties

        InputSchema& dollarschema(const std::optional<QString> & v) { _dollarschema = v; return *this; }
        InputSchema& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
        InputSchema& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

        const std::optional<QString>& dollarschema() const { return _dollarschema; }
        const QJsonObject& additionalProperties() const { return _additionalProperties; }
    };

    /**
     * An optional JSON Schema object defining the structure of the tool's output returned in
     * the structuredContent field of a {@link CallToolResult}. This can be any valid JSON Schema 2020-12.
     *
     * Defaults to JSON Schema 2020-12 when no explicit `$schema` is provided.
     */
    struct OutputSchema {
        std::optional<QString> _dollarschema{};
        QJsonObject _additionalProperties;  //!< additional properties

        OutputSchema& dollarschema(const std::optional<QString> & v) { _dollarschema = v; return *this; }
        OutputSchema& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
        OutputSchema& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

        const std::optional<QString>& dollarschema() const { return _dollarschema; }
        const QJsonObject& additionalProperties() const { return _additionalProperties; }
    };

    std::optional<MetaObject> __meta{};
    /**
     * Optional additional tool information.
     *
     * Display name precedence order is: `title`, `annotations.title`, then `name`.
     */
    std::optional<ToolAnnotations> _annotations{};
    /**
     * A human-readable description of the tool.
     *
     * This can be used by clients to improve the LLM's understanding of available tools. It can be thought of like a "hint" to the model.
     */
    std::optional<QString> _description{};
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
    std::optional<QList<Icon>> _icons{};
    /**
     * A JSON Schema object defining the expected parameters for the tool.
     *
     * Tool arguments are always JSON objects, so `type: "object"` is required at the root.
     * Beyond that, any JSON Schema 2020-12 keyword may appear alongside `type` — including
     * composition keywords (`oneOf`, `anyOf`, `allOf`, `not`), conditional keywords
     * (`if`/`then`/`else`), reference keywords (`$ref`, `$defs`, `$anchor`), and any other
     * standard validation or annotation keywords.
     *
     * Property schemas may carry an `x-mcp-header` annotation to mirror the
     * argument value into an HTTP header on the Streamable HTTP transport. See
     * the Streamable HTTP transport specification for the validity and
     * extraction rules.
     *
     * Defaults to JSON Schema 2020-12 when no explicit `$schema` is provided.
     */
    InputSchema _inputSchema{};
    QString _name{};  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * An optional JSON Schema object defining the structure of the tool's output returned in
     * the structuredContent field of a {@link CallToolResult}. This can be any valid JSON Schema 2020-12.
     *
     * Defaults to JSON Schema 2020-12 when no explicit `$schema` is provided.
     */
    std::optional<OutputSchema> _outputSchema{};
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title{};

    Tool& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    Tool& annotations(const std::optional<ToolAnnotations> & v) { _annotations = v; return *this; }
    Tool& description(const std::optional<QString> & v) { _description = v; return *this; }
    Tool& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    Tool& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    Tool& inputSchema(const InputSchema & v) { _inputSchema = v; return *this; }
    Tool& name(const QString & v) { _name = v; return *this; }
    Tool& outputSchema(const std::optional<OutputSchema> & v) { _outputSchema = v; return *this; }
    Tool& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const std::optional<ToolAnnotations>& annotations() const { return _annotations; }
    const std::optional<QString>& description() const { return _description; }
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
     * - `"auto"`: Model decides whether to use tools (default)
     * - `"required"`: Model MUST use at least one tool before completing
     * - `"none"`: Model MUST NOT use any tools
     */
    enum class Mode {
        auto_,
        none,
        required
    };

    std::optional<Mode> _mode{};

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
     * A request to include context from one or more MCP servers (including the caller), to be attached to the prompt.
     * The client MAY ignore this request.
     *
     * Default is `"none"`. The values `"thisServer"` and `"allServers"` are deprecated (SEP-2596): servers SHOULD
     * omit this field or use `"none"`, and SHOULD only use the deprecated values if the client declares
     * {@link ClientCapabilities.sampling.context}.
     */
    enum class IncludeContext {
        allServers,
        none,
        thisServer
    };

    std::optional<IncludeContext> _includeContext{};
    /**
     * The requested maximum number of tokens to sample (to prevent runaway completions).
     *
     * The client MAY choose to sample fewer tokens than the requested maximum.
     */
    int _maxTokens{};
    QList<SamplingMessage> _messages{};
    std::optional<QJsonObject> _metadata{};  //!< Optional metadata to pass through to the LLM provider. The format of this metadata is provider-specific.
    std::optional<ModelPreferences> _modelPreferences{};  //!< The server's preferences for which model to select. The client MAY ignore these preferences.
    std::optional<QStringList> _stopSequences{};
    std::optional<QString> _systemPrompt{};  //!< An optional system prompt the server wants to use for sampling. The client MAY modify or omit this prompt.
    std::optional<double> _temperature{};
    /**
     * Controls how the model uses tools.
     * The client MUST return an error if this field is provided but {@link ClientCapabilities.sampling.tools} is not declared.
     * Default is `{ mode: "auto" }`.
     */
    std::optional<ToolChoice> _toolChoice{};
    /**
     * Tools that the model may use during generation.
     * The client MUST return an error if this field is provided but {@link ClientCapabilities.sampling.tools} is not declared.
     */
    std::optional<QList<Tool>> _tools{};

    CreateMessageRequestParams& includeContext(const std::optional<IncludeContext> & v) { _includeContext = v; return *this; }
    CreateMessageRequestParams& maxTokens(int v) { _maxTokens = v; return *this; }
    CreateMessageRequestParams& messages(const QList<SamplingMessage> & v) { _messages = v; return *this; }
    CreateMessageRequestParams& addMessage(const SamplingMessage & v) { _messages.append(v); return *this; }
    CreateMessageRequestParams& metadata(const std::optional<QJsonObject> & v) { _metadata = v; return *this; }
    CreateMessageRequestParams& modelPreferences(const std::optional<ModelPreferences> & v) { _modelPreferences = v; return *this; }
    CreateMessageRequestParams& stopSequences(const std::optional<QStringList> & v) { _stopSequences = v; return *this; }
    CreateMessageRequestParams& addStopSequence(const QString & v) { if (!_stopSequences) _stopSequences = QStringList{}; (*_stopSequences).append(v); return *this; }
    CreateMessageRequestParams& systemPrompt(const std::optional<QString> & v) { _systemPrompt = v; return *this; }
    CreateMessageRequestParams& temperature(std::optional<double> v) { _temperature = v; return *this; }
    CreateMessageRequestParams& toolChoice(const std::optional<ToolChoice> & v) { _toolChoice = v; return *this; }
    CreateMessageRequestParams& tools(const std::optional<QList<Tool>> & v) { _tools = v; return *this; }
    CreateMessageRequestParams& addTool(const Tool & v) { if (!_tools) _tools = QList<Tool>{}; (*_tools).append(v); return *this; }

    const std::optional<IncludeContext>& includeContext() const { return _includeContext; }
    const int& maxTokens() const { return _maxTokens; }
    const QList<SamplingMessage>& messages() const { return _messages; }
    const std::optional<QJsonObject>& metadata() const { return _metadata; }
    const std::optional<ModelPreferences>& modelPreferences() const { return _modelPreferences; }
    const std::optional<QStringList>& stopSequences() const { return _stopSequences; }
    const std::optional<QString>& systemPrompt() const { return _systemPrompt; }
    const std::optional<double>& temperature() const { return _temperature; }
    const std::optional<ToolChoice>& toolChoice() const { return _toolChoice; }
    const std::optional<QList<Tool>>& tools() const { return _tools; }
};

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
    CreateMessageRequestParams _params{};

    CreateMessageRequest& params(const CreateMessageRequestParams & v) { _params = v; return *this; }

    const CreateMessageRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CreateMessageRequest> fromJson<CreateMessageRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CreateMessageRequest &data);

/**
 * Use {@link TitledSingleSelectEnumSchema} instead.
 * This interface will be removed in a future version.
 */
struct LegacyTitledEnumSchema {
    std::optional<QString> _default_{};
    std::optional<QString> _description{};
    QStringList _enum_{};
    /**
     * (Legacy) Display names for enum values.
     * Non-standard according to JSON schema 2020-12.
     */
    std::optional<QStringList> _enumNames{};
    std::optional<QString> _title{};

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

    std::optional<double> _default_{};
    std::optional<QString> _description{};
    std::optional<double> _maximum{};
    std::optional<double> _minimum{};
    std::optional<QString> _title{};
    Type _type{};

    NumberSchema& default_(std::optional<double> v) { _default_ = v; return *this; }
    NumberSchema& description(const std::optional<QString> & v) { _description = v; return *this; }
    NumberSchema& maximum(std::optional<double> v) { _maximum = v; return *this; }
    NumberSchema& minimum(std::optional<double> v) { _minimum = v; return *this; }
    NumberSchema& title(const std::optional<QString> & v) { _title = v; return *this; }
    NumberSchema& type(const Type & v) { _type = v; return *this; }

    const std::optional<double>& default_() const { return _default_; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<double>& maximum() const { return _maximum; }
    const std::optional<double>& minimum() const { return _minimum; }
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

    std::optional<QString> _default_{};
    std::optional<QString> _description{};
    std::optional<Format> _format{};
    std::optional<int> _maxLength{};
    std::optional<int> _minLength{};
    std::optional<QString> _title{};

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
            QString _const_{};  //!< The constant enum value.
            QString _title{};  //!< Display title for this option.

            AnyOfItem& const_(const QString & v) { _const_ = v; return *this; }
            AnyOfItem& title(const QString & v) { _title = v; return *this; }

            const QString& const_() const { return _const_; }
            const QString& title() const { return _title; }
        };

        QList<AnyOfItem> _anyOf{};  //!< Array of enum options with values and display labels.

        Items& anyOf(const QList<AnyOfItem> & v) { _anyOf = v; return *this; }
        Items& addAnyOf(const AnyOfItem & v) { _anyOf.append(v); return *this; }

        const QList<AnyOfItem>& anyOf() const { return _anyOf; }
    };

    std::optional<QStringList> _default_{};  //!< Optional default value.
    std::optional<QString> _description{};  //!< Optional description for the enum field.
    Items _items{};  //!< Schema for array items with enum options and display labels.
    std::optional<int> _maxItems{};  //!< Maximum number of items to select.
    std::optional<int> _minItems{};  //!< Minimum number of items to select.
    std::optional<QString> _title{};  //!< Optional title for the enum field.

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
        QString _const_{};  //!< The enum value.
        QString _title{};  //!< Display label for this option.

        OneOfItem& const_(const QString & v) { _const_ = v; return *this; }
        OneOfItem& title(const QString & v) { _title = v; return *this; }

        const QString& const_() const { return _const_; }
        const QString& title() const { return _title; }
    };

    std::optional<QString> _default_{};  //!< Optional default value.
    std::optional<QString> _description{};  //!< Optional description for the enum field.
    QList<OneOfItem> _oneOf{};  //!< Array of enum options with values and display labels.
    std::optional<QString> _title{};  //!< Optional title for the enum field.

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
        QStringList _enum_{};  //!< Array of enum values to choose from.

        Items& enum_(const QStringList & v) { _enum_ = v; return *this; }
        Items& addEnum(const QString & v) { _enum_.append(v); return *this; }

        const QStringList& enum_() const { return _enum_; }
    };

    std::optional<QStringList> _default_{};  //!< Optional default value.
    std::optional<QString> _description{};  //!< Optional description for the enum field.
    Items _items{};  //!< Schema for the array items.
    std::optional<int> _maxItems{};  //!< Maximum number of items to select.
    std::optional<int> _minItems{};  //!< Minimum number of items to select.
    std::optional<QString> _title{};  //!< Optional title for the enum field.

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
    std::optional<QString> _default_{};  //!< Optional default value.
    std::optional<QString> _description{};  //!< Optional description for the enum field.
    QStringList _enum_{};  //!< Array of enum values to choose from.
    std::optional<QString> _title{};  //!< Optional title for the enum field.

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
     * A restricted subset of JSON Schema.
     * Only top-level properties are allowed, without nesting.
     */
    struct RequestedSchema {
        std::optional<QString> _dollarschema{};
        QMap<QString, PrimitiveSchemaDefinition> _properties{};
        std::optional<QStringList> _required{};

        RequestedSchema& dollarschema(const std::optional<QString> & v) { _dollarschema = v; return *this; }
        RequestedSchema& properties(const QMap<QString, PrimitiveSchemaDefinition> & v) { _properties = v; return *this; }
        RequestedSchema& addProperty(const QString &key, const PrimitiveSchemaDefinition & v) { _properties[key] = v; return *this; }
        RequestedSchema& required(const std::optional<QStringList> & v) { _required = v; return *this; }
        RequestedSchema& addRequired(const QString & v) { if (!_required) _required = QStringList{}; (*_required).append(v); return *this; }

        const std::optional<QString>& dollarschema() const { return _dollarschema; }
        const QMap<QString, PrimitiveSchemaDefinition>& properties() const { return _properties; }
        const std::optional<QStringList>& required() const { return _required; }
    };

    QString _message{};  //!< The message to present to the user describing what information is being requested.
    /**
     * A restricted subset of JSON Schema.
     * Only top-level properties are allowed, without nesting.
     */
    RequestedSchema _requestedSchema{};

    ElicitRequestFormParams& message(const QString & v) { _message = v; return *this; }
    ElicitRequestFormParams& requestedSchema(const RequestedSchema & v) { _requestedSchema = v; return *this; }

    const QString& message() const { return _message; }
    const RequestedSchema& requestedSchema() const { return _requestedSchema; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequestFormParams::RequestedSchema> fromJson<ElicitRequestFormParams::RequestedSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequestFormParams::RequestedSchema &data);

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequestFormParams> fromJson<ElicitRequestFormParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequestFormParams &data);

/** The parameters for a request to elicit information from the user via a URL in the client. */
struct ElicitRequestURLParams {
    QString _message{};  //!< The message to present to the user explaining why the interaction is needed.
    QString _url{};  //!< The URL that the user should navigate to.

    ElicitRequestURLParams& message(const QString & v) { _message = v; return *this; }
    ElicitRequestURLParams& url(const QString & v) { _url = v; return *this; }

    const QString& message() const { return _message; }
    const QString& url() const { return _url; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequestURLParams> fromJson<ElicitRequestURLParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequestURLParams &data);

/** The parameters for a request to elicit additional information from the user via the client. */
using ElicitRequestParams = std::variant<ElicitRequestFormParams, ElicitRequestURLParams>;

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
    ElicitRequestParams _params{};

    ElicitRequest& params(const ElicitRequestParams & v) { _params = v; return *this; }

    const ElicitRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ElicitRequest> fromJson<ElicitRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ElicitRequest &data);

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
    struct Params {
        std::optional<MetaObject> __meta{};

        Params& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }

        const std::optional<MetaObject>& _meta() const { return __meta; }
    };

    std::optional<Params> _params{};

    ListRootsRequest& params(const std::optional<Params> & v) { _params = v; return *this; }

    const std::optional<Params>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListRootsRequest::Params> fromJson<ListRootsRequest::Params>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListRootsRequest::Params &data);

template<>
MCPSERVER_EXPORT Utils::Result<ListRootsRequest> fromJson<ListRootsRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListRootsRequest &data);

using InputRequest = std::variant<CreateMessageRequest, ListRootsRequest, ElicitRequest>;

template<>
MCPSERVER_EXPORT Utils::Result<InputRequest> fromJson<InputRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InputRequest &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const InputRequest &val);

/** Returns the 'method' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const InputRequest &val);

using InputRequests = QMap<QString, InputRequest>;
template<>
MCPSERVER_EXPORT Utils::Result<InputRequests> fromJson<InputRequests>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InputRequests &data);

/**
 * An InputRequiredResult sent by the server to indicate that additional input is needed
 * before the request can be completed.
 *
 * At least one of `inputRequests` or `requestState` MUST be present.
 */
struct InputRequiredResult {
    std::optional<ResultMetaObject> __meta{};
    std::optional<InputRequests> _inputRequests{};
    std::optional<QString> _requestState{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};

    InputRequiredResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    InputRequiredResult& inputRequests(const std::optional<InputRequests> & v) { _inputRequests = v; return *this; }
    InputRequiredResult& requestState(const std::optional<QString> & v) { _requestState = v; return *this; }
    InputRequiredResult& resultType(const QString & v) { _resultType = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const std::optional<InputRequests>& inputRequests() const { return _inputRequests; }
    const std::optional<QString>& requestState() const { return _requestState; }
    const QString& resultType() const { return _resultType; }
};

template<>
MCPSERVER_EXPORT Utils::Result<InputRequiredResult> fromJson<InputRequiredResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InputRequiredResult &data);

using CallToolResultResponseResult = std::variant<InputRequiredResult, CallToolResult>;

template<>
MCPSERVER_EXPORT Utils::Result<CallToolResultResponseResult> fromJson<CallToolResultResponseResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const CallToolResultResponseResult &val);

/** A successful response from the server for a {@link CallToolRequesttools/call} request. */
struct CallToolResultResponse {
    RequestId _id{};
    CallToolResultResponseResult _result{};

    CallToolResultResponse& id(const RequestId & v) { _id = v; return *this; }
    CallToolResultResponse& result(const CallToolResultResponseResult & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const CallToolResultResponseResult& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CallToolResultResponse> fromJson<CallToolResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CallToolResultResponse &data);

/**
 * Extends {@link MetaObject} with additional notification-specific fields. All key naming rules from `MetaObject` apply.
 */
struct NotificationMetaObject {
    /**
     * Identifies the subscription stream a notification was delivered on. The
     * server MUST include this key on every notification delivered via a
     * {@link SubscriptionsListenRequestsubscriptions/listen} stream, so the
     * client can correlate the notification with the originating subscription.
     * The key is absent on notifications not delivered via a subscription
     * stream (e.g. progress notifications for an in-flight request), which is
     * why it is optional here.
     *
     * The value is the JSON-RPC ID of the `subscriptions/listen` request that
     * opened the stream.
     */
    std::optional<RequestId> _iodotmodelcontextprotocolslashsubscriptionId{};

    NotificationMetaObject& iodotmodelcontextprotocolslashsubscriptionId(const std::optional<RequestId> & v) { _iodotmodelcontextprotocolslashsubscriptionId = v; return *this; }

    const std::optional<RequestId>& iodotmodelcontextprotocolslashsubscriptionId() const { return _iodotmodelcontextprotocolslashsubscriptionId; }
};

template<>
MCPSERVER_EXPORT Utils::Result<NotificationMetaObject> fromJson<NotificationMetaObject>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const NotificationMetaObject &data);

/** Parameters for a `notifications/cancelled` notification. */
struct CancelledNotificationParams {
    std::optional<NotificationMetaObject> __meta{};
    std::optional<QString> _reason{};  //!< An optional string describing the reason for the cancellation. This MAY be logged or presented to the user.
    /**
     * The ID of the request to cancel.
     *
     * This MUST correspond to the ID of a request the client previously issued.
     */
    RequestId _requestId{};

    CancelledNotificationParams& _meta(const std::optional<NotificationMetaObject> & v) { __meta = v; return *this; }
    CancelledNotificationParams& reason(const std::optional<QString> & v) { _reason = v; return *this; }
    CancelledNotificationParams& requestId(const RequestId & v) { _requestId = v; return *this; }

    const std::optional<NotificationMetaObject>& _meta() const { return __meta; }
    const std::optional<QString>& reason() const { return _reason; }
    const RequestId& requestId() const { return _requestId; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CancelledNotificationParams> fromJson<CancelledNotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CancelledNotificationParams &data);

/**
 * This notification is sent by the client to indicate that it is cancelling a request it previously issued.
 *
 * On stdio, the server also sends this notification, solely to terminate a {@link SubscriptionsListenRequestsubscriptions/listen} stream: it references the ID of the `subscriptions/listen` request that opened the stream. Servers MUST NOT use this notification to cancel any other request.
 *
 * The request SHOULD still be in-flight, but due to communication latency, it is always possible that this notification MAY arrive after the request has already finished.
 *
 * This notification indicates that the result will be unused, so any associated processing SHOULD cease.
 */
struct CancelledNotification {
    CancelledNotificationParams _params{};

    CancelledNotification& params(const CancelledNotificationParams & v) { _params = v; return *this; }

    const CancelledNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CancelledNotification> fromJson<CancelledNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CancelledNotification &data);

/**
 * This notification is sent by the client to indicate that it is cancelling a request it previously issued.
 *
 * On stdio, the server also sends this notification, solely to terminate a {@link SubscriptionsListenRequestsubscriptions/listen} stream: it references the ID of the `subscriptions/listen` request that opened the stream. Servers MUST NOT use this notification to cancel any other request.
 *
 * The request SHOULD still be in-flight, but due to communication latency, it is always possible that this notification MAY arrive after the request has already finished.
 *
 * This notification indicates that the result will be unused, so any associated processing SHOULD cease.
 */
struct ClientNotification {
    CancelledNotificationParams _params{};

    ClientNotification& params(const CancelledNotificationParams & v) { _params = v; return *this; }

    const CancelledNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ClientNotification> fromJson<ClientNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientNotification &data);

/** Identifies a prompt. */
struct PromptReference {
    QString _name{};  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title{};

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
    QString _uri{};  //!< The URI or URI template of the resource.

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
    /** The argument's information */
    struct Argument {
        QString _name{};  //!< The name of the argument
        QString _value{};  //!< The value of the argument to use for completion matching.

        Argument& name(const QString & v) { _name = v; return *this; }
        Argument& value(const QString & v) { _value = v; return *this; }

        const QString& name() const { return _name; }
        const QString& value() const { return _value; }
    };

    /** Additional, optional context for completions */
    struct Context {
        std::optional<QMap<QString, QString>> _arguments{};  //!< Previously-resolved variables in a URI template or prompt.

        Context& arguments(const std::optional<QMap<QString, QString>> & v) { _arguments = v; return *this; }
        Context& addArgument(const QString &key, const QString & v) { if (!_arguments) _arguments = QMap<QString, QString>{}; (*_arguments)[key] = v; return *this; }

        const std::optional<QMap<QString, QString>>& arguments() const { return _arguments; }
    };

    RequestMetaObject __meta{};
    Argument _argument{};  //!< The argument's information
    std::optional<Context> _context{};  //!< Additional, optional context for completions
    CompleteRequestParamsRef _ref{};

    CompleteRequestParams& _meta(const RequestMetaObject & v) { __meta = v; return *this; }
    CompleteRequestParams& argument(const Argument & v) { _argument = v; return *this; }
    CompleteRequestParams& context(const std::optional<Context> & v) { _context = v; return *this; }
    CompleteRequestParams& ref(const CompleteRequestParamsRef & v) { _ref = v; return *this; }

    const RequestMetaObject& _meta() const { return __meta; }
    const Argument& argument() const { return _argument; }
    const std::optional<Context>& context() const { return _context; }
    const CompleteRequestParamsRef& ref() const { return _ref; }
};

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
    RequestId _id{};
    CompleteRequestParams _params{};

    CompleteRequest& id(const RequestId & v) { _id = v; return *this; }
    CompleteRequest& params(const CompleteRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const CompleteRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CompleteRequest> fromJson<CompleteRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteRequest &data);

/** Common params for any request. */
struct RequestParams {
    RequestMetaObject __meta{};

    RequestParams& _meta(const RequestMetaObject & v) { __meta = v; return *this; }

    const RequestMetaObject& _meta() const { return __meta; }
};

template<>
MCPSERVER_EXPORT Utils::Result<RequestParams> fromJson<RequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const RequestParams &data);

/**
 * A request from the client asking the server to advertise its supported
 * protocol versions, capabilities, and other metadata. Servers **MUST**
 * implement `server/discover`. Clients **MAY** call it but are not required
 * to — version negotiation can also happen inline via per-request `_meta`.
 */
struct DiscoverRequest {
    RequestId _id{};
    RequestParams _params{};

    DiscoverRequest& id(const RequestId & v) { _id = v; return *this; }
    DiscoverRequest& params(const RequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const RequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<DiscoverRequest> fromJson<DiscoverRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const DiscoverRequest &data);

/** Parameters for a `prompts/get` request. */
struct GetPromptRequestParams {
    RequestMetaObject __meta{};
    std::optional<QMap<QString, QString>> _arguments{};  //!< Arguments to use for templating the prompt.
    std::optional<InputResponses> _inputResponses{};
    QString _name{};  //!< The name of the prompt or prompt template.
    std::optional<QString> _requestState{};

    GetPromptRequestParams& _meta(const RequestMetaObject & v) { __meta = v; return *this; }
    GetPromptRequestParams& arguments(const std::optional<QMap<QString, QString>> & v) { _arguments = v; return *this; }
    GetPromptRequestParams& addArgument(const QString &key, const QString & v) { if (!_arguments) _arguments = QMap<QString, QString>{}; (*_arguments)[key] = v; return *this; }
    GetPromptRequestParams& inputResponses(const std::optional<InputResponses> & v) { _inputResponses = v; return *this; }
    GetPromptRequestParams& name(const QString & v) { _name = v; return *this; }
    GetPromptRequestParams& requestState(const std::optional<QString> & v) { _requestState = v; return *this; }

    const RequestMetaObject& _meta() const { return __meta; }
    const std::optional<QMap<QString, QString>>& arguments() const { return _arguments; }
    const std::optional<InputResponses>& inputResponses() const { return _inputResponses; }
    const QString& name() const { return _name; }
    const std::optional<QString>& requestState() const { return _requestState; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetPromptRequestParams> fromJson<GetPromptRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetPromptRequestParams &data);

/** Used by the client to get a prompt provided by the server. */
struct GetPromptRequest {
    RequestId _id{};
    GetPromptRequestParams _params{};

    GetPromptRequest& id(const RequestId & v) { _id = v; return *this; }
    GetPromptRequest& params(const GetPromptRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const GetPromptRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetPromptRequest> fromJson<GetPromptRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetPromptRequest &data);

/** Common params for paginated requests. */
struct PaginatedRequestParams {
    RequestMetaObject __meta{};
    /**
     * An opaque token representing the current pagination position.
     * If provided, the server should return results starting after this cursor.
     */
    std::optional<QString> _cursor{};

    PaginatedRequestParams& _meta(const RequestMetaObject & v) { __meta = v; return *this; }
    PaginatedRequestParams& cursor(const std::optional<QString> & v) { _cursor = v; return *this; }

    const RequestMetaObject& _meta() const { return __meta; }
    const std::optional<QString>& cursor() const { return _cursor; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PaginatedRequestParams> fromJson<PaginatedRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PaginatedRequestParams &data);

/** Sent from the client to request a list of prompts and prompt templates the server has. */
struct ListPromptsRequest {
    RequestId _id{};
    PaginatedRequestParams _params{};

    ListPromptsRequest& id(const RequestId & v) { _id = v; return *this; }
    ListPromptsRequest& params(const PaginatedRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const PaginatedRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListPromptsRequest> fromJson<ListPromptsRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListPromptsRequest &data);

/** Sent from the client to request a list of resource templates the server has. */
struct ListResourceTemplatesRequest {
    RequestId _id{};
    PaginatedRequestParams _params{};

    ListResourceTemplatesRequest& id(const RequestId & v) { _id = v; return *this; }
    ListResourceTemplatesRequest& params(const PaginatedRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const PaginatedRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListResourceTemplatesRequest> fromJson<ListResourceTemplatesRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListResourceTemplatesRequest &data);

/** Sent from the client to request a list of resources the server has. */
struct ListResourcesRequest {
    RequestId _id{};
    PaginatedRequestParams _params{};

    ListResourcesRequest& id(const RequestId & v) { _id = v; return *this; }
    ListResourcesRequest& params(const PaginatedRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const PaginatedRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListResourcesRequest> fromJson<ListResourcesRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListResourcesRequest &data);

/** Sent from the client to request a list of tools the server has. */
struct ListToolsRequest {
    RequestId _id{};
    PaginatedRequestParams _params{};

    ListToolsRequest& id(const RequestId & v) { _id = v; return *this; }
    ListToolsRequest& params(const PaginatedRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const PaginatedRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListToolsRequest> fromJson<ListToolsRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListToolsRequest &data);

/** Parameters for a `resources/read` request. */
struct ReadResourceRequestParams {
    RequestMetaObject __meta{};
    std::optional<InputResponses> _inputResponses{};
    std::optional<QString> _requestState{};
    QString _uri{};  //!< The URI of the resource. The URI can use any protocol; it is up to the server how to interpret it.

    ReadResourceRequestParams& _meta(const RequestMetaObject & v) { __meta = v; return *this; }
    ReadResourceRequestParams& inputResponses(const std::optional<InputResponses> & v) { _inputResponses = v; return *this; }
    ReadResourceRequestParams& requestState(const std::optional<QString> & v) { _requestState = v; return *this; }
    ReadResourceRequestParams& uri(const QString & v) { _uri = v; return *this; }

    const RequestMetaObject& _meta() const { return __meta; }
    const std::optional<InputResponses>& inputResponses() const { return _inputResponses; }
    const std::optional<QString>& requestState() const { return _requestState; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ReadResourceRequestParams> fromJson<ReadResourceRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ReadResourceRequestParams &data);

/** Sent from the client to the server, to read a specific resource URI. */
struct ReadResourceRequest {
    RequestId _id{};
    ReadResourceRequestParams _params{};

    ReadResourceRequest& id(const RequestId & v) { _id = v; return *this; }
    ReadResourceRequest& params(const ReadResourceRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const ReadResourceRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ReadResourceRequest> fromJson<ReadResourceRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ReadResourceRequest &data);

/**
 * The set of notification types a client may opt in to on a
 * {@link SubscriptionsListenRequestsubscriptions/listen} request.
 *
 * Each notification type is **opt-in**; the server **MUST NOT** send
 * notification types the client has not explicitly requested here.
 */
struct SubscriptionFilter {
    std::optional<bool> _promptsListChanged{};  //!< If true, receive {@link PromptListChangedNotificationnotifications/prompts/list_changed}.
    /**
     * Subscribe to {@link ResourceUpdatedNotificationnotifications/resources/updated} for these resource URIs.
     * Replaces the former `resources/subscribe` RPC.
     */
    std::optional<QStringList> _resourceSubscriptions{};
    std::optional<bool> _resourcesListChanged{};  //!< If true, receive {@link ResourceListChangedNotificationnotifications/resources/list_changed}.
    std::optional<bool> _toolsListChanged{};  //!< If true, receive {@link ToolListChangedNotificationnotifications/tools/list_changed}.

    SubscriptionFilter& promptsListChanged(std::optional<bool> v) { _promptsListChanged = v; return *this; }
    SubscriptionFilter& resourceSubscriptions(const std::optional<QStringList> & v) { _resourceSubscriptions = v; return *this; }
    SubscriptionFilter& addResourceSubscription(const QString & v) { if (!_resourceSubscriptions) _resourceSubscriptions = QStringList{}; (*_resourceSubscriptions).append(v); return *this; }
    SubscriptionFilter& resourcesListChanged(std::optional<bool> v) { _resourcesListChanged = v; return *this; }
    SubscriptionFilter& toolsListChanged(std::optional<bool> v) { _toolsListChanged = v; return *this; }

    const std::optional<bool>& promptsListChanged() const { return _promptsListChanged; }
    const std::optional<QStringList>& resourceSubscriptions() const { return _resourceSubscriptions; }
    const std::optional<bool>& resourcesListChanged() const { return _resourcesListChanged; }
    const std::optional<bool>& toolsListChanged() const { return _toolsListChanged; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SubscriptionFilter> fromJson<SubscriptionFilter>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscriptionFilter &data);

/** Parameters for a {@link SubscriptionsListenRequestsubscriptions/listen} request. */
struct SubscriptionsListenRequestParams {
    RequestMetaObject __meta{};
    /**
     * The notifications the client opts in to on this stream. The server
     * **MUST NOT** send notification types the client has not explicitly
     * requested.
     */
    SubscriptionFilter _notifications{};

    SubscriptionsListenRequestParams& _meta(const RequestMetaObject & v) { __meta = v; return *this; }
    SubscriptionsListenRequestParams& notifications(const SubscriptionFilter & v) { _notifications = v; return *this; }

    const RequestMetaObject& _meta() const { return __meta; }
    const SubscriptionFilter& notifications() const { return _notifications; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SubscriptionsListenRequestParams> fromJson<SubscriptionsListenRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscriptionsListenRequestParams &data);

/**
 * Sent from the client to open a long-lived channel for receiving notifications
 * outside the context of a specific request. Replaces the previous HTTP GET
 * endpoint and ensures consistent behavior between HTTP and STDIO.
 */
struct SubscriptionsListenRequest {
    RequestId _id{};
    SubscriptionsListenRequestParams _params{};

    SubscriptionsListenRequest& id(const RequestId & v) { _id = v; return *this; }
    SubscriptionsListenRequest& params(const SubscriptionsListenRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const SubscriptionsListenRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SubscriptionsListenRequest> fromJson<SubscriptionsListenRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscriptionsListenRequest &data);

using ClientRequest = std::variant<DiscoverRequest, ListResourcesRequest, ListResourceTemplatesRequest, ReadResourceRequest, SubscriptionsListenRequest, ListPromptsRequest, GetPromptRequest, ListToolsRequest, CallToolRequest, CompleteRequest>;

template<>
MCPSERVER_EXPORT Utils::Result<ClientRequest> fromJson<ClientRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ClientRequest &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ClientRequest &val);

/** Returns the 'method' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const ClientRequest &val);

/** Returns the 'id' field from the active variant. */
MCPSERVER_EXPORT RequestId id(const ClientRequest &val);

/** Common result fields. */
struct Result {
    std::optional<ResultMetaObject> __meta{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};
    QJsonObject _additionalProperties;  //!< additional properties

    Result& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    Result& resultType(const QString & v) { _resultType = v; return *this; }
    Result& additionalProperties(const QString &key, const QJsonValue &v) { _additionalProperties.insert(key, v); return *this; }
    Result& additionalProperties(const QJsonObject &obj) { for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) _additionalProperties.insert(it.key(), it.value()); return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const QString& resultType() const { return _resultType; }
    const QJsonObject& additionalProperties() const { return _additionalProperties; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Result> fromJson<Result>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Result &data);

using ClientResult = Result;

/** The result returned by the server for a {@link CompleteRequestcompletion/complete} request. */
struct CompleteResult {
    struct Completion {
        std::optional<bool> _hasMore{};  //!< Indicates whether there are additional completion options beyond those provided in the current response, even if the exact total is unknown.
        std::optional<int> _total{};  //!< The total number of completion options available. This can exceed the number of values actually sent in the response.
        QStringList _values{};  //!< An array of completion values. Must not exceed 100 items.

        Completion& hasMore(std::optional<bool> v) { _hasMore = v; return *this; }
        Completion& total(std::optional<int> v) { _total = v; return *this; }
        Completion& values(const QStringList & v) { _values = v; return *this; }
        Completion& addValue(const QString & v) { _values.append(v); return *this; }

        const std::optional<bool>& hasMore() const { return _hasMore; }
        const std::optional<int>& total() const { return _total; }
        const QStringList& values() const { return _values; }
    };

    std::optional<ResultMetaObject> __meta{};
    Completion _completion{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};

    CompleteResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    CompleteResult& completion(const Completion & v) { _completion = v; return *this; }
    CompleteResult& resultType(const QString & v) { _resultType = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const Completion& completion() const { return _completion; }
    const QString& resultType() const { return _resultType; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CompleteResult::Completion> fromJson<CompleteResult::Completion>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteResult::Completion &data);

template<>
MCPSERVER_EXPORT Utils::Result<CompleteResult> fromJson<CompleteResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteResult &data);

/** A successful response from the server for a {@link CompleteRequestcompletion/complete} request. */
struct CompleteResultResponse {
    RequestId _id{};
    CompleteResult _result{};

    CompleteResultResponse& id(const RequestId & v) { _id = v; return *this; }
    CompleteResultResponse& result(const CompleteResult & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const CompleteResult& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<CompleteResultResponse> fromJson<CompleteResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const CompleteResultResponse &data);

using Cursor = QString;
template<>
MCPSERVER_EXPORT Utils::Result<Cursor> fromJson<Cursor>(const QJsonValue &val);

/**
 * Capabilities that a server may support. Known capabilities are defined here, in this schema, but this is not a closed set: any server can define its own, additional capabilities.
 */
struct ServerCapabilities {
    /** Present if the server offers any prompt templates. */
    struct Prompts {
        std::optional<bool> _listChanged{};  //!< Whether this server supports notifications for changes to the prompt list.

        Prompts& listChanged(std::optional<bool> v) { _listChanged = v; return *this; }

        const std::optional<bool>& listChanged() const { return _listChanged; }
    };

    /** Present if the server offers any resources to read. */
    struct Resources {
        std::optional<bool> _listChanged{};  //!< Whether this server supports notifications for changes to the resource list.
        std::optional<bool> _subscribe{};  //!< Whether this server supports subscribing to resource updates.

        Resources& listChanged(std::optional<bool> v) { _listChanged = v; return *this; }
        Resources& subscribe(std::optional<bool> v) { _subscribe = v; return *this; }

        const std::optional<bool>& listChanged() const { return _listChanged; }
        const std::optional<bool>& subscribe() const { return _subscribe; }
    };

    /** Present if the server offers any tools to call. */
    struct Tools {
        std::optional<bool> _listChanged{};  //!< Whether this server supports notifications for changes to the tool list.

        Tools& listChanged(std::optional<bool> v) { _listChanged = v; return *this; }

        const std::optional<bool>& listChanged() const { return _listChanged; }
    };

    std::optional<QJsonObject> _completions{};  //!< Present if the server supports argument autocompletion suggestions.
    std::optional<QMap<QString, QJsonObject>> _experimental{};  //!< Experimental, non-standard capabilities that the server supports.
    /**
     * Optional MCP extensions that the server supports. Keys are extension identifiers
     * (e.g., "io.modelcontextprotocol/tasks"), and values are per-extension settings
     * objects. An empty object indicates support with no settings.
     *
     * Keys MUST follow the {@link MetaObject`_meta` key naming rules}, with a
     * mandatory prefix.
     */
    std::optional<QMap<QString, QJsonObject>> _extensions{};
    std::optional<QJsonObject> _logging{};  //!< Present if the server supports sending log messages to the client.
    std::optional<Prompts> _prompts{};  //!< Present if the server offers any prompt templates.
    std::optional<Resources> _resources{};  //!< Present if the server offers any resources to read.
    std::optional<Tools> _tools{};  //!< Present if the server offers any tools to call.

    ServerCapabilities& completions(const std::optional<QJsonObject> & v) { _completions = v; return *this; }
    ServerCapabilities& experimental(const std::optional<QMap<QString, QJsonObject>> & v) { _experimental = v; return *this; }
    ServerCapabilities& addExperimental(const QString &key, const QJsonObject & v) { if (!_experimental) _experimental = QMap<QString, QJsonObject>{}; (*_experimental)[key] = v; return *this; }
    ServerCapabilities& extensions(const std::optional<QMap<QString, QJsonObject>> & v) { _extensions = v; return *this; }
    ServerCapabilities& addExtension(const QString &key, const QJsonObject & v) { if (!_extensions) _extensions = QMap<QString, QJsonObject>{}; (*_extensions)[key] = v; return *this; }
    ServerCapabilities& logging(const std::optional<QJsonObject> & v) { _logging = v; return *this; }
    ServerCapabilities& prompts(const std::optional<Prompts> & v) { _prompts = v; return *this; }
    ServerCapabilities& resources(const std::optional<Resources> & v) { _resources = v; return *this; }
    ServerCapabilities& tools(const std::optional<Tools> & v) { _tools = v; return *this; }

    const std::optional<QJsonObject>& completions() const { return _completions; }
    const std::optional<QMap<QString, QJsonObject>>& experimental() const { return _experimental; }
    const std::optional<QMap<QString, QJsonObject>>& extensions() const { return _extensions; }
    const std::optional<QJsonObject>& logging() const { return _logging; }
    const std::optional<Prompts>& prompts() const { return _prompts; }
    const std::optional<Resources>& resources() const { return _resources; }
    const std::optional<Tools>& tools() const { return _tools; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities::Prompts> fromJson<ServerCapabilities::Prompts>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities::Prompts &data);

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities::Resources> fromJson<ServerCapabilities::Resources>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities::Resources &data);

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities::Tools> fromJson<ServerCapabilities::Tools>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities::Tools &data);

template<>
MCPSERVER_EXPORT Utils::Result<ServerCapabilities> fromJson<ServerCapabilities>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerCapabilities &data);

/** The result returned by the server for a {@link DiscoverRequestserver/discover} request. */
struct DiscoverResult {
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    enum class CacheScope {
        private_,
        public_
    };

    std::optional<ResultMetaObject> __meta{};
    CacheScope _cacheScope{};
    ServerCapabilities _capabilities{};  //!< The capabilities of the server.
    /**
     * Natural-language guidance describing the server and its features.
     *
     * This can be used by clients to improve an LLM's understanding of
     * available tools (e.g., by including it in a system prompt). It should
     * focus on information that helps the model use the server effectively
     * and should not duplicate information already in tool descriptions.
     */
    std::optional<QString> _instructions{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};
    /**
     * MCP Protocol Versions this server supports. The client should choose a
     * version from this list for use in subsequent requests.
     */
    QStringList _supportedVersions{};
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    int _ttlMs{};

    DiscoverResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    DiscoverResult& cacheScope(const CacheScope & v) { _cacheScope = v; return *this; }
    DiscoverResult& capabilities(const ServerCapabilities & v) { _capabilities = v; return *this; }
    DiscoverResult& instructions(const std::optional<QString> & v) { _instructions = v; return *this; }
    DiscoverResult& resultType(const QString & v) { _resultType = v; return *this; }
    DiscoverResult& supportedVersions(const QStringList & v) { _supportedVersions = v; return *this; }
    DiscoverResult& addSupportedVersion(const QString & v) { _supportedVersions.append(v); return *this; }
    DiscoverResult& ttlMs(int v) { _ttlMs = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const CacheScope& cacheScope() const { return _cacheScope; }
    const ServerCapabilities& capabilities() const { return _capabilities; }
    const std::optional<QString>& instructions() const { return _instructions; }
    const QString& resultType() const { return _resultType; }
    const QStringList& supportedVersions() const { return _supportedVersions; }
    const int& ttlMs() const { return _ttlMs; }
};

MCPSERVER_EXPORT QString toString(const DiscoverResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<DiscoverResult::CacheScope> fromJson<DiscoverResult::CacheScope>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const DiscoverResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<DiscoverResult> fromJson<DiscoverResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const DiscoverResult &data);

/** A successful response from the server for a {@link DiscoverRequestserver/discover} request. */
struct DiscoverResultResponse {
    RequestId _id{};
    DiscoverResult _result{};

    DiscoverResultResponse& id(const RequestId & v) { _id = v; return *this; }
    DiscoverResultResponse& result(const DiscoverResult & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const DiscoverResult& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<DiscoverResultResponse> fromJson<DiscoverResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const DiscoverResultResponse &data);

using EmptyResult = Result;

using EnumSchema = std::variant<UntitledSingleSelectEnumSchema, TitledSingleSelectEnumSchema, UntitledMultiSelectEnumSchema, TitledMultiSelectEnumSchema, LegacyTitledEnumSchema>;

template<>
MCPSERVER_EXPORT Utils::Result<EnumSchema> fromJson<EnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const EnumSchema &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const EnumSchema &val);

struct Error {
    int _code{};  //!< The error type that occurred.
    std::optional<QJsonValue> _data{};  //!< Additional information about the error. The value of this member is defined by the sender (e.g. detailed error information, nested errors etc.).
    QString _message{};  //!< A short description of the error. The message SHOULD be limited to a concise single sentence.

    Error& code(int v) { _code = v; return *this; }
    Error& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }
    Error& message(const QString & v) { _message = v; return *this; }

    const int& code() const { return _code; }
    const std::optional<QJsonValue>& data() const { return _data; }
    const QString& message() const { return _message; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Error> fromJson<Error>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Error &data);

/**
 * Describes a message returned as part of a prompt.
 *
 * This is similar to {@link SamplingMessage}, but also supports the embedding of
 * resources from the MCP server.
 */
struct PromptMessage {
    ContentBlock _content{};
    Role _role{};

    PromptMessage& content(const ContentBlock & v) { _content = v; return *this; }
    PromptMessage& role(const Role & v) { _role = v; return *this; }

    const ContentBlock& content() const { return _content; }
    const Role& role() const { return _role; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PromptMessage> fromJson<PromptMessage>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PromptMessage &data);

/** The result returned by the server for a {@link GetPromptRequestprompts/get} request. */
struct GetPromptResult {
    std::optional<ResultMetaObject> __meta{};
    std::optional<QString> _description{};  //!< An optional description for the prompt.
    QList<PromptMessage> _messages{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};

    GetPromptResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    GetPromptResult& description(const std::optional<QString> & v) { _description = v; return *this; }
    GetPromptResult& messages(const QList<PromptMessage> & v) { _messages = v; return *this; }
    GetPromptResult& addMessage(const PromptMessage & v) { _messages.append(v); return *this; }
    GetPromptResult& resultType(const QString & v) { _resultType = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const std::optional<QString>& description() const { return _description; }
    const QList<PromptMessage>& messages() const { return _messages; }
    const QString& resultType() const { return _resultType; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetPromptResult> fromJson<GetPromptResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetPromptResult &data);

using GetPromptResultResponseResult = std::variant<InputRequiredResult, GetPromptResult>;

template<>
MCPSERVER_EXPORT Utils::Result<GetPromptResultResponseResult> fromJson<GetPromptResultResponseResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const GetPromptResultResponseResult &val);

/** A successful response from the server for a {@link GetPromptRequestprompts/get} request. */
struct GetPromptResultResponse {
    RequestId _id{};
    GetPromptResultResponseResult _result{};

    GetPromptResultResponse& id(const RequestId & v) { _id = v; return *this; }
    GetPromptResultResponse& result(const GetPromptResultResponseResult & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const GetPromptResultResponseResult& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<GetPromptResultResponse> fromJson<GetPromptResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const GetPromptResultResponse &data);

/**
 * Returned when a server rejects a request because the values in the HTTP
 * headers do not match the corresponding values in the request body, or
 * because required headers are missing or malformed. For HTTP, the response
 * status code MUST be `400 Bad Request`.
 */
struct HeaderMismatchError {
    Error _error{};
    std::optional<RequestId> _id{};

    HeaderMismatchError& error(const Error & v) { _error = v; return *this; }
    HeaderMismatchError& id(const std::optional<RequestId> & v) { _id = v; return *this; }

    const Error& error() const { return _error; }
    const std::optional<RequestId>& id() const { return _id; }
};

template<>
MCPSERVER_EXPORT Utils::Result<HeaderMismatchError> fromJson<HeaderMismatchError>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const HeaderMismatchError &data);

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
    std::optional<QList<Icon>> _icons{};

    Icons& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    Icons& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }

    const std::optional<QList<Icon>>& icons() const { return _icons; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Icons> fromJson<Icons>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Icons &data);

struct InputResponseRequestParams {
    RequestMetaObject __meta{};
    std::optional<InputResponses> _inputResponses{};
    std::optional<QString> _requestState{};

    InputResponseRequestParams& _meta(const RequestMetaObject & v) { __meta = v; return *this; }
    InputResponseRequestParams& inputResponses(const std::optional<InputResponses> & v) { _inputResponses = v; return *this; }
    InputResponseRequestParams& requestState(const std::optional<QString> & v) { _requestState = v; return *this; }

    const RequestMetaObject& _meta() const { return __meta; }
    const std::optional<InputResponses>& inputResponses() const { return _inputResponses; }
    const std::optional<QString>& requestState() const { return _requestState; }
};

template<>
MCPSERVER_EXPORT Utils::Result<InputResponseRequestParams> fromJson<InputResponseRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InputResponseRequestParams &data);

/**
 * A JSON-RPC error indicating that an internal error occurred on the receiver. This error is returned when the receiver encounters an unexpected condition that prevents it from fulfilling the request.
 */
struct InternalError {
    int _code{};  //!< The error type that occurred.
    std::optional<QJsonValue> _data{};  //!< Additional information about the error. The value of this member is defined by the sender (e.g. detailed error information, nested errors etc.).
    QString _message{};  //!< A short description of the error. The message SHOULD be limited to a concise single sentence.

    InternalError& code(int v) { _code = v; return *this; }
    InternalError& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }
    InternalError& message(const QString & v) { _message = v; return *this; }

    const int& code() const { return _code; }
    const std::optional<QJsonValue>& data() const { return _data; }
    const QString& message() const { return _message; }
};

template<>
MCPSERVER_EXPORT Utils::Result<InternalError> fromJson<InternalError>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InternalError &data);

/**
 * A JSON-RPC error indicating that the method parameters are invalid or malformed.
 *
 * In MCP, this error is returned in various contexts when request parameters fail validation:
 *
 * - **Tools**: Unknown tool name or invalid tool arguments
 * - **Prompts**: Unknown prompt name or missing required arguments
 * - **Pagination**: Invalid or expired cursor values
 * - **Logging**: Invalid log level
 * - **Elicitation**: Server requests an elicitation mode not declared in client capabilities
 * - **Sampling**: Missing tool result or tool results mixed with other content
 */
struct InvalidParamsError {
    int _code{};  //!< The error type that occurred.
    std::optional<QJsonValue> _data{};  //!< Additional information about the error. The value of this member is defined by the sender (e.g. detailed error information, nested errors etc.).
    QString _message{};  //!< A short description of the error. The message SHOULD be limited to a concise single sentence.

    InvalidParamsError& code(int v) { _code = v; return *this; }
    InvalidParamsError& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }
    InvalidParamsError& message(const QString & v) { _message = v; return *this; }

    const int& code() const { return _code; }
    const std::optional<QJsonValue>& data() const { return _data; }
    const QString& message() const { return _message; }
};

template<>
MCPSERVER_EXPORT Utils::Result<InvalidParamsError> fromJson<InvalidParamsError>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InvalidParamsError &data);

/**
 * A JSON-RPC error indicating that the request is not a valid request object. This error is returned when the message structure does not conform to the JSON-RPC 2.0 specification requirements for a request (e.g., missing required fields like `jsonrpc` or `method`, or using invalid types for these fields).
 */
struct InvalidRequestError {
    int _code{};  //!< The error type that occurred.
    std::optional<QJsonValue> _data{};  //!< Additional information about the error. The value of this member is defined by the sender (e.g. detailed error information, nested errors etc.).
    QString _message{};  //!< A short description of the error. The message SHOULD be limited to a concise single sentence.

    InvalidRequestError& code(int v) { _code = v; return *this; }
    InvalidRequestError& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }
    InvalidRequestError& message(const QString & v) { _message = v; return *this; }

    const int& code() const { return _code; }
    const std::optional<QJsonValue>& data() const { return _data; }
    const QString& message() const { return _message; }
};

template<>
MCPSERVER_EXPORT Utils::Result<InvalidRequestError> fromJson<InvalidRequestError>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const InvalidRequestError &data);

/** A response to a request that indicates an error occurred. */
struct JSONRPCErrorResponse {
    Error _error{};
    std::optional<RequestId> _id{};

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
    QString _method{};
    std::optional<QMap<QString, QJsonValue>> _params{};

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
    RequestId _id{};
    QString _method{};
    std::optional<QMap<QString, QJsonValue>> _params{};

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
    RequestId _id{};
    Result _result{};

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
    std::optional<QString> _description{};  //!< A human-readable description of the argument.
    QString _name{};  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    std::optional<bool> _required{};  //!< Whether this argument must be provided.
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title{};

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
    std::optional<MetaObject> __meta{};
    std::optional<QList<PromptArgument>> _arguments{};  //!< A list of arguments to use for templating the prompt.
    std::optional<QString> _description{};  //!< An optional description of what this prompt provides
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
    std::optional<QList<Icon>> _icons{};
    QString _name{};  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title{};

    Prompt& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    Prompt& arguments(const std::optional<QList<PromptArgument>> & v) { _arguments = v; return *this; }
    Prompt& addArgument(const PromptArgument & v) { if (!_arguments) _arguments = QList<PromptArgument>{}; (*_arguments).append(v); return *this; }
    Prompt& description(const std::optional<QString> & v) { _description = v; return *this; }
    Prompt& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    Prompt& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    Prompt& name(const QString & v) { _name = v; return *this; }
    Prompt& title(const std::optional<QString> & v) { _title = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const std::optional<QList<PromptArgument>>& arguments() const { return _arguments; }
    const std::optional<QString>& description() const { return _description; }
    const std::optional<QList<Icon>>& icons() const { return _icons; }
    const QString& name() const { return _name; }
    const std::optional<QString>& title() const { return _title; }
};

template<>
MCPSERVER_EXPORT Utils::Result<Prompt> fromJson<Prompt>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const Prompt &data);

/** The result returned by the server for a {@link ListPromptsRequestprompts/list} request. */
struct ListPromptsResult {
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    enum class CacheScope {
        private_,
        public_
    };

    std::optional<ResultMetaObject> __meta{};
    CacheScope _cacheScope{};
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor{};
    QList<Prompt> _prompts{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    int _ttlMs{};

    ListPromptsResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    ListPromptsResult& cacheScope(const CacheScope & v) { _cacheScope = v; return *this; }
    ListPromptsResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    ListPromptsResult& prompts(const QList<Prompt> & v) { _prompts = v; return *this; }
    ListPromptsResult& addPrompt(const Prompt & v) { _prompts.append(v); return *this; }
    ListPromptsResult& resultType(const QString & v) { _resultType = v; return *this; }
    ListPromptsResult& ttlMs(int v) { _ttlMs = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const CacheScope& cacheScope() const { return _cacheScope; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const QList<Prompt>& prompts() const { return _prompts; }
    const QString& resultType() const { return _resultType; }
    const int& ttlMs() const { return _ttlMs; }
};

MCPSERVER_EXPORT QString toString(const ListPromptsResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<ListPromptsResult::CacheScope> fromJson<ListPromptsResult::CacheScope>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ListPromptsResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<ListPromptsResult> fromJson<ListPromptsResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListPromptsResult &data);

/** A successful response from the server for a {@link ListPromptsRequestprompts/list} request. */
struct ListPromptsResultResponse {
    RequestId _id{};
    ListPromptsResult _result{};

    ListPromptsResultResponse& id(const RequestId & v) { _id = v; return *this; }
    ListPromptsResultResponse& result(const ListPromptsResult & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const ListPromptsResult& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListPromptsResultResponse> fromJson<ListPromptsResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListPromptsResultResponse &data);

/** A template description for resources available on the server. */
struct ResourceTemplate {
    std::optional<MetaObject> __meta{};
    std::optional<Annotations> _annotations{};  //!< Optional annotations for the client.
    /**
     * A description of what this template is for.
     *
     * This can be used by clients to improve the LLM's understanding of available resources. It can be thought of like a "hint" to the model.
     */
    std::optional<QString> _description{};
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
    std::optional<QList<Icon>> _icons{};
    std::optional<QString> _mimeType{};  //!< The MIME type for all resources that match this template. This should only be included if all resources matching this template have the same type.
    QString _name{};  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title{};
    QString _uriTemplate{};  //!< A URI template (according to RFC 6570) that can be used to construct resource URIs.

    ResourceTemplate& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    ResourceTemplate& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    ResourceTemplate& description(const std::optional<QString> & v) { _description = v; return *this; }
    ResourceTemplate& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    ResourceTemplate& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    ResourceTemplate& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    ResourceTemplate& name(const QString & v) { _name = v; return *this; }
    ResourceTemplate& title(const std::optional<QString> & v) { _title = v; return *this; }
    ResourceTemplate& uriTemplate(const QString & v) { _uriTemplate = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
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

/**
 * The result returned by the server for a {@link ListResourceTemplatesRequestresources/templates/list} request.
 */
struct ListResourceTemplatesResult {
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    enum class CacheScope {
        private_,
        public_
    };

    std::optional<ResultMetaObject> __meta{};
    CacheScope _cacheScope{};
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor{};
    QList<ResourceTemplate> _resourceTemplates{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    int _ttlMs{};

    ListResourceTemplatesResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    ListResourceTemplatesResult& cacheScope(const CacheScope & v) { _cacheScope = v; return *this; }
    ListResourceTemplatesResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    ListResourceTemplatesResult& resourceTemplates(const QList<ResourceTemplate> & v) { _resourceTemplates = v; return *this; }
    ListResourceTemplatesResult& addResourceTemplate(const ResourceTemplate & v) { _resourceTemplates.append(v); return *this; }
    ListResourceTemplatesResult& resultType(const QString & v) { _resultType = v; return *this; }
    ListResourceTemplatesResult& ttlMs(int v) { _ttlMs = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const CacheScope& cacheScope() const { return _cacheScope; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const QList<ResourceTemplate>& resourceTemplates() const { return _resourceTemplates; }
    const QString& resultType() const { return _resultType; }
    const int& ttlMs() const { return _ttlMs; }
};

MCPSERVER_EXPORT QString toString(const ListResourceTemplatesResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<ListResourceTemplatesResult::CacheScope> fromJson<ListResourceTemplatesResult::CacheScope>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ListResourceTemplatesResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<ListResourceTemplatesResult> fromJson<ListResourceTemplatesResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListResourceTemplatesResult &data);

/**
 * A successful response from the server for a {@link ListResourceTemplatesRequestresources/templates/list} request.
 */
struct ListResourceTemplatesResultResponse {
    RequestId _id{};
    ListResourceTemplatesResult _result{};

    ListResourceTemplatesResultResponse& id(const RequestId & v) { _id = v; return *this; }
    ListResourceTemplatesResultResponse& result(const ListResourceTemplatesResult & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const ListResourceTemplatesResult& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListResourceTemplatesResultResponse> fromJson<ListResourceTemplatesResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListResourceTemplatesResultResponse &data);

/** A known resource that the server is capable of reading. */
struct Resource {
    std::optional<MetaObject> __meta{};
    std::optional<Annotations> _annotations{};  //!< Optional annotations for the client.
    /**
     * A description of what this resource represents.
     *
     * This can be used by clients to improve the LLM's understanding of available resources. It can be thought of like a "hint" to the model.
     */
    std::optional<QString> _description{};
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
    std::optional<QList<Icon>> _icons{};
    std::optional<QString> _mimeType{};  //!< The MIME type of this resource, if known.
    QString _name{};  //!< Intended for programmatic or logical use, but used as a display name in past specs or fallback (if title isn't present).
    /**
     * The size of the raw resource content, in bytes (i.e., before base64 encoding or any tokenization), if known.
     *
     * This can be used by Hosts to display file sizes and estimate context window usage.
     */
    std::optional<int> _size{};
    /**
     * Intended for UI and end-user contexts — optimized to be human-readable and easily understood,
     * even by those unfamiliar with domain-specific terminology.
     *
     * If not provided, the name should be used for display (except for {@link Tool},
     * where `annotations.title` should be given precedence over using `name`,
     * if present).
     */
    std::optional<QString> _title{};
    QString _uri{};  //!< The URI of this resource.

    Resource& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    Resource& annotations(const std::optional<Annotations> & v) { _annotations = v; return *this; }
    Resource& description(const std::optional<QString> & v) { _description = v; return *this; }
    Resource& icons(const std::optional<QList<Icon>> & v) { _icons = v; return *this; }
    Resource& addIcon(const Icon & v) { if (!_icons) _icons = QList<Icon>{}; (*_icons).append(v); return *this; }
    Resource& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    Resource& name(const QString & v) { _name = v; return *this; }
    Resource& size(std::optional<int> v) { _size = v; return *this; }
    Resource& title(const std::optional<QString> & v) { _title = v; return *this; }
    Resource& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
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

/** The result returned by the server for a {@link ListResourcesRequestresources/list} request. */
struct ListResourcesResult {
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    enum class CacheScope {
        private_,
        public_
    };

    std::optional<ResultMetaObject> __meta{};
    CacheScope _cacheScope{};
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor{};
    QList<Resource> _resources{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    int _ttlMs{};

    ListResourcesResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    ListResourcesResult& cacheScope(const CacheScope & v) { _cacheScope = v; return *this; }
    ListResourcesResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    ListResourcesResult& resources(const QList<Resource> & v) { _resources = v; return *this; }
    ListResourcesResult& addResource(const Resource & v) { _resources.append(v); return *this; }
    ListResourcesResult& resultType(const QString & v) { _resultType = v; return *this; }
    ListResourcesResult& ttlMs(int v) { _ttlMs = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const CacheScope& cacheScope() const { return _cacheScope; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const QList<Resource>& resources() const { return _resources; }
    const QString& resultType() const { return _resultType; }
    const int& ttlMs() const { return _ttlMs; }
};

MCPSERVER_EXPORT QString toString(const ListResourcesResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<ListResourcesResult::CacheScope> fromJson<ListResourcesResult::CacheScope>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ListResourcesResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<ListResourcesResult> fromJson<ListResourcesResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListResourcesResult &data);

/** A successful response from the server for a {@link ListResourcesRequestresources/list} request. */
struct ListResourcesResultResponse {
    RequestId _id{};
    ListResourcesResult _result{};

    ListResourcesResultResponse& id(const RequestId & v) { _id = v; return *this; }
    ListResourcesResultResponse& result(const ListResourcesResult & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const ListResourcesResult& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListResourcesResultResponse> fromJson<ListResourcesResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListResourcesResultResponse &data);

/** The result returned by the server for a {@link ListToolsRequesttools/list} request. */
struct ListToolsResult {
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    enum class CacheScope {
        private_,
        public_
    };

    std::optional<ResultMetaObject> __meta{};
    CacheScope _cacheScope{};
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};
    QList<Tool> _tools{};
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    int _ttlMs{};

    ListToolsResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    ListToolsResult& cacheScope(const CacheScope & v) { _cacheScope = v; return *this; }
    ListToolsResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    ListToolsResult& resultType(const QString & v) { _resultType = v; return *this; }
    ListToolsResult& tools(const QList<Tool> & v) { _tools = v; return *this; }
    ListToolsResult& addTool(const Tool & v) { _tools.append(v); return *this; }
    ListToolsResult& ttlMs(int v) { _ttlMs = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const CacheScope& cacheScope() const { return _cacheScope; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const QString& resultType() const { return _resultType; }
    const QList<Tool>& tools() const { return _tools; }
    const int& ttlMs() const { return _ttlMs; }
};

MCPSERVER_EXPORT QString toString(const ListToolsResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<ListToolsResult::CacheScope> fromJson<ListToolsResult::CacheScope>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ListToolsResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<ListToolsResult> fromJson<ListToolsResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListToolsResult &data);

/** A successful response from the server for a {@link ListToolsRequesttools/list} request. */
struct ListToolsResultResponse {
    RequestId _id{};
    ListToolsResult _result{};

    ListToolsResultResponse& id(const RequestId & v) { _id = v; return *this; }
    ListToolsResultResponse& result(const ListToolsResult & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const ListToolsResult& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ListToolsResultResponse> fromJson<ListToolsResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ListToolsResultResponse &data);

/** Parameters for a `notifications/message` notification. */
struct LoggingMessageNotificationParams {
    std::optional<NotificationMetaObject> __meta{};
    QJsonValue _data{};  //!< The data to be logged, such as a string message or an object. Any JSON serializable type is allowed here.
    LoggingLevel _level{};  //!< The severity of this log message.
    std::optional<QString> _logger{};  //!< An optional name of the logger issuing this message.

    LoggingMessageNotificationParams& _meta(const std::optional<NotificationMetaObject> & v) { __meta = v; return *this; }
    LoggingMessageNotificationParams& data(const QJsonValue & v) { _data = v; return *this; }
    LoggingMessageNotificationParams& level(const LoggingLevel & v) { _level = v; return *this; }
    LoggingMessageNotificationParams& logger(const std::optional<QString> & v) { _logger = v; return *this; }

    const std::optional<NotificationMetaObject>& _meta() const { return __meta; }
    const QJsonValue& data() const { return _data; }
    const LoggingLevel& level() const { return _level; }
    const std::optional<QString>& logger() const { return _logger; }
};

template<>
MCPSERVER_EXPORT Utils::Result<LoggingMessageNotificationParams> fromJson<LoggingMessageNotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const LoggingMessageNotificationParams &data);

/**
 * JSONRPCNotification of a log message passed from server to client. The client opts in by setting `"io.modelcontextprotocol/logLevel"` in a request's `_meta`.
 */
struct LoggingMessageNotification {
    LoggingMessageNotificationParams _params{};

    LoggingMessageNotification& params(const LoggingMessageNotificationParams & v) { _params = v; return *this; }

    const LoggingMessageNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<LoggingMessageNotification> fromJson<LoggingMessageNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const LoggingMessageNotification &data);

/**
 * A JSON-RPC error indicating that the requested method does not exist or is not available.
 *
 * In MCP, a server returns this error when a client invokes a method the server does not implement — either a genuinely unknown method, or one gated behind a server capability the server did not advertise (e.g., calling `prompts/list` when the `prompts` capability was not advertised).
 *
 * A request that requires a client capability the client did not declare is signalled instead by {@link MissingRequiredClientCapabilityError} (`-32021`).
 */
struct MethodNotFoundError {
    int _code{};  //!< The error type that occurred.
    std::optional<QJsonValue> _data{};  //!< Additional information about the error. The value of this member is defined by the sender (e.g. detailed error information, nested errors etc.).
    QString _message{};  //!< A short description of the error. The message SHOULD be limited to a concise single sentence.

    MethodNotFoundError& code(int v) { _code = v; return *this; }
    MethodNotFoundError& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }
    MethodNotFoundError& message(const QString & v) { _message = v; return *this; }

    const int& code() const { return _code; }
    const std::optional<QJsonValue>& data() const { return _data; }
    const QString& message() const { return _message; }
};

template<>
MCPSERVER_EXPORT Utils::Result<MethodNotFoundError> fromJson<MethodNotFoundError>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const MethodNotFoundError &data);

/**
 * Returned when processing a request requires a capability the client did not
 * declare in `clientCapabilities`. For HTTP, the response status code MUST be
 * `400 Bad Request`.
 */
struct MissingRequiredClientCapabilityError {
    Error _error{};
    std::optional<RequestId> _id{};

    MissingRequiredClientCapabilityError& error(const Error & v) { _error = v; return *this; }
    MissingRequiredClientCapabilityError& id(const std::optional<RequestId> & v) { _id = v; return *this; }

    const Error& error() const { return _error; }
    const std::optional<RequestId>& id() const { return _id; }
};

template<>
MCPSERVER_EXPORT Utils::Result<MissingRequiredClientCapabilityError> fromJson<MissingRequiredClientCapabilityError>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const MissingRequiredClientCapabilityError &data);

using MultiSelectEnumSchema = std::variant<UntitledMultiSelectEnumSchema, TitledMultiSelectEnumSchema>;

template<>
MCPSERVER_EXPORT Utils::Result<MultiSelectEnumSchema> fromJson<MultiSelectEnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const MultiSelectEnumSchema &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const MultiSelectEnumSchema &val);

struct Notification {
    QString _method{};
    std::optional<QMap<QString, QJsonValue>> _params{};

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

/** Common params for any notification. */
struct NotificationParams {
    std::optional<NotificationMetaObject> __meta{};

    NotificationParams& _meta(const std::optional<NotificationMetaObject> & v) { __meta = v; return *this; }

    const std::optional<NotificationMetaObject>& _meta() const { return __meta; }
};

template<>
MCPSERVER_EXPORT Utils::Result<NotificationParams> fromJson<NotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const NotificationParams &data);

struct PaginatedRequest {
    RequestId _id{};
    QString _method{};
    PaginatedRequestParams _params{};

    PaginatedRequest& id(const RequestId & v) { _id = v; return *this; }
    PaginatedRequest& method(const QString & v) { _method = v; return *this; }
    PaginatedRequest& params(const PaginatedRequestParams & v) { _params = v; return *this; }

    const RequestId& id() const { return _id; }
    const QString& method() const { return _method; }
    const PaginatedRequestParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PaginatedRequest> fromJson<PaginatedRequest>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PaginatedRequest &data);

struct PaginatedResult {
    std::optional<ResultMetaObject> __meta{};
    /**
     * An opaque token representing the pagination position after the last returned result.
     * If present, there may be more results available.
     */
    std::optional<QString> _nextCursor{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};

    PaginatedResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    PaginatedResult& nextCursor(const std::optional<QString> & v) { _nextCursor = v; return *this; }
    PaginatedResult& resultType(const QString & v) { _resultType = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const std::optional<QString>& nextCursor() const { return _nextCursor; }
    const QString& resultType() const { return _resultType; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PaginatedResult> fromJson<PaginatedResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PaginatedResult &data);

/**
 * A JSON-RPC error indicating that invalid JSON was received by the server. This error is returned when the server cannot parse the JSON text of a message.
 */
struct ParseError {
    int _code{};  //!< The error type that occurred.
    std::optional<QJsonValue> _data{};  //!< Additional information about the error. The value of this member is defined by the sender (e.g. detailed error information, nested errors etc.).
    QString _message{};  //!< A short description of the error. The message SHOULD be limited to a concise single sentence.

    ParseError& code(int v) { _code = v; return *this; }
    ParseError& data(const std::optional<QJsonValue> & v) { _data = v; return *this; }
    ParseError& message(const QString & v) { _message = v; return *this; }

    const int& code() const { return _code; }
    const std::optional<QJsonValue>& data() const { return _data; }
    const QString& message() const { return _message; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ParseError> fromJson<ParseError>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ParseError &data);

/** Parameters for a {@link ProgressNotificationnotifications/progress} notification. */
struct ProgressNotificationParams {
    std::optional<NotificationMetaObject> __meta{};
    std::optional<QString> _message{};  //!< An optional message describing the current progress.
    double _progress{};  //!< The progress thus far. This should increase every time progress is made, even if the total is unknown.
    ProgressToken _progressToken{};  //!< The progress token which was given in the initial request, used to associate this notification with the request that is proceeding.
    std::optional<double> _total{};  //!< Total number of items to process (or total progress required), if known.

    ProgressNotificationParams& _meta(const std::optional<NotificationMetaObject> & v) { __meta = v; return *this; }
    ProgressNotificationParams& message(const std::optional<QString> & v) { _message = v; return *this; }
    ProgressNotificationParams& progress(double v) { _progress = v; return *this; }
    ProgressNotificationParams& progressToken(const ProgressToken & v) { _progressToken = v; return *this; }
    ProgressNotificationParams& total(std::optional<double> v) { _total = v; return *this; }

    const std::optional<NotificationMetaObject>& _meta() const { return __meta; }
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
    ProgressNotificationParams _params{};

    ProgressNotification& params(const ProgressNotificationParams & v) { _params = v; return *this; }

    const ProgressNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ProgressNotification> fromJson<ProgressNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ProgressNotification &data);

/**
 * An optional notification from the server to the client, informing it that the list of prompts it offers has changed. This is only delivered on a {@link SubscriptionsListenRequestsubscriptions/listen} stream when the client requested it via the `promptsListChanged` filter field.
 */
struct PromptListChangedNotification {
    std::optional<NotificationParams> _params{};

    PromptListChangedNotification& params(const std::optional<NotificationParams> & v) { _params = v; return *this; }

    const std::optional<NotificationParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<PromptListChangedNotification> fromJson<PromptListChangedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const PromptListChangedNotification &data);

/** The result returned by the server for a {@link ReadResourceRequestresources/read} request. */
struct ReadResourceResult {
    /**
     * Indicates the intended scope of the cached response, analogous to HTTP
     * `Cache-Control: public` vs `Cache-Control: private`.
     *
     * - `"public"`: The response does not contain user-specific data. Any
     * client or intermediary (e.g., shared gateway, caching proxy) MAY cache
     * the response and serve it across authorization contexts.
     * - `"private"`: The response MAY be cached and reused only within the
     * same authorization context. Caches MUST NOT be shared across
     * authorization contexts (e.g., a different access token requires a
     * different cache).
     */
    enum class CacheScope {
        private_,
        public_
    };

    std::optional<ResultMetaObject> __meta{};
    CacheScope _cacheScope{};
    QList<EmbeddedResourceResource> _contents{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};
    /**
     * A hint from the server indicating how long (in milliseconds) the
     * client MAY cache this response before re-fetching. Semantics are
     * analogous to HTTP Cache-Control max-age.
     *
     * - If 0, The response SHOULD be considered immediately stale,
     * The client MAY re-fetch every time the result is needed.
     * - If positive, the client SHOULD consider the result fresh for this many
     * milliseconds after receiving the response.
     */
    int _ttlMs{};

    ReadResourceResult& _meta(const std::optional<ResultMetaObject> & v) { __meta = v; return *this; }
    ReadResourceResult& cacheScope(const CacheScope & v) { _cacheScope = v; return *this; }
    ReadResourceResult& contents(const QList<EmbeddedResourceResource> & v) { _contents = v; return *this; }
    ReadResourceResult& addContent(const EmbeddedResourceResource & v) { _contents.append(v); return *this; }
    ReadResourceResult& resultType(const QString & v) { _resultType = v; return *this; }
    ReadResourceResult& ttlMs(int v) { _ttlMs = v; return *this; }

    const std::optional<ResultMetaObject>& _meta() const { return __meta; }
    const CacheScope& cacheScope() const { return _cacheScope; }
    const QList<EmbeddedResourceResource>& contents() const { return _contents; }
    const QString& resultType() const { return _resultType; }
    const int& ttlMs() const { return _ttlMs; }
};

MCPSERVER_EXPORT QString toString(const ReadResourceResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<ReadResourceResult::CacheScope> fromJson<ReadResourceResult::CacheScope>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ReadResourceResult::CacheScope &v);

template<>
MCPSERVER_EXPORT Utils::Result<ReadResourceResult> fromJson<ReadResourceResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ReadResourceResult &data);

using ReadResourceResultResponseResult = std::variant<InputRequiredResult, ReadResourceResult>;

template<>
MCPSERVER_EXPORT Utils::Result<ReadResourceResultResponseResult> fromJson<ReadResourceResultResponseResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ReadResourceResultResponseResult &val);

/** A successful response from the server for a {@link ReadResourceRequestresources/read} request. */
struct ReadResourceResultResponse {
    RequestId _id{};
    ReadResourceResultResponseResult _result{};

    ReadResourceResultResponse& id(const RequestId & v) { _id = v; return *this; }
    ReadResourceResultResponse& result(const ReadResourceResultResponseResult & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const ReadResourceResultResponseResult& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ReadResourceResultResponse> fromJson<ReadResourceResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ReadResourceResultResponse &data);

struct Request {
    QString _method{};
    std::optional<QMap<QString, QJsonValue>> _params{};

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
    std::optional<MetaObject> __meta{};
    std::optional<QString> _mimeType{};  //!< The MIME type of this resource, if known.
    QString _uri{};  //!< The URI of this resource.

    ResourceContents& _meta(const std::optional<MetaObject> & v) { __meta = v; return *this; }
    ResourceContents& mimeType(const std::optional<QString> & v) { _mimeType = v; return *this; }
    ResourceContents& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<MetaObject>& _meta() const { return __meta; }
    const std::optional<QString>& mimeType() const { return _mimeType; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceContents> fromJson<ResourceContents>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceContents &data);

/**
 * An optional notification from the server to the client, informing it that the list of resources it can read from has changed. This is only delivered on a {@link SubscriptionsListenRequestsubscriptions/listen} stream when the client requested it via the `resourcesListChanged` filter field.
 */
struct ResourceListChangedNotification {
    std::optional<NotificationParams> _params{};

    ResourceListChangedNotification& params(const std::optional<NotificationParams> & v) { _params = v; return *this; }

    const std::optional<NotificationParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceListChangedNotification> fromJson<ResourceListChangedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceListChangedNotification &data);

/** Common params for resource-related requests. */
struct ResourceRequestParams {
    RequestMetaObject __meta{};
    QString _uri{};  //!< The URI of the resource. The URI can use any protocol; it is up to the server how to interpret it.

    ResourceRequestParams& _meta(const RequestMetaObject & v) { __meta = v; return *this; }
    ResourceRequestParams& uri(const QString & v) { _uri = v; return *this; }

    const RequestMetaObject& _meta() const { return __meta; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceRequestParams> fromJson<ResourceRequestParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceRequestParams &data);

/** Parameters for a `notifications/resources/updated` notification. */
struct ResourceUpdatedNotificationParams {
    std::optional<NotificationMetaObject> __meta{};
    QString _uri{};  //!< The URI of the resource that has been updated. This might be a sub-resource of the one that the client actually subscribed to.

    ResourceUpdatedNotificationParams& _meta(const std::optional<NotificationMetaObject> & v) { __meta = v; return *this; }
    ResourceUpdatedNotificationParams& uri(const QString & v) { _uri = v; return *this; }

    const std::optional<NotificationMetaObject>& _meta() const { return __meta; }
    const QString& uri() const { return _uri; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceUpdatedNotificationParams> fromJson<ResourceUpdatedNotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceUpdatedNotificationParams &data);

/**
 * A notification from the server to the client, informing it that a resource has changed and may need to be read again. This is only sent for resources the client opted in to via the `resourceSubscriptions` field of a {@link SubscriptionsListenRequestsubscriptions/listen} request.
 */
struct ResourceUpdatedNotification {
    ResourceUpdatedNotificationParams _params{};

    ResourceUpdatedNotification& params(const ResourceUpdatedNotificationParams & v) { _params = v; return *this; }

    const ResourceUpdatedNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ResourceUpdatedNotification> fromJson<ResourceUpdatedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ResourceUpdatedNotification &data);

using ResultType = QString;

/**
 * Parameters for a {@link SubscriptionsAcknowledgedNotificationnotifications/subscriptions/acknowledged} notification.
 */
struct SubscriptionsAcknowledgedNotificationParams {
    std::optional<NotificationMetaObject> __meta{};
    /**
     * The subset of requested notification types the server agreed to honor.
     * Only includes notification types the server actually supports; if the
     * client requested an unsupported type (e.g., `promptsListChanged` when
     * the server has no prompts), it is omitted from this set.
     */
    SubscriptionFilter _notifications{};

    SubscriptionsAcknowledgedNotificationParams& _meta(const std::optional<NotificationMetaObject> & v) { __meta = v; return *this; }
    SubscriptionsAcknowledgedNotificationParams& notifications(const SubscriptionFilter & v) { _notifications = v; return *this; }

    const std::optional<NotificationMetaObject>& _meta() const { return __meta; }
    const SubscriptionFilter& notifications() const { return _notifications; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SubscriptionsAcknowledgedNotificationParams> fromJson<SubscriptionsAcknowledgedNotificationParams>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscriptionsAcknowledgedNotificationParams &data);

/**
 * Sent by the server to acknowledge that a
 * {@link SubscriptionsListenRequestsubscriptions/listen} subscription has been
 * established and to report which notification types it agreed to honor.
 *
 * This notification MUST be the first message the server sends carrying the
 * subscription's ID in `io.modelcontextprotocol/subscriptionId`. The server MUST
 * NOT send any notification on the subscription before acknowledging it. On
 * stdio, where every subscription shares one channel, this ordering is defined
 * per subscription ID and not per channel: messages belonging to other
 * subscriptions MAY be interleaved before it.
 */
struct SubscriptionsAcknowledgedNotification {
    SubscriptionsAcknowledgedNotificationParams _params{};

    SubscriptionsAcknowledgedNotification& params(const SubscriptionsAcknowledgedNotificationParams & v) { _params = v; return *this; }

    const SubscriptionsAcknowledgedNotificationParams& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SubscriptionsAcknowledgedNotification> fromJson<SubscriptionsAcknowledgedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscriptionsAcknowledgedNotification &data);

/**
 * An optional notification from the server to the client, informing it that the list of tools it offers has changed. This is only delivered on a {@link SubscriptionsListenRequestsubscriptions/listen} stream when the client requested it via the `toolsListChanged` filter field.
 */
struct ToolListChangedNotification {
    std::optional<NotificationParams> _params{};

    ToolListChangedNotification& params(const std::optional<NotificationParams> & v) { _params = v; return *this; }

    const std::optional<NotificationParams>& params() const { return _params; }
};

template<>
MCPSERVER_EXPORT Utils::Result<ToolListChangedNotification> fromJson<ToolListChangedNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ToolListChangedNotification &data);

using ServerNotification = std::variant<CancelledNotification, ProgressNotification, ResourceListChangedNotification, SubscriptionsAcknowledgedNotification, ResourceUpdatedNotification, PromptListChangedNotification, ToolListChangedNotification, LoggingMessageNotification>;

template<>
MCPSERVER_EXPORT Utils::Result<ServerNotification> fromJson<ServerNotification>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerNotification &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ServerNotification &val);

/** Returns the 'method' dispatch field value for the active variant. */
MCPSERVER_EXPORT QString dispatchValue(const ServerNotification &val);

/**
 * Extends {@link ResultMetaObject} with the subscription-stream identifier carried by a
 * {@link SubscriptionsListenResult}. All key naming rules from `MetaObject` apply.
 */
struct SubscriptionsListenResultMetaObject {
    /**
     * Identifies the server software producing the response. Servers SHOULD
     * include this field on every response unless specifically configured not
     * to do so.
     *
     * The {@link Implementation} schema requires `name` and `version`; other
     * fields are optional.
     *
     * The value is self-reported by the server and is not verified by the
     * protocol. It is intended for display, logging, and debugging. Clients
     * SHOULD NOT use it to change their behavior, and SHOULD NOT rely on it for
     * security decisions.
     */
    std::optional<Implementation> _iodotmodelcontextprotocolslashserverInfo{};
    /**
     * Identifies the subscription stream this response closes, so the client can
     * correlate it with the originating subscription — mirroring the same key on
     * the stream's notifications. The value is the JSON-RPC ID of the
     * `subscriptions/listen` request that opened the stream (and equals this
     * response's `id`).
     */
    RequestId _iodotmodelcontextprotocolslashsubscriptionId{};

    SubscriptionsListenResultMetaObject& iodotmodelcontextprotocolslashserverInfo(const std::optional<Implementation> & v) { _iodotmodelcontextprotocolslashserverInfo = v; return *this; }
    SubscriptionsListenResultMetaObject& iodotmodelcontextprotocolslashsubscriptionId(const RequestId & v) { _iodotmodelcontextprotocolslashsubscriptionId = v; return *this; }

    const std::optional<Implementation>& iodotmodelcontextprotocolslashserverInfo() const { return _iodotmodelcontextprotocolslashserverInfo; }
    const RequestId& iodotmodelcontextprotocolslashsubscriptionId() const { return _iodotmodelcontextprotocolslashsubscriptionId; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SubscriptionsListenResultMetaObject> fromJson<SubscriptionsListenResultMetaObject>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscriptionsListenResultMetaObject &data);

/**
 * The response to a {@link SubscriptionsListenRequestsubscriptions/listen}
 * request, signalling that the subscription has ended gracefully (for example,
 * during server shutdown). Because the listen stream is long-lived, this result
 * is sent only when the server tears the subscription down; an abrupt transport
 * close carries no response. The result body is otherwise empty.
 */
struct SubscriptionsListenResult {
    SubscriptionsListenResultMetaObject __meta{};
    /**
     * Indicates the type of the result, which allows the client to determine
     * how to parse the result object.
     *
     * Servers implementing this protocol version MUST include this field.
     * For backward compatibility, when a client receives a result from a
     * server implementing an earlier protocol version (which does not include
     * `resultType`), the client MUST treat the absent field as `"complete"`.
     */
    QString _resultType{};

    SubscriptionsListenResult& _meta(const SubscriptionsListenResultMetaObject & v) { __meta = v; return *this; }
    SubscriptionsListenResult& resultType(const QString & v) { _resultType = v; return *this; }

    const SubscriptionsListenResultMetaObject& _meta() const { return __meta; }
    const QString& resultType() const { return _resultType; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SubscriptionsListenResult> fromJson<SubscriptionsListenResult>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscriptionsListenResult &data);

using ServerResult = std::variant<Result, InputRequiredResult, DiscoverResult, ListResourcesResult, ListResourceTemplatesResult, ReadResourceResult, SubscriptionsListenResult, ListPromptsResult, GetPromptResult, ListToolsResult, CallToolResult, CompleteResult>;

template<>
MCPSERVER_EXPORT Utils::Result<ServerResult> fromJson<ServerResult>(const QJsonValue &val);

/** Returns the 'resultType' field from the active variant. */
MCPSERVER_EXPORT QString resultType(const ServerResult &val);

MCPSERVER_EXPORT QJsonObject toJson(const ServerResult &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const ServerResult &val);

using SingleSelectEnumSchema = std::variant<UntitledSingleSelectEnumSchema, TitledSingleSelectEnumSchema>;

template<>
MCPSERVER_EXPORT Utils::Result<SingleSelectEnumSchema> fromJson<SingleSelectEnumSchema>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SingleSelectEnumSchema &val);

MCPSERVER_EXPORT QJsonValue toJsonValue(const SingleSelectEnumSchema &val);

/**
 * A successful response from the server for a {@link SubscriptionsListenRequestsubscriptions/listen}
 * request, sent when the server tears the subscription down gracefully.
 */
struct SubscriptionsListenResultResponse {
    RequestId _id{};
    SubscriptionsListenResult _result{};

    SubscriptionsListenResultResponse& id(const RequestId & v) { _id = v; return *this; }
    SubscriptionsListenResultResponse& result(const SubscriptionsListenResult & v) { _result = v; return *this; }

    const RequestId& id() const { return _id; }
    const SubscriptionsListenResult& result() const { return _result; }
};

template<>
MCPSERVER_EXPORT Utils::Result<SubscriptionsListenResultResponse> fromJson<SubscriptionsListenResultResponse>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const SubscriptionsListenResultResponse &data);

/**
 * Returned when the request's protocol version is unknown to the server or
 * unsupported (e.g., a known experimental or draft version the server has
 * chosen not to implement). For HTTP, the response status code MUST be
 * `400 Bad Request`.
 */
struct UnsupportedProtocolVersionError {
    Error _error{};
    std::optional<RequestId> _id{};

    UnsupportedProtocolVersionError& error(const Error & v) { _error = v; return *this; }
    UnsupportedProtocolVersionError& id(const std::optional<RequestId> & v) { _id = v; return *this; }

    const Error& error() const { return _error; }
    const std::optional<RequestId>& id() const { return _id; }
};

template<>
MCPSERVER_EXPORT Utils::Result<UnsupportedProtocolVersionError> fromJson<UnsupportedProtocolVersionError>(const QJsonValue &val);

MCPSERVER_EXPORT QJsonObject toJson(const UnsupportedProtocolVersionError &data);

} // namespace Mcp::Generated::Schema::_2026_07_28
