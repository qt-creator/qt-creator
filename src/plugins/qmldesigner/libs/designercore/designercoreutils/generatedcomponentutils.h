// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <externaldependenciesinterface.h>
#include <qmldesignercorelib_exports.h>

#include <utils/filepath.h>

#include <QString>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT GeneratedComponentUtils {
public:
    GeneratedComponentUtils(ExternalDependenciesInterface &externalDependencies);

    Utils::FilePath generatedComponentsPath() const;
    Utils::FilePath composedEffectsBasePath() const;
    Utils::FilePath composedEffectPath(const QString &effectPath) const;
    Utils::FilePath componentBundlesBasePath() const;
    Utils::FilePath import3dBasePath() const;
    Utils::FilePath materialBundlePath() const;
    Utils::FilePath effectBundlePath() const;
    Utils::FilePath userBundlePath(const QString &bundleId) const;
    Utils::FilePath projectModulePath(bool generateIfNotExists = false) const;

    bool isImport3dPath(const QString &path) const;
    bool isComposedEffectPath(const QString &path) const;
    bool isBundlePath(const QString &path) const;
    bool isGeneratedPath(const QString &path) const;

    QString generatedComponentTypePrefix() const;
    QString import3dTypePrefix() const;
    QString import3dTypePath() const;
    QString componentBundlesTypePrefix() const;
    QString composedEffectsTypePrefix() const;

    QString materialsBundleId() const;
    QString effectsBundleId() const;
    QString userMaterialsBundleId() const;
    QString userEffectsBundleId() const;
    QString user3DBundleId() const;

    QString materialsBundleType() const;
    QString effectsBundleType() const;
    QString userBundleType(const QString &bundleId) const;
    QString userMaterialsBundleType() const;
    QString userEffectsBundleType() const;
    QString user3DBundleType() const;

private:
    ExternalDependenciesInterface &m_externalDependencies;
};

} // namespace QmlDesigner
