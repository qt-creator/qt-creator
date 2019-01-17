/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlpreviewconnectionmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <qtsupport/baseqtversion.h>

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

namespace QmlPreview {
namespace Internal {

QmlPreviewConnectionManager::~QmlPreviewConnectionManager()
{
}

QmlPreviewConnectionManager::QmlPreviewConnectionManager(QObject *parent) :
    QmlDebug::QmlDebugConnectionManager(parent)
{
    setTarget(nullptr);
}

void QmlPreviewConnectionManager::setTarget(ProjectExplorer::Target *target)
{
    QtSupport::BaseQtVersion::populateQmlFileFinder(&m_projectFileFinder, target);
    m_projectFileFinder.setAdditionalSearchDirectories(Utils::FileNameList());
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

void QmlPreviewConnectionManager::createClients()
{
    m_clientPlugin = new QmlPreviewClient(connection());

    QObject::connect(
                this, &QmlPreviewConnectionManager::loadFile, m_clientPlugin.data(),
                [this](const QString &filename, const QString &changedFile,
                       const QByteArray &contents) {
        if (!m_fileClassifier(changedFile)) {
            emit restart();
            return;
        }

        bool success = false;
        const QString remoteChangedFile = m_targetFileFinder.findPath(changedFile, &success);
        if (success)
            m_clientPlugin->announceFile(remoteChangedFile, contents);
        else
            m_clientPlugin->clearCache();

        m_lastLoadedUrl = m_targetFileFinder.findUrl(filename);
        m_clientPlugin->loadUrl(m_lastLoadedUrl);
    });

    QObject::connect(this, &QmlPreviewConnectionManager::rerun,
                     m_clientPlugin.data(), &QmlPreviewClient::rerun);

    QObject::connect(this, &QmlPreviewConnectionManager::zoom,
                     m_clientPlugin.data(), &QmlPreviewClient::zoom);

    QObject::connect(this, &QmlPreviewConnectionManager::language,
                     m_clientPlugin.data(), [this](const QString &locale) {

        // The preview service expects a context URL.
        // Search the parent directories of the last loaded URL for i18n files.

        const QString shortLocale = locale.left(locale.indexOf("_"));
        QString path = m_lastLoadedUrl.path();

        while (!path.isEmpty()) {
            path = path.left(qMax(0, path.lastIndexOf("/")));
            QUrl url = m_lastLoadedUrl;

            auto tryPath = [&](const QString &postfix) {
                url.setPath(path + "/i18n/qml_" + postfix);
                bool success = false;
                m_projectFileFinder.findFile(url, &success);
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
        url.setPath(path);

        m_clientPlugin->language(url, locale);
    });

    QObject::connect(m_clientPlugin.data(), &QmlPreviewClient::pathRequested,
                     this, [this](const QString &path) {
        const bool found = m_projectFileFinder.findFileOrDirectory(
                    path, [&](const QString &filename, int confidence) {
            if (m_fileLoader && confidence == path.length()) {
                bool success = false;
                QByteArray contents = m_fileLoader(filename, &success);
                if (success) {
                    if (!m_fileSystemWatcher.watchesFile(filename)) {
                        m_fileSystemWatcher.addFile(filename,
                            Utils::FileSystemWatcher::WatchModifiedDate);
                    }
                    m_clientPlugin->announceFile(path, contents);
                } else {
                    m_clientPlugin->announceError(path);
                }
            } else {
                m_clientPlugin->announceError(path);
            }
        }, [&](const QStringList &entries, int confidence) {
            if (confidence == path.length())
                m_clientPlugin->announceDirectory(path, entries);
            else
                m_clientPlugin->announceError(path);
        });

        if (!found)
            m_clientPlugin->announceError(path);
    });

    QObject::connect(m_clientPlugin.data(), &QmlPreviewClient::errorReported,
                     this, [](const QString &error) {
        Core::MessageManager::write("Error loading QML Live Preview:");
        Core::MessageManager::write(error);
    });

    QObject::connect(m_clientPlugin.data(), &QmlPreviewClient::fpsReported,
                     this, [this](const QmlPreviewClient::FpsInfo &frames) {
        if (m_fpsHandler) {
            quint16 stats[] = {
                frames.numSyncs, frames.minSync, frames.maxSync, frames.totalSync,
                frames.numRenders, frames.minRender, frames.maxRender, frames.totalRender
            };
            m_fpsHandler(stats);
        }
    });

    QObject::connect(m_clientPlugin.data(), &QmlPreviewClient::debugServiceUnavailable,
                     this, []() {
        QMessageBox::warning(Core::ICore::mainWindow(), "Error loading QML Live Preview",
                             "QML Live Preview is not available for this version of Qt.");
    }, Qt::QueuedConnection); // Queue it, so that it interfere with the connection timer

    QObject::connect(&m_fileSystemWatcher, &Utils::FileSystemWatcher::fileChanged,
                     m_clientPlugin.data(), [this](const QString &changedFile) {
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
            m_clientPlugin->announceFile(remoteChangedFile, contents);
        else
            m_clientPlugin->clearCache();

        m_clientPlugin->loadUrl(m_lastLoadedUrl);
    });
}

void QmlPreviewConnectionManager::destroyClients()
{
    disconnect(m_clientPlugin.data(), nullptr, this, nullptr);
    disconnect(this, nullptr, m_clientPlugin.data(), nullptr);
    m_clientPlugin->deleteLater();
    m_clientPlugin.clear();
    m_fileSystemWatcher.removeFiles(m_fileSystemWatcher.files());
    QTC_ASSERT(m_fileSystemWatcher.directories().isEmpty(),
               m_fileSystemWatcher.removeDirectories(m_fileSystemWatcher.directories()));
}

} // namespace Internal
} // namespace QmlPreview
