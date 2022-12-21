// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <instances/puppetstartdata.h>

namespace QmlDesigner {

class DesignerSettings;

class ExternalDependenciesInterface
{
    // Tis class worksaround external dependencies. In the long run the caller should
    // be refactored and the dependencies cleaned up. Don't use it as a shortcut to provide
    // functionality!
public:
    ExternalDependenciesInterface() = default;
    ExternalDependenciesInterface(const ExternalDependenciesInterface &) = delete;
    ExternalDependenciesInterface &operator=(const ExternalDependenciesInterface &) = delete;

    virtual double formEditorDevicePixelRatio() const = 0;
    virtual QString defaultPuppetFallbackDirectory() const = 0;
    virtual QString qmlPuppetFallbackDirectory() const = 0;
    virtual QString defaultPuppetToplevelBuildDirectory() const = 0;
    virtual QUrl projectUrl() const = 0;
    virtual QString currentProjectDirPath() const = 0;
    virtual QList<QColor> designerSettingsEdit3DViewBackgroundColor() const = 0;
    virtual QColor designerSettingsEdit3DViewGridColor() const = 0;
    virtual QUrl currentResourcePath() const = 0;
    virtual void parseItemLibraryDescriptions() = 0;
    virtual const DesignerSettings &designerSettings() const = 0;
    virtual void undoOnCurrentDesignDocument() = 0;
    virtual bool viewManagerUsesRewriterView(class RewriterView *view) const = 0;
    virtual void viewManagerDiableWidgets() = 0;
    virtual QString itemLibraryImportUserComponentsTitle() const = 0;
    virtual bool isQt6Import() const = 0;
    virtual bool hasStartupTarget() const = 0;
    virtual PuppetStartData puppetStartData(const class Model &model) const = 0;
    virtual bool instantQmlTextUpdate() const = 0;
};

} // namespace QmlDesigner
