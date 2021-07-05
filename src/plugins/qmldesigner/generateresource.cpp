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

void GenerateResource::generateMenuEntry()
{
    Core::ActionContainer *buildMenu =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);

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
        auto projectPath = currentProject->projectFilePath().parentDir().toString();

        static QMap<QString, QString> lastUsedPathes;
        auto saveLastUsedPath = [currentProject] (const QString &lastUsedPath) {
            lastUsedPathes.insert(currentProject->displayName(), lastUsedPath);
        };
        saveLastUsedPath(lastUsedPathes.value(currentProject->displayName(),
            currentProject->projectFilePath().parentDir().parentDir().toString()));

        QString projectFileName = currentProject->displayName() + ".qrc";
        QTemporaryFile temp(projectPath + "/XXXXXXX.create.resource.qrc");
        QFile persistentFile(projectPath + "/" + projectFileName);

        if (!temp.open())
            return;

        temp.close();

        QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(
            currentProject->activeTarget()->kit());
        FilePath rccBinary = qtVersion->rccCommand();

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

        QXmlStreamReader reader(&temp);
        QList<ResourceFile> fileList = {};
        QByteArray firstLine = temp.readLine();

        while (!reader.atEnd()) {
            const auto token = reader.readNext();

            if (token != QXmlStreamReader::StartElement)
                continue;

            if (reader.name() == QLatin1String("file")) {
                QString fileName = reader.readElementText().trimmed();

                if ((!fileName.startsWith("./.")) && (!fileName.startsWith("./XXXXXXX"))
                        && !fileName.endsWith(".qmlproject") && !fileName.endsWith(".pri")
                        && !fileName.endsWith(".pro") && !fileName.endsWith(".user")
                        && !fileName.endsWith(".qrc")) {
                    ResourceFile file;
                    file.fileName = fileName;
                    file.inProject = false;
                    fileList.append(file);
                }
            }
        }

        QDir dir;
        dir.setCurrent(projectPath);

        Utils::FilePaths paths = currentProject->files(ProjectExplorer::Project::AllFiles);
        QStringList projectFiles = {};

         for (const Utils::FilePath &path : paths) {
             QString relativepath = dir.relativeFilePath(path.toString());

            if (!relativepath.endsWith(".qmlproject") && !relativepath.endsWith(".pri")
                    && !relativepath.endsWith(".pro") && !relativepath.endsWith(".user")
                    && !relativepath.endsWith(".qrc")) {
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

        saveLastUsedPath(Utils::FilePath::fromString(projectFileName).parentDir().toString());
    });

    // ToDo: move this to QtCreator and add tr to the string then
    auto rccAction = new QAction(QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                             "Generate RCC Resource File"));
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
        auto projectPath = currentProject->projectFilePath().parentDir().toString();

        static QMap<QString, QString> lastUsedPathes;
        auto saveLastUsedPath = [currentProject] (const QString &lastUsedPath) {
            lastUsedPathes.insert(currentProject->displayName(), lastUsedPath);
        };
        saveLastUsedPath(lastUsedPathes.value(currentProject->displayName(),
            currentProject->projectFilePath().parentDir().parentDir().toString()));

        auto resourceFileName = Core:: DocumentManager::getSaveFileName(
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                "Save Project as Resource"), lastUsedPathes.value(currentProject->displayName())
                + "/" + currentProject->displayName() + ".qmlrc",
                QCoreApplication::translate("QmlDesigner::GenerateResource",
                                            "QML Resource File (*.qmlrc);;Resource File (*.rcc)"));
        if (resourceFileName.isEmpty())
            return;

        Core::MessageManager::writeSilently(
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "Generate a resource file out of project %1 to %2")
                .arg(currentProject->displayName(), QDir::toNativeSeparators(resourceFileName)));

        QString projectFileName = currentProject->displayName() + ".qrc";
        QFile persistentFile(projectPath + "/" + projectFileName);
        QTemporaryFile temp(projectPath + "/XXXXXXX.create.resource.qrc");

        QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(
            currentProject->activeTarget()->kit());
        FilePath rccBinary = qtVersion->rccCommand();

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
                        .arg(resourceFileName));
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
                        && !fileName.endsWith(".qmlproject") && !fileName.endsWith(".pri")
                        && !fileName.endsWith(".pro") && !fileName.endsWith(".user")
                        && !fileName.endsWith(".qrc")) {
                    ResourceFile file;
                    file.fileName = fileName;
                    file.inProject = false;
                    fileList.append(file);
                }
            }
        }

        QDir dir;
        dir.setCurrent(projectPath);

        Utils::FilePaths paths = currentProject->files(ProjectExplorer::Project::AllFiles);
        QStringList projectFiles = {};

        for (const Utils::FilePath &path : paths) {
            QString relativepath = dir.relativeFilePath(path.toString());

            if (!relativepath.endsWith(".qmlproject") && !relativepath.endsWith(".pri")
                    && !relativepath.endsWith(".pro") && !relativepath.endsWith(".user")
                    && !relativepath.endsWith(".qrc")) {
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
        QTemporaryFile tempFile(projectPath + "/XXXXXXX.create.modifiedresource.qrc");

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

        const QStringList arguments2 = {"--binary", "--output", resourceFileName,
                                        tempFile.fileName()};

        for (const auto &arguments : {arguments2}) {
            rccProcess.setCommand({rccBinary, arguments});
            rccProcess.start();
            if (!rccProcess.waitForStarted()) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "Unable to generate resource file: %1")
                        .arg(resourceFileName));
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

        saveLastUsedPath(Utils::FilePath::fromString(resourceFileName).parentDir().toString());
    });
    buildMenu->addAction(cmd, ProjectExplorer::Constants::G_BUILD_RUN);
    buildMenu->addAction(cmd2, ProjectExplorer::Constants::G_BUILD_RUN);
}

} // namespace QmlDesigner

