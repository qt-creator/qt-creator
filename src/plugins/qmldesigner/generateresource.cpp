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
#include <utils/synchronousprocess.h>

#include <QAction>
#include <QTemporaryFile>
#include <QMap>
#include <QProcess>
#include <QByteArray>
#include <QObject>
#include <QDebug>

namespace QmlDesigner {
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

        Core::MessageManager::write(QCoreApplication::translate("QmlDesigner::GenerateResource",
            "Generate a resource file out of project %1 to %2").arg(
            currentProject->displayName(), QDir::toNativeSeparators(resourceFileName)));

        QTemporaryFile temp(projectPath + "/XXXXXXX.create.resource.qrc");
        if (!temp.open())
            return;
        temp.close();

        auto rccBinary = QtSupport::QtKitAspect::qtVersion(currentProject->activeTarget()->kit())->hostBinPath();
        rccBinary = rccBinary.pathAppended(Utils::HostOsInfo::withExecutableSuffix("rcc"));

        QProcess rccProcess;
        rccProcess.setProgram(rccBinary.toString());
        rccProcess.setWorkingDirectory(projectPath);

        const QStringList arguments1 = {"--project", "--output", temp.fileName()};
        const QStringList arguments2 = {"--binary", "--output", resourceFileName, temp.fileName()};

        for (const auto &arguments : {arguments1, arguments2}) {
            rccProcess.start(rccBinary.toString(), arguments);
            if (!rccProcess.waitForStarted()) {
                Core::MessageManager::write(QCoreApplication::translate("QmlDesigner::GenerateResource",
                    "Unable to generate resource file: %1").arg(resourceFileName));
                return;
            }
            QByteArray stdOut;
            QByteArray stdErr;
            if (!Utils::SynchronousProcess::readDataFromProcess(rccProcess, 30, &stdOut, &stdErr, true)) {
                Utils::SynchronousProcess::stopProcess(rccProcess);
                Core::MessageManager::write(QCoreApplication::translate("QmlDesigner::GenerateResource",
                    "A timeout occurred running \"%1\"").arg(rccBinary.toString() + arguments.join(" ")));
                return ;

            }
            if (!stdOut.trimmed().isEmpty()) {
                Core::MessageManager::write(QString::fromLocal8Bit(stdOut));
            }
            if (!stdErr.trimmed().isEmpty())
                Core::MessageManager::write(QString::fromLocal8Bit(stdErr));

            if (rccProcess.exitStatus() != QProcess::NormalExit) {
                Core::MessageManager::write(QCoreApplication::translate("QmlDesigner::GenerateResource",
                    "\"%1\" crashed.").arg(rccBinary.toString() + arguments.join(" ")));
                return;
            }
            if (rccProcess.exitCode() != 0) {
                Core::MessageManager::write(QCoreApplication::translate("QmlDesigner::GenerateResource",
                    "\"%1\" failed (exit code %2).").arg(rccBinary.toString() +
                    " " + arguments.join(" ")).arg(rccProcess.exitCode()));
                return;
            }

        }

        saveLastUsedPath(Utils::FilePath::fromString(resourceFileName).parentDir().toString());
    });
    buildMenu->addAction(cmd, ProjectExplorer::Constants::G_BUILD_RUN);
}

} // namespace QmlDesigner

