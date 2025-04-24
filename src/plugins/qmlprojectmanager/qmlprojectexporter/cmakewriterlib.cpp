// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "cmakewriterlib.h"
#include "cmakegenerator.h"

#include "qmlprojectmanager/buildsystem/qmlbuildsystem.h"

#include <coreplugin/icore.h>

namespace QmlProjectManager {

namespace QmlProjectExporter {

CMakeWriterLib::CMakeWriterLib(CMakeGenerator *parent)
    : CMakeWriterV1(parent)
{ }

QString CMakeWriterLib::mainLibName() const
{
    QTC_ASSERT(parent(), return {});
    return parent()->projectName() + "Lib";
}

void CMakeWriterLib::transformNode(NodePtr &node) const
{
    CMakeWriterV1::transformNode(node);
}

int CMakeWriterLib::identifier() const
{
    return 3;
}

void CMakeWriterLib::writeRootCMakeFile(const NodePtr &node) const
{
    QTC_ASSERT(parent(), return);

    const Utils::FilePath cmakeFolderPath = node->dir.pathAppended("cmake");
    if (!cmakeFolderPath.exists())
        cmakeFolderPath.createDir();

    const Utils::FilePath insightPath = cmakeFolderPath.pathAppended("insight.cmake");
    if (!insightPath.exists()) {
        const QString insightTemplate = readTemplate(":/templates/insight");
        writeFile(insightPath, insightTemplate);
    }

    createDependencies(node->dir);

    const Utils::FilePath sharedFile = node->dir.pathAppended("CMakeLists.txt.shared");
    if (!sharedFile.exists()) {
        const QString sharedTemplate = readTemplate(":/templates/cmake_shared");
        writeFile(sharedFile, sharedTemplate);
    }

    const Utils::FilePath file = node->dir.pathAppended("CMakeLists.txt");
    if (!file.exists()) {
        QString fileSection = "";
        const QString configFile = getEnvironmentVariable(ENV_VARIABLE_CONTROLCONF);
        if (!configFile.isEmpty())
            fileSection = QString("\t\t%1").arg(configFile);

        const QString fileTemplate = readTemplate(":/templates/cmakeroot_lib");
        const QString fileContent = fileTemplate.arg(mainLibName(), fileSection);
        writeFile(file, fileContent);
    }
}

void CMakeWriterLib::writeModuleCMakeFile(const NodePtr &node, const NodePtr &root) const
{
    CMakeWriterV1::writeModuleCMakeFile(node, root);
}

void CMakeWriterLib::writeSourceFiles(const NodePtr &node, const NodePtr &root) const
{
    QTC_ASSERT(parent(), return);
    QTC_ASSERT(parent()->buildSystem(), return);

    const QmlBuildSystem *buildSystem = parent()->buildSystem();

    const Utils::FilePath srcDir = node->dir;
    if (!srcDir.exists())
        srcDir.createDir();

    const Utils::FilePath cmakePath = srcDir.pathAppended("CMakeLists.txt");
    if (!cmakePath.exists()) {
        const QString includeAutogen =
            "\ntarget_include_directories(%1 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})";
        writeFile(cmakePath, includeAutogen.arg(mainLibName()));
    }

    const Utils::FilePath autogenDir = srcDir.pathAppended("autogen");
    if (!autogenDir.exists())
        autogenDir.createDir();

    const Utils::FilePath headerPath = autogenDir.pathAppended("environment.h");

    QString environmentPrefix;
    for (const QString &module : plugins(root))
        environmentPrefix.append(QString("Q_IMPORT_QML_PLUGIN(%1)\n").arg(module + "Plugin"));

    const QString mainFile("const char mainQmlFile[] = \"qrc:/qt/qml/%1\";");
    environmentPrefix.append("\n");
    environmentPrefix.append(mainFile.arg(buildSystem->mainFile()));

    const QString environmentPostfix = makeSetEnvironmentFn();
    const QString headerTemplate = readTemplate(":/templates/environment_h");
    writeFile(headerPath, headerTemplate.arg(environmentPrefix, environmentPostfix));
}

} // namespace QmlProjectExporter
} // namespace QmlProjectManager
