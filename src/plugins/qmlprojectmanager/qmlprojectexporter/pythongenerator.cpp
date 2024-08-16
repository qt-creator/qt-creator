// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythongenerator.h"
#include "cmakewriter.h"

#include "projectexplorer/projectmanager.h"
#include "qmlprojectmanager/qmlproject.h"

#include <QMenu>

namespace QmlProjectManager {

namespace QmlProjectExporter {

const char *PYTHON_MAIN_FILE_TEMPLATE = R"(
import os
import sys
from pathlib import Path

from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine

from autogen.settings import url, import_paths

if __name__ == '__main__':
    app = QGuiApplication(sys.argv)
    engine = QQmlApplicationEngine()

    app_dir = Path(__file__).parent.parent

    engine.addImportPath(os.fspath(app_dir))
    for path in import_paths:
        engine.addImportPath(os.fspath(app_dir / path))

    engine.load(os.fspath(app_dir/url))
    if not engine.rootObjects():
        sys.exit(-1)
    sys.exit(app.exec())
)";

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
    FileGenerator::updateMenuAction(
        "QmlProject.EnablePythonGenerator",
        [this]() { return buildSystem()->enablePythonGeneration(); });
}

void PythonGenerator::updateProject(QmlProject *project)
{
    if (!isEnabled())
        return;

    Utils::FilePath projectPath = project->rootProjectDirectory();
    Utils::FilePath pythonPath = projectPath.pathAppended("Python");
    if (!pythonPath.exists())
        pythonPath.createDir();

    Utils::FilePath mainFile = pythonPath.pathAppended("main.py");
    if (!mainFile.exists()) {
        const QString mainContent = QString::fromUtf8(PYTHON_MAIN_FILE_TEMPLATE);
        CMakeWriter::writeFile(mainFile, mainContent);
    }

    Utils::FilePath autogenPath = pythonPath.pathAppended("autogen");
    if (!autogenPath.exists())
        autogenPath.createDir();

    Utils::FilePath settingsPath = autogenPath.pathAppended("settings.py");
    CMakeWriter::writeFile(settingsPath, settingsFileContent());
}

QString PythonGenerator::settingsFileContent() const
{
    QTC_ASSERT(buildSystem(), return {});

    QString content("\n");
    content.append("url = \"" + buildSystem()->mainFile() + "\"\n");

    content.append("import_paths = [\n");
    for (const QString &path : buildSystem()->importPaths())
        content.append("\t\"" + path + "\",\n");
    content.append("]\n");

    return content;
}

} // namespace QmlProjectExporter.
} // namespace QmlProjectManager.
