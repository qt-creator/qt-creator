/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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

#include "qmlprojectplugin.h"
#include "qmlproject.h"
#include "qmlprojectrunconfiguration.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/session.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/infobar.h>
#include <utils/qtcprocess.h>

#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QPointer>

using namespace ProjectExplorer;

namespace QmlProjectManager {
namespace Internal {

const char openInQDSAppInfoBar[] = "OpenInQDSAppUiQml";
const char alwaysOpenUiQmlInQDS[] = "J.QtQuick/QmlJSEditor.openUiQmlFilesInQDS";

static bool isQmlDesigner(const ExtensionSystem::PluginSpec *spec)
{
    if (!spec)
        return false;

    return spec->name().contains("QmlDesigner");
}

static bool qmlDesignerEnabled()
{
    const auto plugins = ExtensionSystem::PluginManager::plugins();
    const auto it = std::find_if(plugins.begin(), plugins.end(), &isQmlDesigner);
    return it != plugins.end() && (*it)->plugin();
}

static bool alwaysOpenUiQmlfileInQds()
{
    return Core::ICore::settings()->value(alwaysOpenUiQmlInQDS, false).toBool();
}

static void enableAlwaysOpenUiQmlfileInQds()
{
    return Core::ICore::settings()->setValue(alwaysOpenUiQmlInQDS, true);
}

class QmlProjectPluginPrivate
{
public:
    QmlProjectRunConfigurationFactory runConfigFactory;
    RunWorkerFactory runWorkerFactory{RunWorkerFactory::make<SimpleTargetRunner>(),
                                      {ProjectExplorer::Constants::NORMAL_RUN_MODE},
                                      {runConfigFactory.runConfigurationId()}};
    QPointer<QMessageBox> lastMessageBox;
};

QmlProjectPlugin::~QmlProjectPlugin()
{
    if (d->lastMessageBox)
        d->lastMessageBox->deleteLater();
    delete d;
}

void QmlProjectPlugin::openQDS(const Utils::FilePath &fileName)
{
    const Utils::FilePath &qdsPath = QmlProjectPlugin::qdsInstallationEntry();
    bool qdsStarted = false;
    //-a and -client arguments help to append project to open design studio application
    if (Utils::HostOsInfo::isMacHost())
        qdsStarted = Utils::QtcProcess::startDetached(
            {"/usr/bin/open", {"-a", qdsPath.path(), fileName.toString()}});
    else
        qdsStarted = Utils::QtcProcess::startDetached({qdsPath, {"-client", fileName.toString()}});

    if (!qdsStarted) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             fileName.fileName(),
                             QObject::tr("Failed to start Qt Design Studio."));
    }
}

Utils::FilePath QmlProjectPlugin::qdsInstallationEntry()
{
    QSettings *settings = Core::ICore::settings();
    const QString qdsInstallationEntry = "QML/Designer/DesignStudioInstallation"; //set in installer

    return Utils::FilePath::fromUserInput(settings->value(qdsInstallationEntry).toString());
}

bool QmlProjectPlugin::qdsInstallationExists()
{
    return qdsInstallationEntry().exists();
}

Utils::FilePath findQmlProject(const Utils::FilePath &folder)
{
    QDir dir = folder.toDir();
    for (const QString &file : dir.entryList({"*.qmlproject"}))
        return Utils::FilePath::fromString(folder.toString() + "/" + file);
    return {};
}

Utils::FilePath findQmlProjectUpwards(const Utils::FilePath &folder)
{
    auto ret = findQmlProject(folder);
    if (ret.exists())
        return ret;

    QDir dir = folder.toDir();
    if (dir.cdUp())
        return findQmlProjectUpwards(Utils::FilePath::fromString(dir.absolutePath()));
    return {};
}

static bool findAndOpenProject(const Utils::FilePath &filePath)
{

    ProjectExplorer::Project *project
            = ProjectExplorer::SessionManager::projectForFile(filePath);

    if (project) {
        if (project->projectFilePath().suffix() == "qmlproject") {
            QmlProjectPlugin::openQDS(project->projectFilePath());
            return true;
        } else {
            auto projectFolder = project->rootProjectDirectory();
            auto qmlProjectFile = findQmlProject(projectFolder);
            if (qmlProjectFile.exists()) {
                QmlProjectPlugin::openQDS(qmlProjectFile);
                return true;
            }
        }
    }

    auto qmlProjectFile = findQmlProjectUpwards(filePath);
    if (qmlProjectFile.exists()) {
        QmlProjectPlugin::openQDS(qmlProjectFile);
        return true;
    }
    return false;
}

void QmlProjectPlugin::openInQDSWithProject(const Utils::FilePath &filePath)
{
    if (findAndOpenProject(filePath)) {
        openQDS(filePath);
        //The first one might be ignored when QDS is starting up
        QTimer::singleShot(4000, [filePath] { openQDS(filePath); });
    } else {
        Core::AsynchronousMessageBox::warning(
            tr("Qt Design Studio"),
            tr("No project file (*.qmlproject) found for Qt Design "
               "Studio.\n Qt Design Studio requires a .qmlproject "
               "based project to open the .ui.qml file."));
    }
}

bool QmlProjectPlugin::initialize(const QStringList &, QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    d = new QmlProjectPluginPrivate;

    if (!qmlDesignerEnabled()) {
        connect(Core::EditorManager::instance(),
                &Core::EditorManager::currentEditorChanged,
                [this](Core::IEditor *editor) {
                    QmlJS::ModelManagerInterface *modelManager
                        = QmlJS::ModelManagerInterface::instance();

                    if (!editor)
                        return;

                    if (d->lastMessageBox)
                        return;
                    auto filePath = editor->document()->filePath();
                    QmlJS::Document::Ptr document = modelManager->ensuredGetDocumentForPath(
                        filePath.toString());
                    if (!document.isNull()
                        && document->language() == QmlJS::Dialect::QmlQtQuick2Ui) {

                        const QString description = tr("Files of the type ui.qml are intended for Qt Design Studio.");

                        if (!qdsInstallationExists()) {
                            if (Core::ICore::infoBar()->canInfoBeAdded(openInQDSAppInfoBar)) {
                                                Utils::InfoBarEntry
                                                    info(openInQDSAppInfoBar,
                                                         description + tr(" Learn more about Qt Design Studio here: ")
                                                         + "<a href='https://www.qt.io/product/ui-design-tools'>Qt Design Studio</a>",
                                                         Utils::InfoBarEntry::GlobalSuppression::Disabled);
                                                Core::ICore::infoBar()->addInfo(info);
                            }
                            return;
                        }


                        if (alwaysOpenUiQmlfileInQds()) {
                            openInQDSWithProject(filePath);
                        } else if (Core::ICore::infoBar()->canInfoBeAdded(openInQDSAppInfoBar)) {
                            Utils::InfoBarEntry
                                    info(openInQDSAppInfoBar,
                                         description + "\n" + tr("Do you want to open this file always in Qt Design Studio?"),
                                         Utils::InfoBarEntry::GlobalSuppression::Disabled);
                            info.addCustomButton(tr("Always open in Qt Design Studio"), [filePath] {
                                Core::ICore::infoBar()->removeInfo(openInQDSAppInfoBar);

                                enableAlwaysOpenUiQmlfileInQds();
                                openInQDSWithProject(filePath);
                            });
                            Core::ICore::infoBar()->addInfo(info);
                        }
                    }
        });
    }

    ProjectManager::registerProjectType<QmlProject>(QmlJSTools::Constants::QMLPROJECT_MIMETYPE);
    Core::FileIconProvider::registerIconOverlayForSuffix(":/qmlproject/images/qmlproject.png",
                                                         "qmlproject");
    return true;
} // namespace Internal

} // namespace Internal
} // namespace QmlProjectManager
