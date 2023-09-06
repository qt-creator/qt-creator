// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsmodelmanager.h"
#include "qmljstoolsconstants.h"
#include "qmljssemanticinfo.h"
#include "qmljsbundleprovider.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
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
#include <utils/mimeutils.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QSet>
#include <QString>
#include <qglobal.h>

using namespace Utils;
using namespace Core;
using namespace ProjectExplorer;
using namespace QmlJS;

namespace QmlJSTools {
namespace Internal {

static void setupProjectInfoQmlBundles(ModelManagerInterface::ProjectInfo &projectInfo)
{
    Target *activeTarget = nullptr;
    if (projectInfo.project)
        activeTarget = projectInfo.project->activeTarget();
    Kit *activeKit = activeTarget ? activeTarget->kit() : KitManager::defaultKit();
    const QHash<QString, QString> replacements = {{QLatin1String("$(QT_INSTALL_QML)"), projectInfo.qtQmlPath.toString()}};

    for (IBundleProvider *bp : IBundleProvider::allBundleProviders())
        bp->mergeBundlesForKit(activeKit, projectInfo.activeBundle, replacements);

    projectInfo.extendedBundle = projectInfo.activeBundle;

    if (projectInfo.project) {
        QSet<Kit *> currentKits;
        const QList<Target *> targets = projectInfo.project->targets();
        for (const Target *t : targets)
            currentKits.insert(t->kit());
        currentKits.remove(activeKit);
        for (Kit *kit : std::as_const(currentKits)) {
            for (IBundleProvider *bp : IBundleProvider::allBundleProviders())
                bp->mergeBundlesForKit(kit, projectInfo.extendedBundle, replacements);
        }
    }
}

static void findAllQrcFiles(const FilePath &filePath, FilePaths &out)
{
    filePath.iterateDirectory(
        [&out](const FilePath &path) {
            out.append(path.canonicalPath());
            return IterationPolicy::Continue;
        },
        {{"*.qrc"}, QDir::Files});
}

static FilePaths findGeneratedQrcFiles(const ModelManagerInterface::ProjectInfo &pInfo,
                                       const FilePaths &hiddenRccFolders)
{
    FilePaths result;
    // Search in Application Directories for directories named ".rcc"
    // and add all .qrc files in there to the resource file list.
    for (const Utils::FilePath &path : pInfo.applicationDirectories) {
        Utils::FilePath generatedQrcDir = path.pathAppended(".rcc");
        findAllQrcFiles(generatedQrcDir, result);
    }

    for (const Utils::FilePath &hiddenRccFolder : hiddenRccFolders) {
        findAllQrcFiles(hiddenRccFolder, result);
    }

    return result;
}

ModelManagerInterface::ProjectInfo ModelManager::defaultProjectInfoForProject(
    Project *project, const FilePaths &hiddenRccFolders) const
{
    ModelManagerInterface::ProjectInfo projectInfo;
    projectInfo.project = project;
    projectInfo.qmlDumpEnvironment = Utils::Environment::systemEnvironment();
    Target *activeTarget = nullptr;
    if (project) {
        const QSet<QString> qmlTypeNames = { Constants::QML_MIMETYPE ,Constants::QBS_MIMETYPE,
                                             Constants::QMLPROJECT_MIMETYPE,
                                             Constants::QMLTYPES_MIMETYPE,
                                             Constants::QMLUI_MIMETYPE };
        projectInfo.sourceFiles = project->files([&qmlTypeNames](const Node *n) {
            if (!Project::SourceFiles(n))
                return false;
            const FileNode *fn = n->asFileNode();
            return fn && fn->fileType() == FileType::QML
                    && qmlTypeNames.contains(Utils::mimeTypeForFile(fn->filePath(),
                                                                    MimeMatchMode::MatchExtension).name());
        });
        activeTarget = project->activeTarget();
    }
    Kit *activeKit = activeTarget ? activeTarget->kit() : KitManager::defaultKit();
    QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(activeKit);

    projectInfo.tryQmlDump = false;

    if (activeTarget) {
        FilePath baseDir;
        auto addAppDir = [&baseDir, &projectInfo](const FilePath &mdir) {
            auto dir = mdir.cleanPath();
            if (!baseDir.path().isEmpty()) {
                auto rDir = dir.relativePathFrom(baseDir);
                // do not add directories outside the build directory
                // this might happen for example when we think an executable path belongs to
                // a bundle, and we need to remove extra directories, but that was not the case
                if (rDir.path().split(u'/').contains(QStringLiteral(u"..")))
                    return;
            }
            if (!projectInfo.applicationDirectories.contains(dir))
                projectInfo.applicationDirectories.append(dir);
        };

        if (BuildConfiguration *bc = activeTarget->activeBuildConfiguration()) {
            // Append QML2_IMPORT_PATH if it is defined in build configuration.
            // It enables qmlplugindump to correctly dump custom plugins or other dependent
            // plugins that are not installed in default Qt qml installation directory.
            projectInfo.qmlDumpEnvironment.appendOrSet("QML2_IMPORT_PATH", bc->environment().expandedValueForKey("QML2_IMPORT_PATH"), ":");
            // Treat every target (library or application) in the build directory

            FilePath dir = bc->buildDirectory();
            baseDir = dir.absoluteFilePath();
            addAppDir(dir);
        }
        // Qml loads modules from the following sources
        // 1. The build directory of the executable
        // 2. Any QML_IMPORT_PATH (environment variable) or IMPORT_PATH (parameter to qt_add_qml_module)
        // 3. The Qt import path
        // For an IDE things are a bit more complicated because source files might be edited,
        // and the directory of the executable might be outdated.
        // Here we try to get the directory of the executable, adding all targets
        const auto appTargets = activeTarget->buildSystem()->applicationTargets();
        for (const auto &target : appTargets) {
            if (target.targetFilePath.isEmpty())
                continue;
            auto dir = target.targetFilePath.parentDir();
            projectInfo.applicationDirectories.append(dir);
            // unfortunately the build directory of the executable where cmake puts the qml
            // might be different than the directory of the executable:
            if (HostOsInfo::isWindowsHost()) {
                // On Windows systems QML type information is located one directory higher as we build
                // in dedicated "debug" and "release" directories
                addAppDir(dir.parentDir());
            } else if (HostOsInfo::isMacHost()) {
                // On macOS and iOS when building a bundle this is not the case and
                // we have to go up up to three additional directories
                // (BundleName.app/Contents/MacOS or BundleName.app/Contents for iOS)
                if (dir.fileName() == u"MacOS")
                    dir = dir.parentDir();
                if (dir.fileName() == u"Contents")
                    dir = dir.parentDir().parentDir();
                addAppDir(dir);
            }
        }
    }
    if (qtVersion && qtVersion->isValid()) {
        projectInfo.tryQmlDump = project && qtVersion->type() == QLatin1String(QtSupport::Constants::DESKTOPQT);
        projectInfo.qtQmlPath = qtVersion->qmlPath();
        auto v = qtVersion->qtVersion();
        projectInfo.qmllsPath = ModelManagerInterface::qmllsForBinPath(qtVersion->hostBinPath(), v);
        projectInfo.qtVersionString = qtVersion->qtVersionString();
    } else if (!activeKit || !activeKit->value(QtSupport::Constants::FLAGS_SUPPLIES_QTQUICK_IMPORT_PATH, false).toBool()) {
        projectInfo.qtQmlPath = FilePath::fromUserInput(QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath));
        projectInfo.qmllsPath = ModelManagerInterface::qmllsForBinPath(
            FilePath::fromUserInput(QLibraryInfo::path(QLibraryInfo::BinariesPath)), QLibraryInfo::version());
        projectInfo.qtVersionString = QLatin1String(qVersion());
    }

    projectInfo.qmlDumpPath.clear();
    const QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(activeKit);
    if (version && projectInfo.tryQmlDump) {
        projectInfo.qmlDumpPath = version->qmlplugindumpFilePath();
        projectInfo.qmlDumpHasRelocatableFlag = version->hasQmlDumpWithRelocatableFlag();
    }

    setupProjectInfoQmlBundles(projectInfo);
    projectInfo.generatedQrcFiles = findGeneratedQrcFiles(projectInfo, hiddenRccFolders);
    return projectInfo;
}

QHash<QString,Dialect> ModelManager::initLanguageForSuffix() const
{
    QHash<QString,Dialect> res = ModelManagerInterface::languageForSuffix();

    if (ICore::instance()) {
        MimeType jsSourceTy = Utils::mimeTypeForName(Constants::JS_MIMETYPE);
        const QStringList jsSuffixes = jsSourceTy.suffixes();
        for (const QString &suffix : jsSuffixes)
            res[suffix] = Dialect::JavaScript;
        MimeType qmlSourceTy = Utils::mimeTypeForName(Constants::QML_MIMETYPE);
        const QStringList qmlSuffixes = qmlSourceTy.suffixes();
        for (const QString &suffix : qmlSuffixes)
            res[suffix] = Dialect::Qml;
        MimeType qbsSourceTy = Utils::mimeTypeForName(Constants::QBS_MIMETYPE);
        const QStringList qbsSuffixes = qbsSourceTy.suffixes();
        for (const QString &suffix : qbsSuffixes)
            res[suffix] = Dialect::QmlQbs;
        MimeType qmlProjectSourceTy = Utils::mimeTypeForName(Constants::QMLPROJECT_MIMETYPE);
        const QStringList qmlProjSuffixes = qmlProjectSourceTy.suffixes();
        for (const QString &suffix : qmlProjSuffixes)
            res[suffix] = Dialect::QmlProject;
        MimeType qmlUiSourceTy = Utils::mimeTypeForName(Constants::QMLUI_MIMETYPE);
        const QStringList qmlUiSuffixes = qmlUiSourceTy.suffixes();
        for (const QString &suffix : qmlUiSuffixes)
            res[suffix] = Dialect::QmlQtQuick2Ui;
        MimeType jsonSourceTy = Utils::mimeTypeForName(Constants::JSON_MIMETYPE);
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

ModelManager::ModelManager()
{
    qRegisterMetaType<QmlJSTools::SemanticInfo>("QmlJSTools::SemanticInfo");
    CppQmlTypesLoader::defaultObjectsInitializer = [this] { loadDefaultQmlTypeDescriptions(); };
}

ModelManager::~ModelManager() = default;

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

    ViewerContext qbsVContext;
    qbsVContext.language = Dialect::QmlQbs;
    qbsVContext.paths.insert(ICore::resourcePath("qbs"));
    setDefaultVContext(qbsVContext);
}

void ModelManager::loadDefaultQmlTypeDescriptions()
{
    if (ICore::instance()) {
        loadQmlTypeDescriptionsInternal(ICore::resourcePath().toString());
        loadQmlTypeDescriptionsInternal(ICore::userResourcePath().toString());
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

void ModelManager::updateDefaultProjectInfo()
{
    // needs to be performed in the ui thread
    Project *currentProject = ProjectManager::startupProject();
    setDefaultProject(containsProject(currentProject)
                            ? projectInfo(currentProject)
                            : defaultProjectInfoForProject(currentProject, {}),
                      currentProject);
}


void ModelManager::addTaskInternal(const QFuture<void> &result, const QString &msg,
                                   const char *taskId) const
{
    ProgressManager::addTask(result, msg, taskId);
}

} // namespace Internal
} // namespace QmlJSTools
