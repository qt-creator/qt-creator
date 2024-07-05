// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "generatedcomponentutils.h"
#include <qmldesignerconstants.h>

namespace QmlDesigner {

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

    Utils::FilePath assetImportsPath = projectPath.resolvePath(QLatin1String(Constants::OLD_ASSET_IMPORT_FOLDER));
    if (assetImportsPath.exists())
        return assetImportsPath;

    Utils::FilePath componentsPath = projectPath.resolvePath(QLatin1String(Constants::GENERATED_COMPONENTS_FOLDER));
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
    if (basePath.endsWith(Constants::OLD_ASSET_IMPORT_FOLDER))
        effectsImportPath = Constants::OLD_EFFECTS_FOLDER;
    else
        effectsImportPath = Constants::COMPOSED_EFFECTS_TYPE;

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

    if (basePath.endsWith(Constants::GENERATED_COMPONENTS_FOLDER))
        return basePath.resolvePath(QLatin1String(Constants::COMPONENT_BUNDLES_TYPE));

    return basePath.resolvePath(QLatin1String(Constants::OLD_COMPONENT_BUNDLES_TYPE));
}

Utils::FilePath GeneratedComponentUtils::import3dBasePath() const
{
    Utils::FilePath basePath = generatedComponentsPath();

    if (basePath.isEmpty())
        return {};

    Utils::FilePath import3dPath;
    if (basePath.endsWith(Constants::OLD_ASSET_IMPORT_FOLDER))
        return basePath.resolvePath(QLatin1String(Constants::OLD_QUICK_3D_ASSETS_FOLDER));

    return basePath.resolvePath(QLatin1String(Constants::QUICK_3D_COMPONENTS_FOLDER));
}

Utils::FilePath GeneratedComponentUtils::materialBundlePath() const
{
    Utils::FilePath basePath = componentBundlesBasePath();

    if (basePath.isEmpty())
        return {};

    if (basePath.endsWith(Constants::OLD_COMPONENT_BUNDLES_TYPE))
        return basePath.resolvePath(QLatin1String(Constants::OLD_COMPONENT_BUNDLES_MATERIAL_BUNDLE_TYPE));

    return basePath.resolvePath(QLatin1String(Constants::COMPONENT_BUNDLES_MATERIAL_BUNDLE_TYPE));
}

Utils::FilePath GeneratedComponentUtils::effectBundlePath() const
{
    Utils::FilePath basePath = componentBundlesBasePath();

    if (basePath.isEmpty())
        return {};

    if (basePath.endsWith(Constants::OLD_COMPONENT_BUNDLES_TYPE))
        return basePath.resolvePath(QLatin1String(Constants::OLD_COMPONENT_BUNDLES_EFFECT_BUNDLE_TYPE));

    return basePath.resolvePath(QLatin1String(Constants::COMPONENT_BUNDLES_EFFECT_BUNDLE_TYPE));
}

Utils::FilePath GeneratedComponentUtils::userBundlePath(const QString &bundleId) const
{
    Utils::FilePath basePath = componentBundlesBasePath();
    if (basePath.isEmpty())
        return {};

    if (bundleId == userMaterialsBundleId())
        return basePath.pathAppended(Constants::COMPONENT_BUNDLES_USER_MATERIAL_BUNDLE_TYPE);

    if (bundleId == userEffectsBundleId())
        return basePath.pathAppended(Constants::COMPONENT_BUNDLES_USER_EFFECT_BUNDLE_TYPE);

    if (bundleId == user3DBundleId())
        return basePath.pathAppended(Constants::COMPONENT_BUNDLES_USER_3D_BUNDLE_TYPE);

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
    return path.contains('/' + QLatin1String(Constants::OLD_QUICK_3D_ASSETS_FOLDER))
           || path.contains(QLatin1String(Constants::GENERATED_COMPONENTS_FOLDER) + '/'
                            + QLatin1String(Constants::QUICK_3D_COMPONENTS_FOLDER));
}

bool GeneratedComponentUtils::isComposedEffectPath(const QString &path) const
{
    return path.contains(Constants::OLD_EFFECTS_IMPORT_FOLDER)
           || path.contains(QLatin1String(Constants::GENERATED_COMPONENTS_FOLDER) + '/'
                            + QLatin1String(Constants::COMPOSED_EFFECTS_TYPE));
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
    if (basePath.isEmpty() || basePath.endsWith(Constants::OLD_ASSET_IMPORT_FOLDER))
        return {};

    return Constants::GENERATED_COMPONENTS_FOLDER;
}

QString GeneratedComponentUtils::import3dTypePrefix() const
{
    QString basePrefix = generatedComponentTypePrefix();

    if (basePrefix == Constants::GENERATED_COMPONENTS_FOLDER)
        return basePrefix + '.' + QLatin1String(Constants::QUICK_3D_COMPONENTS_FOLDER);

    return Constants::OLD_QUICK_3D_ASSETS_FOLDER;
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

    if (basePrefix.endsWith(Constants::GENERATED_COMPONENTS_FOLDER))
        return basePrefix + '.' + QLatin1String(Constants::COMPONENT_BUNDLES_TYPE);

    return Constants::OLD_COMPONENT_BUNDLES_TYPE;
}

QString GeneratedComponentUtils::composedEffectsTypePrefix() const
{
    QString basePrefix = generatedComponentTypePrefix();

    if (basePrefix == Constants::GENERATED_COMPONENTS_FOLDER)
        return basePrefix + '.' + QLatin1String(Constants::COMPOSED_EFFECTS_TYPE);

    return Constants::OLD_EFFECTS_FOLDER;
}

QString GeneratedComponentUtils::materialsBundleId() const
{
    bool isNewImportDir = generatedComponentTypePrefix().endsWith(Constants::GENERATED_COMPONENTS_FOLDER);

    return QLatin1String(isNewImportDir ? Constants::COMPONENT_BUNDLES_MATERIAL_BUNDLE_TYPE
                                        : Constants::OLD_COMPONENT_BUNDLES_MATERIAL_BUNDLE_TYPE);
}

QString GeneratedComponentUtils::effectsBundleId() const
{
    bool isNewImportDir = generatedComponentTypePrefix().endsWith(Constants::GENERATED_COMPONENTS_FOLDER);

    return QLatin1String(isNewImportDir ? Constants::COMPONENT_BUNDLES_EFFECT_BUNDLE_TYPE
                                        : Constants::OLD_COMPONENT_BUNDLES_EFFECT_BUNDLE_TYPE);
}

QString GeneratedComponentUtils::userMaterialsBundleId() const
{
    return QLatin1String(Constants::COMPONENT_BUNDLES_USER_MATERIAL_BUNDLE_TYPE);
}

QString GeneratedComponentUtils::userEffectsBundleId() const
{
    return QLatin1String(Constants::COMPONENT_BUNDLES_USER_EFFECT_BUNDLE_TYPE);
}

QString GeneratedComponentUtils::user3DBundleId() const
{
    return QLatin1String(Constants::COMPONENT_BUNDLES_USER_3D_BUNDLE_TYPE);
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
