// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprojectnodes.h"

#include "cmakebuildsystem.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"

#include <android/androidconstants.h>

#include <coreplugin/documentmanager.h>

#include <ios/iosconstants.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>

#include <texteditor/texteditor.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

static QString quoteString(const QString &str)
{
    return str.contains(QChar::Space) ? ("\"" + str + "\"") : str;
}

static bool addSubdirectory(const Utils::FilePath &projectPathDir, const Utils::FilePath & subProjectFilePath)
{
    TextEditor::BaseTextEditor *editor = qobject_cast<TextEditor::BaseTextEditor *>(
        Core::EditorManager::openEditorAt(
            {projectPathDir.pathAppended(Constants::CMAKE_LISTS_TXT)},
            Constants::CMAKE_EDITOR_ID,
            Core::EditorManager::DoNotMakeVisible | Core::EditorManager::DoNotChangeCurrentEditor));
    if (!editor)
        return false;

    const QString subDirectory = subProjectFilePath.relativeChildPath(projectPathDir).parentDir().path();
    if (subDirectory.isEmpty())
        return false;

    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (!cursor.block().text().isEmpty())
        cursor.insertText("\n");
    cursor.insertText(QString("add_subdirectory(%1)").arg(quoteString(subDirectory)));

    if (!Core::DocumentManager::saveDocument(editor->document()))
        return false;

    return true;
}

CMakeInputsNode::CMakeInputsNode(const FilePath &cmakeLists) :
    ProjectExplorer::ProjectNode(cmakeLists)
{
    setPriority(Node::DefaultPriority - 10); // Bottom most!
    setDisplayName(Tr::tr("CMake Modules"));
    setIcon(DirectoryIcon(ProjectExplorer::Constants::FILEOVERLAY_MODULES));
    setListInProject(false);
}

CMakePresetsNode::CMakePresetsNode(const FilePath &projectPath) :
    ProjectExplorer::ProjectNode(projectPath)
{
    setPriority(Node::DefaultPriority - 9);
    setDisplayName(Tr::tr("CMake Presets"));
    setIcon(DirectoryIcon(ProjectExplorer::Constants::FILEOVERLAY_PRODUCT));
    setListInProject(false);
}

bool CMakeListsNode::hasSubprojectBuildSupport() const
{
    return m_hasSubprojectBuildSupport;
}

void CMakeListsNode::setHasSubprojectBuildSupport(bool hasSubprojectBuildSupport)
{
    m_hasSubprojectBuildSupport = hasSubprojectBuildSupport;
}

CMakeListsNode::CMakeListsNode(const FilePath &cmakeListPath) :
    ProjectExplorer::ProjectNode(cmakeListPath)
{
    setIcon(DirectoryIcon(Constants::Icons::FILE_OVERLAY));
    setListInProject(false);
    setLocationInfo(
        {{Constants::CMAKE_LISTS_TXT, cmakeListPath.pathAppended(Constants::CMAKE_LISTS_TXT)}});
}

bool CMakeListsNode::showInSimpleTree() const
{
    return false;
}

std::optional<FilePath> CMakeListsNode::visibleAfterAddFileAction() const
{
    return filePath().pathAppended(Constants::CMAKE_LISTS_TXT);
}

bool CMakeListsNode::canAddSubProject(const Utils::FilePath &subProjectFilePath) const
{
    return subProjectFilePath != filePath().pathAppended(Constants::CMAKE_LISTS_TXT)
           && subProjectFilePath.isChildOf(filePath());
}

bool CMakeListsNode::addSubProject(const Utils::FilePath &subProjectFilePath)
{
    return addSubdirectory(filePath(), subProjectFilePath);
}

QStringList CMakeListsNode::subProjectFileNamePatterns() const
{
    return {Constants::CMAKE_LISTS_TXT};
}

CMakeProjectNode::CMakeProjectNode(const FilePath &directory) :
    ProjectExplorer::ProjectNode(directory)
{
    setPriority(Node::DefaultProjectPriority + 1000);
    setIcon(DirectoryIcon(ProjectExplorer::Constants::FILEOVERLAY_PRODUCT));
    setListInProject(false);
}

bool CMakeProjectNode::canAddSubProject(const Utils::FilePath &subProjectFilePath) const
{
    return subProjectFilePath != filePath().pathAppended(Constants::CMAKE_LISTS_TXT)
           && subProjectFilePath.isChildOf(filePath());
}

bool CMakeProjectNode::addSubProject(const Utils::FilePath &subProjectFilePath)
{
    return addSubdirectory(filePath(), subProjectFilePath);
}

QStringList CMakeProjectNode::subProjectFileNamePatterns() const
{
    return {Constants::CMAKE_LISTS_TXT};
}

QString CMakeProjectNode::tooltip() const
{
    return QString();
}

CMakeTargetNode::CMakeTargetNode(const FilePath &directory, const QString &target) :
    ProjectExplorer::ProjectNode(directory)
{
    m_target = target;
    setPriority(Node::DefaultProjectPriority + 900);
    setIcon(":/projectexplorer/images/build.png"); // TODO: Use proper icon!
    setListInProject(false);
    setProductType(ProductType::Other);
}

QString CMakeTargetNode::tooltip() const
{
    return m_tooltip;
}

QString CMakeTargetNode::buildKey() const
{
    return m_target;
}

FilePath CMakeTargetNode::buildDirectory() const
{
    return m_buildDirectory;
}

void CMakeTargetNode::setBuildDirectory(const FilePath &directory)
{
    m_buildDirectory = directory;
}

QVariant CMakeTargetNode::data(Id role) const
{
    auto value = [this](const QByteArray &key) -> QVariant {
        for (const CMakeConfigItem &configItem : m_config) {
            if (configItem.key == key)
                return configItem.value;
        }
        return {};
    };

    auto values = [this](const QByteArray &key) -> QVariant {
        for (const CMakeConfigItem &configItem : m_config) {
            if (configItem.key == key)
                return configItem.values;
        }
        return {};
    };

    if (role == Constants::BUILD_FOLDER_ROLE)
        return m_buildDirectory.toVariant();

    if (role == Android::Constants::AndroidAbi)
        return value(Android::Constants::ANDROID_ABI);

    if (role == Android::Constants::AndroidAbis)
        return value(Android::Constants::ANDROID_ABIS);

    // TODO: Concerns the variables below. Qt 6 uses target properties which cannot be read
    // by the current mechanism, and the variables start with "Qt_" prefix.

    if (role == Android::Constants::AndroidPackageSourceDir)
        return value(Android::Constants::ANDROID_PACKAGE_SOURCE_DIR);

    if (role == Android::Constants::AndroidExtraLibs)
        return value(Android::Constants::ANDROID_EXTRA_LIBS);

    if (role == Android::Constants::AndroidDeploySettingsFile)
        return value(Android::Constants::ANDROID_DEPLOYMENT_SETTINGS_FILE);

    if (role == Android::Constants::AndroidApplicationArgs)
        return value(Android::Constants::ANDROID_APPLICATION_ARGUMENTS);

    if (role == Android::Constants::ANDROID_ABIS)
        return value(Android::Constants::ANDROID_ABIS);

    if (role == Android::Constants::AndroidSoLibPath)
        return values(Android::Constants::ANDROID_SO_LIBS_PATHS);

    if (role == Android::Constants::AndroidTargets)
        return values("TARGETS_BUILD_PATH");

    if (role == Android::Constants::AndroidApk)
        return {};

    if (role == Ios::Constants::IosTarget) {
        // For some reason the artifact is e.g. "Debug/untitled.app/untitled" which is wrong.
        // It actually is e.g. "Debug-iphonesimulator/untitled.app/untitled".
        // Anyway, the iOS plugin is only interested in the app bundle name without .app.
        return m_artifact.fileName();
    }

    if (role == Ios::Constants::IosBuildDir) {
        // This is a path relative to root build directory.
        // When generating Xcode project, CMake may put here a "${EFFECTIVE_PLATFORM_NAME}" macro,
        // which is expanded by Xcode at build time.
        // To get an actual executable path, iOS plugin replaces this macro with either "-iphoneos"
        // or "-iphonesimulator" depending on the device type (which is unavailable here).

        // dir/target.app/target -> dir
        return m_artifact.parentDir().parentDir().path();
    }

    if (role == Ios::Constants::IosCmakeGenerator)
        return value("CMAKE_GENERATOR");

    if (role == ProjectExplorer::Constants::QT_KEYWORDS_ENABLED) // FIXME handle correctly
        return value(role.toString().toUtf8());

    QTC_ASSERT(false, qDebug() << "Unknown role" << role.toString());
    // Better guess than "not present".
    return value(role.toString().toUtf8());
}

void CMakeTargetNode::setConfig(const CMakeConfig &config)
{
    m_config = config;
}

void CMakeTargetNode::setVisibleAfterAddFileAction(bool visibleAfterAddFileAction)
{
    m_visibleAfterAddFileAction = visibleAfterAddFileAction;
}

std::optional<FilePath> CMakeTargetNode::visibleAfterAddFileAction() const
{
    if (m_visibleAfterAddFileAction)
        return filePath().pathAppended(Constants::CMAKE_LISTS_TXT);
    return std::nullopt;
}

void CMakeTargetNode::build()
{
    Project *p = getProject();
    Target *t = p ? p->activeTarget() : nullptr;
    if (t)
        static_cast<CMakeBuildSystem *>(t->buildSystem())->buildCMakeTarget(displayName());
}

void CMakeTargetNode::setTargetInformation(const QList<FilePath> &artifacts, const QString &type)
{
    m_tooltip = Tr::tr("Target type:") + " " + type + "<br>";
    if (artifacts.isEmpty()) {
        m_tooltip += Tr::tr("No build artifacts");
    } else {
        const QStringList tmp = Utils::transform(artifacts, &FilePath::toUserOutput);
        m_tooltip += Tr::tr("Build artifacts:") + "<br>" + tmp.join("<br>");
        m_artifact = artifacts.first();
    }
    if (type == "EXECUTABLE")
        setProductType(ProductType::App);
    else if (type == "SHARED_LIBRARY" || type == "STATIC_LIBRARY")
        setProductType(ProductType::Lib);
}

} // CMakeProjectManager::Internal
