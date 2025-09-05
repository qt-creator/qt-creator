// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelfwd.h>
#include <module.h>

#include <utils/filepath.h>

#include <QTimer>
#include <QVariantHash>

namespace QmlDesigner {

class NodeMetaInfo;

class BundleImporter : public QObject
{
    Q_OBJECT

public:
    BundleImporter(QObject *parent = nullptr);
    ~BundleImporter() = default;

    QString importComponent(const QString &bundleDir, const TypeName &type, const QString &qmlFile,
                            const QStringList &files);
    QString unimportComponent(const TypeName &type, const QString &qmlFile);
    Utils::FilePath resolveBundleImportPath(const QString &bundleId);

signals:
    // The metaInfo parameter will be invalid if an error was encountered during
    // asynchronous part of the import. In this case all remaining pending imports have been
    // terminated, and will not receive separate importFinished notifications.
    void importFinished(const QmlDesigner::TypeName &typeName, const QString &bundleId,
                        bool typeAdded);

    void unimportFinished(const QString &bundleId);
    void aboutToUnimport(const TypeName &type, const QString &bundleId);

private:
    void handleImportTimer();
    QVariantHash loadAssetRefMap(const Utils::FilePath &bundlePath);
    void writeAssetRefMap(const Utils::FilePath &bundlePath, const QVariantHash &assetRefMap);

    QTimer m_importTimer;
    int m_importTimerCount = 0;
    QString m_bundleId;
    struct ImportData
    {
        bool isImport = true; // false = unimport
        TypeName simpleType;
        QString moduleName;
        Module module;
        bool typeAdded = false;
    };
    bool m_pendingFullReset = false; // Reset old QMLJS code model (it's used for code view warnings)

    QHash<TypeName, ImportData> m_pendingImports;
};

} // namespace QmlDesigner
