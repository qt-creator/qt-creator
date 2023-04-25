// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "common.h"
#include "projectitem/qmlprojectitem.h"

using namespace QmlProjectManager;

static QString filePath(testDataRootDir.path() + "/getter-setter/testfile-1.qmlproject");
static QmlProjectItem projectItem{Utils::FilePath::fromString(filePath)};

#define call_mem_fn(ptr) ((projectItem).*(ptr))

template<typename T>
void testerTemplate(void (QmlProjectItem::*setterFunc)(const T &),
                    T (QmlProjectItem::*getterFunc)(void) const,
                    const T &testingData)
{
    call_mem_fn(setterFunc)({testingData});
    ASSERT_EQ(call_mem_fn(getterFunc)(), testingData);
}

template<typename T, typename Y>
void testerTemplate(void (QmlProjectItem::*setterFunc)(const T &),
                    T (QmlProjectItem::*getterFunc)(void) const,
                    void (QmlProjectItem::*adderFunc)(const Y &),
                    const Y &testingData)
{
    call_mem_fn(setterFunc)({testingData});
    ASSERT_EQ(call_mem_fn(getterFunc)(), T{testingData});

    call_mem_fn(setterFunc)({});
    call_mem_fn(adderFunc)(testingData);
    ASSERT_EQ(call_mem_fn(getterFunc)(), T{testingData});
}

TEST(QmlProjectProjectItemSetterTests, SetMainFileProject)
{
    testerTemplate<QString>(&QmlProjectItem::setMainFile, &QmlProjectItem::mainFile, "testing");
}

TEST(QmlProjectProjectItemSetterTests, SetMainUIFileProject)
{
    testerTemplate<QString>(&QmlProjectItem::setMainUiFile, &QmlProjectItem::mainUiFile, "testing");
}

TEST(QmlProjectProjectItemSetterTests, SetImportPaths)
{
    testerTemplate<QStringList, QString>(&QmlProjectItem::setImportPaths,
                                         &QmlProjectItem::importPaths,
                                         &QmlProjectItem::addImportPath,
                                         "testing");
}

TEST(QmlProjectProjectItemSetterTests, SetFileSelectors)
{
    testerTemplate<QStringList, QString>(&QmlProjectItem::setFileSelectors,
                                         &QmlProjectItem::fileSelectors,
                                         &QmlProjectItem::addFileSelector,
                                         "testing");
}

TEST(QmlProjectProjectItemSetterTests, SetMultiLanguageSupport)
{
    testerTemplate<bool>(&QmlProjectItem::setMultilanguageSupport,
                         &QmlProjectItem::multilanguageSupport,
                         true);

    testerTemplate<bool>(&QmlProjectItem::setMultilanguageSupport,
                         &QmlProjectItem::multilanguageSupport,
                         false);
}

TEST(QmlProjectProjectItemSetterTests, SetSupportedLanguages)
{
    testerTemplate<QStringList, QString>(&QmlProjectItem::setSupportedLanguages,
                                         &QmlProjectItem::supportedLanguages,
                                         &QmlProjectItem::addSupportedLanguage,
                                         "testing");
}

TEST(QmlProjectProjectItemSetterTests, SetPrimaryLanguage)
{
    testerTemplate<QString>(&QmlProjectItem::setPrimaryLanguage,
                            &QmlProjectItem::primaryLanguage,
                            "testing");
}

TEST(QmlProjectProjectItemSetterTests, SetWidgetApp)
{
    testerTemplate<bool>(&QmlProjectItem::setWidgetApp, &QmlProjectItem::widgetApp, true);
    testerTemplate<bool>(&QmlProjectItem::setWidgetApp, &QmlProjectItem::widgetApp, false);
}

TEST(QmlProjectProjectItemSetterTests, SetShaderToolArgs)
{
    testerTemplate<QStringList, QString>(&QmlProjectItem::setShaderToolArgs,
                                         &QmlProjectItem::shaderToolArgs,
                                         &QmlProjectItem::addShaderToolArg,
                                         "testing");
}

TEST(QmlProjectProjectItemSetterTests, SetShaderToolFiles)
{
    testerTemplate<QStringList, QString>(&QmlProjectItem::setShaderToolFiles,
                                         &QmlProjectItem::shaderToolFiles,
                                         &QmlProjectItem::addShaderToolFile,
                                         "testing");
}

TEST(QmlProjectProjectItemSetterTests, SetForceFreeType)
{
    testerTemplate<bool>(&QmlProjectItem::setForceFreeType, &QmlProjectItem::forceFreeType, true);
    testerTemplate<bool>(&QmlProjectItem::setForceFreeType, &QmlProjectItem::forceFreeType, false);
}

TEST(QmlProjectProjectItemSetterTests, SetQtVersion)
{
    testerTemplate<QString>(&QmlProjectItem::setVersionQt, &QmlProjectItem::versionQt, "6");
    testerTemplate<QString>(&QmlProjectItem::setVersionQt, &QmlProjectItem::versionQt, "5.3");
}

TEST(QmlProjectProjectItemSetterTests, SetQtQuickVersion)
{
    testerTemplate<QString>(&QmlProjectItem::setVersionQtQuick, &QmlProjectItem::versionQtQuick, "6");
    testerTemplate<QString>(&QmlProjectItem::setVersionQtQuick, &QmlProjectItem::versionQtQuick, "5.3");
}

TEST(QmlProjectProjectItemSetterTests, SetDesignStudio)
{
    testerTemplate<QString>(&QmlProjectItem::setVersionDesignStudio, &QmlProjectItem::versionDesignStudio, "6");
    testerTemplate<QString>(&QmlProjectItem::setVersionDesignStudio, &QmlProjectItem::versionDesignStudio, "5.3");
}

/**
TEST(QmlProjectProjectItemSetterTests, SetEnvironment)
{
    //FIXME: implement this
}

*/

// not available as of now
//TEST(QmlProjectProjectItemSetterTests, SetMcuProject)
//{
//    ASSERT_EQ(dataSet.projectItem1.isQt4McuProject(), true);
//    ASSERT_EQ(dataSet.projectItem2.isQt4McuProject(), false);
//}

// not available as of now
//TEST(QmlProjectProjectItemSetterTests, SetSourceDirectory)
//{
//    ASSERT_EQ(dataSet.projectItem1.sourceDirectory(), testDataDir.path());
//}

// not available as of now
//TEST(QmlProjectProjectItemSetterTests, SetTargetDirectory)
//{
//    ASSERT_EQ(dataSet.projectItem1.targetDirectory(), "/opt/targetDirectory");
//    ASSERT_EQ(dataSet.projectItem2.targetDirectory(), "");
//}
