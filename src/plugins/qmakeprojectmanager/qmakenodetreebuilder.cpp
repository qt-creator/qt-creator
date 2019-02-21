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

    qAddPostRoutine(clearQmakeStaticData);
}

Q_GLOBAL_STATIC(QmakeStaticData, qmakeStaticData)

void clearQmakeStaticData()
{
    qmakeStaticData()->fileTypeData.clear();
    qmakeStaticData()->projectIcon = QIcon();
}

} // namespace

namespace QmakeProjectManager {

static void createTree(const QmakePriFile *pri, QmakePriFileNode *node, const FileNameList &toExclude)
{
    QTC_ASSERT(pri, return);
    QTC_ASSERT(node, return);

    node->setDisplayName(pri->displayName());
    node->setIcon(qmakeStaticData()->projectIcon);

    // .pro/.pri-file itself:
    node->addNode(std::make_unique<FileNode>(pri->filePath(), FileType::Project, false));

    // other normal files:
    const QVector<QmakeStaticData::FileTypeData> &fileTypes = qmakeStaticData()->fileTypeData;
    for (int i = 0; i < fileTypes.size(); ++i) {
        FileType type = fileTypes.at(i).type;
        const QSet<FileName> &newFilePaths = Utils::filtered(pri->files(type), [&toExclude](const Utils::FileName &fn) {
            return !Utils::contains(toExclude, [&fn](const Utils::FileName &ex) { return fn.isChildOf(ex); });
        });

        if (!newFilePaths.isEmpty()) {
            auto vfolder = std::make_unique<VirtualFolderNode>(pri->filePath().parentDir(), Node::DefaultVirtualFolderPriority - i);
            vfolder->setIcon(fileTypes.at(i).icon);
            vfolder->setDisplayName(fileTypes.at(i).typeName);
            vfolder->setAddFileFilter(fileTypes.at(i).addFileFilter);

            if (type == FileType::Resource) {
                for (const FileName &file : newFilePaths) {
                    auto vfs = pri->project()->qmakeVfs();
                    QString contents;
                    QString errorMessage;
                    // Prefer the cumulative file if it's non-empty, based on the assumption
                    // that it contains more "stuff".
                    int cid = vfs->idForFileName(file.toString(), QMakeVfs::VfsCumulative);
                    vfs->readFile(cid, &contents, &errorMessage);
                    // If the cumulative evaluation botched the file too much, try the exact one.
                    if (contents.isEmpty()) {
                        int eid = vfs->idForFileName(file.toString(), QMakeVfs::VfsExact);
                        vfs->readFile(eid, &contents, &errorMessage);
                    }
                    const QString baseName = file.toFileInfo().completeBaseName();
                    const bool generated = baseName.startsWith("qmake_")
                            || baseName.endsWith("_qmlcache");
                    vfolder->addNode(std::make_unique<ResourceEditor::ResourceTopLevelNode>(file, generated, contents, vfolder.get()));
                }
            } else {
                for (const FileName &fn : newFilePaths) {
                    // Qmake will flag everything in SOURCES as source, even when the
                    // qt quick compiler moves qrc files into it:-/ Get better data based on
                    // the filename.
                    type = FileNode::fileTypeForFileName(fn);
                    vfolder->addNestedNode(std::make_unique<FileNode>(fn, type, false));
                }
                for (FolderNode *fn : vfolder->folderNodes())
                    fn->compress();
            }
            node->addNode(std::move(vfolder));
        }
    }

    // Virtual folders:
    for (QmakePriFile *c : pri->children()) {
        std::unique_ptr<QmakePriFileNode> newNode;
        if (auto pf = dynamic_cast<QmakeProFile *>(c))
            newNode = std::make_unique<QmakeProFileNode>(c->project(), c->filePath(), pf);
        else
            newNode = std::make_unique<QmakePriFileNode>(c->project(), node->proFileNode(), c->filePath(), c);
        createTree(c, newNode.get(), toExclude);
        node->addNode(std::move(newNode));
    }
}

std::unique_ptr<QmakeProFileNode> QmakeNodeTreeBuilder::buildTree(QmakeProject *project)
{
    // Remove qmake implementation details that litter up the project data:
    Target *t = project->activeTarget();
    Kit *k = t ? t->kit() : KitManager::defaultKit();
    BaseQtVersion *qt = k ? QtKitInformation::qtVersion(k) : nullptr;

    const FileNameList toExclude = qt ? qt->directoriesToIgnoreInProjectTree() : FileNameList();

    auto root = std::make_unique<QmakeProFileNode>(project, project->projectFilePath(),
                                                   project->rootProFile());
    createTree(project->rootProFile(), root.get(), toExclude);

    return root;
}

} // namespace QmakeProjectManager
