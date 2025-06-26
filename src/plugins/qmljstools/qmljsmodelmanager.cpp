// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsmodelmanager.h"
#include "qmljssemanticinfo.h"
#include "qmljsbundleprovider.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/session.h>

#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsfindexportedcpptypes.h>
#include <qmljs/qmljsplugindumper.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>

#include <QLibraryInfo>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QSet>

using namespace Utils;
using namespace Core;
using namespace ProjectExplorer;
using namespace QmlJS;

namespace QmlJSTools::Internal {

static ModelManagerInterface::ProjectInfo
    fromQmlCodeModelInfo(Project *project, Kit *kit, const QmlCodeModelInfo &info)
{
    ModelManagerInterface::ProjectInfo projectInfo;
    projectInfo.project = project;
    projectInfo.sourceFiles = info.sourceFiles;
    for (const FilePath &path : info.qmlImportPaths)
        projectInfo.importPaths.maybeInsert(path, QmlJS::Dialect::Qml);
    projectInfo.activeResourceFiles = info.activeResourceFiles;
    projectInfo.allResourceFiles = info.allResourceFiles;
    projectInfo.generatedQrcFiles = info.generatedQrcFiles;
    projectInfo.resourceFileContents = info.resourceFileContents;
    projectInfo.applicationDirectories = info.applicationDirectories;
    projectInfo.moduleMappings = info.moduleMappings;

    // whether trying to run qml
    projectInfo.tryQmlDump = info.tryQmlDump;
    projectInfo.qmlDumpHasRelocatableFlag = info.qmlDumpHasRelocatableFlag;
    projectInfo.qmlDumpPath = info.qmlDumpPath;
    projectInfo.qmlDumpEnvironment = info.qmlDumpEnvironment;

    projectInfo.qtQmlPath = info.qtQmlPath;
    projectInfo.qmllsPath = info.qmllsPath;
    projectInfo.qtVersionString = info.qtVersionString;

    const QHash<QString, QString> replacements = {
         {QLatin1String("$(QT_INSTALL_QML)"), projectInfo.qtQmlPath.path()}
    };

    for (IBundleProvider *bp : IBundleProvider::allBundleProviders())
        bp->mergeBundlesForKit(kit, projectInfo.activeBundle, replacements);

    projectInfo.extendedBundle = projectInfo.activeBundle;

    QSet<Kit *> currentKits;
    const QList<Target *> targets = project->targets();
    for (const Target *t : targets)
        currentKits.insert(t->kit());
    currentKits.remove(kit);
    for (Kit *kit : std::as_const(currentKits)) {
        for (IBundleProvider *bp : IBundleProvider::allBundleProviders())
            bp->mergeBundlesForKit(kit, projectInfo.extendedBundle, replacements);
    }

    return projectInfo;
}

ModelManager::ModelManager()
{
    qRegisterMetaType<QmlJSTools::SemanticInfo>("QmlJSTools::SemanticInfo");
    CppQmlTypesLoader::defaultObjectsInitializer = [this] { loadDefaultQmlTypeDescriptions(); };

    Project::setQmlCodeModelIsUsed();

    connect(ProjectManager::instance(), &ProjectManager::extraProjectInfoChanged,
            this, &ModelManager::updateFromBuildConfig);
}

ModelManager::~ModelManager() = default;

QHash<QString,Dialect> ModelManager::initLanguageForSuffix() const
{
    QHash<QString,Dialect> res = ModelManagerInterface::languageForSuffix();

    if (ICore::instance()) {
        using namespace Utils::Constants;;
        MimeType jsSourceTy = Utils::mimeTypeForName(JS_MIMETYPE);
        const QStringList jsSuffixes = jsSourceTy.suffixes();
        for (const QString &suffix : jsSuffixes)
            res[suffix] = Dialect::JavaScript;
        MimeType qmlSourceTy = Utils::mimeTypeForName(QML_MIMETYPE);
        const QStringList qmlSuffixes = qmlSourceTy.suffixes();
        for (const QString &suffix : qmlSuffixes)
            res[suffix] = Dialect::Qml;
        MimeType qbsSourceTy = Utils::mimeTypeForName(QBS_MIMETYPE);
        const QStringList qbsSuffixes = qbsSourceTy.suffixes();
        for (const QString &suffix : qbsSuffixes)
            res[suffix] = Dialect::QmlQbs;
        MimeType qmlProjectSourceTy = Utils::mimeTypeForName(QMLPROJECT_MIMETYPE);
        const QStringList qmlProjSuffixes = qmlProjectSourceTy.suffixes();
        for (const QString &suffix : qmlProjSuffixes)
            res[suffix] = Dialect::QmlProject;
        MimeType qmlUiSourceTy = Utils::mimeTypeForName(QMLUI_MIMETYPE);
        const QStringList qmlUiSuffixes = qmlUiSourceTy.suffixes();
        for (const QString &suffix : qmlUiSuffixes)
            res[suffix] = Dialect::QmlQtQuick2Ui;
        MimeType jsonSourceTy = Utils::mimeTypeForName(JSON_MIMETYPE);
        const QStringList jsonSuffixes = jsonSourceTy.suffixes();
        for (const QString &suffix : jsonSuffixes)
            res[suffix] = Dialect::Json;
    }
    return res;
}

QHash<QString,Dialect> ModelManager::languageForSuffix() const
{
    static QHash<QString,Dialect> res = initLanguageForSuffix();
    return res;
}

void ModelManager::delayedInitialization()
{
    CppEditor::CppModelManager *cppModelManager = CppEditor::CppModelManager::instance();
    // It's important to have a direct connection here so we can prevent
    // the source and AST of the cpp document being cleaned away.
    connect(cppModelManager, &CppEditor::CppModelManager::documentUpdated,
            this, &ModelManagerInterface::maybeQueueCppQmlTypeUpdate, Qt::DirectConnection);

    connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
            this, &ModelManager::removeProjectInfo);
    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
            this, &ModelManager::updateDefaultProjectInfo);
    connect(SessionManager::instance(), &SessionManager::aboutToLoadSession,
            this, &ModelManager::cancelAllThreads);

    ViewerContext qbsVContext;
    qbsVContext.language = Dialect::QmlQbs;
    qbsVContext.paths.insert(ICore::resourcePath("qbs"));
    setDefaultVContext(qbsVContext);
}

void ModelManager::loadDefaultQmlTypeDescriptions()
{
    if (ICore::instance()) {
        loadQmlTypeDescriptionsInternal(ICore::resourcePath().toUrlishString());
        loadQmlTypeDescriptionsInternal(ICore::userResourcePath().toUrlishString());
    }
}

void ModelManager::writeMessageInternal(const QString &msg) const
{
    MessageManager::writeFlashing(msg);
}

ModelManagerInterface::WorkingCopy ModelManager::workingCopyInternal() const
{
    WorkingCopy workingCopy;

    if (!Core::ICore::instance())
        return workingCopy;

    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (IDocument *document : documents) {
        const Utils::FilePath key = document->filePath();
        if (auto textDocument = qobject_cast<const TextEditor::TextDocument *>(document)) {
            // TODO the language should be a property on the document, not the editor
            if (DocumentModel::editorsForDocument(document).constFirst()
                    ->context().contains(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID)) {
                workingCopy.insert(key, textDocument->plainText(),
                                   textDocument->document()->revision());
            }
        }
    }

    return workingCopy;
}

static ModelManagerInterface::ProjectInfo defaultProjectInfoForProject(Project *project)
{
    Kit *activeKit = ProjectExplorer::activeKit(project);
    Kit *kit = activeKit ? activeKit : KitManager::defaultKit();
    QmlCodeModelInfo info = project->gatherQmlCodeModelInfo(kit, project->activeBuildConfiguration());
    return fromQmlCodeModelInfo(project, kit, info);
}

void ModelManager::updateDefaultProjectInfo()
{
    // needs to be performed in the ui thread
    Project *currentProject = ProjectManager::startupProject();
    if (!currentProject)
        return;
    setDefaultProject(containsProject(currentProject)
                            ? projectInfo(currentProject)
                            : defaultProjectInfoForProject(currentProject),
                      currentProject);
}

void ModelManager::updateFromBuildConfig(BuildConfiguration *bc, const QmlCodeModelInfo &info)
{
    Project *project = bc->project();
    ProjectInfo projectInfo = fromQmlCodeModelInfo(project, bc->kit(), info);

    updateProjectInfo(projectInfo, project);
}

void ModelManager::addTaskInternal(const QFuture<void> &result, const QString &msg,
                                   const Id taskId) const
{
    ProgressManager::addTask(result, msg, taskId);
}

Project *projectFromProjectInfo(const QmlJS::ModelManagerInterface::ProjectInfo &projectInfo)
{
    return qobject_cast<Project *>(projectInfo.project);
}

} // namespace QmlJSTools::Internal
