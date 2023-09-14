// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include "nodemetainfo.h"

#include <QTimer>
#include <QVariantHash>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

namespace QmlDesigner::Internal {

class ContentLibraryBundleImporter : public QObject
{
    Q_OBJECT

public:
    ContentLibraryBundleImporter(const QString &bundleDir,
                                 const QString &bundleId,
                                 const QStringList &sharedFiles,
                                 QObject *parent = nullptr);
    ~ContentLibraryBundleImporter() = default;

    QString importComponent(const QString &qmlFile,
                            const QStringList &files);
    QString unimportComponent(const QString &qmlFile);
    Utils::FilePath resolveBundleImportPath();

signals:
    // The metaInfo parameter will be invalid if an error was encountered during
    // asynchronous part of the import. In this case all remaining pending imports have been
    // terminated, and will not receive separate importFinished notifications.
#ifdef QDS_USE_PROJECTSTORAGE
    void importFinished(const QmlDesigner::TypeName &typeName);
#else
    void importFinished(const QmlDesigner::NodeMetaInfo &metaInfo);
#endif
    void unimportFinished(const QmlDesigner::NodeMetaInfo &metaInfo);

private:
    void handleImportTimer();
    QVariantHash loadAssetRefMap(const Utils::FilePath &bundlePath);
    void writeAssetRefMap(const Utils::FilePath &bundlePath, const QVariantHash &assetRefMap);

    Utils::FilePath m_bundleDir;
    QString m_bundleId;
    QString m_moduleName;
    QStringList m_sharedFiles;
    QTimer m_importTimer;
    int m_importTimerCount = 0;
    bool m_importAddPending = false;
    bool m_fullReset = false;
    QHash<QString, bool> m_pendingTypes; // <type, isImport>
};

} // namespace QmlDesigner::Internal
