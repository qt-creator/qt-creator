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

#include "qmljsmodelmanager.h"
#include "qmljstoolsconstants.h"
#include "qmljssemanticinfo.h"
#include "qmljsbundleprovider.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsfindexportedcpptypes.h>
#include <qmljs/qmljsplugindumper.h>
#include <qtsupport/qmldumptool.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <texteditor/textdocument.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <utils/runextensions.h>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QRegExp>
#include <QLibraryInfo>
#include <qglobal.h>

using namespace Utils;
using namespace Core;
using namespace ProjectExplorer;
using namespace QmlJS;

namespace QmlJSTools {
namespace Internal {

ModelManagerInterface::ProjectInfo ModelManager::defaultProjectInfoForProject(
        Project *project) const
{
    ModelManagerInterface::ProjectInfo projectInfo(project);
    Target *activeTarget = 0;
    if (project) {
        QSet<QString> qmlTypeNames;
        qmlTypeNames << QLatin1String(Constants::QML_MIMETYPE)
                     << QLatin1String(Constants::QBS_MIMETYPE)
                     << QLatin1String(Constants::QMLPROJECT_MIMETYPE)
                     << QLatin1String(Constants::QMLTYPES_MIMETYPE)
                     << QLatin1String(Constants::QMLUI_MIMETYPE);
        foreach (const QString &filePath, project->files(Project::SourceFiles)) {
            if (qmlTypeNames.contains(Utils::mimeTypeForFile(
                                          filePath, MimeMatchMode::MatchExtension).name())) {
                projectInfo.sourceFiles << filePath;
            }
        }
        activeTarget = project->activeTarget();
    }
    Kit *activeKit = activeTarget ? activeTarget->kit() : KitManager::defaultKit();
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(activeKit);

    bool preferDebugDump = false;
    bool setPreferDump = false;
    projectInfo.tryQmlDump = false;

    if (activeTarget) {
        if (BuildConfiguration *bc = activeTarget->activeBuildConfiguration()) {
            preferDebugDump = bc->buildType() == BuildConfiguration::Debug;
            setPreferDump = true;
        }
    }
    if (!setPreferDump && qtVersion)
        preferDebugDump = (qtVersion->defaultBuildConfig() & QtSupport::BaseQtVersion::DebugBuild);
    if (qtVersion && qtVersion->isValid()) {
        projectInfo.tryQmlDump = project && qtVersion->type() == QLatin1String(QtSupport::Constants::DESKTOPQT);
        projectInfo.qtQmlPath = QFileInfo(qtVersion->qmakeProperty("QT_INSTALL_QML")).canonicalFilePath();
        projectInfo.qtImportsPath = QFileInfo(qtVersion->qmakeProperty("QT_INSTALL_IMPORTS")).canonicalFilePath();
        projectInfo.qtVersionString = qtVersion->qtVersionString();
    } else {
        projectInfo.qtQmlPath = QFileInfo(QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath)).canonicalFilePath();
        projectInfo.qtImportsPath = QFileInfo(QLibraryInfo::location(QLibraryInfo::ImportsPath)).canonicalFilePath();
        projectInfo.qtVersionString = QLatin1String(qVersion());
    }

    if (projectInfo.tryQmlDump) {
        QtSupport::QmlDumpTool::pathAndEnvironment(activeKit,
                                                   preferDebugDump, &projectInfo.qmlDumpPath,
                                                   &projectInfo.qmlDumpEnvironment);
        projectInfo.qmlDumpHasRelocatableFlag = qtVersion->hasQmlDumpWithRelocatableFlag();
    } else {
        projectInfo.qmlDumpPath.clear();
        projectInfo.qmlDumpEnvironment.clear();
        projectInfo.qmlDumpHasRelocatableFlag = true;
    }
    setupProjectInfoQmlBundles(projectInfo);
    return projectInfo;
}

} // namespace Internal

void setupProjectInfoQmlBundles(ModelManagerInterface::ProjectInfo &projectInfo)
{
    Target *activeTarget = 0;
    if (projectInfo.project)
        activeTarget = projectInfo.project->activeTarget();
    Kit *activeKit = activeTarget ? activeTarget->kit() : KitManager::defaultKit();
    QHash<QString, QString> replacements;
    replacements.insert(QLatin1String("$(QT_INSTALL_IMPORTS)"), projectInfo.qtImportsPath);
    replacements.insert(QLatin1String("$(QT_INSTALL_QML)"), projectInfo.qtQmlPath);

    QList<IBundleProvider *> bundleProviders =
            ExtensionSystem::PluginManager::getObjects<IBundleProvider>();

    foreach (IBundleProvider *bp, bundleProviders) {
        if (bp)
            bp->mergeBundlesForKit(activeKit, projectInfo.activeBundle, replacements);
    }
    projectInfo.extendedBundle = projectInfo.activeBundle;

    if (projectInfo.project) {
        QSet<Kit *> currentKits;
        foreach (const Target *t, projectInfo.project->targets())
            if (t->kit())
                currentKits.insert(t->kit());
        currentKits.remove(activeKit);
        foreach (Kit *kit, currentKits) {
            foreach (IBundleProvider *bp, bundleProviders)
                if (bp)
                    bp->mergeBundlesForKit(kit, projectInfo.extendedBundle, replacements);
        }
    }
}

namespace Internal {


QHash<QString,Dialect> ModelManager::initLanguageForSuffix() const
{
    QHash<QString,Dialect> res = ModelManagerInterface::languageForSuffix();

    if (ICore::instance()) {
        MimeType jsSourceTy = Utils::mimeTypeForName(Constants::JS_MIMETYPE);
        foreach (const QString &suffix, jsSourceTy.suffixes())
            res[suffix] = Dialect::JavaScript;
        MimeType qmlSourceTy = Utils::mimeTypeForName(Constants::QML_MIMETYPE);
        foreach (const QString &suffix, qmlSourceTy.suffixes())
            res[suffix] = Dialect::Qml;
        MimeType qbsSourceTy = Utils::mimeTypeForName(Constants::QBS_MIMETYPE);
        foreach (const QString &suffix, qbsSourceTy.suffixes())
            res[suffix] = Dialect::QmlQbs;
        MimeType qmlProjectSourceTy = Utils::mimeTypeForName(Constants::QMLPROJECT_MIMETYPE);
        foreach (const QString &suffix, qmlProjectSourceTy.suffixes())
            res[suffix] = Dialect::QmlProject;
        MimeType qmlUiSourceTy = Utils::mimeTypeForName(Constants::QMLUI_MIMETYPE);
        foreach (const QString &suffix, qmlUiSourceTy.suffixes())
            res[suffix] = Dialect::QmlQtQuick2Ui;
        MimeType jsonSourceTy = Utils::mimeTypeForName(Constants::JSON_MIMETYPE);
        foreach (const QString &suffix, jsonSourceTy.suffixes())
            res[suffix] = Dialect::Json;
    }
    return res;
}

QHash<QString,Dialect> ModelManager::languageForSuffix() const
{
    static QHash<QString,Dialect> res = initLanguageForSuffix();
    return res;
}

ModelManager::ModelManager(QObject *parent):
        ModelManagerInterface(parent)
{
    qRegisterMetaType<QmlJSTools::SemanticInfo>("QmlJSTools::SemanticInfo");
    loadDefaultQmlTypeDescriptions();
}

ModelManager::~ModelManager()
{
}

void ModelManager::delayedInitialization()
{
    CppTools::CppModelManager *cppModelManager = CppTools::CppModelManager::instance();
    // It's important to have a direct connection here so we can prevent
    // the source and AST of the cpp document being cleaned away.
    connect(cppModelManager, &CppTools::CppModelManager::documentUpdated,
            this, &ModelManagerInterface::maybeQueueCppQmlTypeUpdate, Qt::DirectConnection);

    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, &ModelManager::removeProjectInfo);
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &ModelManager::updateDefaultProjectInfo);

    ViewerContext qbsVContext;
    qbsVContext.language = Dialect::QmlQbs;
    qbsVContext.maybeAddPath(ICore::resourcePath() + QLatin1String("/qbs"));
    setDefaultVContext(qbsVContext);
}

void ModelManager::loadDefaultQmlTypeDescriptions()
{
    if (ICore::instance()) {
        loadQmlTypeDescriptionsInternal(ICore::resourcePath());
        loadQmlTypeDescriptionsInternal(ICore::userResourcePath());
    }
}

void ModelManager::writeMessageInternal(const QString &msg) const
{
    MessageManager::write(msg, MessageManager::Flash);
}

ModelManagerInterface::WorkingCopy ModelManager::workingCopyInternal() const
{
    WorkingCopy workingCopy;

    if (!Core::ICore::instance())
        return workingCopy;

    foreach (IDocument *document, DocumentModel::openedDocuments()) {
        const QString key = document->filePath().toString();
        if (TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(document)) {
            // TODO the language should be a property on the document, not the editor
            if (DocumentModel::editorsForDocument(document).first()
                    ->context().contains(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID)) {
                workingCopy.insert(key, textDocument->plainText(),
                                   textDocument->document()->revision());
            }
        }
    }

    return workingCopy;
}

void ModelManager::updateDefaultProjectInfo()
{
    // needs to be performed in the ui thread
    Project *currentProject = SessionManager::startupProject();
    ProjectInfo newDefaultProjectInfo = projectInfo(currentProject,
                                                    defaultProjectInfoForProject(currentProject));
    setDefaultProject(projectInfo(currentProject,newDefaultProjectInfo), currentProject);
}


void ModelManager::addTaskInternal(QFuture<void> result, const QString &msg, const char *taskId) const
{
    ProgressManager::addTask(result, msg, taskId);
}

} // namespace Internal
} // namespace QmlJSTools
