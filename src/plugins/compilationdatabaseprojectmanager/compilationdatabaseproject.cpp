/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "compilationdatabaseproject.h"

#include "compilationdatabaseconstants.h"
#include "compilationdatabaseutils.h"

#include <coreplugin/icontext.h>
#include <cpptools/projectinfo.h>
#include <cpptools/cppkitinfo.h>
#include <cpptools/cppprojectupdater.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainconfigwidget.h>
#include <projectexplorer/toolchainmanager.h>
#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

using namespace ProjectExplorer;

namespace CompilationDatabaseProjectManager {
namespace Internal {

namespace {
class DBProjectNode : public ProjectNode
{
public:
    explicit DBProjectNode(const Utils::FileName &projectFilePath)
        : ProjectNode(projectFilePath)
    {}
};

QStringList jsonObjectFlags(const QJsonObject &object)
{
    QStringList flags;
    const QJsonArray arguments = object["arguments"].toArray();
    if (arguments.isEmpty()) {
        flags = splitCommandLine(object["command"].toString());
    } else {
        flags.reserve(arguments.size());
        for (const QJsonValue &arg : arguments)
            flags.append(arg.toString());
    }

    return flags;
}

bool isGccCompiler(const QString &compilerName)
{
    return compilerName.contains("gcc") || compilerName.contains("g++");
}

Core::Id getCompilerId(QString compilerName)
{
    if (Utils::HostOsInfo::isWindowsHost()) {
        if (compilerName.endsWith(".exe"))
            compilerName.chop(4);
        if (isGccCompiler(compilerName))
            return ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID;

        // Default is clang-cl
        return ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID;
    }
    if (isGccCompiler(compilerName))
        return ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID;

    // Default is clang
    return ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID;
}

ToolChain *toolchainFromCompilerId(const Core::Id &compilerId, const Core::Id &language)
{
    return ToolChainManager::toolChain([&compilerId, &language](const ToolChain *tc) {
        if (!tc->isValid() || tc->language() != language)
            return false;
        return tc->typeId() == compilerId;
    });
}

QString compilerPath(QString pathFlag)
{
    if (pathFlag.isEmpty())
        return pathFlag;
#ifdef Q_OS_WIN
    // Handle short DOS style file names (cmake can generate them).
    const DWORD pathLength = GetLongPathNameW((LPCWSTR)pathFlag.utf16(), 0, 0);
    wchar_t* buffer = new wchar_t[pathLength];
    GetLongPathNameW((LPCWSTR)pathFlag.utf16(), buffer, pathLength);
    pathFlag = QString::fromUtf16((ushort *)buffer, pathLength - 1);
    delete[] buffer;
#endif
    return QDir::fromNativeSeparators(pathFlag);
}

ToolChain *toolchainFromFlags(const Kit *kit, const QStringList &flags, const Core::Id &language)
{
    if (flags.empty())
        return ToolChainKitInformation::toolChain(kit, language);

    // Try exact compiler match.
    const Utils::FileName compiler = Utils::FileName::fromString(compilerPath(flags.front()));
    ToolChain *toolchain = ToolChainManager::toolChain([&compiler, &language](const ToolChain *tc) {
        return tc->isValid() && tc->language() == language && tc->compilerCommand() == compiler;
    });
    if (toolchain)
        return toolchain;

    Core::Id compilerId = getCompilerId(compiler.fileName());
    if ((toolchain = toolchainFromCompilerId(compilerId, language)))
        return toolchain;

    if (compilerId != ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID &&
            compilerId != ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
        compilerId = Utils::HostOsInfo::isWindowsHost()
                ? ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID
                : ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID;
        if ((toolchain = toolchainFromCompilerId(compilerId, language)))
            return toolchain;
    }

    toolchain = ToolChainKitInformation::toolChain(kit, language);
    qWarning() << "No matching toolchain found, use the default.";
    return toolchain;
}

Utils::FileName jsonObjectFilename(const QJsonObject &object)
{
    const QString workingDir = QDir::fromNativeSeparators(object["directory"].toString());
    Utils::FileName fileName = Utils::FileName::fromString(
                QDir::fromNativeSeparators(object["file"].toString()));
    if (fileName.toFileInfo().isRelative()) {
        fileName = Utils::FileUtils::canonicalPath(
                    Utils::FileName::fromString(workingDir + "/" + fileName.toString()));
    }
    return fileName;
}

void addDriverModeFlagIfNeeded(const ToolChain *toolchain, QStringList &flags)
{
    if (toolchain->typeId() == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID
            && !flags.empty() && !flags.front().endsWith("cl")
            && !flags.front().endsWith("cl.exe")) {
        flags.prepend("--driver-mode=g++");
    }
}

CppTools::RawProjectPart makeRawProjectPart(const Utils::FileName &projectFile,
                                            Kit *kit,
                                            ToolChain *&cToolchain,
                                            ToolChain *&cxxToolchain,
                                            const QString &workingDir,
                                            const Utils::FileName &fileName,
                                            QStringList flags)
{
    HeaderPaths headerPaths;
    Macros macros;
    CppTools::ProjectFile::Kind fileKind = CppTools::ProjectFile::Unclassified;

    const QStringList originalFlags = flags;
    filteredFlags(fileName.fileName(),
                  workingDir,
                  flags,
                  headerPaths,
                  macros,
                  fileKind);

    CppTools::RawProjectPart rpp;
    rpp.setProjectFileLocation(projectFile.toString());
    rpp.setBuildSystemTarget(workingDir);
    rpp.setDisplayName(fileName.fileName());
    rpp.setFiles({fileName.toString()});
    rpp.setHeaderPaths(headerPaths);
    rpp.setMacros(macros);

    if (fileKind == CppTools::ProjectFile::Kind::CHeader
            || fileKind == CppTools::ProjectFile::Kind::CSource) {
        if (!cToolchain) {
            cToolchain = toolchainFromFlags(kit, originalFlags,
                                            ProjectExplorer::Constants::C_LANGUAGE_ID);
            ToolChainKitInformation::setToolChain(kit, cToolchain);
        }
        addDriverModeFlagIfNeeded(cToolchain, flags);
        rpp.setFlagsForC({cToolchain, flags});
    } else {
        if (!cxxToolchain) {
            cxxToolchain = toolchainFromFlags(kit, originalFlags,
                                              ProjectExplorer::Constants::CXX_LANGUAGE_ID);
            ToolChainKitInformation::setToolChain(kit, cxxToolchain);
        }
        addDriverModeFlagIfNeeded(cxxToolchain, flags);
        rpp.setFlagsForCxx({cxxToolchain, flags});
    }

    return rpp;
}

QStringList relativeDirsList(Utils::FileName currentPath, const Utils::FileName &rootPath)
{
    QStringList dirsList;
    while (!currentPath.isEmpty() && currentPath != rootPath) {
        QString dirName = currentPath.fileName();
        if (dirName.isEmpty())
            dirName = currentPath.toString();
        dirsList.prepend(dirName);
        currentPath = currentPath.parentDir();
    }
    return dirsList;
}

FolderNode *addChildFolderNode(FolderNode *parent, const QString &childName)
{
    Utils::FileName parentPath = parent->filePath();
    auto node = std::make_unique<FolderNode>(
                parentPath.appendPath(childName), NodeType::Folder, childName);
    FolderNode *childNode = node.get();
    parent->addNode(std::move(node));

    return childNode;
}

FolderNode *addOrGetChildFolderNode(FolderNode *parent, const QString &childName)
{
    for (FolderNode *folder : parent->folderNodes()) {
        if (folder->filePath().fileName() == childName) {
            return folder;
        }
    }

    return addChildFolderNode(parent, childName);
}

    // Return the node for folderPath.
FolderNode *createFoldersIfNeeded(FolderNode *root, const Utils::FileName &folderPath)
{
    const QStringList dirsList = relativeDirsList(folderPath, root->filePath());

    FolderNode *parent = root;
    for (const QString &dir : dirsList)
        parent = addOrGetChildFolderNode(parent, dir);

    return parent;
}

FileType fileTypeForName(const QString &fileName)
{
    CppTools::ProjectFile::Kind fileKind = CppTools::ProjectFile::classify(fileName);
    if (CppTools::ProjectFile::isHeader(fileKind))
        return FileType::Header;
    return FileType::Source;
}

void createTree(FolderNode *root,
                const Utils::FileName &rootPath,
                const CppTools::RawProjectParts &rpps)
{
    root->setAbsoluteFilePathAndLine(rootPath, -1);

    for (const CppTools::RawProjectPart &rpp : rpps) {
        for (const QString &filePath : rpp.files) {
            Utils::FileName fileName = Utils::FileName::fromString(filePath);
            FolderNode *parentNode = createFoldersIfNeeded(root, fileName.parentDir());
            if (!parentNode->fileNode(fileName)) {
                parentNode->addNode(std::make_unique<FileNode>(fileName,
                                                               fileTypeForName(fileName.fileName()),
                                                               false));
            }
        }
    }
}

struct Entry
{
    QStringList flags;
    Utils::FileName fileName;
    QString workingDir;
};

std::vector<Entry> readJsonObjects(const QString &filePath)
{
    std::vector<Entry> result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return result;

    const QByteArray contents = file.readAll();
    int objectStart = contents.indexOf('{');
    int objectEnd = contents.indexOf('}', objectStart + 1);

    while (objectStart >= 0 && objectEnd >= 0) {
        const QJsonDocument document = QJsonDocument::fromJson(
                    contents.mid(objectStart, objectEnd - objectStart + 1));
        if (document.isNull()) {
            // The end was found incorrectly, search for the next one.
            objectEnd = contents.indexOf('}', objectEnd + 1);
            continue;
        }

        const QJsonObject object = document.object();
        const Utils::FileName fileName = jsonObjectFilename(object);
        const QStringList flags
                = filterFromFileName(jsonObjectFlags(object), fileName.toFileInfo().baseName());
        result.push_back({flags, fileName, object["directory"].toString()});

        objectStart = contents.indexOf('{', objectEnd + 1);
        objectEnd = contents.indexOf('}', objectStart + 1);
    }

    return result;
}

} // anonymous namespace

void CompilationDatabaseProject::buildTreeAndProjectParts(const Utils::FileName &projectFile)
{
    std::vector<Entry> array = readJsonObjects(projectFilePath().toString());
    if (array.empty()) {
        emitParsingFinished(false);
        return;
    }

    auto root = std::make_unique<DBProjectNode>(projectDirectory());

    CppTools::KitInfo kitInfo(this);
    QTC_ASSERT(kitInfo.isValid(), return);
    CppTools::RawProjectParts rpps;
    Utils::FileName commonPath;

    std::sort(array.begin(), array.end(), [](const Entry &lhs, const Entry &rhs) {
        return std::lexicographical_compare(lhs.flags.begin(), lhs.flags.end(),
                                            rhs.flags.begin(), rhs.flags.end());
    });

    const Entry *prevEntry = nullptr;
    for (const Entry &entry : array) {
        if (prevEntry && prevEntry->flags == entry.flags) {
            rpps.back().files.append(entry.fileName.toString());
            continue;
        }

        prevEntry = &entry;

        commonPath = rpps.empty()
                ? entry.fileName.parentDir()
                : Utils::FileUtils::commonPath(commonPath, entry.fileName);

        CppTools::RawProjectPart rpp = makeRawProjectPart(projectFile,
                                                          m_kit.get(),
                                                          kitInfo.cToolChain,
                                                          kitInfo.cxxToolChain,
                                                          entry.workingDir,
                                                          entry.fileName,
                                                          entry.flags);

        rpps.append(rpp);
    }

    createTree(root.get(), commonPath, rpps);

    root->addNode(std::make_unique<FileNode>(
                      projectFile,
                      FileType::Project,
                      false));

    setRootProjectNode(std::move(root));

    m_cppCodeModelUpdater->update({this, kitInfo, rpps});

    emitParsingFinished(true);
}

CompilationDatabaseProject::CompilationDatabaseProject(const Utils::FileName &projectFile)
    : Project(Constants::COMPILATIONDATABASEMIMETYPE, projectFile)
    , m_cppCodeModelUpdater(std::make_unique<CppTools::CppProjectUpdater>())
{
    setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setRequiredKitPredicate([](const Kit *) { return false; });
    setPreferredKitPredicate([](const Kit *) { return false; });

    m_kit.reset(KitManager::defaultKit()->clone());

    connect(this, &CompilationDatabaseProject::parsingFinished, this, [this]() {
        if (!m_hasTarget) {
            addTarget(createTarget(m_kit.get()));
            m_hasTarget = true;
        }
    });

    reparseProject(projectFile);

    m_fileSystemWatcher.addFile(projectFile.toString(), Utils::FileSystemWatcher::WatchModifiedDate);
    connect(&m_fileSystemWatcher,
            &Utils::FileSystemWatcher::fileChanged,
            this,
            [this](const QString &projectFile) {
                reparseProject(Utils::FileName::fromString(projectFile));
            });
}

void CompilationDatabaseProject::reparseProject(const Utils::FileName &projectFile)
{
    emitParsingStarted();
    const QFuture<void> future = ::Utils::runAsync(
        [this, projectFile]() { buildTreeAndProjectParts(projectFile); });
    m_parserWatcher.setFuture(future);
}

CompilationDatabaseProject::~CompilationDatabaseProject()
{
    m_parserWatcher.cancel();
    m_parserWatcher.waitForFinished();
}

static TextEditor::TextDocument *createCompilationDatabaseDocument()
{
    auto doc = new TextEditor::TextDocument;
    doc->setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    doc->setMimeType(Constants::COMPILATIONDATABASEMIMETYPE);
    return doc;
}

CompilationDatabaseEditorFactory::CompilationDatabaseEditorFactory()
{
    setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    setDisplayName("Compilation Database");
    addMimeType(Constants::COMPILATIONDATABASEMIMETYPE);

    setEditorCreator([]() { return new TextEditor::BaseTextEditor; });
    setEditorWidgetCreator([]() { return new TextEditor::TextEditorWidget; });
    setDocumentCreator(createCompilationDatabaseDocument);
    setUseGenericHighlighter(true);
    setCommentDefinition(Utils::CommentDefinition::HashStyle);
    setCodeFoldingSupported(true);
}

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
