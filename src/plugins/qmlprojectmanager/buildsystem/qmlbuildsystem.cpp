// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlbuildsystem.h"

#include "../qmlprojectconstants.h"
#include "../qmlprojectmanagertr.h"
#include "../qmlproject.h"

#include <QtCore5Compat/qtextcodec.h>
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

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include "projectitem/qmlprojectitem.h"
#include "projectnode/qmlprojectnodes.h"

#include "utils/algorithm.h"
#include "utils/qtcassert.h"

#include "texteditor/textdocument.h"

#include <QAction>

using namespace ProjectExplorer;
namespace QmlProjectManager {

namespace {
Q_LOGGING_CATEGORY(infoLogger, "QmlProjectManager.QmlBuildSystem", QtInfoMsg)
}

ExtensionSystem::IPlugin *findMcuSupportPlugin()
{
    const ExtensionSystem::PluginSpec *pluginSpec = Utils::findOrDefault(
        ExtensionSystem::PluginManager::plugins(),
        Utils::equal(&ExtensionSystem::PluginSpec::name, QString("McuSupport")));

    if (pluginSpec)
        return pluginSpec->plugin();
    return nullptr;
}

void updateMcuBuildStep(Target *target, bool mcuEnabled)
{
    if (auto plugin = findMcuSupportPlugin()) {
        QMetaObject::invokeMethod(
            plugin,
            "updateDeployStep",
            Qt::DirectConnection,
            Q_ARG(ProjectExplorer::Target*, target),
            Q_ARG(bool, mcuEnabled));
    } else if (mcuEnabled) {
        qWarning() << "Failed to find McuSupport plugin but qtForMCUs is enabled in the project";
    }
}

QmlBuildSystem::QmlBuildSystem(Target *target)
    : BuildSystem(target)
{
    // refresh first - project information is used e.g. to decide the default RC's
    refresh(RefreshOptions::Project);

    updateDeploymentData();
//    registerMenuButtons(); //is wip

    connect(target->project(), &Project::activeTargetChanged, this, [this](Target *target) {
        refresh(RefreshOptions::NoFileRefresh);
        updateMcuBuildStep(target, qtForMCUs());
    });
    connect(target->project(), &Project::projectFileIsDirty, this, [this]() {
        refresh(RefreshOptions::Project);
        updateMcuBuildStep(project()->activeTarget(), qtForMCUs());
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

    if (DeviceTypeKitAspect::deviceTypeId(kit())
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
    qDebug() << "debug#1-mainfilepath" << mainFilePath();

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
    m_projectItem.reset(new QmlProjectItem{projectFilePath()});
    connect(m_projectItem.get(),
            &QmlProjectItem::qmlFilesChanged,
            this,
            &QmlBuildSystem::refreshFiles);
}

void QmlBuildSystem::parseProjectFiles()
{
    if (auto modelManager = QmlJS::ModelManagerInterface::instance()) {
        modelManager->updateSourceFiles(m_projectItem->files(), true);
    }


    Utils::FilePath mainFilePath{Utils::FilePath::fromString(m_projectItem->mainFile())};
    if (!mainFilePath.isEmpty()) {
        mainFilePath = canonicalProjectDir().resolvePath(m_projectItem->mainFile());
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
    QString error;
    Utils::TextFileFormat textFileFormat;
    const QTextCodec *codec = QTextCodec::codecForName("UTF-8"); // qml files are defined to be utf-8
    Utils::TextFileFormat::ReadResult readResult = Utils::TextFileFormat::readFile(qmlProjectFilePath,
                                                                                   codec,
                                                                                   &fileContent,
                                                                                   &textFileFormat,
                                                                                   &error);
    if (readResult != Utils::TextFileFormat::ReadSuccess) {
        qWarning() << "Failed to read file" << qmlProjectFilePath << ":" << error;
    }

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

    if (!textFileFormat.writeFile(qmlProjectFilePath, fileContent, &error))
        qWarning() << "Failed to write file" << qmlProjectFilePath << ":" << error;

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
    QString error;
    Utils::TextFileFormat textFileFormat;
    const QTextCodec *codec = QTextCodec::codecForName("UTF-8"); // qml files are defined to be utf-8
    if (Utils::TextFileFormat::readFile(mainFilePath(), codec, &fileContent, &textFileFormat, &error)
            != Utils::TextFileFormat::ReadSuccess) {
        qWarning() << "Failed to read file" << mainFilePath() << ":" << error;
    }

    const QString currentMain = QString("%1 {").arg(mainUiFilePath().baseName());
    const QString newMain = QString("%1 {").arg(newMainUiFilePath.baseName());

    if (fileContent.contains(currentMain))
        fileContent.replace(currentMain, newMain);

    if (!textFileFormat.writeFile(mainFilePath(), fileContent, &error))
        qWarning() << "Failed to write file" << mainFilePath() << ":" << error;

    return true;
}

Utils::FilePath QmlBuildSystem::targetDirectory() const
{
    Utils::FilePath result;
    if (DeviceTypeKitAspect::deviceTypeId(kit()) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        result = canonicalProjectDir();
    } else if (IDevice::ConstPtr device = DeviceKitAspect::device(kit())) {
        if (m_projectItem)
            result = device->filePath(m_projectItem->targetDirectory());
    }
    return result;
}

Utils::FilePath QmlBuildSystem::targetFile(const Utils::FilePath &sourceFile) const
{
    const Utils::FilePath sourceDir = m_projectItem ? m_projectItem->sourceDirectory()
                                             : canonicalProjectDir();
    const Utils::FilePath relative = sourceFile.relativePathFrom(sourceDir);
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
        return customFileSelectors();
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
        return mainFilePath().toString();
    if (id == Constants::customImportPaths)
        return customImportPaths();
    if (id == Constants::canonicalProjectDir)
        return canonicalProjectDir().toString();
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
        if (!m_projectItem->matchesFile(filePath.toString()))
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

bool QmlBuildSystem::renameFile(Node *context,
                                const Utils::FilePath &oldFilePath,
                                const Utils::FilePath &newFilePath)
{
    if (dynamic_cast<Internal::QmlProjectNode *>(context)) {
        if (oldFilePath.endsWith(mainFile()))
            return setMainFileInProjectFile(newFilePath);
        if (oldFilePath.endsWith(m_projectItem->mainUiFile()))
            return setMainUiFileInProjectFile(newFilePath);
        return true;
    }

    return BuildSystem::renameFile(context, oldFilePath, newFilePath);
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

QStringList QmlBuildSystem::customImportPaths() const
{
    return m_projectItem->importPaths();
}

QStringList QmlBuildSystem::customFileSelectors() const
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

QStringList QmlBuildSystem::importPaths() const
{
    return m_projectItem->importPaths();
}

QStringList QmlBuildSystem::absoluteImportPaths()
{
    return Utils::transform<QStringList>(m_projectItem->importPaths(), [&](const QString &importPath) {
        Utils::FilePath filePath = Utils::FilePath::fromString(importPath);
        if (!filePath.isAbsolutePath())
            return (projectDirectory() / importPath).toString();
        return projectDirectory().resolvePath(importPath).toString();
    });
}

Utils::FilePaths QmlBuildSystem::files() const
{
    return m_projectItem->files();
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

} // namespace QmlProjectManager
