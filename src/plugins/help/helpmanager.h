/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <coreplugin/helpmanager_implementation.h>

QT_FORWARD_DECLARE_CLASS(QUrl)

#include <QFutureInterface>
#include <QVariant>

namespace Help {
namespace Internal {

class HelpManager : public QObject, public Core::HelpManager::Implementation
{
    Q_OBJECT

public:
    using Filters = QHash<QString, QStringList>;

    explicit HelpManager(QObject *parent = nullptr);
    ~HelpManager() override;

    static HelpManager *instance();
    static QString collectionFilePath();

    void registerDocumentation(const QStringList &fileNames) override;
    void unregisterDocumentation(const QStringList &nameSpaces) override;

    static void registerUserDocumentation(const QStringList &filePaths);
    static QSet<QString> userDocumentationPaths();

    QMap<QString, QUrl> linksForIdentifier(const QString &id) override;
    QMap<QString, QUrl> linksForKeyword(const QString &key) override;

    static QUrl findFile(const QUrl &url);
    QByteArray fileData(const QUrl &url) override;

    static QStringList registeredNamespaces();
    static QString namespaceFromFile(const QString &file);
    static QString fileFromNamespace(const QString &nameSpace);

    static void setCustomValue(const QString &key, const QVariant &value);
    static QVariant customValue(const QString &key, const QVariant &value = QVariant());

    static Filters filters();
    static Filters fixedFilters();

    static Filters userDefinedFilters();
    static void removeUserDefinedFilter(const QString &filter);
    static void addUserDefinedFilter(const QString &filter, const QStringList &attr);

    static void aboutToShutdown();

    Q_INVOKABLE void showHelpUrl(
        const QUrl &url,
        Core::HelpManager::HelpViewerLocation location = Core::HelpManager::HelpModeAlways) override;


    static void setupHelpManager();
    static void registerDocumentationNow(QFutureInterface<bool> &futureInterface,
                                         const QStringList &fileNames);
signals:
    void collectionFileChanged();
    void helpRequested(const QUrl &url, Core::HelpManager::HelpViewerLocation location);
};

} // Internal
} // Core
