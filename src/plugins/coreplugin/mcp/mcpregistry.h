/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 C:\dev\bin\Python313\python.exe \
  scripts/generate_cpp_from_schema.py \
  src/tools/mcp-reg-aggregator/schema.json src/plugins/coreplugin/mcp/mcpregistry.h --namespace Core::McpRegistry --cpp-output src/plugins/coreplugin/mcp/mcpregistry.cpp --no-comments --read-only
*/
#pragma once

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

namespace Core::McpRegistry {

template<typename T> Utils::Result<T> fromJson(const QJsonValue &val) = delete;

struct Icon {
    enum class Theme {
        light,
        dark
    };

    QString _url;
    std::optional<QString> _data;
    std::optional<QString> _mime_type;
    std::optional<QStringList> _sizes;
    std::optional<Theme> _theme;

    const QString& url() const { return _url; }
    const std::optional<QString>& data() const { return _data; }
    const std::optional<QString>& mime_type() const { return _mime_type; }
    const std::optional<QStringList>& sizes() const { return _sizes; }
    const std::optional<Theme>& theme() const { return _theme; }
};

QString toString(const Icon::Theme &v);

template<>
Utils::Result<Icon::Theme> fromJson<Icon::Theme>(const QJsonValue &val);

template<>
Utils::Result<Icon> fromJson<Icon>(const QJsonValue &val);

struct Argument {
    enum class Type {
        positional,
        named
    };

    Type _type;
    std::optional<QString> _name;
    std::optional<QString> _value_hint;
    std::optional<QString> _description;
    bool _required;
    bool _repeated;
    std::optional<QString> _default_;
    std::optional<QString> _value;

    const Type& type() const { return _type; }
    const std::optional<QString>& name() const { return _name; }
    const std::optional<QString>& value_hint() const { return _value_hint; }
    const std::optional<QString>& description() const { return _description; }
    const bool& required() const { return _required; }
    const bool& repeated() const { return _repeated; }
    const std::optional<QString>& default_() const { return _default_; }
    const std::optional<QString>& value() const { return _value; }
};

QString toString(const Argument::Type &v);

template<>
Utils::Result<Argument::Type> fromJson<Argument::Type>(const QJsonValue &val);

template<>
Utils::Result<Argument> fromJson<Argument>(const QJsonValue &val);

struct KeyValueInput {
    QString _name;
    std::optional<QString> _description;
    bool _required;
    bool _secret;
    std::optional<QString> _default_;

    const QString& name() const { return _name; }
    const std::optional<QString>& description() const { return _description; }
    const bool& required() const { return _required; }
    const bool& secret() const { return _secret; }
    const std::optional<QString>& default_() const { return _default_; }
};

template<>
Utils::Result<KeyValueInput> fromJson<KeyValueInput>(const QJsonValue &val);

struct Package {
    enum class Registry_type {
        npm,
        pypi,
        oci,
        nuget,
        mcpb
    };

    enum class Transport_type {
        stdio,
        streamableminushttp,
        sse
    };

    Registry_type _registry_type;
    QString _identifier;
    std::optional<QString> _version;
    Transport_type _transport_type;
    std::optional<QString> _runtime_hint;
    std::optional<QList<Argument>> _runtime_arguments;
    std::optional<QList<Argument>> _package_arguments;
    std::optional<QList<KeyValueInput>> _headers;
    std::optional<QList<KeyValueInput>> _env_vars;

    const Registry_type& registry_type() const { return _registry_type; }
    const QString& identifier() const { return _identifier; }
    const std::optional<QString>& version() const { return _version; }
    const Transport_type& transport_type() const { return _transport_type; }
    const std::optional<QString>& runtime_hint() const { return _runtime_hint; }
    const std::optional<QList<Argument>>& runtime_arguments() const { return _runtime_arguments; }
    const std::optional<QList<Argument>>& package_arguments() const { return _package_arguments; }
    const std::optional<QList<KeyValueInput>>& headers() const { return _headers; }
    const std::optional<QList<KeyValueInput>>& env_vars() const { return _env_vars; }
};

QString toString(const Package::Registry_type &v);

template<>
Utils::Result<Package::Registry_type> fromJson<Package::Registry_type>(const QJsonValue &val);

QString toString(const Package::Transport_type &v);

template<>
Utils::Result<Package::Transport_type> fromJson<Package::Transport_type>(const QJsonValue &val);

template<>
Utils::Result<Package> fromJson<Package>(const QJsonValue &val);

struct Remote {
    enum class Type {
        streamableminushttp,
        sse
    };

    Type _type;
    QString _url;
    std::optional<QList<KeyValueInput>> _headers;

    const Type& type() const { return _type; }
    const QString& url() const { return _url; }
    const std::optional<QList<KeyValueInput>>& headers() const { return _headers; }
};

QString toString(const Remote::Type &v);

template<>
Utils::Result<Remote::Type> fromJson<Remote::Type>(const QJsonValue &val);

template<>
Utils::Result<Remote> fromJson<Remote>(const QJsonValue &val);

struct Server {
    enum class Status {
        active,
        deprecated,
        deleted
    };

    QString _name;
    std::optional<QString> _title;
    QString _description;
    QString _version;
    Status _status;
    std::optional<QString> _repository_url;
    std::optional<QString> _website_url;
    std::optional<QList<Icon>> _icons;
    std::optional<QList<Package>> _packages;
    std::optional<QList<Remote>> _remotes;

    const QString& name() const { return _name; }
    const std::optional<QString>& title() const { return _title; }
    const QString& description() const { return _description; }
    const QString& version() const { return _version; }
    const Status& status() const { return _status; }
    const std::optional<QString>& repository_url() const { return _repository_url; }
    const std::optional<QString>& website_url() const { return _website_url; }
    const std::optional<QList<Icon>>& icons() const { return _icons; }
    const std::optional<QList<Package>>& packages() const { return _packages; }
    const std::optional<QList<Remote>>& remotes() const { return _remotes; }
};

QString toString(const Server::Status &v);

template<>
Utils::Result<Server::Status> fromJson<Server::Status>(const QJsonValue &val);

template<>
Utils::Result<Server> fromJson<Server>(const QJsonValue &val);

struct McpRegistry {
    QString _generated_at;
    int _count;
    QList<Server> _servers;

    const QString& generated_at() const { return _generated_at; }
    const int& count() const { return _count; }
    const QList<Server>& servers() const { return _servers; }
};

template<>
Utils::Result<McpRegistry> fromJson<McpRegistry>(const QJsonValue &val);

} // namespace Core::McpRegistry
