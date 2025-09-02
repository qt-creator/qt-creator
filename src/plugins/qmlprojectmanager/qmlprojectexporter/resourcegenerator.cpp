// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourcegenerator.h"

#include "../qmlprojectmanagertr.h"
#include "../buildsystem/qmlbuildsystem.h"
#include "../qmlprojectconstants.h"
#include "../qmlprojectexporter/filetypes.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <QMessageBox>
#include <QProgressDialog>
#include <QtConcurrent>

using namespace Utils;

namespace QmlProjectManager::QmlProjectExporter {

/*!
    Returns a list of paths of the project resource files relative to the project root directory
    \param project The project to get the resource files from
*/
QStringList getProjectResourceFilesPaths(const ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return {});

    QStringList resourceFilesPaths;
    const Utils::FilePath dir(project->projectFilePath().parentDir());

    for (const FilePath &path : project->files(ProjectExplorer::Project::AllFiles)) {
        const Utils::FilePath relativePath = path.relativePathFromDir(dir);
        if (QmlProjectManager::isResource(relativePath))
            resourceFilesPaths.append(relativePath.path());
    }

    return resourceFilesPaths;
}

ResourceGenerator::ResourceGenerator(QObject *parent)
    : QObject(parent)
{
    connect(&m_rccProcess, &Utils::Process::done, this, [this]() {
        const int exitCode = m_rccProcess.exitCode();
        if (exitCode != 0) {
            auto errorMessage = Tr::tr("\"%1\" failed (exit code %2).")
                                    .arg(m_rccProcess.commandLine().toUserOutput())
                                    .arg(m_rccProcess.exitCode());
            Core::MessageManager::writeDisrupting(errorMessage);
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
    auto qrcAction = new QAction(Tr::tr("Generate QRC Resource File..."), parent);
    qrcAction->setEnabled(ProjectExplorer::ProjectManager::startupProject() != nullptr);
    // todo make it more intelligent when it gets enabled
    auto onStartupProjectChanged = [qrcAction]() {
        if (auto buildSystem = QmlProjectManager::QmlBuildSystem::getStartupBuildSystem())
            qrcAction->setEnabled(!buildSystem->qtForMCUs());
    };
    QObject::connect(ProjectExplorer::ProjectManager::instance(),
                     &ProjectExplorer::ProjectManager::startupProjectChanged,
                     onStartupProjectChanged);

    auto qrcCmd = Core::ActionManager::registerAction(qrcAction, "QmlProject.CreateResource");
    QObject::connect(qrcAction, &QAction::triggered, []() {
        auto project = ProjectExplorer::ProjectManager::startupProject();
        QTC_ASSERT(project, return);
        const FilePath projectPath = project->projectFilePath().parentDir();
        auto qrcFilePath = Core::DocumentManager::getSaveFileNameWithExtension(
            Tr::tr("Save Project as QRC File"),
            projectPath.pathAppended(project->displayName() + ".qrc"),
            Tr::tr("QML Resource File (*.qrc)"));

        if (qrcFilePath.toUrlishString().isEmpty())
            return;

        bool success = ResourceGenerator::createQrc(project, qrcFilePath);
        if (!success) {
            Core::AsynchronousMessageBox::critical(Tr::tr("Error"),
                                                   Tr::tr("Failed to generate QRC resource file.")
                                                       + "\n" + qrcFilePath.toUserOutput());
            return;
        }

        Core::AsynchronousMessageBox::information(Tr::tr("Success"),
                                                  Tr::tr(
                                                      "Successfully generated QRC resource file.")
                                                      + "\n" + qrcFilePath.toUrlishString());
    });

    // ToDo: move this to QtCreator and add tr to the string then
    auto rccAction = new QAction(Tr::tr("Generate Deployable Package..."), parent);
    rccAction->setEnabled(ProjectExplorer::ProjectManager::startupProject() != nullptr);
    QObject::connect(ProjectExplorer::ProjectManager::instance(),
                     &ProjectExplorer::ProjectManager::startupProjectChanged,
                     [rccAction]() {
                         rccAction->setEnabled(ProjectExplorer::ProjectManager::startupProject());
                     });

    auto rccCmd = Core::ActionManager::registerAction(rccAction, "QmlProject.CreateRCCResource");
    QObject::connect(rccAction, &QAction::triggered, []() {
        auto project = ProjectExplorer::ProjectManager::startupProject();
        QTC_ASSERT(project, return);
        const FilePath projectPath = project->projectFilePath().parentDir();
        const FilePath qmlrcFilePath = Core::DocumentManager::getSaveFileNameWithExtension(
            Tr::tr("Save Project as Resource"),
            projectPath.pathAppended(project->displayName() + ".qmlrc"),
            "QML Resource File (*.qmlrc);;Resource File (*.rcc)");

        if (qmlrcFilePath.toUrlishString().isEmpty())
            return;

        QProgressDialog progress;
        progress.setLabelText(Tr::tr("Generating deployable package. Please wait..."));
        progress.setRange(0, 0);
        progress.setWindowModality(Qt::WindowModal);
        progress.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        progress.setCancelButton(nullptr);
        progress.show();

        QFuture<bool> future = QtConcurrent::run([project, qmlrcFilePath]() {
            ResourceGenerator resourceGenerator;
            return resourceGenerator.createQmlrc(project, qmlrcFilePath);
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
        msgBox.setText(Tr::tr("Successfully generated deployable package."));
        msgBox.exec();
    });

    Core::ActionContainer *exportMenu = Core::ActionManager::actionContainer(
        QmlProjectManager::Constants::EXPORT_MENU);
    exportMenu->addAction(qrcCmd, QmlProjectManager::Constants::G_EXPORT_GENERATE);
    exportMenu->addAction(rccCmd, QmlProjectManager::Constants::G_EXPORT_GENERATE);
}

/*!
    Creates a QRC resource file for the specified \a project in the project root directory
    Overwrites the existing file if it exists.
    \param project The project to create the QRC resource file for
    \return the path to the QRC resource file if it was created successfully, an empty optional
            otherwise
*/
std::optional<Utils::FilePath> ResourceGenerator::createQrc(const ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return std::nullopt);
    const FilePath projectPath = project->projectFilePath().parentDir();
    const FilePath qrcFilePath = projectPath.pathAppended(project->displayName() + ".qrc");

    if (!ResourceGenerator::createQrc(project, qrcFilePath))
        return std::nullopt;

    return qrcFilePath;
}

/*!
    Creates a QRC resource file for the specified \a project in the specified \a qrcFilePath
    \param project The project to create the QRC resource file for
    \param qrcFilePath The path to the QRC resource file to create
    \return true if the QRC resource file was created successfully, false otherwise
*/
bool ResourceGenerator::createQrc(const ProjectExplorer::Project *project,
                                  const Utils::FilePath &qrcFilePath)
{
    QTC_ASSERT(project, return false);
    const QStringList projectResources = getProjectResourceFilesPaths(project);
    QFile qrcFile(qrcFilePath.toFSPathString());

    if (!qrcFile.open(QIODeviceBase::WriteOnly | QIODevice::Truncate)) {
        Core::MessageManager::writeDisrupting(
            Tr::tr("Failed to open file \"%1\" to write QRC XML.").arg(qrcFilePath.toUserOutput()));
        return false;
    }

    QXmlStreamWriter writer(&qrcFile);
    writer.setAutoFormatting(true);

    writer.writeStartElement("RCC");
    writer.writeStartElement("qresource");

    for (const QString &resourcePath : projectResources)
        writer.writeTextElement("file", resourcePath.trimmed());

    writer.writeEndElement(); // qresource
    writer.writeEndElement(); // RCC

    qrcFile.close();

    return true;
}

/*!
    Compiles the \a project resources into a QML compiled resources file (.qmlrc) saved in the
    project root directory
    \param project The project to compile the resources for
*/
void ResourceGenerator::createQmlrcAsync(const ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);
    if (m_rccProcess.state() != QProcess::NotRunning) {
        Core::MessageManager::writeDisrupting(Tr::tr("Resource generator is already running."));
        return;
    }

    const FilePath projectPath = project->projectFilePath().parentDir();
    const FilePath qmlrcFilePath = projectPath.pathAppended(project->displayName() + ".qmlrc");
    createQmlrcAsync(project, qmlrcFilePath);
}

/*!
    Compiles the \a project resources into a QML compiled resources file (.qmlrc) saved in
    \a qmlrcFilePath
    \param project The project to create the QML resource file for
    \param qmlrcFilePath The path to the QML resource file to create
*/
void ResourceGenerator::createQmlrcAsync(const ProjectExplorer::Project *project,
                                         const FilePath &qmlrcFilePath)
{
    QTC_ASSERT(project, return);
    if (m_rccProcess.state() != QProcess::NotRunning) {
        Core::MessageManager::writeDisrupting(Tr::tr("Resource generator is already running."));
        return;
    }

    m_qmlrcFilePath = qmlrcFilePath;
    const FilePath tempQrcFile = m_qmlrcFilePath.parentDir().pathAppended("temp.qrc");
    if (!ResourceGenerator::createQrc(project, tempQrcFile))
        return;

    runRcc(qmlrcFilePath, tempQrcFile, true);
}

/*!
    Compiles the \a project resources into a QML compiled resources file (.qmlrc) saved in the
    project root directory
    \param project The project to compile the resources for
    \return the path to the compiled QML resource file if it was created successfully, an empty
            optional otherwise
*/
std::optional<Utils::FilePath> ResourceGenerator::createQmlrc(const ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return std::nullopt);
    const FilePath projectPath = project->projectFilePath().parentDir();
    const FilePath qmlrcFilePath = projectPath.pathAppended(project->displayName() + ".qmlrc");

    if (!createQmlrc(project, qmlrcFilePath))
        return {};

    return qmlrcFilePath;
}

/*!
    Compiles the \a project resources into the specified \a qmlrcFilePath
    \param project The project to compile the resources for
    \param qmlrcFilePath The path to the compiled QML resource file to create
    \return true if the resource file was created successfully, false otherwise
*/
bool ResourceGenerator::createQmlrc(const ProjectExplorer::Project *project,
                                    const FilePath &qmlrcFilePath)
{
    QTC_ASSERT(project, return false);

    if (m_rccProcess.state() != QProcess::NotRunning) {
        Core::MessageManager::writeDisrupting(Tr::tr("Resource generator is already running."));
        return false;
    }

    m_qmlrcFilePath = qmlrcFilePath;
    const FilePath tempQrcFile = m_qmlrcFilePath.parentDir().pathAppended("temp.qrc");
    if (!ResourceGenerator::createQrc(project, tempQrcFile))
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

/*!
    Runs the Qt resource compiler (rcc) on the specified \a qmlrcFilePath and \a qrcFilePath
    \param qmlrcFilePath The output compiled file path
    \param qrcFilePath The path of the QRC resource file to compile
    \param runAsync Whether to run the rcc process asynchronously. False by default
    \return If runAsync is true, returns true if the rcc process was started successfully
            and false otherwise. If runAsync is false, returns true if the rcc process was
            started successfully and finished successfully, false otherwise.
*/
bool ResourceGenerator::runRcc(const FilePath &qmlrcFilePath,
                               const Utils::FilePath &qrcFilePath,
                               const bool runAsync)
{
    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    QTC_ASSERT(project, return false);

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
                                   qmlrcFilePath.toUrlishString(),
                                   qrcFilePath.toUrlishString()};

    m_rccProcess.setCommand({rccBinary, arguments});
    m_rccProcess.start();
    if (!m_rccProcess.waitForStarted()) {
        Core::MessageManager::writeDisrupting(
            Tr::tr("Unable to generate resource file \"%1\".").arg(qmlrcFilePath.toUrlishString()));
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
} // namespace QmlProjectManager::QmlProjectExporter
