// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <externaldependenciesinterface.h>

class ExternalDependenciesMock : public QmlDesigner::ExternalDependenciesInterface
{
public:
    MOCK_METHOD(double, formEditorDevicePixelRatio, (), (const, override));
    MOCK_METHOD(QString, defaultPuppetFallbackDirectory, (), (const, override));
    MOCK_METHOD(QString, qmlPuppetFallbackDirectory, (), (const, override));
    MOCK_METHOD(QString, defaultPuppetToplevelBuildDirectory, (), (const, override));
    MOCK_METHOD(QUrl, projectUrl, (), (const, override));
    MOCK_METHOD(QString, currentProjectDirPath, (), (const, override));
    MOCK_METHOD(QUrl, currentResourcePath, (), (const, override));
    MOCK_METHOD(void, parseItemLibraryDescriptions, (), (override));
    MOCK_METHOD(const QmlDesigner::DesignerSettings &, designerSettings, (), (const, override));
    MOCK_METHOD(void, undoOnCurrentDesignDocument, (), (override));
    MOCK_METHOD(bool,
                viewManagerUsesRewriterView,
                (class QmlDesigner::RewriterView * view),
                (const, override));
    MOCK_METHOD(void, viewManagerDiableWidgets, (), (override));
    MOCK_METHOD(QString, itemLibraryImportUserComponentsTitle, (), (const, override));
    MOCK_METHOD(bool, isQt6Import, (), (const, override));
    MOCK_METHOD(bool, hasStartupTarget, (), (const, override));
    MOCK_METHOD(QmlDesigner::PuppetStartData,
                puppetStartData,
                (const class QmlDesigner::Model &model),
                (const, override));
    MOCK_METHOD(bool, instantQmlTextUpdate, (), (const, override));
    MOCK_METHOD(Utils::FilePath, qmlPuppetPath, (), (const, override));
    MOCK_METHOD(QStringList, modulePaths, (), (const, override));
    MOCK_METHOD(QStringList, projectModulePaths, (), (const, override));
    MOCK_METHOD(bool, isQt6Project, (), (const, override));
    MOCK_METHOD(QString, qtQuickVersion, (), (const, override));
    MOCK_METHOD(Utils::FilePath, resourcePath, (const QString &relativePath), (const, override));
};
