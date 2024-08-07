// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "cmakewriterv1.h"
#include "cmakegenerator.h"

#include "qmlprojectmanager/buildsystem/qmlbuildsystem.h"

namespace QmlProjectManager {

namespace GenerateCmake {

const char TEMPLATE_SRC_CMAKELISTS[] = R"(
target_sources(${CMAKE_PROJECT_NAME} PUBLIC
%2)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Qml))";

CMakeWriterV1::CMakeWriterV1(CMakeGenerator *parent)
    : CMakeWriter(parent)
{}

QString CMakeWriterV1::sourceDirName() const
{
    return "App";
}

void CMakeWriterV1::transformNode(NodePtr &node) const
{
    QTC_ASSERT(parent(), return);

    QString contentDir = parent()->projectName() + "Content";
    if (node->name == contentDir)
        node->type = Node::Type::Module;
}

void CMakeWriterV1::writeRootCMakeFile(const NodePtr &node) const
{
    QTC_ASSERT(parent(), return);
    QTC_ASSERT(parent()->buildSystem(), return);

    const Utils::FilePath cmakeFolderPath = node->dir.pathAppended("cmake");
    if (!cmakeFolderPath.exists())
        cmakeFolderPath.createDir();

    const Utils::FilePath insightPath = cmakeFolderPath.pathAppended("insight.cmake");
    if (!insightPath.exists()) {
        const QString insightTemplate = readTemplate(":/templates/insight");
        writeFile(insightPath, insightTemplate);
    }

    const Utils::FilePath componentPath = cmakeFolderPath.pathAppended("qmlcomponents.cmake");
    if (!componentPath.exists()) {
        const QString compTemplate = readTemplate(":/templates/qmlcomponents");
        writeFile(componentPath, compTemplate);
    }

    const Utils::FilePath sharedFile = node->dir.pathAppended("CMakeLists.txt.shared");
    if (!sharedFile.exists()) {
        const QString sharedTemplate = readTemplate(":/templates/cmake_shared");
        writeFile(sharedFile, sharedTemplate);
    }

    const Utils::FilePath file = node->dir.pathAppended("CMakeLists.txt");
    if (!file.exists()) {
        const QString appName = parent()->projectName() + "App";
        const QString findPackage = makeFindPackageBlock(parent()->buildSystem());

        QString fileSection = "";
        const QString configFile = getEnvironmentVariable(ENV_VARIABLE_CONTROLCONF);
        if (!configFile.isEmpty())
            fileSection = QString("\t\t%1").arg(configFile);

        const QString fileTemplate = readTemplate(":/templates/cmakeroot_v1");
        const QString fileContent = fileTemplate.arg(appName, findPackage, fileSection);
        writeFile(file, fileContent);
    }
}

void CMakeWriterV1::writeModuleCMakeFile(const NodePtr &node, const NodePtr &) const
{
    QTC_ASSERT(parent(), return);

    if (node->type == Node::Type::App) {
        const Utils::FilePath userFile = node->dir.pathAppended("qds.cmake");
        QString userFileContent(DO_NOT_EDIT_FILE);
        userFileContent.append(makeSubdirectoriesBlock(node));
        userFileContent.append("\n");

        QString pluginNames;
        std::vector<QString> plugs = plugins(node);
        for (size_t i = 0; i < plugs.size(); ++i) {
            pluginNames.append("\t" + plugs[i] + "plugin");
            if (i != plugs.size() - 1)
                pluginNames.append("\n");
        }

        QString linkLibrariesTemplate(
            "target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE\n"
            "%1)");

        userFileContent.append(linkLibrariesTemplate.arg(pluginNames));
        writeFile(userFile, userFileContent);
        return;
    }

    Utils::FilePath writeToFile = node->dir.pathAppended("CMakeLists.txt");
    if (node->type == Node::Type::Folder && parent()->hasChildModule(node)) {
        QString content(DO_NOT_EDIT_FILE);
        content.append(makeSubdirectoriesBlock(node));
        writeFile(writeToFile, content);
        return;
    }

    QString prefix;
    prefix.append(makeSubdirectoriesBlock(node));
    prefix.append(makeSingletonBlock(node));

    auto [resources, bigResources] = makeResourcesBlocks(node);
    QString moduleContent;
    moduleContent.append(makeQmlFilesBlock(node));
    moduleContent.append(resources);

    QString postfix;
    postfix.append(bigResources);

    const QString fileTemplate = readTemplate(":/templates/cmakemodule_v1");
    const QString fileContent = fileTemplate
        .arg(node->name, node->uri, prefix, moduleContent, postfix);
    writeFile(writeToFile, fileContent);
}

void CMakeWriterV1::writeSourceFiles(const NodePtr &node, const NodePtr &root) const
{
    QTC_ASSERT(parent(), return);
    QTC_ASSERT(parent()->buildSystem(), return);

    const QmlBuildSystem *buildSystem = parent()->buildSystem();

    const Utils::FilePath srcDir = node->dir;
    if (!srcDir.exists())
        srcDir.createDir();

    const Utils::FilePath autogenDir = srcDir.pathAppended("autogen");
    if (!autogenDir.exists())
        autogenDir.createDir();

    const Utils::FilePath mainCppPath = srcDir.pathAppended("main.cpp");
    if (!mainCppPath.exists()) {
        const QString cppContent = readTemplate(":/templates/main_cpp_v1");
        writeFile(mainCppPath, cppContent);
    }

    const Utils::FilePath cmakePath = srcDir.pathAppended("CMakeLists.txt");
    if (!cmakePath.exists()) {
        std::vector<Utils::FilePath> sourcePaths = sources(node);
        if (sourcePaths.empty())
            sourcePaths.push_back(mainCppPath);

        QString srcs = {};
        for (const Utils::FilePath &src : sourcePaths)
            srcs.append("\t" + makeRelative(node, src) + "\n");

        QString fileTemplate = QString::fromUtf8(TEMPLATE_SRC_CMAKELISTS, -1).arg(srcs);
        writeFile(cmakePath, fileTemplate);
    }

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

} // namespace GenerateCmake
} // namespace QmlProjectManager
