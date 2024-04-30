// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelfwd.h>

#include <utils/filepath.h>

#include <QTimer>
#include <QVariantHash>

namespace QmlDesigner {

class NodeMetaInfo;

class ContentLibraryBundleImporter : public QObject
{
    Q_OBJECT

public:
    ContentLibraryBundleImporter(QObject *parent = nullptr);
    ~ContentLibraryBundleImporter() = default;

    QString importComponent(const QString &bundleDir, const TypeName &type, const QString &qmlFile,
                            const QStringList &files);
    QString unimportComponent(const TypeName &type, const QString &qmlFile);
    Utils::FilePath resolveBundleImportPath(const QString &bundleId);

signals:
    // The metaInfo parameter will be invalid if an error was encountered during
    // asynchronous part of the import. In this case all remaining pending imports have been
    // terminated, and will not receive separate importFinished notifications.
#ifdef QDS_USE_PROJECTSTORAGE
    void importFinished(const QmlDesigner::TypeName &typeName, const QString &bundleId);
#else
    void importFinished(const QmlDesigner::NodeMetaInfo &metaInfo, const QString &bundleId);
#endif
    void unimportFinished(const QmlDesigner::NodeMetaInfo &metaInfo, const QString &bundleId);
    void aboutToUnimport(const TypeName &type, const QString &bundleId);

private:
    void handleImportTimer();
    QVariantHash loadAssetRefMap(const Utils::FilePath &bundlePath);
    void writeAssetRefMap(const Utils::FilePath &bundlePath, const QVariantHash &assetRefMap);

    QTimer m_importTimer;
    int m_importTimerCount = 0;
    QString m_pendingImport;
    QString m_bundleId;
    bool m_fullReset = false;
    QHash<TypeName, bool> m_pendingTypes; // <type, isImport>
};

} // namespace QmlDesigner
