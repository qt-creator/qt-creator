// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <generateresource.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/qmlprojectmanagerconstants.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/fileutils.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QByteArray>
#include <QCheckBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QTemporaryFile>
#include <QXmlStreamReader>

using namespace Utils;

namespace QmlDesigner {

QTableWidget* GenerateResource::createFilesTable(const QList<ResourceFile> &fileNames)
{
    auto table = new QTableWidget(0, 1);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    QStringList labels(QCoreApplication::translate("AddImageToResources","File Name"));
    table->setHorizontalHeaderLabels(labels);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->verticalHeader()->hide();
    table->setShowGrid(false);

    QFont font;
    font.setBold(true);

    for (ResourceFile resource : fileNames){
        QString filePath = resource.fileName;
        auto checkboxItem = new QTableWidgetItem();
        checkboxItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        checkboxItem->setCheckState(Qt::Checked);
        checkboxItem->setText(filePath);

        if (resource.inProject)
            checkboxItem->setFont(font);

        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, checkboxItem);
    }

    return table;
}

QStringList GenerateResource::getFileList(const QList<ResourceFile> &fileNames)
{
    QStringList result;
    QDialog *dialog = new QDialog(Core::ICore::dialogParent());
    dialog->setMinimumWidth(480);
    dialog->setMinimumHeight(640);

    dialog->setModal(true);
    dialog->setWindowTitle(QCoreApplication::translate("AddImageToResources","Add Resources"));
    QTableWidget *table = createFilesTable(fileNames);

    table->setParent(dialog);
    auto mainLayout = new QGridLayout(dialog);
    mainLayout->addWidget(table, 0, 0, 1, 4);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                       | QDialogButtonBox::Cancel);

    mainLayout->addWidget(buttonBox, 3, 2, 1, 2);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, [dialog]() {
        dialog->accept();
        dialog->deleteLater();
    });

    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, [dialog]() {
        dialog->reject();
        dialog->deleteLater();
    });

    QObject::connect(dialog, &QDialog::accepted, [&result, &table]() {
        QStringList fileList;
        QString file;

        for (int i = 0; i < table->rowCount(); ++i) {
            if (table->item(i,0)->checkState()){
                file = table->item(i,0)->text();
                fileList.append(file);
            }
        }

        result = fileList;
    });

    dialog->exec();

    return result;
}

bool skipSuffix(const QString &fileName)
{
    const QStringList suffixes = {".pri",
                                  ".pro",
                                  ".user",
                                  ".qrc",
                                  ".qds",
                                  "CMakeLists.txt",
                                  ".db",
                                  ".tmp",
                                  ".TMP",
                                  ".metainfo",
                                  ".qtds",
                                  ".db-shm",
                                  ".db-wal"};

    for (const auto &suffix : suffixes)
        if (fileName.endsWith(suffix))
            return true;

    return false;
}

QList<GenerateResource::ResourceFile> getFilesFromQrc(QFile *file, bool inProject = false)
{
    QXmlStreamReader reader(file);
    QList<GenerateResource::ResourceFile> fileList = {};

    while (!reader.atEnd()) {
        const auto token = reader.readNext();

        if (token != QXmlStreamReader::StartElement)
            continue;

        if (reader.name() == QLatin1String("file")) {
            QString fileName = reader.readElementText().trimmed();

            if ((!fileName.startsWith("./.")) && (!fileName.startsWith("./XXXXXXX"))
                && !skipSuffix(fileName)) {
                GenerateResource::ResourceFile file;
                file.fileName = fileName;
                file.inProject = inProject;
                fileList.append(file);
            }
        }
    }
    return fileList;
}

static bool runRcc(const CommandLine &command, const FilePath &workingDir,
                   const QString &resourceFile)
{
    Utils::Process rccProcess;
    rccProcess.setWorkingDirectory(workingDir);
    rccProcess.setCommand(command);
    rccProcess.start();
    if (!rccProcess.waitForStarted()) {
        Core::MessageManager::writeDisrupting(QCoreApplication::translate(
              "QmlDesigner::GenerateResource", "Unable to generate resource file: %1")
              .arg(resourceFile));
        return false;
    }
    QByteArray stdOut;
    QByteArray stdErr;
    if (!rccProcess.readDataFromProcess(&stdOut, &stdErr)) {
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
        Core::MessageManager::writeDisrupting(QCoreApplication::translate(
              "QmlDesigner::GenerateResource", "\"%1\" crashed.")
              .arg(rccProcess.commandLine().toUserOutput()));
        return false;
    }
    if (rccProcess.exitCode() != 0) {
        Core::MessageManager::writeDisrupting(QCoreApplication::translate(
              "QmlDesigner::GenerateResource", "\"%1\" failed (exit code %2).")
              .arg(rccProcess.commandLine().toUserOutput()).arg(rccProcess.exitCode()));
        return false;
    }
    return true;
}

void GenerateResource::generateMenuEntry(QObject *parent)
{
    const Core::Context projectContext(QmlProjectManager::Constants::QML_PROJECT_ID);
    // ToDo: move this to QtCreator and add tr to the string then
    auto action = new QAction(QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                          "Generate QRC Resource File..."),
                              parent);
    action->setEnabled(ProjectExplorer::ProjectManager::startupProject() != nullptr);
    // todo make it more intelligent when it gets enabled
    QObject::connect(ProjectExplorer::ProjectManager::instance(),
        &ProjectExplorer::ProjectManager::startupProjectChanged, [action]() {
            action->setEnabled(ProjectExplorer::ProjectManager::startupProject());
    });

    Core::Command *cmd = Core::ActionManager::registerAction(action, "QmlProject.CreateResource");
    QObject::connect(action, &QAction::triggered, [] () {
        auto currentProject = ProjectExplorer::ProjectManager::startupProject();
        QTC_ASSERT(currentProject, return);
        const FilePath projectPath = currentProject->projectFilePath().parentDir();

        auto projectFileName = Core::DocumentManager::getSaveFileNameWithExtension(
            QCoreApplication::translate("QmlDesigner::GenerateResource", "Save Project as QRC File"),
            projectPath.pathAppended(currentProject->displayName() + ".qrc"),
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "QML Resource File (*.qrc)"));
        if (projectFileName.isEmpty())
            return;

        QTemporaryFile temp(projectPath.toString() + "/XXXXXXX.create.resource.qrc");
        QFile persistentFile(projectFileName.toString());

        if (!temp.open())
            return;

        temp.close();

        QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(
            currentProject->activeTarget()->kit());
        const FilePath rccBinary = qtVersion->rccFilePath();

        if (!runRcc({rccBinary, {"--project", "--output", temp.fileName()}},
               projectPath, temp.fileName())) {
            return;
        }

        if (!temp.open())
            return;

        QByteArray firstLine = temp.readLine();
        QList<ResourceFile> fileList = getFilesFromQrc(&temp);

        QFile existingQrcFile(projectFileName.toString());
        if (existingQrcFile.exists()) {
            existingQrcFile.open(QFile::ReadOnly);
            fileList = getFilesFromQrc(&existingQrcFile, true);
            existingQrcFile.close();
        }

        QDir dir;
        dir.setCurrent(projectPath.toString());

        Utils::FilePaths paths = currentProject->files(ProjectExplorer::Project::AllFiles);
        QStringList projectFiles = {};

         for (const Utils::FilePath &path : paths) {
             QString relativepath = dir.relativeFilePath(path.toString());

             if (!skipSuffix(relativepath)) {
                 projectFiles.append(relativepath);

                 bool found = false;
                 QString compareString = "./" + relativepath.trimmed();
                 for (int i = 0; i < fileList.size(); ++i)
                     if (fileList.at(i).fileName == compareString) {
                         fileList[i].inProject = true;
                         found = true;
                         break;
                     }

                 if (!found) {
                     ResourceFile res;
                     res.fileName = "./" + relativepath.trimmed();
                     res.inProject = true;
                     fileList.append(res);
                 }
             }
        }

        temp.close();

        QStringList modifiedList = getFileList(fileList);

        if (!persistentFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
            return;

        QXmlStreamWriter writer(&persistentFile);
        writer.setAutoFormatting(true);
        writer.setAutoFormattingIndent(0);

        persistentFile.write(firstLine.trimmed());
        writer.writeStartElement("qresource");

        for (QString file : modifiedList)
            writer.writeTextElement("file", file.trimmed());

        writer.writeEndElement();
        persistentFile.write("\n</RCC>\n");
        persistentFile.close();

    });

    // ToDo: move this to QtCreator and add tr to the string then
    auto rccAction = new QAction(QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                             "Generate Deployable Package..."),
                                 parent);
    rccAction->setEnabled(ProjectExplorer::ProjectManager::startupProject() != nullptr);
    QObject::connect(ProjectExplorer::ProjectManager::instance(),
        &ProjectExplorer::ProjectManager::startupProjectChanged, [rccAction]() {
            rccAction->setEnabled(ProjectExplorer::ProjectManager::startupProject());
    });

    Core::Command *cmd2 = Core::ActionManager::registerAction(rccAction,
                                         "QmlProject.CreateRCCResource");
    QObject::connect(rccAction, &QAction::triggered, []() {
        auto currentProject = ProjectExplorer::ProjectManager::startupProject();
        QTC_ASSERT(currentProject, return);
        const FilePath projectPath = currentProject->projectFilePath().parentDir();

        const FilePath resourceFileName = Core::DocumentManager::getSaveFileNameWithExtension(
            QCoreApplication::translate("QmlDesigner::GenerateResource", "Save Project as Resource"),
            projectPath.pathAppended(currentProject->displayName() + ".qmlrc"),
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "QML Resource File (*.qmlrc);;Resource File (*.rcc)"));
        if (resourceFileName.isEmpty())
            return;

        Core::MessageManager::writeSilently(
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "Generate a resource file out of project %1 to %2")
                .arg(currentProject->displayName(), resourceFileName.toUserOutput()));

        QString projectFileName = currentProject->displayName() + ".qrc";
        QFile persistentFile(projectPath.toString() + "/" + projectFileName);

        QTemporaryFile temp(projectPath.toString() + "/XXXXXXX.create.resource.qrc");

        QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(
            currentProject->activeTarget()->kit());
        const FilePath rccBinary = qtVersion->rccFilePath();

        QXmlStreamReader reader;
        QByteArray firstLine;

        if (!QFileInfo(persistentFile).exists()) {
            if (!temp.open())
                return;
            temp.close();

            if (!runRcc({rccBinary, {"--project", "--output", temp.fileName()}},
                   projectPath, resourceFileName.toUserOutput())) {
                return;
            }
            reader.setDevice(&temp);
            if (!temp.open())
                return;
            firstLine = temp.readLine();
        } else {
            reader.setDevice(&persistentFile);
            if (!persistentFile.open(QIODevice::ReadWrite))
                return;

            firstLine = persistentFile.readLine();
        }

        QList<ResourceFile> fileList = {};

        while (!reader.atEnd()) {
            const auto token = reader.readNext();

            if (token != QXmlStreamReader::StartElement)
                continue;

            if (reader.name() == QLatin1String("file")) {
                QString fileName = reader.readElementText().trimmed();
                if ((!fileName.startsWith("./.")) && (!fileName.startsWith("./XXXXXXX"))
                    && !skipSuffix(fileName)) {
                    ResourceFile file;
                    file.fileName = fileName;
                    file.inProject = false;
                    fileList.append(file);
                }
            }
        }

        QDir dir;
        dir.setCurrent(projectPath.toString());

        Utils::FilePaths paths = currentProject->files(ProjectExplorer::Project::AllFiles);
        QStringList projectFiles = {};

        for (const Utils::FilePath &path : paths) {
            QString relativepath = dir.relativeFilePath(path.toString());

            if (!skipSuffix(relativepath)) {
                projectFiles.append(relativepath);

                bool found = false;
                QString compareString = "./" + relativepath.trimmed();
                for (int i = 0; i < fileList.size(); ++i)
                    if (fileList.at(i).fileName == compareString) {
                        fileList[i].inProject = true;
                        found = true;
                    }

                if (!found) {
                    ResourceFile res;
                    res.fileName = "./" + relativepath.trimmed();
                    res.inProject = true;
                    fileList.append(res);
                }
            }
        }

        temp.close();
        persistentFile.close();
        QStringList modifiedList = getFileList(fileList);
        QTemporaryFile tempFile(projectPath.toString() + "/XXXXXXX.create.modifiedresource.qrc");

        if (!tempFile.open())
            return;

        QXmlStreamWriter writer(&tempFile);
        writer.setAutoFormatting(true);
        writer.setAutoFormattingIndent(0);

        tempFile.write(firstLine.trimmed());
        writer.writeStartElement("qresource");

        for (QString file : modifiedList)
            writer.writeTextElement("file", file.trimmed());

        writer.writeEndElement();
        tempFile.write("\n</RCC>\n");
        tempFile.close();

        if (!runRcc({rccBinary, {"--binary", "--output", resourceFileName.path(), tempFile.fileName()}},
               projectPath, resourceFileName.path())) {
            return;
        }

        Core::AsynchronousMessageBox::information(
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "Success"),
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "Successfully generated deployable package\n %1")
                .arg(resourceFileName.toString()));
    });

    Core::ActionContainer *exportMenu = Core::ActionManager::actionContainer(
        QmlProjectManager::Constants::EXPORT_MENU);
    exportMenu->addAction(cmd, QmlProjectManager::Constants::G_EXPORT_GENERATE);
    exportMenu->addAction(cmd2, QmlProjectManager::Constants::G_EXPORT_GENERATE);
}

} // namespace QmlDesigner

