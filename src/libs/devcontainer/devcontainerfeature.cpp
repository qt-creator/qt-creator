// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainerfeature.h"

#include "devcontainertr.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/stringutils.h>
#include <utils/utility.h>

#include <QLocale>
#include <QRegularExpression>
#include <QSet>

using namespace Utils;

namespace DevContainer {

Result<Feature> Feature::fromJson(
    const QByteArray &data, const JsonStringToString &jsonStringToString)
{
    const QByteArray cleanedInput = cleanJson(data);

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(cleanedInput, &error);
    if (error.error != QJsonParseError::NoError) {
        return ResultError(
            Tr::tr("Failed to parse the development container feature file: %1")
                .arg(error.errorString()));
    }

    if (!doc.isObject())
        return ResultError(Tr::tr("Invalid development container JSON file: expected an object."));

    QJsonObject json = doc.object();
    return Feature::fromJson(json, jsonStringToString);
}

Result<Feature> Feature::fromJson(
    const QJsonObject &obj, const JsonStringToString &jsonStringToString)
{
    Feature f;
    f.id = jsonStringToString(obj.value("id"));
    if (f.id.isEmpty())
        return ResultError(Tr::tr("The development container feature has no \"id\"."));
    f.version = jsonStringToString(obj.value("version"));

    f.description = jsonStringToString(obj.value("description"));
    f.documentationURL = jsonStringToString(obj.value("documentationURL"));
    f.entrypoint = jsonStringToString(obj.value("entrypoint"));
    f.licenseURL = jsonStringToString(obj.value("licenseURL"));
    f.name = jsonStringToString(obj.value("name"));

    for (const auto &val : obj.value("capAdd").toArray())
        f.capAdd.append(jsonStringToString(val));

    for (const auto &val : obj.value("keywords").toArray())
        f.keywords.append(jsonStringToString(val));

    for (const auto &val : obj.value("installsAfter").toArray())
        f.installsAfter.append(jsonStringToString(val));

    for (const auto &val : obj.value("securityOpt").toArray())
        f.securityOpt.append(jsonStringToString(val));

    for (const auto &val : obj.value("legacyIds").toArray())
        f.legacyIds.append(jsonStringToString(val));

    auto containerEnv = obj.value("containerEnv").toObject();
    for (auto it = containerEnv.begin(); it != containerEnv.end(); ++it)
        f.containerEnv[it.key()] = jsonStringToString(it.value());

    f.customizations = obj.value("customizations").toObject();

    if (obj.contains("dependsOn") && obj["dependsOn"].isObject()) {
        const QJsonObject depsObj = obj["dependsOn"].toObject();
        for (auto it = depsObj.begin(); it != depsObj.end(); ++it) {
            const Result<FeatureDependency> dep
                = FeatureDependency::fromJson(it.key(), it.value(), jsonStringToString);
            if (!dep)
                return ResultError(dep.error());
            f.dependsOn.push_back(*dep);
        }
    }

    const QJsonObject opts = obj.value("options").toObject();
    for (auto it = opts.begin(); it != opts.end(); ++it)
        f.options[it.key()] = FeatureOption::fromJson(it.value().toObject(), jsonStringToString);

    if (obj.contains("mounts") && obj["mounts"].isArray()) {
        const QJsonArray mountsArray = obj["mounts"].toArray();
        for (const QJsonValue &value : mountsArray) {
            const Result<std::variant<Mount, QString>> mount
                = Mount::fromJsonVariant(value, jsonStringToString);
            if (!mount)
                return ResultError(mount.error());
            f.mounts.push_back(*mount);
        }
    }

    f.init = obj.value("init").toBool();
    f.privileged = obj.value("privileged").toBool();
    f.deprecated = obj.value("deprecated").toBool();

    if (obj.contains("onCreateCommand"))
        f.onCreateCommand = parseCommand(obj["onCreateCommand"], jsonStringToString);
    if (obj.contains("updateContentCommand"))
        f.updateContentCommand = parseCommand(obj["updateContentCommand"], jsonStringToString);
    if (obj.contains("postCreateCommand"))
        f.postCreateCommand = parseCommand(obj["postCreateCommand"], jsonStringToString);
    if (obj.contains("postStartCommand"))
        f.postStartCommand = parseCommand(obj["postStartCommand"], jsonStringToString);
    if (obj.contains("postAttachCommand"))
        f.postAttachCommand = parseCommand(obj["postAttachCommand"], jsonStringToString);

    return f;
}

FeatureOption FeatureOption::fromJson(
    const QJsonObject &obj, const JsonStringToString &jsonStringToString)
{
    FeatureOption opt;
    opt.type = jsonStringToString(obj.value("type"));
    opt.description = jsonStringToString(obj.value("description"));
    opt.defaultValue = obj.value("default").toVariant();

    if (obj.contains("enum")) {
        const QJsonArray enumArray = obj.value("enum").toArray();
        for (const auto &e : enumArray)
            opt.enumValues.append(jsonStringToString(e));
    }
    if (obj.contains("proposals")) {
        const QJsonArray propArray = obj.value("proposals").toArray();
        for (const auto &p : propArray)
            opt.proposals.append(jsonStringToString(p));
    }

    return opt;
}

static bool isLoopbackHost(const QString &host)
{
    return host == "localhost" || host == "127.0.0.1" || host == "::1" || host == "[::1]";
}

Result<FeatureReference> FeatureReference::parse(
    const QString &reference, const FilePath &configFolder)
{
    FeatureReference ref;
    ref.userReference = reference.trimmed();
    const QString &str = ref.userReference;

    if (str.isEmpty())
        return ResultError(Tr::tr("The feature reference is empty."));

    if (str.startsWith("./") || str.startsWith("../")) {
        ref.type = Type::Local;
        ref.localPath = configFolder.resolvePath(str).cleanPath();
        ref.id = ref.localPath.fileName();
        return ref;
    }

    if (str.startsWith("https://") || str.startsWith("http://")) {
        // The install script runs as root, so its source must not be open to tampering.
        if (str.startsWith("http://") && !isLoopbackHost(QUrl(str).host())) {
            return ResultError(
                Tr::tr("The feature \"%1\" would be downloaded over plain HTTP. Use an HTTPS URL.")
                    .arg(str));
        }
        ref.type = Type::Tarball;
        ref.url = str;
        static const QRegularExpression fileNameRegex(R"(/devcontainer-feature-([^/]+)\.tgz$)");
        const QRegularExpressionMatch match = fileNameRegex.match(QString(str).section('?', 0, 0));
        if (!match.hasMatch()) {
            return ResultError(
                Tr::tr(
                    "The feature URL \"%1\" does not name a file "
                    "\"devcontainer-feature-<id>.tgz\".")
                    .arg(str));
        }
        ref.id = match.captured(1);
        return ref;
    }

    const auto unsupported = [&str] {
        return ResultError(
            Tr::tr(
                "\"%1\" is not a supported feature reference. Use an OCI reference such as "
                "\"ghcr.io/devcontainers/features/git:1\", a URL to a "
                "\"devcontainer-feature-<id>.tgz\" file, or a path starting with \"./\".")
                .arg(str));
    };

    QString rest = str;
    if (const int at = rest.indexOf('@'); at >= 0) {
        ref.digest = rest.mid(at + 1);
        ref.tag.clear();
        rest = rest.left(at);
        static const QRegularExpression digestRegex("^sha256:[0-9a-f]{64}$");
        if (!digestRegex.match(ref.digest).hasMatch())
            return unsupported();
    } else {
        const int lastSlash = rest.lastIndexOf('/');
        if (const int colon = rest.indexOf(':', lastSlash + 1); colon >= 0) {
            ref.tag = rest.mid(colon + 1);
            rest = rest.left(colon);
        }
    }

    const int firstSlash = rest.indexOf('/');
    if (firstSlash <= 0)
        return unsupported();

    ref.registry = rest.left(firstSlash);
    ref.repository = rest.mid(firstSlash + 1).toLower();

    // A registry is a host name, so a first path segment without a dot or a port is the
    // deprecated GitHub shorthand "owner/repo/feature".
    if (!ref.registry.contains('.') && !ref.registry.contains(':') && ref.registry != "localhost")
        return unsupported();

    static const QRegularExpression repositoryRegex("^[a-z0-9]+([._/-][a-z0-9]+)*$");
    static const QRegularExpression tagRegex("^[A-Za-z0-9_][A-Za-z0-9_.-]{0,127}$");
    if (!repositoryRegex.match(ref.repository).hasMatch())
        return unsupported();
    if (!ref.tag.isEmpty() && !tagRegex.match(ref.tag).hasMatch())
        return unsupported();

    ref.type = Type::Oci;
    ref.id = ref.repository.section('/', -1);
    return ref;
}

QString FeatureReference::resource() const
{
    switch (type) {
    case Type::Oci:
        return registry + '/' + repository;
    case Type::Tarball:
        return url;
    case Type::Local:
        return userReference;
    }
    return userReference;
}

QString featureKey(const FeatureReference &reference, const QJsonObject &options)
{
    return reference.resource() + '\n'
           + QString::fromUtf8(QJsonDocument(options).toJson(QJsonDocument::Compact));
}

QString ResolvedFeature::key() const
{
    return featureKey(reference, userOptions);
}

QJsonObject ResolvedFeature::effectiveOptions() const
{
    QJsonObject result;
    for (auto it = feature.options.cbegin(); it != feature.options.cend(); ++it) {
        if (it->defaultValue.isValid())
            result.insert(it.key(), QJsonValue::fromVariant(it->defaultValue));
    }
    for (auto it = userOptions.begin(); it != userOptions.end(); ++it)
        result.insert(it.key(), it.value());
    return result;
}

QString featureOptionEnvName(const QString &optionName)
{
    static const QRegularExpression invalidChars("[^\\w_]");
    static const QRegularExpression leadingDigits("^[\\d_]+");
    QString name = optionName;
    name.replace(invalidChars, "_");
    name.replace(leadingDigits, "_");
    return name.toUpper();
}

static QString shellQuoted(const QString &value)
{
    QString escaped = value;
    escaped.replace('\\', "\\\\");
    escaped.replace('"', "\\\"");
    escaped.replace('$', "\\$");
    escaped.replace('`', "\\`");
    return '"' + escaped + '"';
}

static QString optionValueToString(const QJsonValue &value)
{
    if (value.isBool())
        return value.toBool() ? QString("true") : QString("false");
    if (value.isDouble())
        return QString::number(value.toDouble(), 'g', QLocale::FloatingPointShortest);
    if (value.isString())
        return value.toString();
    if (value.isNull() || value.isUndefined())
        return {};
    return QString::fromUtf8(
        value.isArray() ? QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact)
                        : QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
}

QByteArray featureEnvFileContents(const ResolvedFeature &feature)
{
    QByteArray contents;
    const QJsonObject options = feature.effectiveOptions();
    for (auto it = options.begin(); it != options.end(); ++it) {
        contents += featureOptionEnvName(it.key()).toUtf8() + '='
                    + shellQuoted(optionValueToString(it.value())).toUtf8() + '\n';
    }
    return contents;
}

// Whether "reference", an entry of "installsAfter" or "overrideFeatureInstallOrder", names
// the feature. The reference may carry a version, which does not take part in the match.
static bool referencesFeature(const QString &reference, const ResolvedFeature &feature)
{
    const Result<FeatureReference> parsed = FeatureReference::parse(reference, {});
    const QString resource = parsed ? parsed->resource() : reference.trimmed();

    if (resource == feature.reference.resource())
        return true;

    if (feature.reference.type != FeatureReference::Type::Oci)
        return false;

    const QString nameSpace = feature.reference.registry + '/'
                              + feature.reference.repository.section('/', 0, -2);
    return Utils::anyOf(feature.feature.legacyIds, [&](const QString &legacyId) {
        return nameSpace + '/' + legacyId == resource;
    });
}

Result<std::vector<ResolvedFeature>> featureInstallOrder(
    std::vector<ResolvedFeature> features, const std::optional<QStringList> &overrideOrder)
{
    // https://containers.dev/implementors/features/#installation-order
    std::vector<ResolvedFeature> unique;
    QSet<QString> keys;
    for (ResolvedFeature &feature : features) {
        const QString key = feature.key();
        if (keys.contains(key))
            continue;
        keys.insert(key);
        unique.push_back(std::move(feature));
    }

    const size_t count = unique.size();
    std::vector<std::vector<size_t>> dependencies(count);
    std::vector<int> priority(count, 0);

    for (size_t i = 0; i < count; ++i) {
        const ResolvedFeature &feature = unique[i];

        for (const FeatureDependency &dependency : feature.feature.dependsOn) {
            const Result<FeatureReference> ref = FeatureReference::parse(dependency.id, {});
            if (!ref)
                return ResultError(ref.error());
            const QString key = featureKey(*ref, dependency.options);
            const auto it = std::find_if(unique.begin(), unique.end(), [&key](const auto &f) {
                return f.key() == key;
            });
            if (it == unique.end()) {
                return ResultError(
                    Tr::tr(
                        "The feature \"%1\" depends on \"%2\", which is "
                        "not available.")
                        .arg(feature.reference.userReference, dependency.id));
            }
            dependencies[i].push_back(size_t(it - unique.begin()));
        }

        for (const QString &after : feature.feature.installsAfter) {
            for (size_t j = 0; j < count; ++j) {
                if (j != i && referencesFeature(after, unique[j]))
                    dependencies[i].push_back(j);
            }
        }

        if (overrideOrder) {
            for (qsizetype k = 0; k < overrideOrder->size(); ++k) {
                if (referencesFeature(overrideOrder->at(k), feature)) {
                    priority[i] = (std::max) (priority[i], int(overrideOrder->size() - k));
                    break;
                }
            }
        }
    }

    std::vector<bool> installed(count, false);
    std::vector<ResolvedFeature> ordered;
    ordered.reserve(count);

    while (ordered.size() < count) {
        std::vector<size_t> candidates;
        for (size_t i = 0; i < count; ++i) {
            if (installed[i])
                continue;
            if (Utils::allOf(dependencies[i], [&installed](size_t d) { return installed[d]; }))
                candidates.push_back(i);
        }

        if (candidates.empty()) {
            QStringList remaining;
            for (size_t i = 0; i < count; ++i) {
                if (!installed[i])
                    remaining << unique[i].reference.userReference;
            }
            return ResultError(
                Tr::tr("The features %1 depend on each other in a cycle.").arg(remaining.join(", ")));
        }

        int maxPriority = 0;
        for (size_t c : candidates)
            maxPriority = (std::max) (maxPriority, priority[c]);
        std::erase_if(candidates, [&](size_t c) { return priority[c] != maxPriority; });

        std::sort(candidates.begin(), candidates.end(), [&unique](size_t a, size_t b) {
            return unique[a].key() < unique[b].key();
        });

        for (size_t c : candidates) {
            installed[c] = true;
            ordered.push_back(unique[c]);
        }
    }

    return ordered;
}

static QString dockerfileEnvValue(const QString &value)
{
    QString escaped = value;
    escaped.replace('\\', "\\\\");
    escaped.replace('"', "\\\"");
    return '"' + escaped + '"';
}

static QString userHomeLookup(const QString &variable)
{
    return QString(R"($(awk -F: -v u="$%1" '$1 == u || $3 == u { print $6; exit }' /etc/passwd))")
        .arg(variable);
}

static QByteArray builtinEnvFileContents(const FeatureBuildContext &context)
{
    const QString containerUser = context.containerUser.isEmpty() ? QString("root")
                                                                  : context.containerUser;
    const QString remoteUser = context.remoteUser.isEmpty() ? containerUser : context.remoteUser;

    // The file is sourced before each install script, so a user created by an earlier feature
    // already has its home directory here.
    const QString contents = QString(
                                 "_CONTAINER_USER=%1\n"
                                 "_REMOTE_USER=%2\n"
                                 "_CONTAINER_USER_HOME=\"%3\"\n"
                                 "_REMOTE_USER_HOME=\"%4\"\n")
                                 .arg(
                                     shellQuoted(containerUser),
                                     shellQuoted(remoteUser),
                                     userHomeLookup("_CONTAINER_USER"),
                                     userHomeLookup("_REMOTE_USER"));
    return contents.toUtf8();
}

QString featuresDockerfile(
    const std::vector<ResolvedFeature> &features, const FeatureBuildContext &context)
{
    QString dockerfile = "ARG _DEV_CONTAINERS_BASE_IMAGE=placeholder\n"
                         "FROM $_DEV_CONTAINERS_BASE_IMAGE\n"
                         "USER root\n"
                         "COPY ./dev-container-features/ /tmp/dev-container-features/\n";

    for (const ResolvedFeature &feature : features) {
        dockerfile += QString("\n# %1\n").arg(feature.reference.userReference);
        for (const auto &[key, value] : feature.feature.containerEnv)
            dockerfile += QString("ENV %1=%2\n").arg(key, dockerfileEnvValue(value));

        dockerfile += QString(
                          "RUN cd /tmp/dev-container-features/%1 \\\n"
                          "    && set -a \\\n"
                          "    && . ../devcontainer-features.builtin.env \\\n"
                          "    && . ./devcontainer-features.env \\\n"
                          "    && set +a \\\n"
                          "    && chmod +x ./install.sh \\\n"
                          "    && ./install.sh\n")
                          .arg(feature.folder.fileName());
    }

    if (!context.imageUser.isEmpty())
        dockerfile += QString("\nUSER %1\n").arg(context.imageUser);

    return dockerfile;
}

Result<FilePath> writeFeaturesBuildContext(
    const std::vector<ResolvedFeature> &features,
    const FeatureBuildContext &context,
    const FilePath &contextFolder)
{
    const FilePath featuresFolder = contextFolder / "dev-container-features";
    if (const Result<> res = featuresFolder.ensureWritableDir(); !res)
        return ResultError(res.error());

    if (const Result<qint64> res = (featuresFolder / "devcontainer-features.builtin.env")
                                       .writeFileContents(builtinEnvFileContents(context));
        !res) {
        return ResultError(res.error());
    }

    for (const ResolvedFeature &feature : features) {
        if (feature.folder.parentDir() != featuresFolder) {
            return ResultError(
                Tr::tr("The feature \"%1\" is not in the build context \"%2\".")
                    .arg(feature.reference.userReference, featuresFolder.toUserOutput()));
        }
        if (!(feature.folder / "install.sh").isFile()) {
            return ResultError(
                Tr::tr("The feature \"%1\" has no install.sh.").arg(feature.reference.userReference));
        }
        if (const Result<qint64> res = (feature.folder / "devcontainer-features.env")
                                           .writeFileContents(featureEnvFileContents(feature));
            !res) {
            return ResultError(res.error());
        }
    }

    const FilePath dockerfile = contextFolder / "Dockerfile.features";
    if (const Result<qint64> res = dockerfile.writeFileContents(
            featuresDockerfile(features, context).toUtf8());
        !res) {
        return ResultError(res.error());
    }
    return dockerfile;
}

QJsonObject mergeCustomizations(QJsonObject left, const QJsonObject &right)
{
    for (QJsonObject::const_iterator it = right.constBegin(); it != right.constEnd(); ++it) {
        if (left.contains(it.key())) {
            QJsonValueRef leftMember = left[it.key()];
            if (leftMember.isObject() && it.value().isObject()) {
                leftMember = mergeCustomizations(leftMember.toObject(), it.value().toObject());
            } else if (leftMember.isArray() && it.value().isArray()) {
                QJsonArray leftArray = leftMember.toArray();
                const QJsonArray rightArray = it.value().toArray();
                for (const QJsonValue &value : rightArray)
                    leftArray.append(value);
                leftMember = leftArray;
            } else {
                leftMember = it.value();
            }
        } else {
            left.insert(it.key(), it.value());
        }
    }
    return left;
}

QJsonObject mergedCustomizations(
    const std::vector<ResolvedFeature> &features, const QJsonObject &configCustomizations)
{
    QJsonObject result;
    for (const ResolvedFeature &feature : features)
        result = mergeCustomizations(result, feature.feature.customizations);
    return mergeCustomizations(result, configCustomizations);
}

bool isLocalRegistry(const QString &registry)
{
    // A registry is "host" or "host:port", with an IPv6 host in brackets.
    const QString host = registry.startsWith('[') ? registry.section(']', 0, 0) + ']'
                                                  : registry.section(':', 0, 0);
    return isLoopbackHost(host);
}

bool isSameOrigin(const QUrl &first, const QUrl &second)
{
    const auto defaultPort = [](const QUrl &url) {
        return url.scheme() == "https" ? 443 : url.scheme() == "http" ? 80 : -1;
    };
    return first.scheme() == second.scheme() && first.host() == second.host()
           && first.port(defaultPort(first)) == second.port(defaultPort(second));
}

QString mountTarget(const std::variant<Mount, QString> &mount)
{
    return std::visit(
        overloaded{
            [](const Mount &m) { return m.target; },
            [](const QString &m) {
                for (const QString &part : m.split(',', Qt::SkipEmptyParts)) {
                    const auto [key, value] = Utils::splitAtFirst(part, '=');
                    const QStringView k = key.trimmed();
                    if (k == u"target" || k == u"destination" || k == u"dst")
                        return value.trimmed().toString();
                }
                return QString();
            }},
        mount);
}

std::vector<std::variant<Mount, QString>> mergeMounts(
    const std::vector<std::variant<Mount, QString>> &mounts)
{
    std::vector<std::variant<Mount, QString>> result;
    QSet<QString> targets;
    for (auto it = mounts.rbegin(); it != mounts.rend(); ++it) {
        const QString target = mountTarget(*it);
        if (!target.isEmpty()) {
            if (targets.contains(target))
                continue;
            targets.insert(target);
        }
        result.insert(result.begin(), *it);
    }
    return result;
}

// "https://index.docker.io/v1/" and "ghcr.io" both name a registry by its host.
static bool isDockerHub(const QString &host)
{
    return host == "docker.io" || host == "index.docker.io" || host == "registry-1.docker.io";
}

QString registryApiHost(const QString &registry)
{
    return isDockerHub(registry.toLower()) ? QString("registry-1.docker.io") : registry;
}

// The Docker CLI keys Docker Hub as "https://index.docker.io/v1/".
static const char dockerHubServerUrl[] = "https://index.docker.io/v1/";

static QString registryHost(const QString &key)
{
    QString host = key.trimmed().toLower();
    if (const qsizetype scheme = host.indexOf("://"); scheme >= 0)
        host = host.mid(scheme + 3);
    host = host.section('/', 0, 0);
    return isDockerHub(host) ? QString("docker.io") : host;
}

RegistryCredentialSource registryCredentialSource(
    const QString &registry, const Environment &environment)
{
    RegistryCredentialSource source;
    source.serverUrl = isDockerHub(registry.toLower()) ? QString(dockerHubServerUrl) : registry;

    FilePath configFile;
    if (const QString dockerConfig = environment.value("DOCKER_CONFIG"); !dockerConfig.isEmpty()) {
        configFile = FilePath::fromUserInput(dockerConfig) / "config.json";
    } else {
        const QString home = environment.value(
            HostOsInfo::isWindowsHost() ? QString("USERPROFILE") : QString("HOME"));
        if (home.isEmpty())
            return source;
        configFile = FilePath::fromUserInput(home) / ".docker" / "config.json";
    }

    const Result<QByteArray> contents = configFile.fileContents();
    if (!contents)
        return source;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(*contents, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        source.error = Tr::tr("Cannot read the Docker configuration \"%1\": %2")
                           .arg(
                               configFile.toUserOutput(),
                               parseError.error != QJsonParseError::NoError
                                   ? parseError.errorString()
                                   : Tr::tr("The configuration is not a JSON object."));
        return source;
    }

    const QJsonObject config = doc.object();
    const QString host = registryHost(registry);

    const QJsonObject credHelpers = config.value("credHelpers").toObject();
    for (auto it = credHelpers.begin(); it != credHelpers.end(); ++it) {
        if (registryHost(it.key()) == host) {
            source.helper = it.value().toString();
            source.serverUrl = it.key();
            return source;
        }
    }

    const QJsonObject auths = config.value("auths").toObject();
    for (auto it = auths.begin(); it != auths.end(); ++it) {
        if (registryHost(it.key()) != host)
            continue;
        source.serverUrl = it.key();
        const QJsonObject entry = it.value().toObject();
        if (const QString token = entry.value("identitytoken").toString(); !token.isEmpty()) {
            source.credentials = RegistryCredentials{"<token>", token};
        } else if (const QString auth = entry.value("auth").toString(); !auth.isEmpty()) {
            const QString decoded = QString::fromUtf8(QByteArray::fromBase64(auth.toLatin1()));
            const auto [username, secret] = Utils::splitAtFirst(decoded, ':');
            source.credentials = RegistryCredentials{username.toString(), secret.toString()};
        }
        break;
    }

    if (!source.credentials)
        source.helper = config.value("credsStore").toString();
    return source;
}

Result<RegistryCredentials> parseCredentialHelperOutput(const QByteArray &output)
{
    const QJsonObject obj = QJsonDocument::fromJson(output).object();
    const QString secret = obj.value("Secret").toString();
    if (secret.isEmpty())
        return ResultError(Tr::tr("The credential helper did not return credentials."));
    return RegistryCredentials{obj.value("Username").toString(), secret};
}

QString updateUidDockerfile()
{
    return QString(R"dockerfile(ARG _DEV_CONTAINERS_BASE_IMAGE=placeholder
FROM $_DEV_CONTAINERS_BASE_IMAGE
USER root
ARG REMOTE_USER
ARG NEW_UID
ARG NEW_GID
ARG IMAGE_USER=root
RUN set -e; \
    entry="$(awk -F: -v u="$REMOTE_USER" '$1 == u { print; exit }' /etc/passwd)"; \
    if [ -z "$entry" ]; then echo "The user $REMOTE_USER does not exist."; exit 0; fi; \
    old_uid="$(echo "$entry" | cut -d: -f3)"; \
    old_gid="$(echo "$entry" | cut -d: -f4)"; \
    home="$(echo "$entry" | cut -d: -f6)"; \
    if [ "$old_uid" = "$NEW_UID" ] && [ "$old_gid" = "$NEW_GID" ]; then \
        echo "The IDs of $REMOTE_USER already match."; exit 0; \
    fi; \
    if [ "$old_uid" != "$NEW_UID" ] \
        && awk -F: -v id="$NEW_UID" '$3 == id { f = 1 } END { exit !f }' /etc/passwd; then \
        echo "Another user has the UID $NEW_UID, keeping $old_uid."; exit 0; \
    fi; \
    if [ "$old_gid" != "$NEW_GID" ] \
        && awk -F: -v id="$NEW_GID" '$3 == id { f = 1 } END { exit !f }' /etc/group; then \
        echo "Another group has the GID $NEW_GID, keeping $old_gid."; NEW_GID="$old_gid"; \
    fi; \
    awk -F: -v OFS=: -v u="$REMOTE_USER" -v uid="$NEW_UID" -v gid="$NEW_GID" \
        '$1 == u { $3 = uid; $4 = gid } { print }' /etc/passwd > /tmp/passwd.new; \
    cat /tmp/passwd.new > /etc/passwd; rm -f /tmp/passwd.new; \
    if [ "$old_gid" != "$NEW_GID" ]; then \
        awk -F: -v OFS=: -v old="$old_gid" -v gid="$NEW_GID" \
            '$3 == old { $3 = gid } { print }' /etc/group > /tmp/group.new; \
        cat /tmp/group.new > /etc/group; rm -f /tmp/group.new; \
    fi; \
    if [ -d "$home" ]; then chown -R "$NEW_UID:$NEW_GID" "$home"; fi; \
    echo "Changed $REMOTE_USER from $old_uid:$old_gid to $NEW_UID:$NEW_GID."
USER $IMAGE_USER
)dockerfile");
}

} // namespace DevContainer
