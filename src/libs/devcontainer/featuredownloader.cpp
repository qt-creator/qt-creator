// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "featuredownloader.h"

#include "devcontainertr.h"

#include <utils/networkaccessmanager.h>
#include <utils/qtcassert.h>

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrlQuery>

using namespace QtTaskTree;
using namespace Utils;

namespace DevContainer::Internal {

// https://containers.dev/implementors/features-distribution/#oci-registry
static const char featureLayerMediaType[] = "application/vnd.devcontainers.layer.v1+tar";
static const char manifestMediaType[] = "application/vnd.oci.image.manifest.v1+json";
static const int maxRedirects = 5;
static constexpr std::chrono::milliseconds transferTimeout = std::chrono::minutes(1);
static constexpr qint64 maxFeatureSize = 512 * 1024 * 1024; // 512 MiB

static QString sha256Hex(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

static int httpStatus(QNetworkReply *reply)
{
    return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
}

// Whether the server could not be reached at all, as opposed to answering with an error.
static bool isUnreachable(QNetworkReply *reply)
{
    return reply->error() != QNetworkReply::NoError && httpStatus(reply) == 0;
}

FeatureDownloader::FeatureDownloader() = default;

FeatureDownloader::~FeatureDownloader()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
    }
}

void FeatureDownloader::setReference(const FeatureReference &reference)
{
    m_reference = reference;
}

void FeatureDownloader::setNetworkAccessManager(QNetworkAccessManager *networkAccessManager)
{
    m_networkAccessManager = networkAccessManager;
}

void FeatureDownloader::setCredentials(const std::optional<RegistryCredentials> &credentials)
{
    m_credentials = credentials;
}

void FeatureDownloader::setCacheFolder(const FilePath &cacheFolder)
{
    m_cacheFolder = cacheFolder;
}

void FeatureDownloader::setLogFunction(const std::function<void(const QString &)> &logFunction)
{
    m_logFunction = logFunction;
}

void FeatureDownloader::start()
{
    if (!m_networkAccessManager)
        m_networkAccessManager = NetworkAccessManager::instance();

    emit started();

    switch (m_reference.type) {
    case FeatureReference::Type::Oci: {
        // A digest pins the content, so a cached copy is as good as a download.
        if (!m_reference.digest.isEmpty()) {
            const Result<QByteArray> layerDigest = refCacheFile().fileContents();
            if (layerDigest
                && useCachedBlob(
                    QString::fromLatin1(*layerDigest),
                    Tr::tr("Using the cached copy of the feature \"%1\".")
                        .arg(m_reference.userReference))) {
                return;
            }
        }
        requestManifest();
        return;
    }
    case FeatureReference::Type::Tarball:
        get(QNetworkRequest(QUrl(m_reference.url)), &FeatureDownloader::onTarball);
        return;
    case FeatureReference::Type::Local:
        break;
    }
    finish(Tr::tr("The feature \"%1\" is not a remote feature.").arg(m_reference.userReference));
}

void FeatureDownloader::track(QNetworkReply *reply, Handler handler)
{
    m_reply = reply;
    m_tooLarge = false;
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, reply](qint64 received, qint64 total) {
        if (received > maxFeatureSize || total > maxFeatureSize) {
            m_tooLarge = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, handler, reply] {
        reply->deleteLater();
        m_reply.clear();
        (this->*handler)(reply);
    });
}

void FeatureDownloader::get(QNetworkRequest request, Handler handler)
{
    request.setTransferTimeout(transferTimeout);
    track(m_networkAccessManager->get(request), handler);
}

void FeatureDownloader::post(QNetworkRequest request, const QByteArray &data, Handler handler)
{
    request.setTransferTimeout(transferTimeout);
    track(m_networkAccessManager->post(request, data), handler);
}

QUrl FeatureDownloader::registryUrl(const QString &path) const
{
    return QUrl(QString("%1://%2/v2/%3/%4")
                    .arg(
                        isLocalRegistry(m_reference.registry) ? QString("http") : QString("https"),
                        registryApiHost(m_reference.registry),
                        m_reference.repository,
                        path));
}

QByteArray FeatureDownloader::basicAuthorization() const
{
    QTC_ASSERT(m_credentials, return {});
    return "Basic " + (m_credentials->username + ':' + m_credentials->secret).toUtf8().toBase64();
}

FilePath FeatureDownloader::blobCacheFile(const QString &digest) const
{
    if (m_cacheFolder.isEmpty() || !digest.startsWith("sha256:"))
        return {};
    return m_cacheFolder / "blobs" / "sha256" / digest.mid(7);
}

FilePath FeatureDownloader::refCacheFile() const
{
    if (m_cacheFolder.isEmpty())
        return {};
    const QString ref = m_reference.resource() + '@'
                        + (m_reference.digest.isEmpty() ? m_reference.tag : m_reference.digest);
    return m_cacheFolder / "refs" / sha256Hex(ref.toUtf8());
}

FilePath FeatureDownloader::urlCacheFile() const
{
    if (m_cacheFolder.isEmpty())
        return {};
    return m_cacheFolder / "urls" / sha256Hex(m_reference.url.toUtf8());
}

bool FeatureDownloader::useCachedBlob(const QString &digest, const QString &message)
{
    const FilePath file = blobCacheFile(digest);
    if (file.isEmpty())
        return false;
    const Result<QByteArray> data = file.fileContents();
    if (!data || "sha256:" + sha256Hex(*data) != digest)
        return false;
    log(message);
    m_archive = *data;
    finish();
    return true;
}

void FeatureDownloader::storeInCache(const FilePath &file, const QByteArray &data)
{
    if (file.isEmpty())
        return;
    const Result<> dir = file.parentDir().ensureWritableDir();
    const Result<qint64> written = dir ? file.writeFileContents(data)
                                       : Result<qint64>(ResultError(dir.error()));
    if (!written) {
        log(Tr::tr("Cannot cache the feature \"%1\": %2")
                .arg(m_reference.userReference, written.error()));
    }
}

void FeatureDownloader::requestManifest()
{
    const QString reference = m_reference.digest.isEmpty() ? m_reference.tag : m_reference.digest;
    QNetworkRequest request(registryUrl("manifests/" + reference));
    request.setRawHeader("Accept", manifestMediaType);
    if (!m_authorization.isEmpty())
        request.setRawHeader("Authorization", m_authorization);
    get(request, &FeatureDownloader::onManifest);
}

void FeatureDownloader::onManifest(QNetworkReply *reply)
{
    const int status = httpStatus(reply);
    if (status == 401 && !m_triedAuthentication) {
        authenticate(reply->rawHeader("WWW-Authenticate"));
        return;
    }

    if (isUnreachable(reply)) {
        const Result<QByteArray> layerDigest = refCacheFile().fileContents();
        if (layerDigest
            && useCachedBlob(
                QString::fromLatin1(*layerDigest),
                Tr::tr(
                    "Cannot reach the registry \"%1\" (%2). Using the cached copy of the "
                    "feature \"%3\".")
                    .arg(m_reference.registry, reply->errorString(), m_reference.userReference))) {
            return;
        }
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (status == 404) {
            finish(
                Tr::tr("The feature \"%1\" does not exist in the registry.")
                    .arg(m_reference.userReference));
        } else if (status == 401 || status == 403) {
            finish(
                Tr::tr(
                    "Access to the feature \"%1\" was denied. Log in to the registry with "
                    "\"docker login %2\".")
                    .arg(m_reference.userReference, m_reference.registry));
        } else {
            finish(
                Tr::tr("Cannot fetch the manifest of the feature \"%1\": %2")
                    .arg(m_reference.userReference, reply->errorString()));
        }
        return;
    }

    const QByteArray body = reply->readAll();
    if (!m_reference.digest.isEmpty() && "sha256:" + sha256Hex(body) != m_reference.digest) {
        finish(
            Tr::tr("The manifest of the feature \"%1\" does not match its digest.")
                .arg(m_reference.userReference));
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        finish(
            Tr::tr("Cannot read the manifest of the feature \"%1\": %2")
                .arg(
                    m_reference.userReference,
                    parseError.error != QJsonParseError::NoError
                        ? parseError.errorString()
                        : Tr::tr("The manifest is not a JSON object.")));
        return;
    }
    const QJsonArray layers = doc.object().value("layers").toArray();
    QJsonObject layer;
    for (const QJsonValue &value : layers) {
        if (value.toObject().value("mediaType").toString() == QLatin1String(featureLayerMediaType)) {
            layer = value.toObject();
            break;
        }
    }
    if (layer.isEmpty() && layers.size() == 1)
        layer = layers.first().toObject();

    m_blobDigest = layer.value("digest").toString();
    if (!m_blobDigest.startsWith("sha256:")) {
        finish(
            Tr::tr("The manifest of the feature \"%1\" does not contain a feature layer.")
                .arg(m_reference.userReference));
        return;
    }

    storeInCache(refCacheFile(), m_blobDigest.toLatin1());

    if (useCachedBlob(
            m_blobDigest,
            Tr::tr("The feature \"%1\" is cached already.").arg(m_reference.userReference))) {
        return;
    }

    requestBlob(registryUrl("blobs/" + m_blobDigest), SendAuthorization::Yes);
}

// Parses a challenge like: Bearer realm="https://ghcr.io/token",service="ghcr.io",scope="..."
static QMap<QString, QString> parseChallenge(const QByteArray &challenge, QString *scheme)
{
    const QString str = QString::fromLatin1(challenge).trimmed();
    *scheme = str.section(' ', 0, 0);

    QMap<QString, QString> params;
    static const QRegularExpression paramRegex(R"re((\w+)="([^"]*)")re");
    auto it = paramRegex.globalMatch(str.section(' ', 1));
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        params.insert(match.captured(1).toLower(), match.captured(2));
    }
    return params;
}

// https://distribution.github.io/distribution/spec/auth/token/
void FeatureDownloader::authenticate(const QByteArray &challenge)
{
    m_triedAuthentication = true;

    QString scheme;
    const QMap<QString, QString> params = parseChallenge(challenge, &scheme);

    if (scheme.compare("Basic", Qt::CaseInsensitive) == 0) {
        if (!m_credentials) {
            finish(
                Tr::tr(
                    "The registry \"%1\" requires credentials. Log in with \"docker login "
                    "%1\".")
                    .arg(m_reference.registry));
            return;
        }
        m_authorization = basicAuthorization();
        requestManifest();
        return;
    }

    if (scheme.compare("Bearer", Qt::CaseInsensitive) != 0 || !params.contains("realm")) {
        finish(
            Tr::tr("The registry \"%1\" uses the unsupported authentication scheme \"%2\".")
                .arg(m_reference.registry, scheme));
        return;
    }

    const QString scope
        = params.value("scope", QString("repository:%1:pull").arg(m_reference.repository));
    QUrl url(params.value("realm"));

    // The token server gets the credentials, so it must not be reachable in clear text.
    const QString realmHost = url.host().contains(':') ? '[' + url.host() + ']' : url.host();
    if (url.scheme() != "https" && !(url.scheme() == "http" && isLocalRegistry(realmHost))) {
        finish(
            Tr::tr(
                "The registry \"%1\" asks for authentication at \"%2\", which does not "
                "use HTTPS.")
                .arg(m_reference.registry, url.toDisplayString()));
        return;
    }

    // QUrlQuery leaves "+" and "%" alone, so the values have to be encoded up front.
    const auto encoded = [](const QString &value) {
        return QString::fromLatin1(QUrl::toPercentEncoding(value));
    };

    // An identity token is a refresh token for the OAuth2 flow of the token server.
    if (m_credentials && m_credentials->username == "<token>") {
        QUrlQuery form;
        form.addQueryItem("grant_type", "refresh_token");
        form.addQueryItem("refresh_token", encoded(m_credentials->secret));
        form.addQueryItem("service", encoded(params.value("service")));
        form.addQueryItem("scope", encoded(scope));
        form.addQueryItem("client_id", "qtcreator");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        post(request, form.toString(QUrl::FullyEncoded).toUtf8(), &FeatureDownloader::onToken);
        return;
    }

    QUrlQuery query(url);
    if (params.contains("service"))
        query.addQueryItem("service", encoded(params.value("service")));
    query.addQueryItem("scope", encoded(scope));
    url.setQuery(query);

    QNetworkRequest request(url);
    if (m_credentials)
        request.setRawHeader("Authorization", basicAuthorization());
    get(request, &FeatureDownloader::onToken);
}

void FeatureDownloader::onToken(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        const int status = httpStatus(reply);
        if (status == 401 || status == 403) {
            finish(
                Tr::tr(
                    "Access to the feature \"%1\" was denied. Log in to the registry with "
                    "\"docker login %2\".")
                    .arg(m_reference.userReference, m_reference.registry));
        } else {
            finish(
                Tr::tr("Cannot get access to the feature \"%1\": %2")
                    .arg(m_reference.userReference, reply->errorString()));
        }
        return;
    }

    const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
    const QByteArray token
        = obj.value("token").toString(obj.value("access_token").toString()).toUtf8();
    if (token.isEmpty()) {
        finish(
            Tr::tr("The registry \"%1\" did not provide an access token.").arg(m_reference.registry));
        return;
    }

    m_authorization = "Bearer " + token;
    requestManifest();
}

void FeatureDownloader::requestBlob(const QUrl &url, SendAuthorization sendAuthorization)
{
    m_blobUrl = url;
    QNetworkRequest request(url);
    // Redirects are followed by hand, as the credentials must not be sent on to the storage host.
    request.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    if (sendAuthorization == SendAuthorization::Yes && !m_authorization.isEmpty())
        request.setRawHeader("Authorization", m_authorization);
    get(request, &FeatureDownloader::onBlob);
}

void FeatureDownloader::onBlob(QNetworkReply *reply)
{
    const int status = httpStatus(reply);
    if (status >= 300 && status < 400) {
        const QUrl target = m_blobUrl.resolved(
            reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
        if (!target.isValid() || ++m_redirects > maxRedirects) {
            finish(downloadError(Tr::tr("Too many redirects.")));
            return;
        }
        // Only the registry itself gets the credentials, not any host it sends us to.
        requestBlob(
            target,
            isSameOrigin(target, registryUrl({})) ? SendAuthorization::Yes : SendAuthorization::No);
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (m_tooLarge) {
            finish(downloadError(Tr::tr("The archive is too large.")));
        } else {
            finish(downloadError(reply->errorString()));
        }
        return;
    }

    const QByteArray data = reply->readAll();
    const QString actualDigest = "sha256:" + sha256Hex(data);
    if (actualDigest != m_blobDigest) {
        finish(Tr::tr("The download is corrupt: Expected the digest %1, got %2.")
                   .arg(m_blobDigest, actualDigest));
        return;
    }

    storeInCache(blobCacheFile(m_blobDigest), data);
    m_archive = data;
    finish();
}

void FeatureDownloader::onTarball(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        if (m_tooLarge) {
            finish(downloadError(Tr::tr("The archive is too large.")));
            return;
        }
        if (isUnreachable(reply)) {
            if (const Result<QByteArray> cached = urlCacheFile().fileContents(); cached) {
                log(Tr::tr("Cannot reach \"%1\" (%2). Using the cached copy of the feature.")
                        .arg(m_reference.url, reply->errorString()));
                m_archive = *cached;
                finish();
                return;
            }
        }
        finish(downloadError(reply->errorString()));
        return;
    }
    m_archive = reply->readAll();
    storeInCache(urlCacheFile(), m_archive);
    finish();
}

void FeatureDownloader::log(const QString &message) const
{
    if (m_logFunction)
        m_logFunction(message);
}

QString FeatureDownloader::downloadError(const QString &details) const
{
    return Tr::tr("Cannot download the feature \"%1\": %2")
        .arg(m_reference.userReference, details);
}

void FeatureDownloader::finish(const QString &errorString)
{
    m_errorString = errorString;
    emit done(errorString.isEmpty() ? DoneResult::Success : DoneResult::Error);
}

} // namespace DevContainer::Internal
