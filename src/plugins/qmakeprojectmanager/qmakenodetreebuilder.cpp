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

#include "qmakenodetreebuilder.h"

#include "qmakeproject.h"

#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <resourceeditor/resourcenode.h>

#include <utils/algorithm.h>
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
    { FileType::Header, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFile", "Headers"),
      ProjectExplorer::Constants::FILEOVERLAY_H, "*.h; *.hh; *.hpp; *.hxx;"},
    { FileType::Source, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFile", "Sources"),
      ProjectExplorer::Constants::FILEOVERLAY_CPP, "*.c; *.cc; *.cpp; *.cp; *.cxx; *.c++;" },
    { FileType::Form, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFile", "Forms"),
      ProjectExplorer::Constants::FILEOVERLAY_UI, "*.ui;" },
    { FileType::StateChart, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFile", "State charts"),
      ProjectExplorer::Constants::FILEOVERLAY_SCXML, "*.scxml;" },
    { FileType::Resource, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFile", "Resources"),
      ProjectExplorer::Constants::FILEOVERLAY_QRC, "*.qrc;" },
    { FileType::QML, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFile", "QML"),
      ProjectExplorer::Constants::FILEOVERLAY_QML, "*.qml;" },
    { FileType::Unknown, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFile", "Other files"),
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
        const QString desc = QCoreApplication::translate("QmakeProjectManager::QmakePriFile", fileType.typeName);
        const QString filter = QString::fromUtf8(fileType.addFileFilter);
        fileTypeData.push_back(QmakeStaticData::FileTypeData(fileType.type,
                                                             desc, filter,
                                                             Core::FileIconProvider::directoryIcon(QLatin1String(fileType.icon))));
    }
    // Project icon
    projectIcon = Core::FileIconProvider::directoryIcon(ProjectExplorer::Constants::FILEOVERLAY_QT);
    productIcon = Core::FileIconProvider::directoryIcon(ProjectExplorer::Constants::FILEOVERLAY_PRODUCT);
    groupIcon = Core::FileIconProvider::directoryIcon(ProjectExplorer::Constants::FILEOVERLAY_GROUP);

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
            return !Utils::contains(toExclude, [&fn](const Utils::FilePath &ex) { return fn.first.isChildOf(ex); });
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

            if (type == FileType::Resource) {
                for (const auto &file : newFilePaths) {
                    auto vfs = buildSystem->qmakeVfs();
                    QString contents;
                    QString errorMessage;
                    // Prefer the cumulative file if it's non-empty, based on the assumption
                    // that it contains more "stuff".
                    int cid = vfs->idForFileName(file.first.toString(), QMakeVfs::VfsCumulative);
                    vfs->readFile(cid, &contents, &errorMessage);
                    // If the cumulative evaluation botched the file too much, try the exact one.
                    if (contents.isEmpty()) {
                        int eid = vfs->idForFileName(file.first.toString(), QMakeVfs::VfsExact);
                        vfs->readFile(eid, &contents, &errorMessage);
                    }
                    auto topLevel = std::make_unique<ResourceEditor::ResourceTopLevelNode>
                                     (file.first, vfolder->filePath(), contents);
                    topLevel->setEnabled(file.second == FileOrigin::ExactParse);
                    const QString baseName = file.first.toFileInfo().completeBaseName();
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
                for (FolderNode *fn : vfolder->folderNodes())
                    fn->compress();
            }
            node->addNode(std::move(vfolder));
        }
    }

    if (!generatedFiles.empty()) {
        QTC_CHECK(proFile);
        const FilePath baseDir = generatedFiles.size() == 1 ? generatedFiles.first().parentDir()
                                                            : buildSystem->buildDir(proFile->filePath());
        auto genFolder = std::make_unique<VirtualFolderNode>(baseDir);
        genFolder->setDisplayName(QCoreApplication::translate("QmakeProjectManager::QmakePriFile",
                                                              "Generated Files"));
        genFolder->setIsGenerated(true);
        for (const FilePath &fp : generatedFiles) {
            auto fileNode = std::make_unique<FileNode>(fp, FileNode::fileTypeForFileName(fp));
            fileNode->setIsGenerated(true);
            genFolder->addNestedNode(std::move(fileNode));
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
    BaseQtVersion *qt = QtKitAspect::qtVersion(buildSystem->kit());

    const FilePaths toExclude = qt ? qt->directoriesToIgnoreInProjectTree() : FilePaths();

    auto root = std::make_unique<QmakeProFileNode>(buildSystem,
                                                   buildSystem->projectFilePath(),
                                                   buildSystem->rootProFile());
    root->setIcon(iconForProfile(buildSystem->rootProFile()));
    createTree(buildSystem, buildSystem->rootProFile(), root.get(), toExclude);

    return root;
}

} // namespace QmakeProjectManager
