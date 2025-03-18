// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/uniqueobjectptr.h>

#include <QPixmap>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QTemporaryDir)
QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QZipWriter)

namespace QmlDesigner {

class AbstractView;
class BundleImporter;
class ModelNode;
class NodeMetaInfo;

class AssetPath
{
public:
    AssetPath(const Utils::FilePath &bsPath, const QString &relPath, const QStringList &imports = {})
        : basePath(bsPath)
        , relativePath(relPath)
        , importsToRemove(imports)
    {}

    Utils::FilePath absFilPath() const;
    QByteArray fileContent() const;

    bool operator==(const AssetPath &other) const
    {
        return basePath == other.basePath && relativePath == other.relativePath;
    }

    Utils::FilePath basePath;
    QString relativePath;
    QStringList importsToRemove;

private:
    friend size_t qHash(const AssetPath &asset) { return ::qHash(asset.relativePath); }
};

class BundleHelper
{
public:
    BundleHelper(AbstractView *view, QWidget *widget);
    ~BundleHelper();

    void importBundleToProject();
    void exportBundle(const QList<ModelNode> &nodes, const QPixmap &iconPixmap = QPixmap());
    void getImageFromCache(const QString &qmlPath,
                           std::function<void(const QImage &image)> successCallback);
    QString nodeNameToComponentFileName(const QString &name) const;
    QPair<QString, QSet<AssetPath>> modelNodeToQmlString(const ModelNode &node, int depth = 0);
    QString getImportPath() const;
    QSet<AssetPath> getComponentDependencies(const Utils::FilePath &filePath,
                                             const Utils::FilePath &mainCompDir) const;

private:
    void createImporter();
    QString getExportPath(const ModelNode &node) const;
    bool isMaterialBundle(const QString &bundleId) const;
    bool isItemBundle(const QString &bundleId) const;
    void addIconToZip(const QString &iconPath, const auto &image);
    Utils::FilePath componentPath(const ModelNode &node) const;
    QSet<AssetPath> getBundleComponentDependencies(const ModelNode &node) const;
    QJsonObject exportComponent(const ModelNode &node);
    QJsonObject exportNode(const ModelNode &node, const QPixmap &iconPixmap = QPixmap());
    void maybeCloseZip();

    QPointer<AbstractView> m_view;
    QPointer<QWidget> m_widget;
    Utils::UniqueObjectPtr<BundleImporter> m_importer;
    std::unique_ptr<QZipWriter> m_zipWriter;
    std::unique_ptr<QTemporaryDir> m_tempDir;
    int m_remainingFiles = 0;

    static constexpr char BUNDLE_VERSION[] = "1.0";
};

} // namespace QmlDesigner
