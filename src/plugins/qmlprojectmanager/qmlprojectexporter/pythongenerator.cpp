// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythongenerator.h"

#include "cmakewriter.h"
#include "resourcegenerator.h"
#include "../qmlproject.h"

#include <projectexplorer/projectmanager.h>

#include <QMenu>

namespace QmlProjectManager::QmlProjectExporter {

void PythonGenerator::createMenuAction(QObject *parent)
{
    QAction *action = FileGenerator::createMenuAction(parent,
                                                      "Enable Python Generator",
                                                      "QmlProject.EnablePythonGenerator");

    QObject::connect(ProjectExplorer::ProjectManager::instance(),
                     &ProjectExplorer::ProjectManager::startupProjectChanged,
                     [action]() {
                         if (auto buildSystem = QmlBuildSystem::getStartupBuildSystem()) {
                             action->setEnabled(!buildSystem->qtForMCUs());
                             action->setChecked(buildSystem->enablePythonGeneration());
                         }
                     });

    QObject::connect(action, &QAction::toggled, [](bool checked) {
        if (auto buildSystem = QmlBuildSystem::getStartupBuildSystem())
            buildSystem->setEnablePythonGeneration(checked);
    });
}

PythonGenerator::PythonGenerator(QmlBuildSystem *bs)
    : FileGenerator(bs)
{}

void PythonGenerator::updateMenuAction()
{
    FileGenerator::updateMenuAction("QmlProject.EnablePythonGenerator",
                                    [this]() { return buildSystem()->enablePythonGeneration(); });
}

void PythonGenerator::updateProject(QmlProject *project)
{
    if (!isEnabled())
        return;

    Utils::FilePath projectPath = project->rootProjectDirectory();
    Utils::FilePath pythonFolderPath = projectPath.pathAppended("Python");
    if (!pythonFolderPath.exists())
        pythonFolderPath.createDir();

    Utils::FilePath mainFilePath = pythonFolderPath.pathAppended("main.py");
    if (!mainFilePath.exists()) {
        const QString mainFileTemplate = CMakeWriter::readTemplate(
            ":/templates/python_generator_main");
        CMakeWriter::writeFile(mainFilePath, mainFileTemplate);
    }

    Utils::FilePath pyprojectFilePath = pythonFolderPath.pathAppended("pyproject.toml");
    if (!pyprojectFilePath.exists()) {
        const QString pyprojectFileTemplate = CMakeWriter::readTemplate(
            ":/templates/python_pyproject_toml");
        const QString pyprojectFileContent = pyprojectFileTemplate.arg(project->displayName());
        CMakeWriter::writeFile(pyprojectFilePath, pyprojectFileContent);
    }

    Utils::FilePath autogenFolderPath = pythonFolderPath.pathAppended("autogen");
    if (!autogenFolderPath.exists())
        autogenFolderPath.createDir();

    Utils::FilePath settingsFilePath = autogenFolderPath.pathAppended("settings.py");
    const QString settingsFileTemplate = CMakeWriter::readTemplate(
        ":/templates/python_generator_settings");
    const QString settingsFileContent = settingsFileTemplate.arg(buildSystem()->mainFile());
    CMakeWriter::writeFile(settingsFilePath, settingsFileContent);

    // Python code uses the Qt resources collection file (.qrc)
    ResourceGenerator::createQrc(project);
}

/*!
    Regenerates the .qrc resources file
*/
void PythonGenerator::update(const QSet<QString> &added, const QSet<QString> &removed) {
    Q_UNUSED(added)
    Q_UNUSED(removed)
    ResourceGenerator::createQrc(qmlProject());
    // Generated Python code does not need to be updated
};

} // namespace QmlProjectExporter::QmlProjectManager.
