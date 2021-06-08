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

namespace QmlDesigner {

QTableWidget* GenerateResource::createFilesTable(const QStringList &fileNames)
{
    auto table = new QTableWidget(0, 1);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    QStringList labels(QCoreApplication::translate("AddImageToResources","File Name"));
    table->setHorizontalHeaderLabels(labels);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->verticalHeader()->hide();
    table->setShowGrid(false);

    for (const QString &filePath : fileNames) {
        auto checkboxItem = new QTableWidgetItem();
        checkboxItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        checkboxItem->setCheckState(Qt::Checked);
        checkboxItem->setText(filePath);

        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, checkboxItem);
    }

    return table;
}

QStringList GenerateResource::getFileList(const QStringList &fileNames)
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

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, [dialog](){
        dialog->accept();
        dialog->deleteLater();
    });

    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, [dialog](){
        dialog->reject();
        dialog->deleteLater();
    });

    QObject::connect(dialog, &QDialog::accepted, [&result, &table](){
        QStringList fileList;
        QString file;

        for (int i = 0; i < table->rowCount(); ++i){
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
    auto action = new QAction(QCoreApplication::translate("QmlDesigner::GenerateResource", "Generate Resource File"));
    action->setEnabled(ProjectExplorer::SessionManager::startupProject() != nullptr);
    // todo make it more intelligent when it gets enabled
    QObject::connect(ProjectExplorer::SessionManager::instance(), &ProjectExplorer::SessionManager::startupProjectChanged, [action]() {
            action->setEnabled(ProjectExplorer::SessionManager::startupProject());
    });

    Core::Command *cmd = Core::ActionManager::registerAction(action, "QmlProject.CreateResource");
    QObject::connect(action, &QAction::triggered, [] () {
        auto currentProject = ProjectExplorer::SessionManager::startupProject();
        auto projectPath = currentProject->projectFilePath().parentDir().toString();

        static QMap<QString, QString> lastUsedPathes;
        auto saveLastUsedPath = [currentProject] (const QString &lastUsedPath) {
            lastUsedPathes.insert(currentProject->displayName(), lastUsedPath);
        };
        saveLastUsedPath(lastUsedPathes.value(currentProject->displayName(),
            currentProject->projectFilePath().parentDir().parentDir().toString()));

        auto resourceFileName = Core:: DocumentManager::getSaveFileName(
            QCoreApplication::translate("QmlDesigner::GenerateResource", "Save Project as Resource"),
            lastUsedPathes.value(currentProject->displayName()) + "/" + currentProject->displayName() + ".qmlrc",
            QCoreApplication::translate("QmlDesigner::GenerateResource", "QML Resource File (*.qmlrc)"));
        if (resourceFileName.isEmpty())
            return;

        Core::MessageManager::writeSilently(
            QCoreApplication::translate("QmlDesigner::GenerateResource",
                                        "Generate a resource file out of project %1 to %2")
                .arg(currentProject->displayName(), QDir::toNativeSeparators(resourceFileName)));

        QTemporaryFile temp(projectPath + "/XXXXXXX.create.resource.qrc");
        if (!temp.open())
            return;
        temp.close();

        QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(
            currentProject->activeTarget()->kit());
        QString rccBinary = qtVersion->rccCommand();

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
                        .arg(rccBinary + " " + arguments.join(" ")));
                return;
            }
            if (!stdOut.trimmed().isEmpty()) {
                Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdOut));
            }
            if (!stdErr.trimmed().isEmpty())
                Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdErr));

            if (rccProcess.exitStatus() != QProcess::NormalExit) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource", "\"%1\" crashed.")
                        .arg(rccBinary + " " + arguments.join(" ")));
                return;
            }
            if (rccProcess.exitCode() != 0) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "\"%1\" failed (exit code %2).")
                        .arg(rccBinary + " " + arguments.join(" "))
                        .arg(rccProcess.exitCode()));
                return;
            }

        }

        if (!temp.open())
            return;

        QXmlStreamReader reader(&temp);
        QStringList fileList = {};
        QByteArray firstLine = temp.readLine();

        while (!reader.atEnd()) {
            const auto token = reader.readNext();

            if (token != QXmlStreamReader::StartElement)
                continue;

            if (reader.name() == QLatin1String("file")) {
                QString fileName = reader.readElementText().trimmed();
                if ((!fileName.startsWith("./.")) && (!fileName.startsWith("./XXXXXXX")))
                    fileList.append(fileName);
            }
        }

        temp.close();
        QStringList modifiedList = getFileList(fileList);
        QTemporaryFile tempFile(projectPath + "/XXXXXXX.create.modifiedresource.qrc");

        if (!tempFile.open())
            return;

        QXmlStreamWriter writer(&tempFile);
        writer.setAutoFormatting(true);
        writer.setAutoFormattingIndent(0);

        tempFile.write(firstLine.trimmed());
        writer.writeStartElement("qresource");

        for (int i = 0; i < modifiedList.count(); ++i)
            writer.writeTextElement("file", modifiedList.at(i).trimmed());

        writer.writeEndElement();
        tempFile.write("\n</RCC>\n");
        tempFile.close();

        const QStringList arguments2 = {"--binary", "--output", resourceFileName, tempFile.fileName()};

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
                        .arg(rccBinary + " " + arguments.join(" ")));
                return ;

            }
            if (!stdOut.trimmed().isEmpty()) {
                Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdOut));
            }
            if (!stdErr.trimmed().isEmpty())
                Core::MessageManager::writeFlashing(QString::fromLocal8Bit(stdErr));

            if (rccProcess.exitStatus() != QProcess::NormalExit) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource", "\"%1\" crashed.")
                        .arg(rccBinary + " " + arguments.join(" ")));
                return;
            }
            if (rccProcess.exitCode() != 0) {
                Core::MessageManager::writeDisrupting(
                    QCoreApplication::translate("QmlDesigner::GenerateResource",
                                                "\"%1\" failed (exit code %2).")
                        .arg(rccBinary + " " + arguments.join(" "))
                        .arg(rccProcess.exitCode()));
                return;
            }

        }

        saveLastUsedPath(Utils::FilePath::fromString(resourceFileName).parentDir().toString());
    });
    buildMenu->addAction(cmd, ProjectExplorer::Constants::G_BUILD_RUN);
}

} // namespace QmlDesigner

