// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "cmakewriterv0.h"
#include "cmakegenerator.h"

namespace QmlProjectManager {

namespace GenerateCmake {

const char TEMPLATE_ADD_QML_MODULE[] = R"(
qt6_add_qml_module(%1
    URI "%2"
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
%3))";

CMakeWriterV0::CMakeWriterV0(CMakeGenerator *parent)
    : CMakeWriter(parent)
{}

bool CMakeWriterV0::isPlugin(const NodePtr &node) const
{
    if (node->type == Node::Type::App)
        return !node->files.empty() || !node->singletons.empty() || !node->assets.empty();

    return CMakeWriter::isPlugin(node);
}

void CMakeWriterV0::transformNode(NodePtr &node) const
{
    QTC_ASSERT(parent(), return);

    if (node->name == "src") {
        node->type = Node::Type::Folder;
    } else if (node->name == "content") {
        node->type = Node::Type::Module;
    } else if (node->type == Node::Type::App) {
        Utils::FilePath path = node->dir.pathAppended("main.qml");
        if (!path.exists()) {
            QString text("Expected File not found.");
            CMakeGenerator::logIssue(ProjectExplorer::Task::Error, text, path);
            return;
        }
        if (!parent()->findFile(path))
            node->files.push_back(path);
    }
}

void CMakeWriterV0::writeRootCMakeFile(const NodePtr &node) const
{
    QTC_ASSERT(parent(), return);

    const Utils::FilePath quickControlsPath = node->dir.pathAppended("qtquickcontrols2.conf");
    if (!quickControlsPath.exists()) {
        const QString quickControlsTemplate = readTemplate(":/templates/qtquickcontrols_conf");
        writeFile(quickControlsPath, quickControlsTemplate);
    }

    const Utils::FilePath insightPath = node->dir.pathAppended("insight");
    if (!insightPath.exists()) {
        const QString insightTemplate = readTemplate(":/templates/insight");
        writeFile(insightPath, insightTemplate);
    }

    const Utils::FilePath componentPath = node->dir.pathAppended("qmlcomponents");
    if (!componentPath.exists()) {
        const QString compTemplate = readTemplate(":/templates/qmlcomponents");
        writeFile(componentPath, compTemplate);
    }

    const QString appName = parent()->projectName() + "App";
    const QString qtcontrolsConfFile = getEnvironmentVariable(ENV_VARIABLE_CONTROLCONF);

    QString fileSection = "";
    if (!qtcontrolsConfFile.isEmpty())
        fileSection = QString("\tFILES\n\t\t%1").arg(qtcontrolsConfFile);

    QStringList srcs;
    for (const Utils::FilePath &path : sources(node))
        srcs.push_back(makeRelative(node, path));

    const QString fileTemplate = readTemplate(":/templates/cmakeroot_v0");
    const QString fileContent = fileTemplate.arg(appName, srcs.join(" "), fileSection);

    const Utils::FilePath cmakeFile = node->dir.pathAppended("CMakeLists.txt");
    writeFile(cmakeFile, fileContent);
}

void CMakeWriterV0::writeModuleCMakeFile(const NodePtr &node, const NodePtr &root) const
{
    QTC_ASSERT(parent(), return);

    Utils::FilePath writeToFile = node->dir.pathAppended("CMakeLists.txt");

    QString content(DO_NOT_EDIT_FILE);
    if (node->type == Node::Type::Folder && parent()->hasChildModule(node)) {
        content.append(makeSubdirectoriesBlock(node));
        writeFile(writeToFile, content);
        return;
    }

    content.append(makeSubdirectoriesBlock(node));
    content.append("\n");
    content.append(makeSingletonBlock(node));

    QString qmlModulesContent;
    qmlModulesContent.append(makeQmlFilesBlock(node));

    auto [resources, bigResources] = makeResourcesBlocks(node);
    qmlModulesContent.append(resources);

    if (!qmlModulesContent.isEmpty()) {
        const QString addLibraryTemplate("qt_add_library(%1 STATIC)");
        const QString addModuleTemplate(TEMPLATE_ADD_QML_MODULE);

        content.append(addLibraryTemplate.arg(node->name));
        content.append(addModuleTemplate.arg(node->name, node->uri, qmlModulesContent));
        content.append("\n\n");
    }

    content.append(bigResources);

    if (node->type == Node::Type::App) {
        writeToFile = node->dir.pathAppended("qmlModules");
        QString pluginNames;
        for (const QString &moduleName : plugins(root))
            pluginNames.append("\t" + moduleName + "plugin\n");

        if (!pluginNames.isEmpty())
            content += QString::fromUtf8(TEMPLATE_LINK_LIBRARIES, -1).arg(pluginNames);
    }

    writeFile(writeToFile, content);
}

void CMakeWriterV0::writeSourceFiles(const NodePtr &node, const NodePtr &root) const
{
    QTC_ASSERT(parent(), return);

    const Utils::FilePath srcDir = node->dir;
    if (!srcDir.exists()) {
        srcDir.createDir();

        const Utils::FilePath componentsHeaderPath = srcDir.pathAppended(
            "import_qml_components_plugins.h");
        const QString componentsHeaderContent = readTemplate(
            ":/templates/import_qml_components_h");
        writeFile(componentsHeaderPath, componentsHeaderContent);

        const Utils::FilePath cppFilePath = srcDir.pathAppended("main.cpp");
        const QString cppContent = readTemplate(":/templates/main_cpp_v0");
        writeFile(cppFilePath, cppContent);
    }

    QString fileHeader(
        "/*\n"
        " * This file is automatically generated by Qt Design Studio.\n"
        " * Do not change\n"
        "*/\n\n");

    const Utils::FilePath envHeaderPath = srcDir.pathAppended("app_environment.h");
    QString envHeaderContent(fileHeader);
    envHeaderContent.append("#include <QGuiApplication>\n\n");
    envHeaderContent.append(makeSetEnvironmentFn());
    writeFile(envHeaderPath, envHeaderContent);

    QString importPluginsContent;
    for (const QString &module : plugins(root))
        importPluginsContent.append(QString("Q_IMPORT_QML_PLUGIN(%1)\n").arg(module + "Plugin"));

    QString importPluginsHeader(fileHeader);
    importPluginsHeader.append("#include <QtQml/qqmlextensionplugin.h>\n\n");
    importPluginsHeader.append(importPluginsContent);

    const Utils::FilePath headerFilePath = srcDir.pathAppended("import_qml_plugins.h");
    writeFile(headerFilePath, importPluginsHeader);
}

} // namespace GenerateCmake
} // namespace QmlProjectManager
