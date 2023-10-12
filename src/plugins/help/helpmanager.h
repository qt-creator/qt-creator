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

    void registerDocumentation(const QStringList &fileNames) override;
    void unregisterDocumentation(const QStringList &fileNames) override;

    static void registerUserDocumentation(const QStringList &filePaths);
    static QSet<QString> userDocumentationPaths();

    QMultiMap<QString, QUrl> linksForIdentifier(const QString &id) override;
    QMultiMap<QString, QUrl> linksForKeyword(const QString &key) override;
    static QMultiMap<QString, QUrl> linksForKeyword(QHelpEngineCore *engine,
                                                    const QString &key,
                                                    std::optional<QString> filterName);

    static QUrl findFile(const QUrl &url);
    QByteArray fileData(const QUrl &url) override;

    static QStringList registeredNamespaces();
    static QString namespaceFromFile(const QString &file);
    static QString fileFromNamespace(const QString &nameSpace);

    static void setCustomValue(const QString &key, const QVariant &value);
    static QVariant customValue(const QString &key, const QVariant &value = QVariant());

    static void aboutToShutdown();

    Q_INVOKABLE void showHelpUrl(
        const QUrl &url,
        Core::HelpManager::HelpViewerLocation location = Core::HelpManager::HelpModeAlways) override;

    static void setupHelpManager();

signals:
    void collectionFileChanged();
    void helpRequested(const QUrl &url, Core::HelpManager::HelpViewerLocation location);
};

} // Internal
} // Core
