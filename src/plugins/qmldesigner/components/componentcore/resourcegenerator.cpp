// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <resourcegenerator.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/buildsystem/qmlbuildsystem.h>
#include <qmlprojectmanager/qmlprojectconstants.h>
#include <qmlprojectmanager/qmlprojectexporter/filetypes.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QMessageBox>
#include <QProgressDialog>
#include <QTemporaryFile>
#include <QXmlStreamWriter>
#include <QtConcurrent>

using namespace Utils;

namespace QmlDesigner::ResourceGenerator {

void generateMenuEntry(QObject *parent)
{
    const Core::Context projectContext(QmlProjectManager::Constants::QML_PROJECT_ID);
    // ToDo: move this to QtCreator and add tr to the string then
    auto action = new QAction(QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                          "Generate QRC Resource File..."),
                              parent);
    action->setEnabled(ProjectExplorer::ProjectManager::startupProject() != nullptr);
    // todo make it more intelligent when it gets enabled
    QObject::connect(ProjectExplorer::ProjectManager::instance(),
                     &ProjectExplorer::ProjectManager::startupProjectChanged,
                     [action]() {
                         if (auto buildSystem = QmlProjectManager::QmlBuildSystem::getStartupBuildSystem())
                             action->setEnabled(!buildSystem->qtForMCUs());
                     });

    Core::Command *cmd = Core::ActionManager::registerAction(action, "QmlProject.CreateResource");
    QObject::connect(action, &QAction::triggered, []() {
        auto project = ProjectExplorer::ProjectManager::startupProject();
        QTC_ASSERT(project, return);
        const FilePath projectPath = project->projectFilePath().parentDir();
        auto qrcFilePath = Core::DocumentManager::getSaveFileNameWithExtension(
            QCoreApplication::translate("QmlDesigner::GenerateResource", "Save Project as QRC File"),
            projectPath.pathAppended(project->displayName() + ".qrc"),
            QCoreApplication::translate("QmlDesigner::GenerateResource", "QML Resource File (*.qrc)"));

        if (qrcFilePath.toString().isEmpty())
            return;

        createQrcFile(qrcFilePath);

        Core::AsynchronousMessageBox::information(
            QCoreApplication::translate("QmlDesigner::GenerateResource", "Success"),
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "Successfully generated QRC resource file\n %1")
                .arg(qrcFilePath.toString()));
    });

    // ToDo: move this to QtCreator and add tr to the string then
    auto rccAction = new QAction(QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                             "Generate Deployable Package..."),
                                 parent);
    rccAction->setEnabled(ProjectExplorer::ProjectManager::startupProject() != nullptr);
    QObject::connect(ProjectExplorer::ProjectManager::instance(),
                     &ProjectExplorer::ProjectManager::startupProjectChanged,
                     [rccAction]() {
                         rccAction->setEnabled(ProjectExplorer::ProjectManager::startupProject());
                     });

    Core::Command *cmd2 = Core::ActionManager::registerAction(rccAction,
                                                              "QmlProject.CreateRCCResource");
    QObject::connect(rccAction, &QAction::triggered, []() {
        auto project = ProjectExplorer::ProjectManager::startupProject();
        QTC_ASSERT(project, return);
        const FilePath projectPath = project->projectFilePath().parentDir();
        const FilePath qmlrcFilePath = Core::DocumentManager::getSaveFileNameWithExtension(
            QCoreApplication::translate("QmlDesigner::GenerateResource", "Save Project as Resource"),
            projectPath.pathAppended(project->displayName() + ".qmlrc"),
            "QML Resource File (*.qmlrc);;Resource File (*.rcc)");

        if (qmlrcFilePath.toString().isEmpty())
            return;

        QProgressDialog progress;
        progress.setLabelText(
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "Generating deployable package. Please wait..."));
        progress.setRange(0, 0);
        progress.setWindowModality(Qt::WindowModal);
        progress.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        progress.setCancelButton(nullptr);
        progress.show();

        QFuture<bool> future = QtConcurrent::run(
            [qmlrcFilePath]() { return createQmlrcFile(qmlrcFilePath); });

        while (!future.isFinished())
            QCoreApplication::processEvents();

        progress.close();

        if (future.isCanceled()) {
            qDebug() << "Operation canceled by user";
            return;
        }

        if (!future.result()) {
            Core::MessageManager::writeDisrupting(
                QCoreApplication::translate("QmlDesigner::GenerateResource",
                                            "Failed to generate deployable package!"));
            QMessageBox msgBox;
            msgBox.setWindowTitle(
                QCoreApplication::translate("QmlDesigner::GenerateResource", "Error"));
            msgBox.setText(QCoreApplication::translate(
                "QmlDesigner::GenerateResource",
                "Failed to generate deployable package!\n\nPlease check "
                "the output pane for more information."));
            msgBox.exec();
            return;
        }

        QMessageBox msgBox;
        msgBox.setWindowTitle(QCoreApplication::translate("QmlDesigner::GenerateResource", "Success"));
        msgBox.setText(QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                   "Successfully generated deployable package"));
        msgBox.exec();
    });

    Core::ActionContainer *exportMenu = Core::ActionManager::actionContainer(
        QmlProjectManager::Constants::EXPORT_MENU);
    exportMenu->addAction(cmd, QmlProjectManager::Constants::G_EXPORT_GENERATE);
    exportMenu->addAction(cmd2, QmlProjectManager::Constants::G_EXPORT_GENERATE);
}

QStringList getProjectFileList()
{
    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    const FilePaths paths = project->files(ProjectExplorer::Project::AllFiles);

    QStringList selectedFileList;
    const Utils::FilePath dir(project->projectFilePath().parentDir());
    for (const FilePath &path : paths) {
        const Utils::FilePath relativePath = path.relativePathFrom(dir);
        if (QmlProjectManager::isResource(relativePath))
            selectedFileList.append(relativePath.path());
    }

    return selectedFileList;
}

bool createQrcFile(const FilePath &qrcFilePath)
{
    QFile qrcFile(qrcFilePath.toString());

    if (!qrcFile.open(QIODeviceBase::WriteOnly | QIODevice::Truncate))
        return false;

    QXmlStreamWriter writer(&qrcFile);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(0);

    writer.writeStartElement("RCC");
    writer.writeStartElement("qresource");

    for (const QString &fileName : getProjectFileList())
        writer.writeTextElement("file", fileName.trimmed());

    writer.writeEndElement();
    writer.writeEndElement();
    qrcFile.close();

    return true;
}

bool createQmlrcFile(const FilePath &qmlrcFilePath)
{
    const FilePath tempQrcFile = qmlrcFilePath.parentDir().pathAppended("temp.qrc");
    if (!createQrcFile(tempQrcFile))
        return false;

    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    const QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(
        project->activeTarget()->kit());
    const FilePath rccBinary = qtVersion->rccFilePath();

    Utils::Process rccProcess;
    rccProcess.setWorkingDirectory(project->projectDirectory());

    const QStringList arguments = {"--binary",
                                   "--no-zstd",
                                   "--compress",
                                   "9",
                                   "--threshold",
                                   "30",
                                   "--output",
                                   qmlrcFilePath.toString(),
                                   tempQrcFile.toString()};

    rccProcess.setCommand({rccBinary, arguments});
    rccProcess.start();
    if (!rccProcess.waitForStarted()) {
        Core::MessageManager::writeDisrupting(
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "Unable to generate resource file: %1")
                .arg(qmlrcFilePath.toString()));
        return false;
    }

    QByteArray stdOut;
    QByteArray stdErr;
    if (!rccProcess.readDataFromProcess(&stdOut, &stdErr)) {
        rccProcess.stop();
        Core::MessageManager::writeDisrupting(
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "A timeout occurred running \"%1\".")
                .arg(rccProcess.commandLine().toUserOutput()));
        return false;
    }

    if (!stdOut.trimmed().isEmpty())
        Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdOut));

    if (!stdErr.trimmed().isEmpty())
        Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdErr));

    if (rccProcess.exitStatus() != QProcess::NormalExit) {
        Core::MessageManager::writeDisrupting(
            QCoreApplication::translate("QmlDesigner::GenerateResource", "\"%1\" crashed.")
                .arg(rccProcess.commandLine().toUserOutput()));
        return false;
    }
    if (rccProcess.exitCode() != 0) {
        Core::MessageManager::writeDisrupting(
            QCoreApplication::translate("QmlDesigner::GenerateResource", "\"%1\" failed (exit code %2).")
                .arg(rccProcess.commandLine().toUserOutput())
                .arg(rccProcess.exitCode()));
        return false;
    }
    return true;
}

} // namespace QmlDesigner::ResourceGenerator
