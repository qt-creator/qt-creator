// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "designersettings.h"

#include <externaldependenciesinterface.h>

namespace QmlDesigner {

class ExternalDependencies : public ExternalDependenciesInterface
{
public:
    ExternalDependencies(const DesignerSettings &designerSettings)
        : m_designerSettings(designerSettings)
    {}

    double formEditorDevicePixelRatio() const override;
    QString defaultPuppetFallbackDirectory() const override;
    QString qmlPuppetFallbackDirectory() const override;
    QString defaultPuppetToplevelBuildDirectory() const override;
    QUrl projectUrl() const override;
    QString currentProjectDirPath() const override;
    QUrl currentResourcePath() const override;
    void parseItemLibraryDescriptions() override;
    const DesignerSettings &designerSettings() const override;
    void undoOnCurrentDesignDocument() override;
    bool viewManagerUsesRewriterView(RewriterView *view) const override;
    void viewManagerDiableWidgets() override;
    QString itemLibraryImportUserComponentsTitle() const override;
    bool isQt6Import() const override;
    bool hasStartupTarget() const override;
    PuppetStartData puppetStartData(const class Model &model) const override;
    bool instantQmlTextUpdate() const override;
    Utils::FilePath qmlPuppetPath() const override;
    QStringList modulePaths() const override;
    QStringList projectModulePaths() const override;
    bool isQt6Project() const override;
    QString qtQuickVersion() const override;
    Utils::FilePath resourcePath(const QString &relativePath) const override;

private:
    const DesignerSettings &m_designerSettings;
};

} // namespace QmlDesigner
