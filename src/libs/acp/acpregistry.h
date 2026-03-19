/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 /Users/mtillmanns/projects/qt/qtc-work/master/src/plugins/mcpserver/.venv/bin/python3 \
  scripts/generate_cpp_from_schema.py \
  src/libs/acp/schema/registry.schema.json src/libs/acp/acpregistry.h --namespace Acp::Registry --cpp-output src/libs/acp/acpregistry.cpp --export-macro ACPLIB_EXPORT --export-header acp_global.h
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

namespace Acp::Registry {

template<typename T> Utils::Result<T> fromJson(const QJsonValue &val) = delete;

struct binaryTarget {
    QString _archive;  //!< URL to download archive (.zip, .tar.gz, .tgz, .tar.bz2, .tbz2, or raw binary). Installer formats (.dmg, .pkg, .deb, .rpm) are not supported.
    QString _cmd;  //!< Command to execute after extraction
    std::optional<QStringList> _args;  //!< Command line arguments
    std::optional<QMap<QString, QString>> _env;  //!< Environment variables

    binaryTarget& archive(const QString & v) { _archive = v; return *this; }
    binaryTarget& cmd(const QString & v) { _cmd = v; return *this; }
    binaryTarget& args(const std::optional<QStringList> & v) { _args = v; return *this; }
    binaryTarget& addArg(const QString & v) { if (!_args) _args = QStringList{}; (*_args).append(v); return *this; }
    binaryTarget& env(const std::optional<QMap<QString, QString>> & v) { _env = v; return *this; }
    binaryTarget& addEnv(const QString &key, const QString & v) { if (!_env) _env = QMap<QString, QString>{}; (*_env)[key] = v; return *this; }

    const QString& archive() const { return _archive; }
    const QString& cmd() const { return _cmd; }
    const std::optional<QStringList>& args() const { return _args; }
    const std::optional<QMap<QString, QString>>& env() const { return _env; }
};

template<>
ACPLIB_EXPORT Utils::Result<binaryTarget> fromJson<binaryTarget>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const binaryTarget &data);

struct binaryDistribution {
    std::optional<binaryTarget> _darwinminusaarch64;
    std::optional<binaryTarget> _darwinminusx86_64;
    std::optional<binaryTarget> _linuxminusaarch64;
    std::optional<binaryTarget> _linuxminusx86_64;
    std::optional<binaryTarget> _windowsminusaarch64;
    std::optional<binaryTarget> _windowsminusx86_64;

    binaryDistribution& darwinminusaarch64(const std::optional<binaryTarget> & v) { _darwinminusaarch64 = v; return *this; }
    binaryDistribution& darwinminusx86_64(const std::optional<binaryTarget> & v) { _darwinminusx86_64 = v; return *this; }
    binaryDistribution& linuxminusaarch64(const std::optional<binaryTarget> & v) { _linuxminusaarch64 = v; return *this; }
    binaryDistribution& linuxminusx86_64(const std::optional<binaryTarget> & v) { _linuxminusx86_64 = v; return *this; }
    binaryDistribution& windowsminusaarch64(const std::optional<binaryTarget> & v) { _windowsminusaarch64 = v; return *this; }
    binaryDistribution& windowsminusx86_64(const std::optional<binaryTarget> & v) { _windowsminusx86_64 = v; return *this; }

    const std::optional<binaryTarget>& darwinminusaarch64() const { return _darwinminusaarch64; }
    const std::optional<binaryTarget>& darwinminusx86_64() const { return _darwinminusx86_64; }
    const std::optional<binaryTarget>& linuxminusaarch64() const { return _linuxminusaarch64; }
    const std::optional<binaryTarget>& linuxminusx86_64() const { return _linuxminusx86_64; }
    const std::optional<binaryTarget>& windowsminusaarch64() const { return _windowsminusaarch64; }
    const std::optional<binaryTarget>& windowsminusx86_64() const { return _windowsminusx86_64; }
};

template<>
ACPLIB_EXPORT Utils::Result<binaryDistribution> fromJson<binaryDistribution>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const binaryDistribution &data);

struct packageDistribution {
    QString _package;  //!< Package name (with optional version)
    std::optional<QStringList> _args;  //!< Command line arguments
    std::optional<QMap<QString, QString>> _env;  //!< Environment variables

    packageDistribution& package(const QString & v) { _package = v; return *this; }
    packageDistribution& args(const std::optional<QStringList> & v) { _args = v; return *this; }
    packageDistribution& addArg(const QString & v) { if (!_args) _args = QStringList{}; (*_args).append(v); return *this; }
    packageDistribution& env(const std::optional<QMap<QString, QString>> & v) { _env = v; return *this; }
    packageDistribution& addEnv(const QString &key, const QString & v) { if (!_env) _env = QMap<QString, QString>{}; (*_env)[key] = v; return *this; }

    const QString& package() const { return _package; }
    const std::optional<QStringList>& args() const { return _args; }
    const std::optional<QMap<QString, QString>>& env() const { return _env; }
};

template<>
ACPLIB_EXPORT Utils::Result<packageDistribution> fromJson<packageDistribution>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const packageDistribution &data);

/** Schema for ACP agent registry entries */
struct ACPAgent {
    struct Distribution {
        std::optional<binaryDistribution> _binary;
        std::optional<packageDistribution> _npx;
        std::optional<packageDistribution> _uvx;

        Distribution& binary(const std::optional<binaryDistribution> & v) { _binary = v; return *this; }
        Distribution& npx(const std::optional<packageDistribution> & v) { _npx = v; return *this; }
        Distribution& uvx(const std::optional<packageDistribution> & v) { _uvx = v; return *this; }

        const std::optional<binaryDistribution>& binary() const { return _binary; }
        const std::optional<packageDistribution>& npx() const { return _npx; }
        const std::optional<packageDistribution>& uvx() const { return _uvx; }
    };

    QString _id;  //!< Unique agent identifier (lowercase, hyphens allowed)
    QString _name;  //!< Display name
    QString _version;  //!< Semantic version
    QString _description;  //!< Brief description of the agent
    std::optional<QString> _repository;  //!< Source code repository URL
    std::optional<QStringList> _authors;  //!< List of authors
    std::optional<QString> _license;  //!< SPDX license identifier or 'proprietary'
    std::optional<QString> _icon;  //!< Icon URL (set automatically by the build from the required icon.svg file)
    Distribution _distribution;

    ACPAgent& id(const QString & v) { _id = v; return *this; }
    ACPAgent& name(const QString & v) { _name = v; return *this; }
    ACPAgent& version(const QString & v) { _version = v; return *this; }
    ACPAgent& description(const QString & v) { _description = v; return *this; }
    ACPAgent& repository(const std::optional<QString> & v) { _repository = v; return *this; }
    ACPAgent& authors(const std::optional<QStringList> & v) { _authors = v; return *this; }
    ACPAgent& addAuthor(const QString & v) { if (!_authors) _authors = QStringList{}; (*_authors).append(v); return *this; }
    ACPAgent& license(const std::optional<QString> & v) { _license = v; return *this; }
    ACPAgent& icon(const std::optional<QString> & v) { _icon = v; return *this; }
    ACPAgent& distribution(const Distribution & v) { _distribution = v; return *this; }

    const QString& id() const { return _id; }
    const QString& name() const { return _name; }
    const QString& version() const { return _version; }
    const QString& description() const { return _description; }
    const std::optional<QString>& repository() const { return _repository; }
    const std::optional<QStringList>& authors() const { return _authors; }
    const std::optional<QString>& license() const { return _license; }
    const std::optional<QString>& icon() const { return _icon; }
    const Distribution& distribution() const { return _distribution; }
};

template<>
ACPLIB_EXPORT Utils::Result<ACPAgent::Distribution> fromJson<ACPAgent::Distribution>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ACPAgent::Distribution &data);

template<>
ACPLIB_EXPORT Utils::Result<ACPAgent> fromJson<ACPAgent>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ACPAgent &data);

/** Schema for the aggregated ACP agent registry index */
struct ACPAgentRegistry {
    QString _version;  //!< Registry schema version
    QList<ACPAgent> _agents;  //!< List of registered agents

    ACPAgentRegistry& version(const QString & v) { _version = v; return *this; }
    ACPAgentRegistry& agents(const QList<ACPAgent> & v) { _agents = v; return *this; }
    ACPAgentRegistry& addAgent(const ACPAgent & v) { _agents.append(v); return *this; }

    const QString& version() const { return _version; }
    const QList<ACPAgent>& agents() const { return _agents; }
};

template<>
ACPLIB_EXPORT Utils::Result<ACPAgentRegistry> fromJson<ACPAgentRegistry>(const QJsonValue &val);

ACPLIB_EXPORT QJsonObject toJson(const ACPAgentRegistry &data);

} // namespace Acp::Registry
