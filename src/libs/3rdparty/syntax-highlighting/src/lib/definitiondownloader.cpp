/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "definitiondownloader.h"
#include "definition.h"
#include "ksyntaxhighlighting_logging.h"
#include "ksyntaxhighlighting_version.h"
#include "repository.h"

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTimer>
#include <QXmlStreamReader>

using namespace KSyntaxHighlighting;

class KSyntaxHighlighting::DefinitionDownloaderPrivate
{
public:
    DefinitionDownloader *q;
    Repository *repo;
    QNetworkAccessManager *nam;
    QString downloadLocation;
    int pendingDownloads;
    bool needsReload;

    void definitionListDownloadFinished(QNetworkReply *reply);
    void updateDefinition(QXmlStreamReader &parser);
    void downloadDefinition(const QUrl &url);
    void downloadDefinitionFinished(QNetworkReply *reply);
    void checkDone();
};

void DefinitionDownloaderPrivate::definitionListDownloadFinished(QNetworkReply *reply)
{
    const auto networkError = reply->error();
    if (networkError != QNetworkReply::NoError) {
        qCWarning(Log) << networkError;
        emit q->done(); // TODO return error
        return;
    }

    QXmlStreamReader parser(reply);
    while (!parser.atEnd()) {
        switch (parser.readNext()) {
        case QXmlStreamReader::StartElement:
            if (parser.name() == QLatin1String("Definition"))
                updateDefinition(parser);
            break;
        default:
            break;
        }
    }

    if (pendingDownloads == 0)
        emit q->informationMessage(QObject::tr("All syntax definitions are up-to-date."));
    checkDone();
}

void DefinitionDownloaderPrivate::updateDefinition(QXmlStreamReader &parser)
{
    const auto name = parser.attributes().value(QLatin1String("name"));
    if (name.isEmpty())
        return;

    auto localDef = repo->definitionForName(name.toString());
    if (!localDef.isValid()) {
        emit q->informationMessage(QObject::tr("Downloading new syntax definition for '%1'...").arg(name.toString()));
        downloadDefinition(QUrl(parser.attributes().value(QLatin1String("url")).toString()));
        return;
    }

    const auto version = parser.attributes().value(QLatin1String("version"));
    if (localDef.version() < version.toFloat()) {
        emit q->informationMessage(QObject::tr("Updating syntax definition for '%1' to version %2...").arg(name.toString(), version.toString()));
        downloadDefinition(QUrl(parser.attributes().value(QLatin1String("url")).toString()));
    }
}

void DefinitionDownloaderPrivate::downloadDefinition(const QUrl &downloadUrl)
{
    if (!downloadUrl.isValid())
        return;
    auto url = downloadUrl;
    if (url.scheme() == QLatin1String("http"))
        url.setScheme(QStringLiteral("https"));

    QNetworkRequest req(url);
    auto reply = nam->get(req);
    QObject::connect(reply, &QNetworkReply::finished, q, [this, reply]() { downloadDefinitionFinished(reply); });
    ++pendingDownloads;
    needsReload = true;
}

void DefinitionDownloaderPrivate::downloadDefinitionFinished(QNetworkReply *reply)
{
    --pendingDownloads;

    const auto networkError = reply->error();
    if (networkError != QNetworkReply::NoError) {
        qCWarning(Log) << "Failed to download definition file" << reply->url() << networkError;
        checkDone();
        return;
    }

    // handle redirects
    // needs to be done manually, download server redirects to unsafe http links
    const auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (!redirectUrl.isEmpty()) {
        downloadDefinition(reply->url().resolved(redirectUrl));
        checkDone();
        return;
    }

    QFile file(downloadLocation + QLatin1Char('/') + reply->url().fileName());
    if (!file.open(QFile::WriteOnly)) {
        qCWarning(Log) << "Failed to open" << file.fileName() << file.error();
    } else {
        file.write(reply->readAll());
    }
    checkDone();
}

void DefinitionDownloaderPrivate::checkDone()
{
    if (pendingDownloads == 0) {
        if (needsReload)
            repo->reload();

        emit QTimer::singleShot(0, q, &DefinitionDownloader::done);
    }
}

DefinitionDownloader::DefinitionDownloader(Repository *repo, QObject *parent)
    : QObject(parent)
    , d(new DefinitionDownloaderPrivate())
{
    Q_ASSERT(repo);

    d->q = this;
    d->repo = repo;
    d->nam = new QNetworkAccessManager(this);
    d->pendingDownloads = 0;
    d->needsReload = false;

    d->downloadLocation = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/org.kde.syntax-highlighting/syntax");
    QDir().mkpath(d->downloadLocation);
    Q_ASSERT(QFile::exists(d->downloadLocation));
}

DefinitionDownloader::~DefinitionDownloader()
{
}

void DefinitionDownloader::start()
{
    const QString url = QLatin1String("https://www.kate-editor.org/syntax/update-") + QString::number(SyntaxHighlighting_VERSION_MAJOR) + QLatin1Char('.') + QString::number(SyntaxHighlighting_VERSION_MINOR) + QLatin1String(".xml");
    auto req = QNetworkRequest(QUrl(url));
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    auto reply = d->nam->get(req);
    QObject::connect(reply, &QNetworkReply::finished, this, [=]() { d->definitionListDownloadFinished(reply); });
}
