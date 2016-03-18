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

#include "core_global.h"

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QMap>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Core {
struct HelpManagerPrivate;

namespace Internal {
class CorePlugin;
class MainWindow;
}

class CORE_EXPORT HelpManager : public QObject
{
    Q_OBJECT

public:
    enum HelpViewerLocation {
        SideBySideIfPossible = 0,
        SideBySideAlways = 1,
        HelpModeAlways = 2,
        ExternalHelpAlways = 3
    };

    typedef QHash<QString, QStringList> Filters;

    static HelpManager *instance();
    static QString collectionFilePath();

    static void registerDocumentation(const QStringList &fileNames);
    static void unregisterDocumentation(const QStringList &nameSpaces);

    static void registerUserDocumentation(const QStringList &filePaths);
    static QSet<QString> userDocumentationPaths();

    static QMap<QString, QUrl> linksForKeyword(const QString &key);
    static QMap<QString, QUrl> linksForIdentifier(const QString &id);

    static QUrl findFile(const QUrl &url);
    static QByteArray fileData(const QUrl &url);

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

public slots:
    static void handleHelpRequest(const QUrl &url,
                                  Core::HelpManager::HelpViewerLocation location = HelpModeAlways);
    static void handleHelpRequest(const QString &url,
                                  Core::HelpManager::HelpViewerLocation location = HelpModeAlways);

signals:
    void setupFinished();
    void documentationChanged();
    void collectionFileChanged();
    void helpRequested(const QUrl &url, Core::HelpManager::HelpViewerLocation location);

private:
    explicit HelpManager(QObject *parent = 0);
    ~HelpManager();

    static void setupHelpManager();
    friend class Core::Internal::CorePlugin; // setupHelpManager
    friend class Core::Internal::MainWindow; // constructor/destructor
};

}   // Core
