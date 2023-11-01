// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakenodetreebuilder.h"

#include "qmakeproject.h"
#include "qmakeprojectmanagertr.h"

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <resourceeditor/resourcenode.h>

#include <utils/algorithm.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

using namespace QmakeProjectManager::Internal;

namespace {

// Static cached data in struct QmakeStaticData providing information and icons
// for file types and the project. Do some magic via qAddPostRoutine()
// to make sure the icons do not outlive QApplication, triggering warnings on X11.

class FileTypeDataStorage {
public:
    FileType type;
    const char *typeName;
    const char *icon;
    const char *addFileFilter;
};

const FileTypeDataStorage fileTypeDataStorage[] = {
    { FileType::Header, QT_TRANSLATE_NOOP("QtC::QmakeProjectManager", "Headers"),
      ProjectExplorer::Constants::FILEOVERLAY_H, "*.h; *.hh; *.hpp; *.hxx;"},
    { FileType::Source, QT_TRANSLATE_NOOP("QtC::QmakeProjectManager", "Sources"),
      ProjectExplorer::Constants::FILEOVERLAY_CPP, "*.c; *.cc; *.cpp; *.cp; *.cxx; *.c++;" },
    { FileType::Form, QT_TRANSLATE_NOOP("QtC::QmakeProjectManager", "Forms"),
      ProjectExplorer::Constants::FILEOVERLAY_UI, "*.ui;" },
    { FileType::StateChart, QT_TRANSLATE_NOOP("QtC::QmakeProjectManager", "State charts"),
      ProjectExplorer::Constants::FILEOVERLAY_SCXML, "*.scxml;" },
    { FileType::Resource, QT_TRANSLATE_NOOP("QtC::QmakeProjectManager", "Resources"),
      ProjectExplorer::Constants::FILEOVERLAY_QRC, "*.qrc;" },
    { FileType::QML, QT_TRANSLATE_NOOP("QtC::QmakeProjectManager", "QML"),
      ProjectExplorer::Constants::FILEOVERLAY_QML, "*.qml;" },
    { FileType::Unknown, QT_TRANSLATE_NOOP("QtC::QmakeProjectManager", "Other files"),
      ProjectExplorer::Constants::FILEOVERLAY_UNKNOWN, "*;" }
};

class QmakeStaticData {
public:
    class FileTypeData {
    public:
        FileTypeData(FileType t = FileType::Unknown,
                     const QString &tN = QString(),
                     const QString &aff = QString(),
                     const QIcon &i = QIcon()) :
        type(t), typeName(tN), addFileFilter(aff), icon(i) { }

        FileType type;
        QString typeName;
        QString addFileFilter;
        QIcon icon;
    };

    QmakeStaticData();

    QVector<FileTypeData> fileTypeData;
    QIcon projectIcon;
    QIcon productIcon;
    QIcon groupIcon;
};

void clearQmakeStaticData();

QmakeStaticData::QmakeStaticData()
{
    // File type data
    const unsigned count = sizeof(fileTypeDataStorage)/sizeof(FileTypeDataStorage);
    fileTypeData.reserve(count);

    for (const FileTypeDataStorage &fileType : fileTypeDataStorage) {
        const QString desc = QmakeProjectManager::Tr::tr(fileType.typeName);
        const QString filter = QString::fromUtf8(fileType.addFileFilter);
        fileTypeData.push_back(QmakeStaticData::FileTypeData(fileType.type, desc, filter,
                               FileIconProvider::directoryIcon(QLatin1String(fileType.icon))));
    }
    // Project icon
    projectIcon = FileIconProvider::directoryIcon(ProjectExplorer::Constants::FILEOVERLAY_QT);
    productIcon = FileIconProvider::directoryIcon(ProjectExplorer::Constants::FILEOVERLAY_PRODUCT);
    groupIcon = FileIconProvider::directoryIcon(ProjectExplorer::Constants::FILEOVERLAY_GROUP);

    qAddPostRoutine(clearQmakeStaticData);
}

Q_GLOBAL_STATIC(QmakeStaticData, qmakeStaticData)

void clearQmakeStaticData()
{
    qmakeStaticData()->fileTypeData.clear();
    qmakeStaticData()->projectIcon = QIcon();
    qmakeStaticData()->productIcon = QIcon();
    qmakeStaticData()->groupIcon = QIcon();
}

} // namespace

namespace QmakeProjectManager {

static QIcon iconForProfile(const QmakeProFile *proFile)
{
    return proFile->projectType() == ProjectType::SubDirsTemplate ? qmakeStaticData()->projectIcon
                                                                  : qmakeStaticData()->productIcon;
}

static void createTree(QmakeBuildSystem *buildSystem,
                       const QmakePriFile *pri,
                       QmakePriFileNode *node,
                       const FilePaths &toExclude)
{
    QTC_ASSERT(pri, return);
    QTC_ASSERT(node, return);

    node->setDisplayName(pri->displayName());

    // .pro/.pri-file itself:
    node->addNode(std::make_unique<FileNode>(pri->filePath(), FileType::Project));

    // other normal files:
    const QVector<QmakeStaticData::FileTypeData> &fileTypes = qmakeStaticData()->fileTypeData;
    FilePaths generatedFiles;
    const auto proFile = dynamic_cast<const QmakeProFile *>(pri);
    for (int i = 0; i < fileTypes.size(); ++i) {
        FileType type = fileTypes.at(i).type;
        const SourceFiles &newFilePaths = Utils::filtered(pri->files(type), [&toExclude](const SourceFile &fn) {
            return !Utils::contains(toExclude, [&fn](const FilePath &ex) { return fn.first.isChildOf(ex); });
        });
        if (proFile) {
            for (const SourceFile &fp : newFilePaths) {
                for (const ExtraCompiler *ec : proFile->extraCompilers()) {
                    if (ec->source() == fp.first)
                        generatedFiles << ec->targets();
                }
            }
        }

        if (!newFilePaths.isEmpty()) {
            auto vfolder = std::make_unique<VirtualFolderNode>(pri->filePath().parentDir());
            vfolder->setPriority(Node::DefaultVirtualFolderPriority - i);
            vfolder->setIcon(fileTypes.at(i).icon);
            vfolder->setDisplayName(fileTypes.at(i).typeName);
            vfolder->setAddFileFilter(fileTypes.at(i).addFileFilter);
            vfolder->setIsSourcesOrHeaders(type == FileType::Source || type == FileType::Header
                                           || type == FileType::Form);

            if (type == FileType::Resource) {
                for (const SourceFile &file : newFilePaths) {
                    auto vfs = buildSystem->qmakeVfs();
                    QString contents;
                    QString errorMessage;
                    // Prefer the cumulative file if it's non-empty, based on the assumption
                    // that it contains more "stuff".
                    int cid = vfs->idForFileName(file.first.toFSPathString(), QMakeVfs::VfsCumulative);
                    vfs->readFile(cid, &contents, &errorMessage);
                    // If the cumulative evaluation botched the file too much, try the exact one.
                    if (contents.isEmpty()) {
                        int eid = vfs->idForFileName(file.first.toFSPathString(), QMakeVfs::VfsExact);
                        vfs->readFile(eid, &contents, &errorMessage);
                    }
                    auto topLevel = std::make_unique<ResourceEditor::ResourceTopLevelNode>
                                     (file.first, vfolder->filePath(), contents);
                    topLevel->setEnabled(file.second == FileOrigin::ExactParse);
                    const QString baseName = file.first.completeBaseName();
                    topLevel->setIsGenerated(baseName.startsWith("qmake_")
                            || baseName.endsWith("_qmlcache"));
                    vfolder->addNode(std::move(topLevel));
                }
            } else {
                for (const auto &fn : newFilePaths) {
                    // Qmake will flag everything in SOURCES as source, even when the
                    // qt quick compiler moves qrc files into it:-/ Get better data based on
                    // the filename.
                    type = FileNode::fileTypeForFileName(fn.first);
                    auto fileNode = std::make_unique<FileNode>(fn.first, type);
                    fileNode->setEnabled(fn.second == FileOrigin::ExactParse);
                    vfolder->addNestedNode(std::move(fileNode));
                }
            }
            node->addNode(std::move(vfolder));
        }
    }

    FileType targetFileType = FileType::Unknown;
    FilePath targetBinary;
    if (proFile && proFile->targetInformation().valid) {
        if (proFile->projectType() == ProjectType::ApplicationTemplate) {
            targetFileType = FileType::App;
            targetBinary = buildSystem->executableFor(proFile);
        } else if (proFile->projectType() == ProjectType::SharedLibraryTemplate
                   || proFile->projectType() == ProjectType::StaticLibraryTemplate) {
            targetFileType = FileType::Lib;
            const FilePaths libs = Utils::sorted(buildSystem->allLibraryTargetFiles(proFile),
                                                 [](const FilePath &fp1, const FilePath &fp2) {
                return fp1.fileName().length() < fp2.fileName().length(); });
            if (!libs.isEmpty())
                targetBinary = libs.last(); // Longest file name is the one that's not a symlink.
        }
    }
    if (!generatedFiles.empty() || !targetBinary.isEmpty()) {
        QTC_CHECK(proFile);
        const FilePath baseDir = generatedFiles.size() == 1 ? generatedFiles.first().parentDir()
                                                            : buildSystem->buildDir(proFile->filePath());
        auto genFolder = std::make_unique<VirtualFolderNode>(baseDir);
        genFolder->setDisplayName(Tr::tr("Generated Files"));
        genFolder->setIsGenerated(true);
        for (const FilePath &fp : std::as_const(generatedFiles)) {
            auto fileNode = std::make_unique<FileNode>(fp, FileNode::fileTypeForFileName(fp));
            fileNode->setIsGenerated(true);
            genFolder->addNestedNode(std::move(fileNode));
        }
        if (!targetBinary.isEmpty()) {
            auto targetFileNode = std::make_unique<FileNode>(targetBinary, targetFileType);
            targetFileNode->setIsGenerated(true);
            genFolder->addNestedNode(std::move(targetFileNode));
        }
        node->addNode(std::move(genFolder));
    }

    // Virtual folders:
    for (QmakePriFile *c : pri->children()) {
        std::unique_ptr<QmakePriFileNode> newNode;
        if (auto pf = dynamic_cast<QmakeProFile *>(c)) {
            newNode = std::make_unique<QmakeProFileNode>(c->buildSystem(), c->filePath(), pf);
            newNode->setIcon(iconForProfile(pf));
        } else {
            newNode = std::make_unique<QmakePriFileNode>(c->buildSystem(), node->proFileNode(),
                                                         c->filePath(), c);
            newNode->setIcon(qmakeStaticData->groupIcon);
        }
        createTree(buildSystem, c, newNode.get(), toExclude);
        node->addNode(std::move(newNode));
    }
}

std::unique_ptr<QmakeProFileNode> QmakeNodeTreeBuilder::buildTree(QmakeBuildSystem *buildSystem)
{
    // Remove qmake implementation details that litter up the project data:
    QtVersion *qt = QtKitAspect::qtVersion(buildSystem->kit());

    const FilePaths toExclude = qt ? qt->directoriesToIgnoreInProjectTree() : FilePaths();

    auto root = std::make_unique<QmakeProFileNode>(buildSystem,
                                                   buildSystem->projectFilePath(),
                                                   buildSystem->rootProFile());
    root->setIcon(iconForProfile(buildSystem->rootProFile()));
    createTree(buildSystem, buildSystem->rootProFile(), root.get(), toExclude);
    root->compress();

    return root;
}

} // namespace QmakeProjectManager
