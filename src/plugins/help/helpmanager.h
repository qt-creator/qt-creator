// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/helpmanager_implementation.h>

#include <QHelpEngineCore>
#include <QVariant>

QT_BEGIN_NAMESPACE
template <typename T>
class QPromise;
class QUrl;
QT_END_NAMESPACE

namespace Help {
namespace Internal {

class HelpManager : public QObject, public Core::HelpManager::Implementation
{
    Q_OBJECT

public:
    explicit HelpManager(QObject *parent = nullptr);
    ~HelpManager() override;

    static HelpManager *instance();
    static QString collectionFilePath();

    void registerDocumentation(const Utils::FilePaths &fileNames) override;
    void setBlockedDocumentation(const Utils::FilePaths &fileNames) override;
    void unregisterDocumentation(const Utils::FilePaths &fileNames) override;

    static void registerUserDocumentation(const Utils::FilePaths &filePaths);
    static QSet<Utils::FilePath> userDocumentationPaths();

    QList<Core::HelpLink> linksForIdentifier(const QString &id) override;
    QList<Core::HelpLink> linksForKeyword(const QString &key) override;
    static QList<Core::HelpLink> linksForKeyword(QHelpEngineCore *engine,
                                                 const QString &key,
                                                 std::optional<QString> filterName);

    static QUrl findFile(const QUrl &url);
    QByteArray fileData(const QUrl &url) override;

    static QStringList registeredNamespaces();
    static QString namespaceFromFile(const QString &file);
    static Utils::FilePath fileFromNamespace(const QString &nameSpace);

    Q_INVOKABLE void showHelpUrl(
        const QUrl &url,
        Core::HelpManager::HelpViewerLocation location = Core::HelpManager::HelpModeAlways) override;

    void addOnlineHelpHandler(const Core::HelpManager::OnlineHelpHandler &handler) override;

    static void setupHelpManager();

signals:
    void collectionFileChanged();
    void helpRequested(const QUrl &url, Core::HelpManager::HelpViewerLocation location);
};

} // Internal
} // Core
