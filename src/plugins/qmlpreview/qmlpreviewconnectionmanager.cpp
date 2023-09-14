// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewconnectionmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <qtsupport/baseqtversion.h>

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

namespace QmlPreview {

QmlPreviewConnectionManager::QmlPreviewConnectionManager(QObject *parent) :
    QmlDebug::QmlDebugConnectionManager(parent)
{
    setTarget(nullptr);
}

QmlPreviewConnectionManager::~QmlPreviewConnectionManager() = default;

void QmlPreviewConnectionManager::setTarget(ProjectExplorer::Target *target)
{
    QtSupport::QtVersion::populateQmlFileFinder(&m_projectFileFinder, target);
    m_projectFileFinder.setAdditionalSearchDirectories(Utils::FilePaths());
    m_targetFileFinder.setTarget(target);
}

void QmlPreviewConnectionManager::setFileLoader(QmlPreviewFileLoader fileLoader)
{
    m_fileLoader = fileLoader;
}

void QmlPreviewConnectionManager::setFileClassifier(QmlPreviewFileClassifier fileClassifier)
{
    m_fileClassifier = fileClassifier;
}

void QmlPreviewConnectionManager::setFpsHandler(QmlPreviewFpsHandler fpsHandler)
{
    m_fpsHandler = fpsHandler;
}

void QmlPreviewConnectionManager::setQmlDebugTranslationClientCreator(QmlDebugTranslationClientFactoryFunction creator)
{
    m_createDebugTranslationClientMethod = creator;
}

void QmlPreviewConnectionManager::createClients()
{
    createPreviewClient();
    createDebugTranslationClient();
}

QUrl QmlPreviewConnectionManager::findValidI18nDirectoryAsUrl(const QString &locale)
{
    QTC_ASSERT(!m_lastLoadedUrl.isEmpty(), return {};);

    const QString shortLocale = locale.left(locale.indexOf("_"));
    QString path = m_lastLoadedUrl.path();

    QString foundPath;
    while (!path.isEmpty()) {
        path = path.left(qMax(0, path.lastIndexOf("/")));
        QUrl url = m_lastLoadedUrl;


        auto tryPath = [&](const QString &postfix) {
            url.setPath(path + "/i18n/qml_" + postfix);
            bool success = false;
            foundPath = m_projectFileFinder.findFile(url, &success).constFirst().toString();
            foundPath = foundPath.left(qMax(0, foundPath.lastIndexOf("/i18n")));
            return success;
        };

        if (tryPath(locale + ".qm"))
            break;
        if (tryPath(locale))
            break;
        if (tryPath(shortLocale + ".qm"))
            break;
        if (tryPath(shortLocale))
            break;
    }

    QUrl url = m_lastLoadedUrl;
    if (foundPath.isEmpty())
        url.setPath(path);
    else
        url.setPath(foundPath);
    return url;
}

void QmlPreviewConnectionManager::createDebugTranslationClient()
{
    m_qmlDebugTranslationClient = m_createDebugTranslationClientMethod(connection());
    connect(this, &QmlPreviewConnectionManager::language,
                     m_qmlDebugTranslationClient.get(), [this](const QString &locale) {
        m_lastUsedLanguage = locale;
        // findValidI18nDirectoryAsUrl does not work if we didn't load any file
        // service expects a context URL.
        if (!m_lastLoadedUrl.isEmpty()) {
            // Search the parent directories of the last loaded URL for i18n files.
            m_qmlDebugTranslationClient->changeLanguage(findValidI18nDirectoryAsUrl(locale), locale);
        }
    });
    connect(m_qmlDebugTranslationClient.get(), &QmlDebugTranslationClient::debugServiceUnavailable,
                     this, []() {
        QMessageBox::warning(Core::ICore::dialogParent(), "Error connect to QML DebugTranslation service",
                             "QML DebugTranslation feature is not available for this version of Qt.");
    }, Qt::QueuedConnection); // Queue it, so that it interfere with the connection timer
}

void QmlPreviewConnectionManager::createPreviewClient()
{
    m_qmlPreviewClient = new QmlPreviewClient(connection());

    connect(this, &QmlPreviewConnectionManager::loadFile, m_qmlPreviewClient.data(),
                [this](const QString &filename, const QString &changedFile,
                       const QByteArray &contents) {
        if (!m_fileClassifier(changedFile)) {
            emit restart();
            return;
        }

        bool success = false;
        const QString remoteChangedFile = m_targetFileFinder.findPath(changedFile, &success);
        if (success)
            m_qmlPreviewClient->announceFile(remoteChangedFile, contents);
        else
            m_qmlPreviewClient->clearCache();

        m_lastLoadedUrl = m_targetFileFinder.findUrl(filename);
        m_qmlPreviewClient->loadUrl(m_lastLoadedUrl);
        // emit language after a file was loaded and do it every time,
        // because this also triggers the check for missing translations
        emit language(m_lastUsedLanguage);
    });

    connect(this, &QmlPreviewConnectionManager::rerun,
                     m_qmlPreviewClient.data(), &QmlPreviewClient::rerun);

    connect(this, &QmlPreviewConnectionManager::zoom,
                     m_qmlPreviewClient.data(), &QmlPreviewClient::zoom);

    connect(m_qmlPreviewClient.data(),
            &QmlPreviewClient::pathRequested,
            this,
            [this](const QString &path) {
                const bool found = m_projectFileFinder.findFileOrDirectory(
                    Utils::FilePath::fromString(path),
                    [&](const Utils::FilePath &filename, int confidence) {
                        if (m_fileLoader && confidence == path.length()) {
                            bool success = false;
                            QByteArray contents = m_fileLoader(filename.toFSPathString(), &success);
                            if (success) {
                                if (!m_fileSystemWatcher.watchesFile(filename)) {
                                    m_fileSystemWatcher
                                        .addFile(filename,
                                                 Utils::FileSystemWatcher::WatchModifiedDate);
                                }
                                m_qmlPreviewClient->announceFile(path, contents);
                            } else {
                                m_qmlPreviewClient->announceError(path);
                            }
                        } else {
                            m_qmlPreviewClient->announceError(path);
                        }
                    },
                    [&](const QStringList &entries, int confidence) {
                        if (confidence == path.length())
                            m_qmlPreviewClient->announceDirectory(path, entries);
                        else
                            m_qmlPreviewClient->announceError(path);
                    });

                if (!found)
                    m_qmlPreviewClient->announceError(path);
            });

    connect(m_qmlPreviewClient.data(),
            &QmlPreviewClient::errorReported,
            this,
            [](const QString &error) {
                Core::MessageManager::writeFlashing("Error loading QML Live Preview:");
                Core::MessageManager::writeSilently(error);
            });

    connect(m_qmlPreviewClient.data(), &QmlPreviewClient::fpsReported,
                     this, [this](const QmlPreviewClient::FpsInfo &frames) {
        if (m_fpsHandler) {
            quint16 stats[] = {
                frames.numSyncs, frames.minSync, frames.maxSync, frames.totalSync,
                frames.numRenders, frames.minRender, frames.maxRender, frames.totalRender
            };
            m_fpsHandler(stats);
        }
    });

    connect(m_qmlPreviewClient.data(), &QmlPreviewClient::debugServiceUnavailable,
                     this, []() {
        QMessageBox::warning(Core::ICore::dialogParent(), "Error loading QML Live Preview",
                             "QML Live Preview is not available for this version of Qt.");
    }, Qt::QueuedConnection); // Queue it, so that it interfere with the connection timer

    connect(&m_fileSystemWatcher, &Utils::FileSystemWatcher::fileChanged,
                     m_qmlPreviewClient.data(), [this](const QString &changedFile) {
        if (!m_fileLoader || !m_lastLoadedUrl.isValid())
            return;

        bool success = false;

        const QByteArray contents = m_fileLoader(changedFile, &success);
        if (!success)
            return;

        if (!m_fileClassifier(changedFile)) {
            emit restart();
            return;
        }

        const QString remoteChangedFile = m_targetFileFinder.findPath(changedFile, &success);
        if (success)
            m_qmlPreviewClient->announceFile(remoteChangedFile, contents);
        else
            m_qmlPreviewClient->clearCache();

        m_qmlPreviewClient->loadUrl(m_lastLoadedUrl);
    });
}

void QmlPreviewConnectionManager::clearClient(QObject *client)
{
    if (client) {
        disconnect(client, nullptr, this, nullptr);
        disconnect(this, nullptr, client, nullptr);
        client->deleteLater();
    }
};


void QmlPreviewConnectionManager::destroyClients()
{
    clearClient(m_qmlPreviewClient);
    clearClient(m_qmlDebugTranslationClient.release());
    m_fileSystemWatcher.clear();
}

} // namespace QmlPreview
