// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <resourcegenerator.h>

#include <qmldesignertr.h>

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

#include <QMessageBox>
#include <QProgressDialog>
#include <QtConcurrent>

using namespace Utils;

namespace QmlDesigner {
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

ResourceGenerator::ResourceGenerator(QObject *parent)
    : QObject(parent)
{
    connect(&m_rccProcess, &Utils::Process::done, this, [this]() {
        const int exitCode = m_rccProcess.exitCode();
        if (exitCode != 0) {
            Core::MessageManager::writeDisrupting(Tr::tr("\"%1\" failed (exit code %2).")
                                                      .arg(m_rccProcess.commandLine().toUserOutput())
                                                      .arg(m_rccProcess.exitCode()));
            emit errorOccurred(Tr::tr("Failed to generate deployable package!"));
            return;
        }

        if (m_rccProcess.exitStatus() != QProcess::NormalExit) {
            Core::MessageManager::writeDisrupting(
                Tr::tr("\"%1\" crashed.").arg(m_rccProcess.commandLine().toUserOutput()));
            emit errorOccurred(Tr::tr("Failed to generate deployable package!"));
            return;
        }

        emit qmlrcCreated(m_qmlrcFilePath);
    });

    connect(&m_rccProcess, &Utils::Process::textOnStandardError, this, [](const QString &text) {
        Core::MessageManager::writeFlashing(QString::fromLocal8Bit(text.toLocal8Bit()));
    });

    connect(&m_rccProcess, &Utils::Process::textOnStandardOutput, this, [](const QString &text) {
        Core::MessageManager::writeFlashing(QString::fromLocal8Bit(text.toLocal8Bit()));
    });
}

void ResourceGenerator::generateMenuEntry(QObject *parent)
{
    const Core::Context projectContext(QmlProjectManager::Constants::QML_PROJECT_ID);
    // ToDo: move this to QtCreator and add tr to the string then
    auto action = new QAction(Tr::tr("Generate QRC Resource File..."), parent);
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
            Tr::tr("Save Project as QRC File"),
            projectPath.pathAppended(project->displayName() + ".qrc"),
            Tr::tr("QML Resource File (*.qrc)"));

        if (qrcFilePath.toString().isEmpty())
            return;

        ResourceGenerator resourceGenerator;
        resourceGenerator.createQrc(qrcFilePath);

        Core::AsynchronousMessageBox::information(
            Tr::tr("Success"),
            Tr::tr("Successfully generated QRC resource file\n %1").arg(qrcFilePath.toString()));
    });

    // ToDo: move this to QtCreator and add tr to the string then
    auto rccAction = new QAction(Tr::tr("Generate Deployable Package..."), parent);
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
            Tr::tr("Save Project as Resource"),
            projectPath.pathAppended(project->displayName() + ".qmlrc"),
            "QML Resource File (*.qmlrc);;Resource File (*.rcc)");

        if (qmlrcFilePath.toString().isEmpty())
            return;

        QProgressDialog progress;
        progress.setLabelText(Tr::tr("Generating deployable package. Please wait..."));
        progress.setRange(0, 0);
        progress.setWindowModality(Qt::WindowModal);
        progress.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        progress.setCancelButton(nullptr);
        progress.show();

        QFuture<bool> future = QtConcurrent::run([qmlrcFilePath]() {
            ResourceGenerator resourceGenerator;
            return resourceGenerator.createQmlrcWithPath(qmlrcFilePath);
        });

        while (!future.isFinished())
            QCoreApplication::processEvents();

        progress.close();

        if (future.isCanceled()) {
            qDebug() << "Operation canceled by user";
            return;
        }

        if (!future.result()) {
            Core::MessageManager::writeDisrupting(Tr::tr("Failed to generate deployable package!"));
            QMessageBox msgBox;
            msgBox.setWindowTitle(Tr::tr("Error"));
            msgBox.setText(Tr::tr("Failed to generate deployable package!\n\nPlease check "
                                  "the output pane for more information."));
            msgBox.exec();
            return;
        }

        QMessageBox msgBox;
        msgBox.setWindowTitle(Tr::tr("Success"));
        msgBox.setText(Tr::tr("Successfully generated deployable package"));
        msgBox.exec();
    });

    Core::ActionContainer *exportMenu = Core::ActionManager::actionContainer(
        QmlProjectManager::Constants::EXPORT_MENU);
    exportMenu->addAction(cmd, QmlProjectManager::Constants::G_EXPORT_GENERATE);
    exportMenu->addAction(cmd2, QmlProjectManager::Constants::G_EXPORT_GENERATE);
}

std::optional<Utils::FilePath> ResourceGenerator::createQrc(const QString &projectName)
{
    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    const FilePath projectPath = project->projectFilePath().parentDir();
    const FilePath qrcFilePath = projectPath.pathAppended(projectName + ".qrc");

    if (!createQrc(qrcFilePath))
        return {};

    return qrcFilePath;
}

bool ResourceGenerator::createQrc(const Utils::FilePath &qrcFilePath)
{
    QFile qrcFile(qrcFilePath.toString());

    if (!qrcFile.open(QIODeviceBase::WriteOnly | QIODevice::Truncate)) {
        Core::MessageManager::writeDisrupting(
            Tr::tr("Failed to open file to write QRC XML: %1").arg(qrcFilePath.toString()));
        return false;
    }

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

void ResourceGenerator::createQmlrcAsyncWithName(const QString &projectName)
{
    if (m_rccProcess.state() != QProcess::NotRunning) {
        Core::MessageManager::writeDisrupting(Tr::tr("Resource generator is already running."));
        return;
    }

    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    const FilePath projectPath = project->projectFilePath().parentDir();
    const FilePath qmlrcFilePath = projectPath.pathAppended(projectName + ".qmlrc");

    createQmlrcAsyncWithPath(qmlrcFilePath);
}

void ResourceGenerator::createQmlrcAsyncWithPath(const FilePath &qmlrcFilePath)
{
    if (m_rccProcess.state() != QProcess::NotRunning) {
        Core::MessageManager::writeDisrupting(Tr::tr("Resource generator is already running."));
        return;
    }

    m_qmlrcFilePath = qmlrcFilePath;
    const FilePath tempQrcFile = m_qmlrcFilePath.parentDir().pathAppended("temp.qrc");
    if (!createQrc(tempQrcFile))
        return;

    runRcc(qmlrcFilePath, tempQrcFile, true);
}

std::optional<Utils::FilePath> ResourceGenerator::createQmlrcWithName(const QString &projectName)
{
    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    const FilePath projectPath = project->projectFilePath().parentDir();
    const FilePath qmlrcFilePath = projectPath.pathAppended(projectName + ".qmlrc");

    if (!createQmlrcWithPath(qmlrcFilePath))
        return {};

    return qmlrcFilePath;
}

bool ResourceGenerator::createQmlrcWithPath(const FilePath &qmlrcFilePath)
{
    if (m_rccProcess.state() != QProcess::NotRunning) {
        Core::MessageManager::writeDisrupting(Tr::tr("Resource generator is already running."));
        return false;
    }

    m_qmlrcFilePath = qmlrcFilePath;
    const FilePath tempQrcFile = m_qmlrcFilePath.parentDir().pathAppended("temp.qrc");
    if (!createQrc(tempQrcFile))
        return false;

    bool retVal = true;
    if (!runRcc(qmlrcFilePath, tempQrcFile)) {
        retVal = false;
        if (qmlrcFilePath.exists())
            qmlrcFilePath.removeFile();
    }

    tempQrcFile.removeFile();
    return retVal;
}

bool ResourceGenerator::runRcc(const FilePath &qmlrcFilePath,
                               const Utils::FilePath &qrcFilePath,
                               const bool runAsync)
{
    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    const QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(
        project->activeTarget()->kit());
    QTC_ASSERT(qtVersion, return false);

    const FilePath rccBinary = qtVersion->rccFilePath();

    m_rccProcess.setWorkingDirectory(project->projectDirectory());

    const QStringList arguments = {"--binary",
                                   "--no-zstd",
                                   "--compress",
                                   "9",
                                   "--threshold",
                                   "30",
                                   "--output",
                                   qmlrcFilePath.toString(),
                                   qrcFilePath.toString()};

    m_rccProcess.setCommand({rccBinary, arguments});
    m_rccProcess.start();
    if (!m_rccProcess.waitForStarted()) {
        Core::MessageManager::writeDisrupting(
            Tr::tr("Unable to generate resource file: %1").arg(qmlrcFilePath.toString()));
        return false;
    }

    if (!runAsync) {
        QByteArray stdOut;
        QByteArray stdErr;
        if (!m_rccProcess.readDataFromProcess(&stdOut, &stdErr)) {
            m_rccProcess.stop();
            Core::MessageManager::writeDisrupting(Tr::tr("A timeout occurred running \"%1\".")
                                                      .arg(m_rccProcess.commandLine().toUserOutput()));
            return false;
        }

        if (m_rccProcess.exitStatus() != QProcess::NormalExit || m_rccProcess.exitCode() != 0)
            return false;
    }

    return true;
}

void ResourceGenerator::cancel()
{
    m_rccProcess.kill();
}
} // namespace QmlDesigner
