// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "generatedcomponentutils.h"

#include <qmldesignercoreconstants.h>

namespace QmlDesigner {

constexpr QLatin1String componentBundlesMaterialBundleType{"Materials"};
constexpr QLatin1String componentBundlesType{"Bundles"};
constexpr QLatin1String componentBundlesUserMaterialBundleType{"UserMaterials"};
constexpr QLatin1String composedEffectType{"Effects"};
constexpr QLatin1String generatedComponentsFolder{"Generated"};
constexpr QLatin1String oldAssetImportFolder{"asset_imports"};
constexpr QLatin1String oldComponentBundleType{"ComponentBundles"};
constexpr QLatin1String oldComponentsBundlesMaterialBundleType{"MaterialBundle"};
constexpr QLatin1String oldEffectFolder{"Effects"};

namespace Constants {} // namespace Constants

bool couldBeProjectModule(const Utils::FilePath &path, const QString &projectName)
{
    if (!path.exists())
        return false;

    Utils::FilePath qmlDirPath = path.pathAppended("qmldir");
    if (qmlDirPath.exists()) {
        Utils::expected_str<QByteArray> qmldirContents = qmlDirPath.fileContents();
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

    if (basePath.endsWith(Constants::quick3DComponentsFolder))
        return basePath.resolvePath(oldComponentsBundlesMaterialBundleType);

    return basePath.resolvePath(QLatin1String(componentBundlesMaterialBundleType));
}

Utils::FilePath GeneratedComponentUtils::effectBundlePath() const
{
    Utils::FilePath basePath = componentBundlesBasePath();

    if (basePath.isEmpty())
        return {};

    if (basePath.endsWith(Constants::quick3DComponentsFolder))
        return basePath.resolvePath(componentBundlesMaterialBundleType);

    return basePath.resolvePath(componentBundlesMaterialBundleType);
}

Utils::FilePath GeneratedComponentUtils::userBundlePath(const QString &bundleId) const
{
    Utils::FilePath basePath = componentBundlesBasePath();
    if (basePath.isEmpty())
        return {};

    if (bundleId == userMaterialsBundleId())
        return basePath.pathAppended(componentBundlesUserMaterialBundleType);

    if (bundleId == userEffectsBundleId())
        return basePath.pathAppended(componentBundlesUserMaterialBundleType);

    if (bundleId == user3DBundleId())
        return basePath.pathAppended(componentBundlesUserMaterialBundleType);

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

QString GeneratedComponentUtils::import3dTypePath() const
{
    QString prefix = import3dTypePrefix();
    prefix.replace('.', '/');
    return prefix;
}

QString GeneratedComponentUtils::componentBundlesTypePrefix() const
{
    QString basePrefix = generatedComponentTypePrefix();

    if (basePrefix.endsWith(generatedComponentsFolder))
        return basePrefix + '.' + componentBundlesType;

    return componentBundlesType;
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
                          : oldComponentsBundlesMaterialBundleType;
}

QString GeneratedComponentUtils::effectsBundleId() const
{
    bool isNewImportDir = generatedComponentTypePrefix().endsWith(generatedComponentsFolder);

    return QLatin1String(isNewImportDir ? componentBundlesMaterialBundleType
                                        : componentBundlesMaterialBundleType);
}

QString GeneratedComponentUtils::userMaterialsBundleId() const
{
    return componentBundlesMaterialBundleType;
}

QString GeneratedComponentUtils::userEffectsBundleId() const
{
    return componentBundlesUserMaterialBundleType;
}

QString GeneratedComponentUtils::user3DBundleId() const
{
    return componentBundlesMaterialBundleType;
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

QString GeneratedComponentUtils::user3DBundleType() const
{
    return componentBundlesTypePrefix() + '.' + user3DBundleId();
}

} // namespace QmlDesigner
