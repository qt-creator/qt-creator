// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "common.h"
#include "projectitem/qmlprojectitem.h"

static QString testDataDir{testDataRootDir.path() + "/getter-setter"};
static QString qmlProjectFilePath1(testDataDir + "/testfile-1.qmlproject");
static QString qmlProjectFilePath2(testDataDir + "/testfile-2.qmlproject");

struct TestDataSet
{
public:
    const QmlProjectManager::QmlProjectItem projectItem1{
        Utils::FilePath::fromString(qmlProjectFilePath1)};
    QmlProjectManager::QmlProjectItem projectItem2{Utils::FilePath::fromString(qmlProjectFilePath2)};
} dataSet;

TEST(QmlProjectProjectItemGetterTests, GetMainFileProject)
{
    ASSERT_EQ(dataSet.projectItem1.mainFile(), "content/App.qml");
    ASSERT_EQ(dataSet.projectItem2.mainFile(), "");
}

TEST(QmlProjectProjectItemGetterTests, GetMainUIFileProject)
{
    ASSERT_EQ(dataSet.projectItem1.mainUiFile(), "Screen01.ui.qml");
    ASSERT_EQ(dataSet.projectItem2.mainUiFile(), "");
}

TEST(QmlProjectProjectItemGetterTests, GetMcuProject)
{
    ASSERT_EQ(dataSet.projectItem1.isQt4McuProject(), true);
    ASSERT_EQ(dataSet.projectItem2.isQt4McuProject(), false);
}

TEST(QmlProjectProjectItemGetterTests, GetQt6Project)
{
    ASSERT_EQ(dataSet.projectItem1.isQt6Project(), true);
    ASSERT_EQ(dataSet.projectItem2.isQt6Project(), false);
}

TEST(QmlProjectProjectItemGetterTests, GetSourceDirectory)
{
    ASSERT_EQ(dataSet.projectItem1.sourceDirectory().path(), testDataDir);
}

TEST(QmlProjectProjectItemGetterTests, GetTargetDirectory)
{
    ASSERT_EQ(dataSet.projectItem1.targetDirectory(), "/opt/targetDirectory");
    ASSERT_EQ(dataSet.projectItem2.targetDirectory(), "");
}

TEST(QmlProjectProjectItemGetterTests, GetImportPaths)
{
    QString valsToCompare1 = dataSet.projectItem1.importPaths().join(";");
    QString valsToCompare2 = dataSet.projectItem2.importPaths().join(";");

    ASSERT_EQ(valsToCompare1.toStdString(), "imports;asset_imports");
    ASSERT_EQ(valsToCompare2.toStdString(), "");
}

TEST(QmlProjectProjectItemGetterTests, GetFileSelectors)
{
    QString valsToCompare1 = dataSet.projectItem1.fileSelectors().join(";");
    QString valsToCompare2 = dataSet.projectItem2.fileSelectors().join(";");

    ASSERT_EQ(valsToCompare1.toStdString(), "WXGA;darkTheme;ShowIndicator");
    ASSERT_EQ(valsToCompare2.toStdString(), "");
}

TEST(QmlProjectProjectItemGetterTests, GetMultiLanguageSupport)
{
    ASSERT_EQ(dataSet.projectItem1.multilanguageSupport(), true);
    ASSERT_EQ(dataSet.projectItem2.multilanguageSupport(), false);
}

TEST(QmlProjectProjectItemGetterTests, GetSupportedLanguages)
{
    QString valsToCompare1 = dataSet.projectItem1.supportedLanguages().join(";");
    QString valsToCompare2 = dataSet.projectItem2.supportedLanguages().join(";");

    ASSERT_EQ(valsToCompare1.toStdString(), "en;fr");
    ASSERT_EQ(valsToCompare2.toStdString(), "");
}

TEST(QmlProjectProjectItemGetterTests, GetPrimaryLanguage)
{
    ASSERT_EQ(dataSet.projectItem1.primaryLanguage(), "en");
    ASSERT_EQ(dataSet.projectItem2.primaryLanguage(), "");
}

TEST(QmlProjectProjectItemGetterTests, GetWidgetApp)
{
    ASSERT_EQ(dataSet.projectItem1.widgetApp(), true);
    ASSERT_EQ(dataSet.projectItem2.widgetApp(), false);
}

TEST(QmlProjectProjectItemGetterTests, GetFileList)
{
    QString valsToCompare1, valsToCompare2;

    for (const auto &file : dataSet.projectItem1.files()) {
        valsToCompare1.append(file.path()).append(";");
    }

    for (const auto &file : dataSet.projectItem2.files()) {
        valsToCompare2.append(file.path()).append(";");
    }

    valsToCompare1.remove(valsToCompare1.length() - 1, 1);
    valsToCompare2.remove(valsToCompare2.length() - 1, 1);

    ASSERT_EQ(valsToCompare1.toStdString(),
              testDataDir.toStdString() + "/qtquickcontrols2.conf");
    ASSERT_EQ(valsToCompare2.toStdString(), "");
}

TEST(QmlProjectProjectItemGetterTests, GetShaderToolArgs)
{
    QString valsToCompare1 = dataSet.projectItem1.shaderToolArgs().join(";");
    QString valsToCompare2 = dataSet.projectItem2.shaderToolArgs().join(";");

    ASSERT_EQ(valsToCompare1.toStdString(), "-s;--glsl;\"100 es,120,150\";--hlsl;50;--msl;12");
    ASSERT_EQ(valsToCompare2.toStdString(), "");
}

TEST(QmlProjectProjectItemGetterTests, GetShaderToolFiles)
{
    QString valsToCompare1 = dataSet.projectItem1.shaderToolFiles().join(";");
    QString valsToCompare2 = dataSet.projectItem2.shaderToolFiles().join(";");

    ASSERT_EQ(valsToCompare1.toStdString(), "content/shaders/*");
    ASSERT_EQ(valsToCompare2.toStdString(), "");
}

TEST(QmlProjectProjectItemGetterTests, GetEnvironment)
{
    Utils::EnvironmentItems env1 = dataSet.projectItem1.environment();
    Utils::EnvironmentItems env2 = dataSet.projectItem2.environment();

    ASSERT_EQ(env1[0].value.toStdString(), "qtquickcontrols2.conf");
    ASSERT_EQ(env2.isEmpty(), true);
}

TEST(QmlProjectProjectItemGetterTests, GetForceFreeType)
{
    ASSERT_EQ(dataSet.projectItem1.forceFreeType(), true);
    ASSERT_EQ(dataSet.projectItem2.forceFreeType(), false);
}
