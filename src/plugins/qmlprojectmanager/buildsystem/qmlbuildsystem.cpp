// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlbuildsystem.h"

#include "../qmlprojectconstants.h"
#include "../qmlprojectmanagertr.h"
#include "../qmlproject.h"
#include "projectitem/qmlprojectitem.h"
#include "projectnode/qmlprojectnodes.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/session.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/filesystemwatcher.h>
#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>

#include <texteditor/textdocument.h>

#include <QAction>
#include <QtCore5Compat/qtextcodec.h>

using namespace ProjectExplorer;
using namespace Utils;
using namespace ExtensionSystem;

namespace QmlProjectManager {

namespace {
Q_LOGGING_CATEGORY(infoLogger, "QmlProjectManager.QmlBuildSystem", QtInfoMsg)
}

IPlugin *findMcuSupportPlugin()
{
    const PluginSpec *pluginSpec = PluginManager::specById(QString("mcusupport"));
    return pluginSpec ? pluginSpec->plugin() : nullptr;
}

void updateMcuBuildStep(BuildConfiguration *bc, bool mcuEnabled)
{
    if (auto plugin = findMcuSupportPlugin()) {
        QMetaObject::invokeMethod(
            plugin,
            "updateDeployStep",
            Qt::DirectConnection,
            Q_ARG(ProjectExplorer::BuildConfiguration *, bc),
            Q_ARG(bool, mcuEnabled));
    } else if (mcuEnabled) {
        qWarning() << "Failed to find McuSupport plugin but qtForMCUs is enabled in the project";
    }
}

QmlBuildSystem::QmlBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc)
    , m_fileGen(new QmlProjectExporter::Exporter(this))
{
    // refresh first - project information is used e.g. to decide the default RC's
    refresh(RefreshOptions::Project);

    updateDeploymentData();
//    registerMenuButtons(); //is wip

    connect(project(), &Project::activeBuildConfigurationChanged, this, [this](BuildConfiguration *bc) {
        refresh(RefreshOptions::NoFileRefresh);
        m_fileGen->updateProject(qmlProject());
        updateMcuBuildStep(bc, qtForMCUs());
    });
    connect(project(), &Project::projectFileIsDirty, this, [this] {
        refresh(RefreshOptions::Project);
        m_fileGen->updateProject(qmlProject());
        m_fileGen->updateMenuAction();
        updateMcuBuildStep(project()->activeBuildConfiguration(), qtForMCUs());
    });

    // FIXME: Check. Probably bogus after the BuildSystem move.
    //    // addedTarget calls updateEnabled on the runconfigurations
    //    // which needs to happen after refresh
    //    foreach (Target *t, targets())
    //        addedTarget(t);
}

void QmlBuildSystem::updateDeploymentData()
{
    if (!m_projectItem)
        return;

    if (RunDeviceTypeKitAspect::deviceTypeId(kit())
        == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        return;
    }

    ProjectExplorer::DeploymentData deploymentData;
    for (const auto &file : m_projectItem->files()) {
        deploymentData.addFile(file, targetFile(file).parentDir().path());
    }

    setDeploymentData(deploymentData);
}

//probably this method needs to be moved into QmlProjectPlugin::initialize to be called only once
void QmlBuildSystem::registerMenuButtons()
{
    Core::ActionContainer *menu = Core::ActionManager::actionContainer(Core::Constants::M_FILE);

    // QML Project file update button
    // This button saves the current configuration into the .qmlproject file
    auto action = new QAction(Tr::tr("Update QmlProject File"), this);
    //this registerAction registers a new action for each opened project,
    //causes the "action is already registered" warning if you have multiple opened projects,
    //is not a big thing for qds, but is annoying for qtc and should be fixed.
    Core::Command *cmd = Core::ActionManager::registerAction(action, "QmlProject.ProjectManager");
    menu->addAction(cmd, Core::Constants::G_FILE_SAVE);
    QObject::connect(action, &QAction::triggered, this, &QmlBuildSystem::updateProjectFile);
}

//wip:
bool QmlBuildSystem::updateProjectFile()
{
    QFile file(mainFilePath().fileName().append("project-test"));
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        qCritical() << "Cannot open Qml Project file for editing!";
        return false;
    }

    QTextStream ts(&file);
    // License
    ts << "/* "
          "File generated by Qt Creator"
          "Copyright (C) 2016 The Qt Company Ltd."
          "SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH "
          "Qt-GPL-exception-1.0"
          "*/"
       << Qt::endl
       << Qt::endl;

    // Components
    ts << "import QmlProject 1.1" << Qt::endl << Qt::endl;

    return true;
}

QmlProject *QmlBuildSystem::qmlProject() const
{
    return qobject_cast<QmlProject *>(project());
}

void QmlBuildSystem::triggerParsing()
{
    refresh(RefreshOptions::Project);
}

Utils::FilePath QmlBuildSystem::canonicalProjectDir() const
{
    return projectFilePath()
            .canonicalPath()
            .normalizedPathName()
            .parentDir();
}

void QmlBuildSystem::refresh(RefreshOptions options)
{
    ParseGuard guard = guardParsingRun();
    switch (options) {
    case RefreshOptions::NoFileRefresh:
        break;
    case RefreshOptions::Project:
        initProjectItem();
        [[fallthrough]];
    case RefreshOptions::Files:
        parseProjectFiles();
    }

    auto modelManager = QmlJS::ModelManagerInterface::instance();
    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo
            = modelManager->defaultProjectInfoForProject(project(),
                                                         project()->files(Project::HiddenRccFolders));

    for (const QString &importPath : absoluteImportPaths()) {
        projectInfo.importPaths.maybeInsert(Utils::FilePath::fromString(importPath),
                                            QmlJS::Dialect::Qml);
    }

    modelManager->updateProjectInfo(projectInfo, project());

    guard.markAsSuccess();

    emit projectChanged();
}

void QmlBuildSystem::initProjectItem()
{
    auto projectPath = projectFilePath();

    m_projectItem.reset(new QmlProjectItem{projectPath});

    connect(m_projectItem.data(), &QmlProjectItem::filesChanged, this, &QmlBuildSystem::refreshFiles);
    m_fileGen->updateProjectItem(m_projectItem.data(), true);
    initMcuProjectItems();
}

void QmlBuildSystem::initMcuProjectItems()
{
    m_mcuProjectItems.clear();
    m_mcuProjectFilesWatcher.clear();

    const QStringList mcuProjectFiles = m_projectItem->qmlProjectModules();
    for (const QString &mcuProjectFile : mcuProjectFiles) {
        Utils::FilePath mcuProjectFilePath = projectFilePath().parentDir().resolvePath(mcuProjectFile);
        auto qmlProjectItem = QSharedPointer<QmlProjectItem>(new QmlProjectItem{mcuProjectFilePath});

        m_mcuProjectItems.append(qmlProjectItem);
        connect(qmlProjectItem.data(), &QmlProjectItem::filesChanged, this, &QmlBuildSystem::refreshFiles);
        m_fileGen->updateProjectItem(m_projectItem.data(), false);

        m_mcuProjectFilesWatcher.addFile(mcuProjectFilePath, FileSystemWatcher::WatchModifiedDate);

        connect(&m_mcuProjectFilesWatcher,
                &Utils::FileSystemWatcher::fileChanged,
                this,
                [this](const FilePath &file) {
                    Q_UNUSED(file)
                    initMcuProjectItems();
                    refresh(RefreshOptions::Files);
                });
    }
}

void QmlBuildSystem::parseProjectFiles()
{
    if (auto modelManager = QmlJS::ModelManagerInterface::instance()) {
        modelManager->updateSourceFiles(m_projectItem->files(), true);
    }

    const QString mainFileName = m_projectItem->mainFile();
    if (!mainFileName.isEmpty()) {
        Utils::FilePath mainFilePath = canonicalProjectDir().resolvePath(mainFileName);
        Utils::FileReader reader;
        QString errorMessage;
        if (!reader.fetch(mainFilePath, &errorMessage)) {
            Core::MessageManager::writeFlashing(
                Tr::tr("Warning while loading project file %1.").arg(projectFilePath().toUserOutput()));
            Core::MessageManager::writeSilently(errorMessage);
        }
    }

    generateProjectTree();
}

void QmlBuildSystem::generateProjectTree()
{
    auto newRoot = std::make_unique<Internal::QmlProjectNode>(project());

    for (const auto &file : m_projectItem->files()) {
        const FileType fileType = (file == projectFilePath())
                ? FileType::Project
                : FileNode::fileTypeForFileName(file);

        newRoot->addNestedNode(std::make_unique<FileNode>(file, fileType));
    }

    for (const auto &mcuProjectItem : std::as_const(m_mcuProjectItems)) {
        for (const auto &file : mcuProjectItem->files()) {
            // newRoot->addNestedNode(std::make_unique<FileNode>(file, FileType::Project));
            const FileType fileType = (file == projectFilePath())
                                          ? FileType::Project
                                          : FileNode::fileTypeForFileName(file);
            newRoot->addNestedNode(std::make_unique<FileNode>(file, fileType));
        }
    }
    if (!projectFilePath().endsWith(Constants::fakeProjectName))
        newRoot->addNestedNode(std::make_unique<FileNode>(projectFilePath(), FileType::Project));

    setRootProjectNode(std::move(newRoot));
    updateDeploymentData();
}

bool QmlBuildSystem::setFileSettingInProjectFile(const QString &setting,
                                                 const Utils::FilePath &mainFilePath,
                                                 const QString &oldFile)
{
    // make sure to change it also in the qmlproject file
    const Utils::FilePath qmlProjectFilePath = project()->projectFilePath();
    Core::FileChangeBlocker fileChangeBlocker(qmlProjectFilePath);
    const QList<Core::IEditor *> editors = Core::DocumentModel::editorsForFilePath(
                qmlProjectFilePath);
    TextEditor::TextDocument *document = nullptr;
    if (!editors.isEmpty()) {
        document = qobject_cast<TextEditor::TextDocument *>(editors.first()->document());
        if (document && document->isModified())
            if (!Core::DocumentManager::saveDocument(document))
                return false;
    }

    QString fileContent;
    TextFileFormat textFileFormat;
    const QTextCodec *codec = QTextCodec::codecForName("UTF-8"); // qml files are defined to be utf-8
    TextFileFormat::ReadResult readResult = TextFileFormat::readFile(qmlProjectFilePath,
                                                                     codec,
                                                                     &fileContent,
                                                                     &textFileFormat);
    if (readResult.code != TextFileFormat::ReadSuccess)
        qWarning() << "Failed to read file" << qmlProjectFilePath << ":" << readResult.error;

    const QString settingQmlCode = setting + ":";

    const Utils::FilePath projectDir = project()->projectFilePath().parentDir();
    const QString relativePath = mainFilePath.relativeChildPath(projectDir).path();

    if (fileContent.indexOf(settingQmlCode) < 0) {
        QString addedText = QString("\n    %1 \"%2\"\n").arg(settingQmlCode, relativePath);
        auto index = fileContent.lastIndexOf("}");
        fileContent.insert(index, addedText);
    } else {
        QString originalFileName = oldFile;
        originalFileName.replace(".", "\\.");
        const QRegularExpression expression(
            QString("%1\\s*\"(%2)\"").arg(settingQmlCode, originalFileName));

        const QRegularExpressionMatch match = expression.match(fileContent);

        fileContent.replace(match.capturedStart(1), match.capturedLength(1), relativePath);
    }

    if (const Result<> res = textFileFormat.writeFile(qmlProjectFilePath, fileContent); !res)
        qWarning() << "Failed to write file" << qmlProjectFilePath << ":" << res.error();

    refresh(RefreshOptions::Project);
    return true;
}

bool QmlBuildSystem::blockFilesUpdate() const
{
    return m_blockFilesUpdate;
}

void QmlBuildSystem::setBlockFilesUpdate(bool newBlockFilesUpdate)
{
    m_blockFilesUpdate = newBlockFilesUpdate;
}

Utils::FilePath QmlBuildSystem::getStartupQmlFileWithFallback() const
{
    const auto currentProject = project();

    if (!currentProject)
        return {};

    if (!target())
        return {};

    if (projectFilePath().endsWith(Constants::fakeProjectName))
        return {};

    const auto getFirstFittingFile = [](const Utils::FilePaths &files) -> Utils::FilePath {
        for (const auto &file : files) {
            if (file.exists())
                return file;
        }
        return {};
    };

    const QStringView uiqmlstr = u"ui.qml";
    const QStringView qmlstr = u"qml";

    //we will check mainUiFile and mainFile twice:
    //first priority if it's ui.qml file, second if it's just a qml file
    const Utils::FilePath mainUiFile = mainUiFilePath();
    if (mainUiFile.exists() && mainUiFile.completeSuffix() == uiqmlstr)
        return mainUiFile;

    const Utils::FilePath mainQmlFile = mainFilePath();
    if (mainQmlFile.exists() && mainQmlFile.completeSuffix() == uiqmlstr)
        return mainQmlFile;

    const Utils::FilePaths uiFiles = currentProject->files([&](const ProjectExplorer::Node *node) {
        return node->filePath().completeSuffix() == uiqmlstr;
    });
    if (!uiFiles.isEmpty()) {
        if (const auto file = getFirstFittingFile(uiFiles); !file.isEmpty())
            return file;
    }

    //check the suffix of mainUiFiles again, since there are no ui.qml files:
    if (mainUiFile.exists() && mainUiFile.completeSuffix() == qmlstr)
        return mainUiFile;

    if (mainQmlFile.exists() && mainQmlFile.completeSuffix() == qmlstr)
        return mainQmlFile;

    //maybe it's also worth priotizing qml files containing common words like "Screen"?
    const Utils::FilePaths qmlFiles = currentProject->files([&](const ProjectExplorer::Node *node) {
        return node->filePath().completeSuffix() == qmlstr;
    });
    if (!qmlFiles.isEmpty()) {
        if (const auto file = getFirstFittingFile(qmlFiles); !file.isEmpty())
            return file;
    }

    //if no source files exist in the project, lets try to open the .qmlproject file itself
    const Utils::FilePath projectFile = projectFilePath();
    if (projectFile.exists())
        return projectFile;

    return {};
}

QmlBuildSystem *QmlBuildSystem::getStartupBuildSystem()
{
    return qobject_cast<QmlProjectManager::QmlBuildSystem *>(activeBuildSystemForActiveProject());
}

void QmlBuildSystem::addQmlProjectModule(const Utils::FilePath &path)
{
    m_projectItem->addQmlProjectModule(path.toFSPathString());
}

Utils::FilePath QmlBuildSystem::mainFilePath() const
{
    const QString fileName = mainFile();
    if (fileName.isEmpty() || fileName.isNull()) {
        return {};
    }
    return projectDirectory().pathAppended(fileName);
}

Utils::FilePath QmlBuildSystem::mainUiFilePath() const
{
    const QString fileName = mainUiFile();
    if (fileName.isEmpty() || fileName.isNull()) {
        return {};
    }
    return projectDirectory().pathAppended(fileName);
}

bool QmlBuildSystem::setMainFileInProjectFile(const Utils::FilePath &newMainFilePath)
{
    return setFileSettingInProjectFile("mainFile", newMainFilePath, mainFile());
}

bool QmlBuildSystem::setMainUiFileInProjectFile(const Utils::FilePath &newMainUiFilePath)
{
    return setMainUiFileInMainFile(newMainUiFilePath)
           && setFileSettingInProjectFile("mainUiFile", newMainUiFilePath, m_projectItem->mainUiFile());
}

bool QmlBuildSystem::setMainUiFileInMainFile(const Utils::FilePath &newMainUiFilePath)
{
    Core::FileChangeBlocker fileChangeBlocker(mainFilePath());
    const QList<Core::IEditor *> editors = Core::DocumentModel::editorsForFilePath(mainFilePath());
    TextEditor::TextDocument *document = nullptr;
    if (!editors.isEmpty()) {
        document = qobject_cast<TextEditor::TextDocument *>(editors.first()->document());
        if (document && document->isModified())
            if (!Core::DocumentManager::saveDocument(document))
                return false;
    }

    QString fileContent;
    TextFileFormat textFileFormat;
    const QTextCodec *codec = QTextCodec::codecForName("UTF-8"); // qml files are defined to be utf-8
    const TextFileFormat::ReadResult res =
            TextFileFormat::readFile(mainFilePath(), codec, &fileContent, &textFileFormat);
    if (res.code != TextFileFormat::ReadSuccess)
        qWarning() << "Failed to read file" << mainFilePath() << ":" << res.error;

    const QString currentMain = QString("%1 {").arg(mainUiFilePath().baseName());
    const QString newMain = QString("%1 {").arg(newMainUiFilePath.baseName());

    if (fileContent.contains(currentMain))
        fileContent.replace(currentMain, newMain);

    if (const Result<> res = textFileFormat.writeFile(mainFilePath(), fileContent); !res)
        qWarning() << "Failed to write file" << mainFilePath() << ":" << res.error();

    return true;
}

Utils::FilePath QmlBuildSystem::targetDirectory() const
{
    Utils::FilePath result;
    if (RunDeviceTypeKitAspect::deviceTypeId(kit()) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        result = canonicalProjectDir();
    } else if (IDevice::ConstPtr device = RunDeviceKitAspect::device(kit())) {
        if (m_projectItem)
            result = device->filePath(m_projectItem->targetDirectory());
    }
    return result;
}

Utils::FilePath QmlBuildSystem::targetFile(const Utils::FilePath &sourceFile) const
{
    const FilePath sourceDir = m_projectItem ? m_projectItem->sourceDirectory()
                                             : canonicalProjectDir();
    const FilePath relative = sourceFile.relativePathFromDir(sourceDir);
    return targetDirectory().resolvePath(relative);
}

void QmlBuildSystem::setSupportedLanguages(QStringList languages)
{
        m_projectItem->setSupportedLanguages(languages);
}

void QmlBuildSystem::setPrimaryLanguage(QString language)
{
        m_projectItem->setPrimaryLanguage(language);
}

bool QmlBuildSystem::enableCMakeGeneration() const
{
    return m_projectItem->enableCMakeGeneration();
}

void QmlBuildSystem::setEnableCMakeGeneration(bool enable)
{
    if (enable != enableCMakeGeneration())
        m_projectItem->setEnableCMakeGeneration(enable);
}

bool QmlBuildSystem::enablePythonGeneration() const
{
    return m_projectItem->enablePythonGeneration();
}

void QmlBuildSystem::setEnablePythonGeneration(bool enable)
{
    if (enable != enablePythonGeneration())
        m_projectItem->setEnablePythonGeneration(enable);
}

bool QmlBuildSystem::standaloneApp() const
{
    return m_projectItem->standaloneApp();
}

void QmlBuildSystem::setStandaloneApp(bool value)
{
    if (value != standaloneApp())
        m_projectItem->setStandaloneApp(value);
}

void QmlBuildSystem::refreshFiles(const QSet<QString> & /*added*/, const QSet<QString> &removed)
{
    if (m_blockFilesUpdate) {
        qCDebug(infoLogger) << "Auto files refresh blocked.";
        return;
    }
    refresh(RefreshOptions::Files);
    if (!removed.isEmpty()) {
        if (auto modelManager = QmlJS::ModelManagerInterface::instance()) {
            modelManager->removeFiles(
                        Utils::transform<QList<Utils::FilePath>>(removed, [](const QString &s) {
                return Utils::FilePath::fromString(s);
            }));
        }
    }
    updateDeploymentData();
}

QVariant QmlBuildSystem::additionalData(Utils::Id id) const
{
    if (id == Constants::customFileSelectorsData)
        return fileSelectors();
    if (id == Constants::supportedLanguagesData)
        return supportedLanguages();
    if (id == Constants::primaryLanguageData)
        return primaryLanguage();
    if (id == Constants::customForceFreeTypeData)
        return forceFreeType();
    if (id == Constants::customQtForMCUs)
        return qtForMCUs();
    if (id == Constants::customQt6Project)
        return qt6Project();
    if (id == Constants::mainFilePath)
        return mainFilePath().toUrlishString();
    if (id == Constants::canonicalProjectDir)
        return canonicalProjectDir().toUrlishString();
    return {};
}

bool QmlBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    if (dynamic_cast<Internal::QmlProjectNode *>(context)) {
        if (action == AddNewFile || action == EraseFile)
            return true;
        QTC_ASSERT(node, return false);

        if (action == Rename && node->asFileNode()) {
            const FileNode *fileNode = node->asFileNode();
            QTC_ASSERT(fileNode, return false);
            return fileNode->fileType() != FileType::Project;
        }
        return false;
    }
    return BuildSystem::supportsAction(context, action, node);
}

bool QmlBuildSystem::addFiles(Node *context, const Utils::FilePaths &filePaths, Utils::FilePaths *)
{
    if (!dynamic_cast<Internal::QmlProjectNode *>(context))
        return false;

    Utils::FilePaths toAdd;
    for (const Utils::FilePath &filePath : filePaths) {
        if (!m_projectItem->matchesFile(filePath.toUrlishString()))
            toAdd << filePaths;
    }
    return toAdd.isEmpty();
}

bool QmlBuildSystem::deleteFiles(Node *context, const Utils::FilePaths &filePaths)
{
    if (dynamic_cast<Internal::QmlProjectNode *>(context))
        return true;

    return BuildSystem::deleteFiles(context, filePaths);
}

bool QmlBuildSystem::renameFiles(Node *context,
                                 const Utils::FilePairs &filesToRename,
                                 Utils::FilePaths *notRenamed)
{
    if (!dynamic_cast<Internal::QmlProjectNode *>(context))
        return BuildSystem::renameFiles(context, filesToRename, notRenamed);

    bool success = true;
    for (const auto &[oldFilePath, newFilePath] : filesToRename) {
        const auto fail = [&, oldFilePath = oldFilePath] {
            success = false;
            if (notRenamed)
                *notRenamed << oldFilePath;
        };
        if (oldFilePath.endsWith(mainFile())) {
            if (!setMainFileInProjectFile(newFilePath))
                fail();
            continue;
        }
        if (oldFilePath.endsWith(m_projectItem->mainUiFile())) {
            if (!setMainUiFileInProjectFile(newFilePath))
                fail();
            continue;
        }

        // Why is this not an error?
    }

    return success;
}

QString QmlBuildSystem::mainFile() const
{
    return m_projectItem->mainFile();
}

QString QmlBuildSystem::mainUiFile() const
{
    return m_projectItem->mainUiFile();
}

bool QmlBuildSystem::qtForMCUs() const
{
    return m_projectItem->isQt4McuProject();
}

bool QmlBuildSystem::qt6Project() const
{
    return m_projectItem->versionQt() == "6";
}

Utils::EnvironmentItems QmlBuildSystem::environment() const
{
    return m_projectItem->environment();
}

QStringList QmlBuildSystem::fileSelectors() const
{
    return m_projectItem->fileSelectors();
}

bool QmlBuildSystem::multilanguageSupport() const
{
    return m_projectItem->multilanguageSupport();
}

QStringList QmlBuildSystem::supportedLanguages() const
{
    return m_projectItem->supportedLanguages();
}

QString QmlBuildSystem::primaryLanguage() const
{
    return m_projectItem->primaryLanguage();
}

bool QmlBuildSystem::forceFreeType() const
{
    return m_projectItem->forceFreeType();
}

bool QmlBuildSystem::widgetApp() const
{
    return m_projectItem->widgetApp();
}

QStringList QmlBuildSystem::shaderToolArgs() const
{
    return m_projectItem->shaderToolArgs();
}

QStringList QmlBuildSystem::shaderToolFiles() const
{
    return m_projectItem->shaderToolFiles();
}

QStringList QmlBuildSystem::allImports() const
{
    return m_projectItem->importPaths() + m_projectItem->mockImports();
}

QStringList QmlBuildSystem::importPaths() const
{
    return m_projectItem->importPaths();
}

void QmlBuildSystem::addImportPath(const Utils::FilePath &path)
{
    m_projectItem->addImportPath(path.toFSPathString());
}

QStringList QmlBuildSystem::mockImports() const
{
    return m_projectItem->mockImports();
}

QStringList QmlBuildSystem::absoluteImportPaths() const
{
    return Utils::transform<QStringList>(allImports(), [&](const QString &importPath) {
        Utils::FilePath filePath = Utils::FilePath::fromString(importPath);
        if (filePath.isAbsolutePath())
            return projectDirectory().resolvePath(importPath).path();
        return (projectDirectory() / importPath).path();
    });
}

QStringList QmlBuildSystem::targetImportPaths() const
{
    return Utils::transform<QStringList>(allImports(), [&](const QString &importPath) {
        const Utils::FilePath filePath = Utils::FilePath::fromString(importPath);
        if (filePath.isAbsolutePath()) {
            return importPath;
        }
        return (targetDirectory() / importPath).path();
    });
}

QString QmlBuildSystem::versionQt() const
{
    return m_projectItem->versionQt();
}

QString QmlBuildSystem::versionQtQuick() const
{
    return m_projectItem->versionQtQuick();
}

QString QmlBuildSystem::versionDesignStudio() const
{
    return m_projectItem->versionDesignStudio();
}

class QmlBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    QmlBuildConfigurationFactory()
    {
        registerBuildConfiguration<BuildConfiguration>("QmlBuildConfiguration");
        setSupportedProjectType(QmlProjectManager::Constants::QML_PROJECT_ID);
        setSupportedProjectMimeTypeName(Utils::Constants::QMLPROJECT_MIMETYPE);
        setBuildGenerator(
            [](const Kit *, const FilePath &projectPath, bool /* forSetup */) -> QList<BuildInfo> {
                BuildInfo bi;
                bi.buildDirectory = projectPath;
                bi.displayName = bi.typeName = Tr::tr("Default");
                bi.showBuildConfigs = bi.showBuildDirConfigWidget = false;
                return {bi};
            });
    }
};

void setupQmlBuildConfiguration()
{
    static const QmlBuildConfigurationFactory theQmlBuildConfigurationFactory;
}

} // namespace QmlProjectManager
