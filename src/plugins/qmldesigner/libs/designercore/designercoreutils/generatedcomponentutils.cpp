// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "generatedcomponentutils.h"

#include <qmldesignercoreconstants.h>

namespace QmlDesigner {

constexpr QLatin1String componentBundlesMaterialBundleType{"Materials"};
constexpr QLatin1String componentBundlesEffectBundleType{"Effects"};
constexpr QLatin1String componentBundlesType{"Bundles"};
constexpr QLatin1String componentBundlesUser2DBundleType{"User2D"};
constexpr QLatin1String componentBundlesUser3DBundleType{"User3D"};
constexpr QLatin1String componentBundlesUserEffectsBundleType{"UserEffects"};
constexpr QLatin1String componentBundlesUserMaterialBundleType{"UserMaterials"};
constexpr QLatin1String composedEffectType{"Effects"};
constexpr QLatin1String generatedComponentsFolder{"Generated"};
constexpr QLatin1String oldAssetImportFolder{"asset_imports"};
constexpr QLatin1String oldComponentBundleType{"ComponentBundles"};
constexpr QLatin1String oldComponentBundlesMaterialBundleType{"MaterialBundle"};
constexpr QLatin1String oldComponentBundlesEffectBundleType{"EffectBundle"};
constexpr QLatin1String oldEffectFolder{"Effects"};

namespace Constants {} // namespace Constants

bool couldBeProjectModule(const Utils::FilePath &path, const QString &projectName)
{
    if (!path.exists())
        return false;

    Utils::FilePath qmlDirPath = path.pathAppended("qmldir");
    if (qmlDirPath.exists()) {
        Utils::Result<QByteArray> qmldirContents = qmlDirPath.fileContents();
        if (!qmldirContents.has_value())
            return false;

        const QString expectedLine = QLatin1String("module %1").arg(projectName);
        QByteArray fileContents = *qmldirContents;
        QTextStream stream(fileContents);
        while (!stream.atEnd()) {
            QString lineData = stream.readLine().trimmed();
            if (lineData.startsWith(u"module "))
                return lineData == expectedLine;
        }
    }
    if (path.endsWith(projectName))
        return true;

    return false;
}

GeneratedComponentUtils::GeneratedComponentUtils(ExternalDependenciesInterface &externalDependencies)
    : m_externalDependencies(externalDependencies)
{
}

Utils::FilePath GeneratedComponentUtils::generatedComponentsPath() const
{
    Utils::FilePath projectPath = Utils::FilePath::fromString(m_externalDependencies.currentProjectDirPath());
    if (projectPath.isEmpty())
        return {};

    Utils::FilePath assetImportsPath = projectPath.resolvePath(oldAssetImportFolder);
    if (assetImportsPath.exists())
        return assetImportsPath;

    Utils::FilePath componentsPath = projectPath.resolvePath(generatedComponentsFolder);
    if (!componentsPath.exists())
        componentsPath.createDir();

    return componentsPath;
}

Utils::FilePath GeneratedComponentUtils::composedEffectsBasePath() const
{
    Utils::FilePath basePath = generatedComponentsPath();
    if (basePath.isEmpty())
        return {};

    QString effectsImportPath;
    if (basePath.endsWith(oldAssetImportFolder))
        effectsImportPath = oldEffectFolder;
    else
        effectsImportPath = composedEffectType;

    return basePath.resolvePath(effectsImportPath);
}

Utils::FilePath GeneratedComponentUtils::composedEffectPath(const QString &effectPath) const
{
    Utils::FilePath effectsBasePath = composedEffectsBasePath();

    QString effectName = Utils::FilePath::fromString(effectPath).baseName();

    return effectsBasePath.resolvePath(effectName + "/" + effectName + ".qml");
}

Utils::FilePath GeneratedComponentUtils::componentBundlesBasePath() const
{
    Utils::FilePath basePath = generatedComponentsPath();

    if (basePath.isEmpty())
        return {};

    if (basePath.endsWith(generatedComponentsFolder))
        return basePath.resolvePath(componentBundlesType);

    return basePath.resolvePath(oldComponentBundleType);
}

Utils::FilePath GeneratedComponentUtils::import3dBasePath() const
{
    Utils::FilePath basePath = generatedComponentsPath();

    if (basePath.isEmpty())
        return {};

    Utils::FilePath import3dPath;
    if (basePath.endsWith(oldAssetImportFolder))
        return basePath.resolvePath(Constants::oldQuick3dAssetsFolder);

    return basePath.resolvePath(Constants::quick3DComponentsFolder);
}

Utils::FilePath GeneratedComponentUtils::materialBundlePath() const
{
    Utils::FilePath basePath = componentBundlesBasePath();

    if (basePath.isEmpty())
        return {};

    if (basePath.endsWith(oldComponentBundleType))
        return basePath.resolvePath(oldComponentBundlesMaterialBundleType);

    return basePath.resolvePath(componentBundlesMaterialBundleType);
}

Utils::FilePath GeneratedComponentUtils::effectBundlePath() const
{
    Utils::FilePath basePath = componentBundlesBasePath();

    if (basePath.isEmpty())
        return {};

    if (basePath.endsWith(oldComponentBundleType))
        return basePath.resolvePath(oldComponentBundlesEffectBundleType);

    return basePath.resolvePath(componentBundlesEffectBundleType);
}

Utils::FilePath GeneratedComponentUtils::userBundlePath(const QString &bundleId) const
{
    Utils::FilePath basePath = componentBundlesBasePath();
    if (basePath.isEmpty())
        return {};

    if (bundleId == userMaterialsBundleId())
        return basePath.pathAppended(componentBundlesUserMaterialBundleType);

    if (bundleId == userEffectsBundleId())
        return basePath.pathAppended(componentBundlesUserEffectsBundleType);

    if (bundleId == user2DBundleId())
        return basePath.pathAppended(componentBundlesUser2DBundleType);

    if (bundleId == user3DBundleId())
        return basePath.pathAppended(componentBundlesUser3DBundleType);

    qWarning() << __FUNCTION__ << "no bundleType for bundleId:" << bundleId;
    return {};
}

Utils::FilePath GeneratedComponentUtils::projectModulePath(bool generateIfNotExists) const
{
    using Utils::FilePath;
    FilePath projectPath = FilePath::fromString(m_externalDependencies.currentProjectDirPath());

    if (projectPath.isEmpty())
        return {};

    const QString projectName = m_externalDependencies.projectName();

    FilePath newImportDirectory = projectPath.pathAppended(projectName);
    if (couldBeProjectModule(newImportDirectory, projectName))
        return newImportDirectory;

    FilePath oldImportDirectory = projectPath.resolvePath(QLatin1String("imports/") + projectName);
    if (couldBeProjectModule(oldImportDirectory, projectName))
        return oldImportDirectory;

    for (const QString &path : m_externalDependencies.projectModulePaths()) {
        FilePath dir = FilePath::fromString(path);
        if (couldBeProjectModule(dir, projectName))
            return dir;
    }

    if (generateIfNotExists)
        newImportDirectory.createDir();

    return newImportDirectory;
}

bool GeneratedComponentUtils::isImport3dPath(const QString &path) const
{
    return path.contains('/' + QLatin1String(Constants::oldQuick3dAssetsFolder))
           || path.contains(generatedComponentsFolder + '/'
                            + QLatin1String(Constants::quick3DComponentsFolder));
}

bool GeneratedComponentUtils::isComposedEffectPath(const QString &path) const
{
    return path.contains(oldAssetImportFolder)
           || path.contains(generatedComponentsFolder + '/' + composedEffectType);
}

bool GeneratedComponentUtils::isBundlePath(const QString &path) const
{
    return path.contains(componentBundlesTypePrefix().replace('.', '/'));
}

bool GeneratedComponentUtils::isGeneratedPath(const QString &path) const
{
    return path.startsWith(generatedComponentsPath().toFSPathString());
}

QString GeneratedComponentUtils::generatedComponentTypePrefix() const
{
    Utils::FilePath basePath = generatedComponentsPath();
    if (basePath.isEmpty() || basePath.endsWith(oldAssetImportFolder))
        return {};

    return generatedComponentsFolder;
}

QString GeneratedComponentUtils::import3dTypePrefix() const
{
    QString basePrefix = generatedComponentTypePrefix();

    if (basePrefix == generatedComponentsFolder)
        return basePrefix + '.' + Constants::quick3DComponentsFolder;

    return Constants::oldQuick3dAssetsFolder;
}

QString GeneratedComponentUtils::componentBundlesTypePrefix() const
{
    QString basePrefix = generatedComponentTypePrefix();

    if (basePrefix.endsWith(generatedComponentsFolder))
        return basePrefix + '.' + componentBundlesType;

    return oldComponentBundleType;
}

QString GeneratedComponentUtils::composedEffectsTypePrefix() const
{
    QString basePrefix = generatedComponentTypePrefix();

    if (basePrefix == generatedComponentsFolder)
        return basePrefix + '.' + composedEffectType;

    return oldEffectFolder;
}

QString GeneratedComponentUtils::materialsBundleId() const
{
    bool isNewImportDir = generatedComponentTypePrefix().endsWith(generatedComponentsFolder);

    return isNewImportDir ? componentBundlesMaterialBundleType
                          : oldComponentBundlesMaterialBundleType;
}

QString GeneratedComponentUtils::effectsBundleId() const
{
    bool isNewImportDir = generatedComponentTypePrefix().endsWith(generatedComponentsFolder);

    return isNewImportDir ? componentBundlesEffectBundleType
                          : oldComponentBundlesEffectBundleType;
}

QString GeneratedComponentUtils::userMaterialsBundleId() const
{
    return componentBundlesUserMaterialBundleType;
}

QString GeneratedComponentUtils::userEffectsBundleId() const
{
    return componentBundlesUserEffectsBundleType;
}

QString GeneratedComponentUtils::user2DBundleId() const
{
    return componentBundlesUser2DBundleType;
}

QString GeneratedComponentUtils::user3DBundleId() const
{
    return componentBundlesUser3DBundleType;
}

QString GeneratedComponentUtils::materialsBundleType() const
{
    return componentBundlesTypePrefix() + '.' + materialsBundleId();
}

QString GeneratedComponentUtils::effectsBundleType() const
{
    return componentBundlesTypePrefix() + '.' + effectsBundleId();
}

QString GeneratedComponentUtils::userBundleType(const QString &bundleId) const
{
    if (bundleId == userMaterialsBundleId())
        return userMaterialsBundleType();

    if (bundleId == userEffectsBundleId())
        return userEffectsBundleType();

    if (bundleId == user2DBundleId())
        return user2DBundleType();

    if (bundleId == user3DBundleId())
        return user3DBundleType();

    qWarning() << __FUNCTION__ << "no bundleType for bundleId:" << bundleId;
    return {};
}

QString GeneratedComponentUtils::userMaterialsBundleType() const
{
    return componentBundlesTypePrefix() + '.' + userMaterialsBundleId();
}

QString GeneratedComponentUtils::userEffectsBundleType() const
{
    return componentBundlesTypePrefix() + '.' + userEffectsBundleId();
}

QString GeneratedComponentUtils::user2DBundleType() const
{
    return componentBundlesTypePrefix() + '.' + user2DBundleId();
}

QString GeneratedComponentUtils::user3DBundleType() const
{
    return componentBundlesTypePrefix() + '.' + user3DBundleId();
}

QList<Utils::FilePath> GeneratedComponentUtils::imported3dComponents() const
{
    auto fullPath = import3dBasePath();

    if (fullPath.isEmpty())
        return {};

    return collectFiles(fullPath, "qml");
}

QString GeneratedComponentUtils::getImported3dImportName(const Utils::FilePath &qmlFile) const
{
    const QStringList sl = qmlFile.toFSPathString().split('/');
    int i = sl.size() - 4;
    if (i >= 0) {
        if (sl[i] == oldAssetImportFolder)
            return QStringView(u"%1.%2").arg(sl[i + 1], sl[i + 2]);
        else
            return QStringView(u"%1.%2.%3").arg(sl[i], sl[i + 1], sl[i + 2]);
    }
    return {};
}

Utils::FilePath GeneratedComponentUtils::getImported3dQml(const QString &assetPath) const
{
    Utils::FilePath assetFilePath = Utils::FilePath::fromString(assetPath);
    const Utils::Result<QByteArray> data = assetFilePath.fileContents();

    if (!data)
        return {};

    Utils::FilePath assetQmlFilePath = Utils::FilePath::fromUtf8(data.value());
    Utils::FilePath projectPath = Utils::FilePath::fromString(m_externalDependencies.currentProjectDirPath());

    assetQmlFilePath = projectPath.resolvePath(assetQmlFilePath);

    return assetQmlFilePath;
}

// Recursively find files of certain suffix in a dir
QList<Utils::FilePath> GeneratedComponentUtils::collectFiles(const Utils::FilePath &dirPath,
                                                             const QString &suffix) const
{
    if (dirPath.isEmpty())
        return {};

    QList<Utils::FilePath> files;

    const Utils::FilePaths entryList = dirPath.dirEntries(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const Utils::FilePath &entry : entryList) {
        if (entry.isDir())
            files.append(collectFiles(entry.absoluteFilePath(), suffix));
        else if (entry.suffix() == suffix)
            files.append(entry);
    }

    return files;
}

} // namespace QmlDesigner
