/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <generateresource.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qmlprojectmanager/qmlprojectmanagerconstants.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <utils/qtcprocess.h>

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

void GenerateResource::generateMenuEntry()
{
    Core::ActionContainer *menu =
            Core::ActionManager::actionContainer(Core::Constants::M_FILE);

    const Core::Context projectContext(QmlProjectManager::Constants::QML_PROJECT_ID);
    // ToDo: move this to QtCreator and add tr to the string then
    auto action = new QAction(QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                          "Generate QRC Resource File"));
    action->setEnabled(ProjectExplorer::SessionManager::startupProject() != nullptr);
    // todo make it more intelligent when it gets enabled
    QObject::connect(ProjectExplorer::SessionManager::instance(),
        &ProjectExplorer::SessionManager::startupProjectChanged, [action]() {
            action->setEnabled(ProjectExplorer::SessionManager::startupProject());
    });

    Core::Command *cmd = Core::ActionManager::registerAction(action, "QmlProject.CreateResource");
    QObject::connect(action, &QAction::triggered, [] () {
        auto currentProject = ProjectExplorer::SessionManager::startupProject();
        QTC_ASSERT(currentProject, return);
        const FilePath projectPath = currentProject->projectFilePath().parentDir();

        auto projectFileName = Core::DocumentManager::getSaveFileName(
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
        FilePath rccBinary = qtVersion->rccFilePath();

        Utils::QtcProcess rccProcess;
        rccProcess.setWorkingDirectory(projectPath);

        const QStringList arguments1 = {"--project", "--output", temp.fileName()};

        for (const auto &arguments : {arguments1}) {
            rccProcess.setCommand({rccBinary, arguments});
            rccProcess.start();
            if (!rccProcess.waitForStarted()) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "Unable to generate resource file: %1")
                        .arg(temp.fileName()));
                return;
            }
            QByteArray stdOut;
            QByteArray stdErr;
            if (!rccProcess.readDataFromProcess(30, &stdOut, &stdErr, true)) {
                rccProcess.stopProcess();
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "A timeout occurred running \"%1\"")
                        .arg(rccProcess.commandLine().toUserOutput()));
                return;

            }
            if (!stdOut.trimmed().isEmpty())
                Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdOut));

            if (!stdErr.trimmed().isEmpty())
                Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdErr));

            if (rccProcess.exitStatus() != QProcess::NormalExit) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource", "\"%1\" crashed.")
                        .arg(rccProcess.commandLine().toUserOutput()));
                return;
            }
            if (rccProcess.exitCode() != 0) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "\"%1\" failed (exit code %2).")
                        .arg(rccProcess.commandLine().toUserOutput())
                        .arg(rccProcess.exitCode()));
                return;
            }
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
                 for (int i = 0; i < fileList.count(); ++i)
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
                                                             "Generate Deployable Package"));
    rccAction->setEnabled(ProjectExplorer::SessionManager::startupProject() != nullptr);
    QObject::connect(ProjectExplorer::SessionManager::instance(),
        &ProjectExplorer::SessionManager::startupProjectChanged, [rccAction]() {
            rccAction->setEnabled(ProjectExplorer::SessionManager::startupProject());
    });

    Core::Command *cmd2 = Core::ActionManager::registerAction(rccAction,
                                         "QmlProject.CreateRCCResource");
    QObject::connect(rccAction, &QAction::triggered, [] () {
        auto currentProject = ProjectExplorer::SessionManager::startupProject();
        QTC_ASSERT(currentProject, return);
        const FilePath projectPath = currentProject->projectFilePath().parentDir();

        const FilePath resourceFileName = Core::DocumentManager::getSaveFileName(
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
        FilePath rccBinary = qtVersion->rccFilePath();

        QtcProcess rccProcess;
        rccProcess.setWorkingDirectory(projectPath);

        QXmlStreamReader reader;
        QByteArray firstLine;

        if (!QFileInfo(persistentFile).exists()) {
            if (!temp.open())
                return;
            temp.close();

            const QStringList arguments1 = {"--project", "--output", temp.fileName()};

            for (const auto &arguments : {arguments1}) {
                rccProcess.setCommand({rccBinary, arguments});
                rccProcess.start();
                if (!rccProcess.waitForStarted()) {
                    Core::MessageManager::writeDisrupting(
                        QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "Unable to generate resource file: %1")
                        .arg(resourceFileName.toUserOutput()));
                    return;
                }
                QByteArray stdOut;
                QByteArray stdErr;
                if (!rccProcess.readDataFromProcess(30, &stdOut, &stdErr, true)) {
                    rccProcess.stopProcess();
                    Core::MessageManager::writeDisrupting(
                        QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "A timeout occurred running \"%1\"")
                        .arg(rccProcess.commandLine().toUserOutput()));
                    return;
                }
                if (!stdOut.trimmed().isEmpty())
                    Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdOut));

                if (!stdErr.trimmed().isEmpty())
                    Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdErr));

                if (rccProcess.exitStatus() != QProcess::NormalExit) {
                    Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource", "\"%1\" crashed.")
                        .arg(rccProcess.commandLine().toUserOutput()));
                    return;
                }
                if (rccProcess.exitCode() != 0) {
                    Core::MessageManager::writeDisrupting(
                        QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "\"%1\" failed (exit code %2).")
                        .arg(rccProcess.commandLine().toUserOutput())
                        .arg(rccProcess.exitCode()));
                    return;
                }
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
                for (int i = 0; i < fileList.count(); ++i)
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

        const QStringList arguments2 = {"--binary", "--output", resourceFileName.path(),
                                        tempFile.fileName()};

        for (const auto &arguments : {arguments2}) {
            rccProcess.setCommand({rccBinary, arguments});
            rccProcess.start();
            if (!rccProcess.waitForStarted()) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "Unable to generate resource file: %1")
                        .arg(resourceFileName.toUserOutput()));
                return;
            }
            QByteArray stdOut;
            QByteArray stdErr;
            if (!rccProcess.readDataFromProcess(30, &stdOut, &stdErr, true)) {
                rccProcess.stopProcess();
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "A timeout occurred running \"%1\"")
                        .arg(rccProcess.commandLine().toUserOutput()));
                return;

            }
            if (!stdOut.trimmed().isEmpty())
                Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdOut));

            if (!stdErr.trimmed().isEmpty())
                Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdErr));

            if (rccProcess.exitStatus() != QProcess::NormalExit) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource", "\"%1\" crashed.")
                        .arg(rccProcess.commandLine().toUserOutput()));
                return;
            }
            if (rccProcess.exitCode() != 0) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "\"%1\" failed (exit code %2).")
                        .arg(rccProcess.commandLine().toUserOutput())
                        .arg(rccProcess.exitCode()));
                return;
            }
        }
    });
    menu->addAction(cmd, Core::Constants::G_FILE_EXPORT);
    menu->addAction(cmd2, Core::Constants::G_FILE_EXPORT);
}

} // namespace QmlDesigner

