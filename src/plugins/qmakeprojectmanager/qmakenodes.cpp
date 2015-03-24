/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmakenodes.h"
#include "qmakeproject.h"
#include "qmakeprojectmanager.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakebuildconfiguration.h"
#include "qmakerunconfigurationfactory.h"

#include <projectexplorer/nodesvisitor.h>
#include <projectexplorer/projectexplorer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/dialogs/readonlyfilesdialog.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projecttree.h>
#include <qtsupport/profilereader.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/uicodemodelsupport.h>

#include <resourceeditor/resourcenode.h>

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>
#include <proparser/prowriter.h>
#include <proparser/qmakevfs.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

#include <QMessageBox>
#include <utils/QtConcurrentTools>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

// Static cached data in struct QmakeNodeStaticData providing information and icons
// for file types and the project. Do some magic via qAddPostRoutine()
// to make sure the icons do not outlive QApplication, triggering warnings on X11.

struct FileTypeDataStorage {
    FileType type;
    const char *typeName;
    const char *icon;
    Theme::ImageFile themeImage;
};

static const FileTypeDataStorage fileTypeDataStorage[] = {
    { HeaderType, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFileNode", "Headers"),
      ":/qmakeprojectmanager/images/headers.png", Theme::ProjectExplorerHeader },
    { SourceType, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFileNode", "Sources"),
      ":/qmakeprojectmanager/images/sources.png", Theme::ProjectExplorerSource },
    { FormType, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFileNode", "Forms"),
      ":/qtsupport/images/forms.png", Theme::ProjectExplorerForm },
    { ResourceType, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFileNode", "Resources"),
      ":/qtsupport/images/qt_qrc.png", Theme::ProjectExplorerResource },
    { QMLType, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFileNode", "QML"),
      ":/qtsupport/images/qml.png", Theme::ProjectExplorerQML },
    { UnknownFileType, QT_TRANSLATE_NOOP("QmakeProjectManager::QmakePriFileNode", "Other files"),
      ":/qmakeprojectmanager/images/unknown.png", Theme::ProjectExplorerOtherFiles }
};

class SortByPath
{
public:
    bool operator()(Node *a, Node *b)
    { return operator()(a->path(), b->path()); }
    bool operator()(Node *a, const FileName &b)
    { return operator()(a->path(), b); }
    bool operator()(const FileName &a, Node *b)
    { return operator()(a, b->path()); }
    // Compare as strings to correctly detect case-only file rename
    bool operator()(const FileName &a, const FileName &b)
    { return a.toString() < b.toString(); }
};

class QmakeNodeStaticData {
public:
    class FileTypeData {
    public:
        FileTypeData(FileType t = UnknownFileType,
                     const QString &tN = QString(),
                     const QIcon &i = QIcon()) :
        type(t), typeName(tN), icon(i) { }

        FileType type;
        QString typeName;
        QIcon icon;
    };

    QmakeNodeStaticData();

    QVector<FileTypeData> fileTypeData;
    QIcon projectIcon;
};

static void clearQmakeNodeStaticData();

QmakeNodeStaticData::QmakeNodeStaticData()
{
    // File type data
    const unsigned count = sizeof(fileTypeDataStorage)/sizeof(FileTypeDataStorage);
    fileTypeData.reserve(count);

    // Overlay the SP_DirIcon with the custom icons
    const QSize desiredSize = QSize(16, 16);

    for (unsigned i = 0 ; i < count; ++i) {
        QIcon overlayIcon;
        const QString iconFile = creatorTheme()->imageFile(fileTypeDataStorage[i].themeImage,
                                                           QString::fromLatin1(fileTypeDataStorage[i].icon));
        overlayIcon = QIcon(iconFile);
        const QPixmap folderPixmap =
                Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon,
                                                    overlayIcon, desiredSize);
        QIcon folderIcon;
        folderIcon.addPixmap(folderPixmap);
        const QString desc = QCoreApplication::translate("QmakeProjectManager::QmakePriFileNode", fileTypeDataStorage[i].typeName);
        fileTypeData.push_back(QmakeNodeStaticData::FileTypeData(fileTypeDataStorage[i].type,
                                                               desc, folderIcon));
    }
    // Project icon
    const QString fileName = creatorTheme()->imageFile(Theme::ProjectFileIcon,
                                                       QLatin1String(":/qtsupport/images/qt_project.png"));
    const QIcon projectBaseIcon(fileName);
    const QPixmap projectPixmap = Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon,
                                                                      projectBaseIcon,
                                                                      desiredSize);
    projectIcon.addPixmap(projectPixmap);

    qAddPostRoutine(clearQmakeNodeStaticData);
}

Q_GLOBAL_STATIC(QmakeNodeStaticData, qmakeNodeStaticData)

static void clearQmakeNodeStaticData()
{
    qmakeNodeStaticData()->fileTypeData.clear();
    qmakeNodeStaticData()->projectIcon = QIcon();
}

enum { debug = 0 };

using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;

namespace QmakeProjectManager {
namespace Internal {
class EvalInput
{
public:
    QString projectDir;
    FileName projectFilePath;
    QString buildDirectory;
    QtSupport::ProFileReader *readerExact;
    QtSupport::ProFileReader *readerCumulative;
    ProFileGlobals *qmakeGlobals;
    QMakeVfs *qmakeVfs;
    bool isQt5;
};

class PriFileEvalResult
{
public:
    QStringList folders;
    QSet<FileName> recursiveEnumerateFiles;
    QMap<FileType, QSet<FileName> > foundFiles;
};

class EvalResult
{
public:
    enum EvalResultState { EvalAbort, EvalFail, EvalPartial, EvalOk };
    EvalResultState state;
    QmakeProjectType projectType;

    QStringList subProjectsNotToDeploy;
    QHash<FileName, ProFile*> includeFilesExact;
    FileNameList newProjectFilesExact;
    QSet<FileName> exactSubdirs;
    ProFile *fileForCurrentProjectExact; // probably only used in parser thread
    QHash<FileName, ProFile*> includeFilesCumlative;
    FileNameList newProjectFilesCumlative;
    ProFile *fileForCurrentProjectCumlative; // probably only used in parser thread
    TargetInformation targetInformation;
    InstallsList installsList;
    QHash<QmakeVariable, QStringList> newVarValues;
    bool isDeployable;
    QHash<FileName, PriFileEvalResult> priFileResults;
    QStringList errors;
};
}
}

QmakePriFile::QmakePriFile(QmakeProjectManager::QmakePriFileNode *qmakePriFile)
    : IDocument(0), m_priFile(qmakePriFile)
{
    setId("Qmake.PriFile");
    setMimeType(QLatin1String(QmakeProjectManager::Constants::PROFILE_MIMETYPE));
    setFilePath(m_priFile->path());
}

bool QmakePriFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString);
    Q_UNUSED(fileName);
    Q_UNUSED(autoSave);
    return false;
}

QString QmakePriFile::defaultPath() const
{
    return QString();
}

QString QmakePriFile::suggestedFileName() const
{
    return QString();
}

bool QmakePriFile::isModified() const
{
    return false;
}

bool QmakePriFile::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior QmakePriFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool QmakePriFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    if (type == TypePermissions)
        return true;
    m_priFile->scheduleUpdate();
    return true;
}

/*!
  \class QmakePriFileNode
  Implements abstract ProjectNode class
  */

namespace QmakeProjectManager {

QmakePriFileNode::QmakePriFileNode(QmakeProject *project, QmakeProFileNode *qmakeProFileNode,
                                   const FileName &filePath)
        : ProjectNode(filePath),
          m_project(project),
          m_qmakeProFileNode(qmakeProFileNode),
          m_projectFilePath(filePath),
          m_projectDir(filePath.toFileInfo().absolutePath()),
          m_includedInExactParse(true)
{
    Q_ASSERT(project);
    m_qmakePriFile = new QmakePriFile(this);
    Core::DocumentManager::addDocument(m_qmakePriFile);

    setDisplayName(filePath.toFileInfo().completeBaseName());
    setIcon(qmakeNodeStaticData()->projectIcon);
}

QmakePriFileNode::~QmakePriFileNode()
{
    watchFolders(QSet<QString>());
    delete m_qmakePriFile;
}

void QmakePriFileNode::scheduleUpdate()
{
    QtSupport::ProFileCacheManager::instance()->discardFile(m_projectFilePath.toString());
    m_qmakeProFileNode->scheduleUpdate(QmakeProFileNode::ParseLater);
}

namespace Internal {
struct InternalNode
{
    QList<InternalNode *> virtualfolders;
    QMap<QString, InternalNode *> subnodes;
    FileNameList files;
    FileType type;
    int priority;
    QString displayName;
    QString typeName;
    QString fullPath;
    QIcon icon;

    InternalNode()
    {
        type = UnknownFileType;
        priority = 0;
    }

    ~InternalNode()
    {
        qDeleteAll(virtualfolders);
        qDeleteAll(subnodes);
    }

    // Creates: a tree structure from a list of absolute file paths.
    // Empty directories are compressed into a single entry with a longer path.
    // * project
    //    * /absolute/path
    //       * file1
    //    * relative
    //       * path1
    //          * file1
    //          * file2
    //       * path2
    //          * file1
    // The function first creates a tree that looks like the directory structure, i.e.
    //    * /
    //       * absolute
    //          * path
    // ...
    // and afterwards calls compress() which merges directory nodes with single children, i.e. to
    //    * /absolute/path
    void create(const QString &projectDir, const QSet<FileName> &newFilePaths, FileType type)
    {
        static const QChar separator = QLatin1Char('/');
        const FileName projectDirFileName = FileName::fromString(projectDir);
        foreach (const FileName &file, newFilePaths) {
            FileName fileWithoutPrefix;
            bool isRelative;
            if (file.isChildOf(projectDirFileName)) {
                isRelative = true;
                fileWithoutPrefix = file.relativeChildPath(projectDirFileName);
            } else {
                isRelative = false;
                fileWithoutPrefix = file;
            }
            QStringList parts = fileWithoutPrefix.toString().split(separator, QString::SkipEmptyParts);
            if (!HostOsInfo::isWindowsHost() && !isRelative && parts.count() > 0)
                parts[0].prepend(separator);
            QStringListIterator it(parts);
            InternalNode *currentNode = this;
            QString path = (isRelative ? (projectDirFileName.toString() + QLatin1Char('/')) : QString());
            while (it.hasNext()) {
                const QString &key = it.next();
                if (it.hasNext()) { // key is directory
                    path += key;
                    if (!currentNode->subnodes.contains(path)) {
                        InternalNode *val = new InternalNode;
                        val->type = type;
                        val->fullPath = path;
                        val->displayName = key;
                        currentNode->subnodes.insert(path, val);
                        currentNode = val;
                    } else {
                        currentNode = currentNode->subnodes.value(path);
                    }
                    path += separator;
                } else { // key is filename
                    currentNode->files.append(file);
                }
            }
        }
        this->compress();
    }

    // Removes folder nodes with only a single sub folder in it
    void compress()
    {
        QMap<QString, InternalNode*> newSubnodes;
        QMapIterator<QString, InternalNode*> i(subnodes);
        while (i.hasNext()) {
            i.next();
            i.value()->compress();
            if (i.value()->files.isEmpty() && i.value()->subnodes.size() == 1) {
                // replace i.value() by i.value()->subnodes.begin()
                QString key = i.value()->subnodes.begin().key();
                InternalNode *keep = i.value()->subnodes.value(key);
                keep->displayName = i.value()->displayName + QLatin1Char('/') + keep->displayName;
                newSubnodes.insert(key, keep);
                i.value()->subnodes.clear();
                delete i.value();
            } else {
                newSubnodes.insert(i.key(), i.value());
            }
        }
        subnodes = newSubnodes;
    }

    FolderNode *createFolderNode(InternalNode *node)
    {
        FolderNode *newNode = 0;
        if (node->typeName.isEmpty()) {
            newNode = new FolderNode(FileName::fromString(node->fullPath));
        } else {
            newNode = new ProVirtualFolderNode(FileName::fromString(node->fullPath),
                                               node->priority, node->typeName);
        }

        newNode->setDisplayName(node->displayName);
        if (!node->icon.isNull())
            newNode->setIcon(node->icon);
        return newNode;
    }

    // Makes the projectNode's subtree below the given folder match this internal node's subtree
    void updateSubFolders(FolderNode *folder)
    {
        if (type == ResourceType)
            updateResourceFiles(folder);
        else
            updateFiles(folder, type);

        // updateFolders
        QMultiMap<QString, FolderNode *> existingFolderNodes;
        foreach (FolderNode *node, folder->subFolderNodes())
            if (node->nodeType() != ProjectNodeType && !dynamic_cast<ResourceEditor::ResourceTopLevelNode *>(node))
                existingFolderNodes.insert(node->path().toString(), node);

        QList<FolderNode *> foldersToRemove;
        QList<FolderNode *> foldersToAdd;
        typedef QPair<InternalNode *, FolderNode *> NodePair;
        QList<NodePair> nodesToUpdate;

        // Check virtual
        {
            QList<InternalNode *>::const_iterator it = virtualfolders.constBegin();
            QList<InternalNode *>::const_iterator end = virtualfolders.constEnd();
            for ( ; it != end; ++it) {
                bool found = false;
                QString path = (*it)->fullPath;
                QMultiMap<QString, FolderNode *>::const_iterator oldit
                        = existingFolderNodes.constFind(path);
                while (oldit != existingFolderNodes.constEnd() && oldit.key() == path) {
                    if (oldit.value()->nodeType() == VirtualFolderNodeType) {
                        VirtualFolderNode *vfn = dynamic_cast<VirtualFolderNode *>(oldit.value());
                        if (vfn->priority() == (*it)->priority) {
                            found = true;
                            break;
                        }
                    }
                    ++oldit;
                }
                if (found) {
                    nodesToUpdate << NodePair(*it, *oldit);
                } else {
                    FolderNode *newNode = createFolderNode(*it);
                    foldersToAdd << newNode;
                    nodesToUpdate << NodePair(*it, newNode);
                }
            }
        }
        // Check subnodes
        {
            QMap<QString, InternalNode *>::const_iterator it = subnodes.constBegin();
            QMap<QString, InternalNode *>::const_iterator end = subnodes.constEnd();

            for ( ; it != end; ++it) {
                bool found = false;
                QString path = it.value()->fullPath;
                QMultiMap<QString, FolderNode *>::const_iterator oldit
                        = existingFolderNodes.constFind(path);
                while (oldit != existingFolderNodes.constEnd() && oldit.key() == path) {
                    if (oldit.value()->nodeType() == FolderNodeType) {
                        found = true;
                        break;
                    }
                    ++oldit;
                }
                if (found) {
                    nodesToUpdate << NodePair(it.value(), *oldit);
                } else {
                    FolderNode *newNode = createFolderNode(it.value());
                    foldersToAdd << newNode;
                    nodesToUpdate << NodePair(it.value(), newNode);
                }
            }
        }

        QSet<FolderNode *> toKeep;
        foreach (const NodePair &np, nodesToUpdate)
            toKeep << np.second;

        QMultiMap<QString, FolderNode *>::const_iterator jit = existingFolderNodes.constBegin();
        QMultiMap<QString, FolderNode *>::const_iterator jend = existingFolderNodes.constEnd();
        for ( ; jit != jend; ++jit)
            if (!toKeep.contains(jit.value()))
                foldersToRemove << jit.value();

        if (!foldersToRemove.isEmpty())
            folder->removeFolderNodes(foldersToRemove);
        if (!foldersToAdd.isEmpty())
            folder->addFolderNodes(foldersToAdd);

        foreach (const NodePair &np, nodesToUpdate)
            np.first->updateSubFolders(np.second);
    }

    // Makes the folder's files match this internal node's file list
    void updateFiles(FolderNode *folder, FileType type)
    {
        QList<FileNode*> existingFileNodes;
        foreach (FileNode *fileNode, folder->fileNodes()) {
            if (fileNode->fileType() == type && !fileNode->isGenerated())
                existingFileNodes << fileNode;
        }

        QList<FileNode*> filesToRemove;
        FileNameList filesToAdd;

        SortByPath sortByPath;
        Utils::sort(files, sortByPath);
        Utils::sort(existingFileNodes, sortByPath);

        ProjectExplorer::compareSortedLists(existingFileNodes, files, filesToRemove, filesToAdd, sortByPath);

        QList<FileNode *> nodesToAdd;
        foreach (const FileName &file, filesToAdd)
            nodesToAdd << new FileNode(file, type, false);

        folder->removeFileNodes(filesToRemove);
        folder->addFileNodes(nodesToAdd);
    }

    // Makes the folder's files match this internal node's file list
    void updateResourceFiles(FolderNode *folder)
    {
        QList<FolderNode *> existingResourceNodes; // for resource special handling
        foreach (FolderNode *folderNode, folder->subFolderNodes()) {
            if (ResourceEditor::ResourceTopLevelNode *rn = dynamic_cast<ResourceEditor::ResourceTopLevelNode *>(folderNode))
                existingResourceNodes << rn;
        }

        QList<FolderNode *> resourcesToRemove;
        FileNameList resourcesToAdd;

        SortByPath sortByPath;
        Utils::sort(files, sortByPath);
        Utils::sort(existingResourceNodes, sortByPath);

        ProjectExplorer::compareSortedLists(existingResourceNodes, files, resourcesToRemove, resourcesToAdd, sortByPath);

        QList<FolderNode *> nodesToAdd;
        nodesToAdd.reserve(resourcesToAdd.size());

        foreach (const FileName &file, resourcesToAdd)
            nodesToAdd.append(new ResourceEditor::ResourceTopLevelNode(file, folder));

        folder->removeFolderNodes(resourcesToRemove);
        folder->addFolderNodes(nodesToAdd);

        foreach (FolderNode *fn, nodesToAdd)
            dynamic_cast<ResourceEditor::ResourceTopLevelNode *>(fn)->update();
    }
};
}

QStringList QmakePriFileNode::baseVPaths(QtSupport::ProFileReader *reader, const QString &projectDir, const QString &buildDir)
{
    QStringList result;
    if (!reader)
        return result;
    result += reader->absolutePathValues(QLatin1String("VPATH"), projectDir);
    result << projectDir; // QMAKE_ABSOLUTE_SOURCE_PATH
    result << buildDir;
    result.removeDuplicates();
    return result;
}

QStringList QmakePriFileNode::fullVPaths(const QStringList &baseVPaths, QtSupport::ProFileReader *reader,
                                       const QString &qmakeVariable, const QString &projectDir)
{
    QStringList vPaths;
    if (!reader)
        return vPaths;
    vPaths = reader->absolutePathValues(QLatin1String("VPATH_") + qmakeVariable, projectDir);
    vPaths += baseVPaths;
    vPaths.removeDuplicates();
    return vPaths;
}

QSet<FileName> QmakePriFileNode::recursiveEnumerate(const QString &folder)
{
    QSet<FileName> result;
    QFileInfo fi(folder);
    if (fi.isDir()) {
        QDir dir(folder);
        dir.setFilter(dir.filter() | QDir::NoDotAndDotDot);

        foreach (const QFileInfo &file, dir.entryInfoList()) {
            if (file.isDir() && !file.isSymLink())
                result += recursiveEnumerate(file.absoluteFilePath());
            else if (!Core::EditorManager::isAutoSaveFile(file.fileName()))
                result += FileName(file);
        }
    } else if (fi.exists()) {
        result << FileName(fi);
    }
    return result;
}

PriFileEvalResult QmakePriFileNode::extractValues(const EvalInput &input, ProFile *includeFileExact, ProFile *includeFileCumlative,
                                                  const QList<QList<VariableAndVPathInformation>> &variableAndVPathInformation)
{
    PriFileEvalResult result;

    // Figure out DEPLOYMENT and INSTALL folders
    QStringList dynamicVariables = dynamicVarNames(input.readerExact, input.readerCumulative, input.isQt5);
    if (includeFileExact)
        foreach (const QString &dynamicVar, dynamicVariables) {
            result.folders += input.readerExact->values(dynamicVar, includeFileExact);
            // Ignore stuff from cumulative parse
            // we are recursively enumerating all the files from those folders
            // and add watchers for them, that's too dangerous if we get the folders
            // wrong and enumerate the whole project tree multiple times
        }


    for (int i=0; i < result.folders.size(); ++i) {
        const QFileInfo fi(result.folders.at(i));
        if (fi.isRelative())
            result.folders[i] = QDir::cleanPath(input.projectDir + QLatin1Char('/') + result.folders.at(i));
    }

    result.folders.removeDuplicates();

    // Remove non existing items and non folders
    QStringList::iterator it = result.folders.begin();
    while (it != result.folders.end()) {
        QFileInfo fi(*it);
        if (fi.exists()) {
            if (fi.isDir()) {
                // keep directories
                ++it;
            } else {
                // move files directly to recursiveEnumerateFiles
                result.recursiveEnumerateFiles << FileName::fromString(*it);
                it = result.folders.erase(it);
            }
        } else {
            // do remove non exsting stuff
            it = result.folders.erase(it);
        }
    }

    foreach (const QString &folder, result.folders)
        result.recursiveEnumerateFiles += recursiveEnumerate(folder);

    const QVector<QmakeNodeStaticData::FileTypeData> &fileTypes = qmakeNodeStaticData()->fileTypeData;
    // update files
    QFileInfo tmpFi;
    for (int i = 0; i < fileTypes.size(); ++i) {
        FileType type = fileTypes.at(i).type;
        const QList<VariableAndVPathInformation> &qmakeVariables = variableAndVPathInformation.at(i);
        QSet<FileName> newFilePaths;
        foreach (const VariableAndVPathInformation &qmakeVariable, qmakeVariables) {
            if (includeFileExact) {
                QStringList tmp = input.readerExact->absoluteFileValues(qmakeVariable.variable, input.projectDir, qmakeVariable.vPathsExact, includeFileExact);
                foreach (const QString &t, tmp) {
                    tmpFi.setFile(t);
                    if (tmpFi.isFile())
                        newFilePaths += FileName::fromString(t);
                }
            }
            if (includeFileCumlative) {
                QStringList tmp = input.readerCumulative->absoluteFileValues(qmakeVariable.variable, input.projectDir, qmakeVariable.vPathsCumulative, includeFileCumlative);
                foreach (const QString &t, tmp) {
                    tmpFi.setFile(t);
                    if (tmpFi.isFile())
                        newFilePaths += FileName::fromString(t);
                }
            }
        }

        result.foundFiles[type] = newFilePaths;
        result.recursiveEnumerateFiles.subtract(newFilePaths);
    }


    for (int i = 0; i < fileTypes.size(); ++i) {
        FileType type = fileTypes.at(i).type;
        QSet<FileName> newFilePaths = filterFilesProVariables(type, result.foundFiles[type]);
        newFilePaths += filterFilesRecursiveEnumerata(type, result.recursiveEnumerateFiles);
        result.foundFiles[type] = newFilePaths;
    }


    return result;
}

void QmakePriFileNode::update(const Internal::PriFileEvalResult &result)
{
    // add project file node
    if (m_fileNodes.isEmpty())
        addFileNodes(QList<FileNode *>() << new FileNode(m_projectFilePath, ProjectFileType, false));

    m_recursiveEnumerateFiles = result.recursiveEnumerateFiles;
    watchFolders(result.folders.toSet());

    InternalNode contents;
    const QVector<QmakeNodeStaticData::FileTypeData> &fileTypes = qmakeNodeStaticData()->fileTypeData;
    for (int i = 0; i < fileTypes.size(); ++i) {
        FileType type = fileTypes.at(i).type;
        const QSet<FileName> &newFilePaths = result.foundFiles.value(type);
        // We only need to save this information if
        // we are watching folders
        if (!result.folders.isEmpty())
            m_files[type] = newFilePaths;
        else
            m_files[type].clear();

        if (!newFilePaths.isEmpty()) {
            InternalNode *subfolder = new InternalNode;
            subfolder->type = type;
            subfolder->icon = fileTypes.at(i).icon;
            subfolder->fullPath = m_projectDir;
            subfolder->typeName = fileTypes.at(i).typeName;
            subfolder->priority = -i;
            subfolder->displayName = fileTypes.at(i).typeName;
            contents.virtualfolders.append(subfolder);
            // create the hierarchy with subdirectories
            subfolder->create(m_projectDir, newFilePaths, type);
        }
    }

    contents.updateSubFolders(this);
}

void QmakePriFileNode::watchFolders(const QSet<QString> &folders)
{
    QSet<QString> toUnwatch = m_watchedFolders;
    toUnwatch.subtract(folders);

    QSet<QString> toWatch = folders;
    toWatch.subtract(m_watchedFolders);

    if (!toUnwatch.isEmpty())
        m_project->unwatchFolders(toUnwatch.toList(), this);
    if (!toWatch.isEmpty())
        m_project->watchFolders(toWatch.toList(), this);

    m_watchedFolders = folders;
}

bool QmakePriFileNode::folderChanged(const QString &changedFolder, const QSet<FileName> &newFiles)
{
    //qDebug()<<"########## QmakePriFileNode::folderChanged";
    // So, we need to figure out which files changed.

    QSet<FileName> addedFiles = newFiles;
    addedFiles.subtract(m_recursiveEnumerateFiles);

    QSet<FileName> removedFiles = m_recursiveEnumerateFiles;
    removedFiles.subtract(newFiles);

    foreach (const FileName &file, removedFiles) {
        if (!file.isChildOf(FileName::fromString(changedFolder)))
            removedFiles.remove(file);
    }

    if (addedFiles.isEmpty() && removedFiles.isEmpty())
        return false;

    m_recursiveEnumerateFiles = newFiles;

    // Apply the differences
    // per file type
    const QVector<QmakeNodeStaticData::FileTypeData> &fileTypes = qmakeNodeStaticData()->fileTypeData;
    for (int i = 0; i < fileTypes.size(); ++i) {
        FileType type = fileTypes.at(i).type;
        QSet<FileName> add = filterFilesRecursiveEnumerata(type, addedFiles);
        QSet<FileName> remove = filterFilesRecursiveEnumerata(type, removedFiles);

        if (!add.isEmpty() || !remove.isEmpty()) {
            // Scream :)
//            qDebug()<<"For type"<<fileTypes.at(i).typeName<<"\n"
//                    <<"added files"<<add<<"\n"
//                    <<"removed files"<<remove;

            m_files[type].unite(add);
            m_files[type].subtract(remove);
        }
    }

    // Now apply stuff
    InternalNode contents;
    for (int i = 0; i < fileTypes.size(); ++i) {
        FileType type = fileTypes.at(i).type;
        if (!m_files[type].isEmpty()) {
            InternalNode *subfolder = new InternalNode;
            subfolder->type = type;
            subfolder->icon = fileTypes.at(i).icon;
            subfolder->fullPath = m_projectDir;
            subfolder->typeName = fileTypes.at(i).typeName;
            subfolder->priority = -i;
            subfolder->displayName = fileTypes.at(i).typeName;
            contents.virtualfolders.append(subfolder);
            // create the hierarchy with subdirectories
            subfolder->create(m_projectDir, m_files[type], type);
        }
    }

    contents.updateSubFolders(this);
    return true;
}

bool QmakePriFileNode::deploysFolder(const QString &folder) const
{
    QString f = folder;
    const QChar slash = QLatin1Char('/');
    if (!f.endsWith(slash))
        f.append(slash);

    foreach (const QString &wf, m_watchedFolders) {
        if (f.startsWith(wf)
            && (wf.endsWith(slash)
                || (wf.length() < f.length() && f.at(wf.length()) == slash)))
            return true;
    }
    return false;
}

QList<RunConfiguration *> QmakePriFileNode::runConfigurations() const
{
    QmakeRunConfigurationFactory *factory = QmakeRunConfigurationFactory::find(m_project->activeTarget());
    if (factory)
        return factory->runConfigurationsForNode(m_project->activeTarget(), this);
    return QList<RunConfiguration *>();
}

QList<QmakePriFileNode *> QmakePriFileNode::subProjectNodesExact() const
{
    QList<QmakePriFileNode *> nodes;
    foreach (ProjectNode *node, subProjectNodes()) {
        QmakePriFileNode *n = dynamic_cast<QmakePriFileNode *>(node);
        if (n && n->includedInExactParse())
            nodes << n;
    }
    return nodes;
}

QmakeProFileNode *QmakePriFileNode::proFileNode() const
{
    return m_qmakeProFileNode;
}

bool QmakePriFileNode::includedInExactParse() const
{
    return m_includedInExactParse;
}

void QmakePriFileNode::setIncludedInExactParse(bool b)
{
    m_includedInExactParse = b;
}

QList<ProjectAction> QmakePriFileNode::supportedActions(Node *node) const
{
    QList<ProjectAction> actions;

    const FolderNode *folderNode = this;
    const QmakeProFileNode *proFileNode;
    while (!(proFileNode = dynamic_cast<const QmakeProFileNode*>(folderNode)))
        folderNode = folderNode->parentFolderNode();
    Q_ASSERT(proFileNode);

    switch (proFileNode->projectType()) {
    case ApplicationTemplate:
    case StaticLibraryTemplate:
    case SharedLibraryTemplate:
    case AuxTemplate: {
        // TODO: Some of the file types don't make much sense for aux
        // projects (e.g. cpp). It'd be nice if the "add" action could
        // work on a subset of the file types according to project type.

        actions << AddNewFile;
        if (m_recursiveEnumerateFiles.contains(node->path()))
            actions << EraseFile;
        else
            actions << RemoveFile;

        bool addExistingFiles = true;
        if (node->nodeType() == VirtualFolderNodeType) {
            // A virtual folder, we do what the projectexplorer does
            FolderNode *folder = node->asFolderNode();
            if (folder) {
                QStringList list;
                foreach (FolderNode *f, folder->subFolderNodes())
                    list << f->path().toString() + QLatin1Char('/');
                if (deploysFolder(Utils::commonPath(list)))
                    addExistingFiles = false;
            }
        }

        addExistingFiles = addExistingFiles && !deploysFolder(node->path().toString());

        if (addExistingFiles)
            actions << AddExistingFile << AddExistingDirectory;

        break;
    }
    case SubDirsTemplate:
        actions << AddSubProject << RemoveSubProject;
        break;
    default:
        break;
    }

    FileNode *fileNode = node->asFileNode();
    if ((fileNode && fileNode->fileType() != ProjectFileType)
            || dynamic_cast<ResourceEditor::ResourceTopLevelNode *>(node))
        actions << Rename;


    Target *target = m_project->activeTarget();
    QmakeRunConfigurationFactory *factory = QmakeRunConfigurationFactory::find(target);
    if (factory && !factory->runConfigurationsForNode(target, node).isEmpty())
        actions << HasSubProjectRunConfigurations;

    return actions;
}

bool QmakePriFileNode::canAddSubProject(const QString &proFilePath) const
{
    QFileInfo fi(proFilePath);
    if (fi.suffix() == QLatin1String("pro")
        || fi.suffix() == QLatin1String("pri"))
        return true;
    return false;
}

static QString simplifyProFilePath(const QString &proFilePath)
{
    // if proFilePath is like: _path_/projectName/projectName.pro
    // we simplify it to: _path_/projectName
    QFileInfo fi(proFilePath);
    const QString parentPath = fi.absolutePath();
    QFileInfo parentFi(parentPath);
    if (parentFi.fileName() == fi.completeBaseName())
        return parentPath;
    return proFilePath;
}

bool QmakePriFileNode::addSubProjects(const QStringList &proFilePaths)
{
    FindAllFilesVisitor visitor;
    accept(&visitor);
    const FileNameList &allFiles = visitor.filePaths();

    QStringList uniqueProFilePaths;
    foreach (const QString &proFile, proFilePaths)
        if (!allFiles.contains(FileName::fromString(proFile)))
            uniqueProFilePaths.append(simplifyProFilePath(proFile));

    QStringList failedFiles;
    changeFiles(QLatin1String(Constants::PROFILE_MIMETYPE), uniqueProFilePaths, &failedFiles, AddToProFile);

    return failedFiles.isEmpty();
}

bool QmakePriFileNode::removeSubProjects(const QStringList &proFilePaths)
{
    QStringList failedOriginalFiles;
    changeFiles(QLatin1String(Constants::PROFILE_MIMETYPE), proFilePaths, &failedOriginalFiles, RemoveFromProFile);

    QStringList simplifiedProFiles = Utils::transform(failedOriginalFiles, &simplifyProFilePath);

    QStringList failedSimplifiedFiles;
    changeFiles(QLatin1String(Constants::PROFILE_MIMETYPE), simplifiedProFiles, &failedSimplifiedFiles, RemoveFromProFile);

    return failedSimplifiedFiles.isEmpty();
}

bool QmakePriFileNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    // If a file is already referenced in the .pro file then we don't add them.
    // That ignores scopes and which variable was used to reference the file
    // So it's obviously a bit limited, but in those cases you need to edit the
    // project files manually anyway.

    FindAllFilesVisitor visitor;
    accept(&visitor);
    const FileNameList &allFiles = visitor.filePaths();

    typedef QMap<QString, QStringList> TypeFileMap;
    // Split into lists by file type and bulk-add them.
    TypeFileMap typeFileMap;
    Utils::MimeDatabase mdb;
    foreach (const QString &file, filePaths) {
        const Utils::MimeType mt = mdb.mimeTypeForFile(file);
        typeFileMap[mt.name()] << file;
    }

    QStringList failedFiles;
    foreach (const QString &type, typeFileMap.keys()) {
        const QStringList typeFiles = typeFileMap.value(type);
        QStringList qrcFiles; // the list of qrc files referenced from ui files
        if (type == QLatin1String(ProjectExplorer::Constants::RESOURCE_MIMETYPE)) {
            foreach (const QString &formFile, typeFiles) {
                QStringList resourceFiles = formResources(formFile);
                foreach (const QString &resourceFile, resourceFiles)
                    if (!qrcFiles.contains(resourceFile))
                        qrcFiles.append(resourceFile);
            }
        }

        QStringList uniqueQrcFiles;
        foreach (const QString &file, qrcFiles) {
            if (!allFiles.contains(FileName::fromString(file)))
                uniqueQrcFiles.append(file);
        }

        QStringList uniqueFilePaths;
        foreach (const QString &file, typeFiles) {
            if (!allFiles.contains(FileName::fromString(file)))
                uniqueFilePaths.append(file);
        }

        changeFiles(type, uniqueFilePaths, &failedFiles, AddToProFile);
        if (notAdded)
            *notAdded += failedFiles;
        changeFiles(QLatin1String(ProjectExplorer::Constants::RESOURCE_MIMETYPE), uniqueQrcFiles, &failedFiles, AddToProFile);
        if (notAdded)
            *notAdded += failedFiles;
    }
    return failedFiles.isEmpty();
}

bool QmakePriFileNode::removeFiles(const QStringList &filePaths,
                              QStringList *notRemoved)
{
    QStringList failedFiles;
    typedef QMap<QString, QStringList> TypeFileMap;
    // Split into lists by file type and bulk-add them.
    TypeFileMap typeFileMap;
    Utils::MimeDatabase mdb;
    foreach (const QString &file, filePaths) {
        const Utils::MimeType mt = mdb.mimeTypeForFile(file);
        typeFileMap[mt.name()] << file;
    }
    foreach (const QString &type, typeFileMap.keys()) {
        const QStringList typeFiles = typeFileMap.value(type);
        changeFiles(type, typeFiles, &failedFiles, RemoveFromProFile);
        if (notRemoved)
            *notRemoved = failedFiles;
    }
    return failedFiles.isEmpty();
}

bool QmakePriFileNode::deleteFiles(const QStringList &filePaths)
{
    removeFiles(filePaths);
    return true;
}

bool QmakePriFileNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    if (newFilePath.isEmpty())
        return false;

    bool changeProFileOptional = deploysFolder(QFileInfo(filePath).absolutePath());
    Utils::MimeDatabase mdb;
    const Utils::MimeType mt = mdb.mimeTypeForFile(newFilePath);
    QStringList dummy;

    changeFiles(mt.name(), QStringList() << filePath, &dummy, RemoveFromProFile);
    if (!dummy.isEmpty() && !changeProFileOptional)
        return false;
    changeFiles(mt.name(), QStringList() << newFilePath, &dummy, AddToProFile);
    if (!dummy.isEmpty() && !changeProFileOptional)
        return false;
    return true;
}

FolderNode::AddNewInformation QmakePriFileNode::addNewInformation(const QStringList &files, Node *context) const
{
    Q_UNUSED(files)
    return FolderNode::AddNewInformation(path().fileName(), context && context->projectNode() == this ? 120 : 90);
}

bool QmakePriFileNode::priFileWritable(const QString &path)
{
    ReadOnlyFilesDialog roDialog(path, ICore::mainWindow());
    roDialog.setShowFailWarning(true);
    return roDialog.exec() != ReadOnlyFilesDialog::RO_Cancel;
}

bool QmakePriFileNode::saveModifiedEditors()
{
    Core::IDocument *document
            = Core::DocumentModel::documentForFilePath(m_projectFilePath.toString());
    if (!document || !document->isModified())
        return true;

    if (!Core::DocumentManager::saveDocument(document))
        return false;

    // force instant reload of ourselves
    QtSupport::ProFileCacheManager::instance()->discardFile(m_projectFilePath.toString());
    m_project->qmakeProjectManager()->notifyChanged(m_projectFilePath);
    return true;
}

QStringList QmakePriFileNode::formResources(const QString &formFile) const
{
    QStringList resourceFiles;
    QFile file(formFile);
    if (!file.open(QIODevice::ReadOnly))
        return resourceFiles;

    QXmlStreamReader reader(&file);

    QFileInfo fi(formFile);
    QDir formDir = fi.absoluteDir();
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("iconset")) {
                const QXmlStreamAttributes attributes = reader.attributes();
                if (attributes.hasAttribute(QLatin1String("resource")))
                    resourceFiles.append(QDir::cleanPath(formDir.absoluteFilePath(
                                  attributes.value(QLatin1String("resource")).toString())));
            } else if (reader.name() == QLatin1String("include")) {
                const QXmlStreamAttributes attributes = reader.attributes();
                if (attributes.hasAttribute(QLatin1String("location")))
                    resourceFiles.append(QDir::cleanPath(formDir.absoluteFilePath(
                                  attributes.value(QLatin1String("location")).toString())));

            }
        }
    }

    if (reader.hasError())
        qWarning() << "Could not read form file:" << formFile;

    return resourceFiles;
}

bool QmakePriFileNode::ensureWriteableProFile(const QString &file)
{
    // Ensure that the file is not read only
    QFileInfo fi(file);
    if (!fi.isWritable()) {
        // Try via vcs manager
        Core::IVersionControl *versionControl = Core::VcsManager::findVersionControlForDirectory(fi.absolutePath());
        if (!versionControl || !versionControl->vcsOpen(file)) {
            bool makeWritable = QFile::setPermissions(file, fi.permissions() | QFile::WriteUser);
            if (!makeWritable) {
                QMessageBox::warning(Core::ICore::mainWindow(),
                                     QCoreApplication::translate("QmakePriFileNode", "Failed"),
                                     QCoreApplication::translate("QmakePriFileNode", "Could not write project file %1.").arg(file));
                return false;
            }
        }
    }
    return true;
}

QPair<ProFile *, QStringList> QmakePriFileNode::readProFile(const QString &file)
{
    QStringList lines;
    ProFile *includeFile = 0;
    {
        QString contents;
        {
            FileReader reader;
            if (!reader.fetch(file, QIODevice::Text)) {
                QmakeProject::proFileParseError(reader.errorString());
                return qMakePair(includeFile, lines);
            }
            contents = QString::fromLocal8Bit(reader.data());
            lines = contents.split(QLatin1Char('\n'));
        }

        QMakeVfs vfs;
        QtSupport::ProMessageHandler handler;
        QMakeParser parser(0, &vfs, &handler);
        includeFile = parser.parsedProBlock(contents, file, 1);
    }
    return qMakePair(includeFile, lines);
}

void QmakePriFileNode::changeFiles(const QString &mimeType,
                                 const QStringList &filePaths,
                                 QStringList *notChanged,
                                 ChangeType change)
{
    if (filePaths.isEmpty())
        return;

    *notChanged = filePaths;

    // Check for modified editors
    if (!saveModifiedEditors())
        return;

    if (!ensureWriteableProFile(m_projectFilePath.toString()))
        return;
    QPair<ProFile *, QStringList> pair = readProFile(m_projectFilePath.toString());
    ProFile *includeFile = pair.first;
    QStringList lines = pair.second;

    if (!includeFile)
        return;


    if (change == AddToProFile) {
        // Use the first variable for adding.
        ProWriter::addFiles(includeFile, &lines, filePaths, varNameForAdding(mimeType));
        notChanged->clear();
    } else { // RemoveFromProFile
        QDir priFileDir = QDir(m_qmakeProFileNode->m_projectDir);
        *notChanged = ProWriter::removeFiles(includeFile, &lines, priFileDir, filePaths, varNamesForRemoving());
    }

    // save file
    save(lines);
    includeFile->deref();
}

bool QmakePriFileNode::setProVariable(const QString &var, const QStringList &values, const QString &scope, int flags)
{
    if (!ensureWriteableProFile(m_projectFilePath.toString()))
        return false;

    QPair<ProFile *, QStringList> pair = readProFile(m_projectFilePath.toString());
    ProFile *includeFile = pair.first;
    QStringList lines = pair.second;

    ProWriter::putVarValues(includeFile, &lines, values, var,
                            ProWriter::PutFlags(flags),
                            scope);

    if (!includeFile)
        return false;
    save(lines);
    includeFile->deref();
    return true;
}

void QmakePriFileNode::save(const QStringList &lines)
{
    Core::DocumentManager::expectFileChange(m_projectFilePath.toString());
    FileSaver saver(m_projectFilePath.toString(), QIODevice::Text);
    saver.write(lines.join(QLatin1Char('\n')).toLocal8Bit());
    saver.finalize(Core::ICore::mainWindow());

    m_project->qmakeProjectManager()->notifyChanged(m_projectFilePath);
    Core::DocumentManager::unexpectFileChange(m_projectFilePath.toString());
    // This is a hack.
    // We are saving twice in a very short timeframe, once the editor and once the ProFile.
    // So the modification time might not change between those two saves.
    // We manually tell each editor to reload it's file.
    // (The .pro files are notified by the file system watcher.)
    QStringList errorStrings;
    Core::IDocument *document = Core::DocumentModel::documentForFilePath(m_projectFilePath.toString());
    if (document) {
        QString errorString;
        if (!document->reload(&errorString, Core::IDocument::FlagReload, Core::IDocument::TypeContents))
            errorStrings << errorString;
    }
    if (!errorStrings.isEmpty())
        QMessageBox::warning(Core::ICore::mainWindow(), QCoreApplication::translate("QmakePriFileNode", "File Error"),
                             errorStrings.join(QLatin1Char('\n')));
}

QStringList QmakePriFileNode::varNames(FileType type, QtSupport::ProFileReader *readerExact)
{
    QStringList vars;
    switch (type) {
    case HeaderType:
        vars << QLatin1String("HEADERS");
        vars << QLatin1String("PRECOMPILED_HEADER");
        break;
    case SourceType: {
        vars << QLatin1String("SOURCES");
        QStringList listOfExtraCompilers = readerExact->values(QLatin1String("QMAKE_EXTRA_COMPILERS"));
        foreach (const QString &var, listOfExtraCompilers) {
            QStringList inputs = readerExact->values(var + QLatin1String(".input"));
            foreach (const QString &input, inputs)
                // FORMS and RESOURCES are handled below
                if (input != QLatin1String("FORMS")
                        && input != QLatin1String("RESOURCES")
                        && input != QLatin1String("HEADERS"))
                    vars << input;
        }
        break;
    }
    case ResourceType:
        vars << QLatin1String("RESOURCES");
        break;
    case FormType:
        vars << QLatin1String("FORMS");
        break;
    case ProjectFileType:
        vars << QLatin1String("SUBDIRS");
        break;
    case QMLType:
        vars << QLatin1String("OTHER_FILES");
        vars << QLatin1String("DISTFILES");
        break;
    default:
        vars << QLatin1String("OTHER_FILES");
        vars << QLatin1String("DISTFILES");
        vars << QLatin1String("ICON");
        vars << QLatin1String("QMAKE_INFO_PLIST");
        break;
    }
    return vars;
}

//!
//! \brief QmakePriFileNode::varNames
//! \param mimeType
//! \return the qmake variable name for the mime type
//! Note: Only used for adding.
//!
QString QmakePriFileNode::varNameForAdding(const QString &mimeType)
{
    if (mimeType == QLatin1String(ProjectExplorer::Constants::CPP_HEADER_MIMETYPE)
            || mimeType == QLatin1String(ProjectExplorer::Constants::C_HEADER_MIMETYPE)) {
        return QLatin1String("HEADERS");
    }

    if (mimeType == QLatin1String(ProjectExplorer::Constants::CPP_SOURCE_MIMETYPE)
               || mimeType == QLatin1String(ProjectExplorer::Constants::C_SOURCE_MIMETYPE)) {
        return QLatin1String("SOURCES");
    }

    if (mimeType == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE))
        return QLatin1String("OBJECTIVE_SOURCES");

    if (mimeType == QLatin1String(ProjectExplorer::Constants::RESOURCE_MIMETYPE))
        return QLatin1String("RESOURCES");

    if (mimeType == QLatin1String(ProjectExplorer::Constants::FORM_MIMETYPE))
        return QLatin1String("FORMS");

    if (mimeType == QLatin1String(ProjectExplorer::Constants::QML_MIMETYPE))
        return QLatin1String("DISTFILES");

    if (mimeType == QLatin1String(Constants::PROFILE_MIMETYPE))
        return QLatin1String("SUBDIRS");

    return QLatin1String("DISTFILES");
}

//!
//! \brief QmakePriFileNode::varNamesForRemoving
//! \return all qmake variables which are displayed in the project tree
//! Note: Only used for removing.
//!
QStringList QmakePriFileNode::varNamesForRemoving()
{
    QStringList vars;
    vars << QLatin1String("HEADERS");
    vars << QLatin1String("OBJECTIVE_HEADERS");
    vars << QLatin1String("PRECOMPILED_HEADER");
    vars << QLatin1String("SOURCES");
    vars << QLatin1String("OBJECTIVE_SOURCES");
    vars << QLatin1String("RESOURCES");
    vars << QLatin1String("FORMS");
    vars << QLatin1String("OTHER_FILES");
    vars << QLatin1String("SUBDIRS");
    vars << QLatin1String("DISTFILES");
    vars << QLatin1String("ICON");
    vars << QLatin1String("QMAKE_INFO_PLIST");
    return vars;
}

QStringList QmakePriFileNode::dynamicVarNames(QtSupport::ProFileReader *readerExact, QtSupport::ProFileReader *readerCumulative,
                                            bool isQt5)
{
    QStringList result;

    // Figure out DEPLOYMENT and INSTALLS
    const QString deployment = QLatin1String("DEPLOYMENT");
    const QString sources = QLatin1String(isQt5 ? ".files" : ".sources");
    QStringList listOfVars = readerExact->values(deployment);
    foreach (const QString &var, listOfVars) {
        result << (var + sources);
    }
    if (readerCumulative) {
        QStringList listOfVars = readerCumulative->values(deployment);
        foreach (const QString &var, listOfVars) {
            result << (var + sources);
        }
    }

    const QString installs = QLatin1String("INSTALLS");
    const QString files = QLatin1String(".files");
    listOfVars = readerExact->values(installs);
    foreach (const QString &var, listOfVars) {
        result << (var + files);
    }
    if (readerCumulative) {
        QStringList listOfVars = readerCumulative->values(installs);
        foreach (const QString &var, listOfVars) {
            result << (var + files);
        }
    }
    result.removeDuplicates();
    return result;
}

QSet<FileName> QmakePriFileNode::filterFilesProVariables(FileType fileType, const QSet<FileName> &files)
{
    if (fileType != QMLType && fileType != UnknownFileType)
        return files;
    QSet<FileName> result;
    if (fileType == QMLType) {
        foreach (const FileName &file, files)
            if (file.toString().endsWith(QLatin1String(".qml")))
                result << file;
    } else {
        foreach (const FileName &file, files)
            if (!file.toString().endsWith(QLatin1String(".qml")))
                result << file;
    }
    return result;
}

QSet<FileName> QmakePriFileNode::filterFilesRecursiveEnumerata(FileType fileType, const QSet<FileName> &files)
{
    QSet<FileName> result;
    if (fileType != QMLType && fileType != UnknownFileType)
        return result;
    if (fileType == QMLType) {
        foreach (const FileName &file, files)
            if (file.toString().endsWith(QLatin1String(".qml")))
                result << file;
    } else {
        foreach (const FileName &file, files)
            if (!file.toString().endsWith(QLatin1String(".qml")))
                result << file;
    }
    return result;
}

} // namespace QmakeProjectManager

static QmakeProjectType proFileTemplateTypeToProjectType(ProFileEvaluator::TemplateType type)
{
    switch (type) {
    case ProFileEvaluator::TT_Unknown:
    case ProFileEvaluator::TT_Application:
        return ApplicationTemplate;
    case ProFileEvaluator::TT_StaticLibrary:
        return StaticLibraryTemplate;
    case ProFileEvaluator::TT_SharedLibrary:
        return SharedLibraryTemplate;
    case ProFileEvaluator::TT_Script:
        return ScriptTemplate;
    case ProFileEvaluator::TT_Aux:
        return AuxTemplate;
    case ProFileEvaluator::TT_Subdirs:
        return SubDirsTemplate;
    default:
        return InvalidProject;
    }
}

namespace {
    // find all ui files in project
    class FindUiFileNodesVisitor : public NodesVisitor {
    public:
        void visitProjectNode(ProjectNode *projectNode)
        {
            visitFolderNode(projectNode);
        }
        void visitFolderNode(FolderNode *folderNode)
        {
            foreach (FileNode *fileNode, folderNode->fileNodes()) {
                if (fileNode->fileType() == FormType)
                    uiFileNodes << fileNode;
            }
        }
        QList<FileNode*> uiFileNodes;
    };
}

QmakeProFileNode *QmakeProFileNode::findProFileFor(const FileName &fileName) const
{
    if (fileName == path())
        return const_cast<QmakeProFileNode *>(this);
    foreach (ProjectNode *pn, subProjectNodes())
        if (QmakeProFileNode *qmakeProFileNode = dynamic_cast<QmakeProFileNode *>(pn))
            if (QmakeProFileNode *result = qmakeProFileNode->findProFileFor(fileName))
                return result;
    return 0;
}

QString QmakeProFileNode::makefile() const
{
    return singleVariableValue(Makefile);
}

QString QmakeProFileNode::objectExtension() const
{
    if (m_varValues[ObjectExt].isEmpty())
        return HostOsInfo::isWindowsHost() ? QLatin1String(".obj") : QLatin1String(".o");
    return m_varValues[ObjectExt].first();
}

QString QmakeProFileNode::objectsDirectory() const
{
    return singleVariableValue(ObjectsDir);
}

QByteArray QmakeProFileNode::cxxDefines() const
{
    QByteArray result;
    foreach (const QString &def, variableValue(DefinesVar)) {
        // 'def' is shell input, so interpret it.
        QtcProcess::SplitError error = QtcProcess::SplitOk;
        const QStringList args = QtcProcess::splitArgs(def, HostOsInfo::hostOs(), false, &error);
        if (error != QtcProcess::SplitOk || args.size() == 0)
            continue;

        result += "#define ";
        const QString defInterpreted = args.first();
        const int index = defInterpreted.indexOf(QLatin1Char('='));
        if (index == -1) {
            result += defInterpreted.toLatin1();
            result += " 1\n";
        } else {
            const QString name = defInterpreted.left(index);
            const QString value = defInterpreted.mid(index + 1);
            result += name.toLatin1();
            result += ' ';
            result += value.toLocal8Bit();
            result += '\n';
        }
    }
    return result;
}

bool QmakeProFileNode::isDeployable() const
{
    return m_isDeployable;
}

/*!
  \class QmakeProFileNode
  Implements abstract ProjectNode class
  */
QmakeProFileNode::QmakeProFileNode(QmakeProject *project,
                               const FileName &filePath)
        : QmakePriFileNode(project, this, filePath),
          m_validParse(false),
          m_parseInProgress(true),
          m_projectType(InvalidProject),
          m_readerExact(0),
          m_readerCumulative(0)
{
    // The slot is a lambda, so that QmakeProFileNode does not need to be
    // a qobject. The lifetime of the m_parserFutureWatcher is shorter
    // than of this, so this is all safe
    QObject::connect(&m_parseFutureWatcher, &QFutureWatcherBase::finished,
                     [this](){
                         applyAsyncEvaluate();
                     });
}

QmakeProFileNode::~QmakeProFileNode()
{
    m_parseFutureWatcher.waitForFinished();
    if (m_readerExact)
        applyAsyncEvaluate();
}

bool QmakeProFileNode::isParent(QmakeProFileNode *node)
{
    while ((node = dynamic_cast<QmakeProFileNode *>(node->parentFolderNode()))) {
        if (node == this)
            return true;
    }
    return false;
}

bool QmakeProFileNode::showInSimpleTree() const
{
    return showInSimpleTree(projectType()) || m_project->rootProjectNode() == this;
}

FolderNode::AddNewInformation QmakeProFileNode::addNewInformation(const QStringList &files, Node *context) const
{
    Q_UNUSED(files)
    return AddNewInformation(path().fileName(), context && context->projectNode() == this ? 120 : 100);
}

bool QmakeProFileNode::showInSimpleTree(QmakeProjectType projectType) const
{
    return (projectType == ApplicationTemplate || projectType == SharedLibraryTemplate  || projectType == StaticLibraryTemplate);
}

bool QmakeProFileNode::isDebugAndRelease() const
{
    const QStringList configValues = m_varValues.value(ConfigVar);
    return configValues.contains(QLatin1String("debug_and_release"));
}

bool QmakeProFileNode::isQtcRunnable() const
{
    const QStringList configValues = m_varValues.value(ConfigVar);
    return configValues.contains(QLatin1String("qtc_runnable"));
}

QmakeProjectType QmakeProFileNode::projectType() const
{
    return m_projectType;
}

QStringList QmakeProFileNode::variableValue(const QmakeVariable var) const
{
    return m_varValues.value(var);
}

QString QmakeProFileNode::singleVariableValue(const QmakeVariable var) const
{
    const QStringList &values = variableValue(var);
    return values.isEmpty() ? QString() : values.first();
}

QHash<QString, QString> QmakeProFileNode::uiFiles() const
{
    return m_uiFiles;
}

void QmakeProFileNode::emitProFileUpdatedRecursive()
{
    emit m_project->proFileUpdated(this, m_validParse, m_parseInProgress);

    foreach (ProjectNode *subNode, subProjectNodes()) {
        if (QmakeProFileNode *node = dynamic_cast<QmakeProFileNode *>(subNode))
            node->emitProFileUpdatedRecursive();
    }
}

void QmakeProFileNode::setParseInProgressRecursive(bool b)
{
    setParseInProgress(b);
    foreach (ProjectNode *subNode, subProjectNodes()) {
        if (QmakeProFileNode *node = dynamic_cast<QmakeProFileNode *>(subNode))
            node->setParseInProgressRecursive(b);
    }
}

void QmakeProFileNode::setParseInProgress(bool b)
{
    if (m_parseInProgress == b)
        return;
    m_parseInProgress = b;
    emit m_project->proFileUpdated(this, m_validParse, m_parseInProgress);
}

void QmakeProFileNode::setValidParseRecursive(bool b)
{
    setValidParse(b);
    foreach (ProjectNode *subNode, subProjectNodes()) {
        if (QmakeProFileNode *node = dynamic_cast<QmakeProFileNode *>(subNode))
            node->setValidParseRecursive(b);
    }
}

// Do note the absence of signal emission, always set validParse
// before setParseInProgress, as that will emit the signals
void QmakeProFileNode::setValidParse(bool b)
{
    if (m_validParse == b)
        return;
    m_validParse = b;
}

bool QmakeProFileNode::validParse() const
{
    return m_validParse;
}

bool QmakeProFileNode::parseInProgress() const
{
    return m_parseInProgress;
}

void QmakeProFileNode::scheduleUpdate(QmakeProFileNode::AsyncUpdateDelay delay)
{
    setParseInProgressRecursive(true);
    m_project->scheduleAsyncUpdate(this, delay);
}

void QmakeProFileNode::asyncUpdate()
{
    m_project->incrementPendingEvaluateFutures();
    setupReader();
    m_parseFutureWatcher.waitForFinished();
    EvalInput input = evalInput();
    QFuture<EvalResult *> future = QtConcurrent::run(&QmakeProFileNode::asyncEvaluate, this, input);
    m_parseFutureWatcher.setFuture(future);
}

EvalInput QmakeProFileNode::evalInput() const
{
    EvalInput input;
    input.projectDir = m_projectDir;
    input.projectFilePath = m_projectFilePath;
    input.buildDirectory = buildDir();
    input.readerExact = m_readerExact;
    input.readerCumulative = m_readerCumulative;
    Target *t = m_project->activeTarget();
    Kit *k = t ? t->kit() : KitManager::defaultKit();
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k);
    input.isQt5 = !qtVersion || qtVersion->qtVersion() >= QtSupport::QtVersionNumber(5,0,0);
    input.qmakeGlobals = m_project->qmakeGlobals();
    input.qmakeVfs = m_project->qmakeVfs();
    return input;
}

void QmakeProFileNode::setupReader()
{
    Q_ASSERT(!m_readerExact);
    Q_ASSERT(!m_readerCumulative);

    m_readerExact = m_project->createProFileReader(this);

    m_readerCumulative = m_project->createProFileReader(this);
    m_readerCumulative->setCumulative(true);
}

static FileNameList mergeList(const FileNameList &listA, const FileNameList &listB)
{
    FileNameList result;
    result.reserve(qMax(listA.size(), listB.size()));
    auto ait = listA.constBegin();
    auto aend = listA.constEnd();
    auto bit = listB.constBegin();
    auto bend = listB.constEnd();
    while (ait != aend && bit != bend) {
        const FileName &a = *ait;
        const FileName &b = *bit;
        if (a < b) {
            result.append(a);
            ++ait;
        } else if (b < a) {
            result.append(b);
            ++bit;
        } else {
            result.append(a);
            ++ait;
            ++bit;
        }
    }
    while (ait != aend) {
        result.append(*ait);
        ++ait;
    }
    while (bit != bend) {
        result.append(*bit);
        ++bit;
    }
    return result;
}

EvalResult *QmakeProFileNode::evaluate(const EvalInput &input)
{
    EvalResult *result = new EvalResult;
    if (ProFile *pro = input.readerExact->parsedProFile(input.projectFilePath.toString())) {
        bool exactOk = input.readerExact->accept(pro, QMakeEvaluator::LoadAll);
        bool cumulOk = input.readerCumulative->accept(pro, QMakeEvaluator::LoadPreFiles);
        pro->deref();
        result->state = exactOk ? EvalResult::EvalOk : cumulOk ? EvalResult::EvalPartial : EvalResult::EvalFail;
    } else {
        result->state = EvalResult::EvalFail;
    }

    if (result->state == EvalResult::EvalFail)
        return result;

    result->projectType = proFileTemplateTypeToProjectType(
                (result->state == EvalResult::EvalOk ? input.readerExact
                                                     : input.readerCumulative)->templateType());
    result->fileForCurrentProjectExact = 0;
    result->fileForCurrentProjectCumlative = 0;

    if (result->state == EvalResult::EvalOk) {
        if (result->projectType == SubDirsTemplate) {
            QStringList errors;
            result->newProjectFilesExact = subDirsPaths(input.readerExact, input.projectDir, &result->subProjectsNotToDeploy, &errors);
            result->errors.append(errors);
            result->exactSubdirs = result->newProjectFilesExact.toSet();
        }
        foreach (ProFile *includeFile, input.readerExact->includeFiles()) {
            if (includeFile->fileName() == input.projectFilePath.toString()) { // this file
                result->fileForCurrentProjectExact = includeFile;
            } else {
                const FileName includeFileName = FileName::fromString(includeFile->fileName());
                result->newProjectFilesExact << includeFileName;
                result->includeFilesExact.insert(includeFileName, includeFile);
            }
        }
    }

    if (result->projectType == SubDirsTemplate)
        result->newProjectFilesCumlative = subDirsPaths(input.readerCumulative, input.projectDir, 0, 0);
    foreach (ProFile *includeFile, input.readerCumulative->includeFiles()) {
        if (includeFile->fileName() == input.projectFilePath.toString()) {
            result->fileForCurrentProjectCumlative = includeFile;
        } else {
            const FileName includeFileName = FileName::fromString(includeFile->fileName());
            result->newProjectFilesCumlative << includeFileName;
            result->includeFilesCumlative.insert(includeFileName, includeFile);
        }
    }
    SortByPath sortByPath;
    Utils::sort(result->newProjectFilesExact, sortByPath);
    Utils::sort(result->newProjectFilesCumlative, sortByPath);

    if (result->state == EvalResult::EvalOk) {
        // create build_pass reader
        QtSupport::ProFileReader *readerBuildPass = 0;
        QStringList builds = input.readerExact->values(QLatin1String("BUILDS"));
        if (builds.isEmpty()) {
            readerBuildPass = input.readerExact;
        } else {
            QString build = builds.first();
            QHash<QString, QStringList> basevars;
            QStringList basecfgs = input.readerExact->values(build + QLatin1String(".CONFIG"));
            basecfgs += build;
            basecfgs += QLatin1String("build_pass");
            basevars[QLatin1String("BUILD_PASS")] = QStringList(build);
            QStringList buildname = input.readerExact->values(build + QLatin1String(".name"));
            basevars[QLatin1String("BUILD_NAME")] = (buildname.isEmpty() ? QStringList(build) : buildname);

            // We don't increase/decrease m_qmakeGlobalsRefCnt here, because the outer profilereaders keep m_qmakeGlobals alive anyway
            readerBuildPass = new QtSupport::ProFileReader(input.qmakeGlobals, input.qmakeVfs); // needs to access m_qmakeGlobals, m_qmakeVfs
            readerBuildPass->setOutputDir(input.buildDirectory);
            readerBuildPass->setExtraVars(basevars);
            readerBuildPass->setExtraConfigs(basecfgs);

            EvalResult::EvalResultState evalResultBuildPass = EvalResult::EvalOk;
            if (ProFile *pro = readerBuildPass->parsedProFile(input.projectFilePath.toString())) {
                if (!readerBuildPass->accept(pro, QMakeEvaluator::LoadAll))
                    evalResultBuildPass = EvalResult::EvalPartial;
                pro->deref();
            } else {
                evalResultBuildPass = EvalResult::EvalFail;
            }

            if (evalResultBuildPass != EvalResult::EvalOk) {
                delete readerBuildPass;
                readerBuildPass = 0;
            }
        }
        result->targetInformation = targetInformation(input.readerExact, readerBuildPass,
                                                      input.buildDirectory, input.projectFilePath.toString());
        result->installsList = installsList(readerBuildPass, input.projectFilePath.toString(), input.projectDir);

        // update other variables
        result->newVarValues[DefinesVar] = input.readerExact->values(QLatin1String("DEFINES"));
        result->newVarValues[IncludePathVar] = includePaths(input.readerExact, input.buildDirectory, input.projectDir);
        result->newVarValues[CppFlagsVar] = input.readerExact->values(QLatin1String("QMAKE_CXXFLAGS"));
        result->newVarValues[CppHeaderVar] = fileListForVar(input.readerExact, input.readerCumulative,
                                                    QLatin1String("HEADERS"), input.projectDir, input.buildDirectory);
        result->newVarValues[CppSourceVar] = fileListForVar(input.readerExact, input.readerCumulative,
                                                    QLatin1String("SOURCES"), input.projectDir, input.buildDirectory);
        result->newVarValues[ObjCSourceVar] = fileListForVar(input.readerExact, input.readerCumulative,
                                                     QLatin1String("OBJECTIVE_SOURCES"), input.projectDir, input.buildDirectory);
        result->newVarValues[ObjCHeaderVar] = fileListForVar(input.readerExact, input.readerCumulative,
                                                     QLatin1String("OBJECTIVE_HEADERS"), input.projectDir, input.buildDirectory);
        result->newVarValues[UiDirVar] = QStringList() << uiDirPath(input.readerExact, input.buildDirectory);
        result->newVarValues[MocDirVar] = QStringList() << mocDirPath(input.readerExact, input.buildDirectory);
        result->newVarValues[ResourceVar] = fileListForVar(input.readerExact, input.readerCumulative,
                                                   QLatin1String("RESOURCES"), input.projectDir, input.buildDirectory);
        result->newVarValues[ExactResourceVar] = fileListForVar(input.readerExact, 0,
                                                        QLatin1String("RESOURCES"), input.projectDir, input.buildDirectory);
        result->newVarValues[PkgConfigVar] = input.readerExact->values(QLatin1String("PKGCONFIG"));
        result->newVarValues[PrecompiledHeaderVar] =
                input.readerExact->absoluteFileValues(QLatin1String("PRECOMPILED_HEADER"),
                                                      input.projectDir,
                                                      QStringList() << input.projectDir,
                                                      0);
        result->newVarValues[LibDirectoriesVar] = libDirectories(input.readerExact);
        result->newVarValues[ConfigVar] = input.readerExact->values(QLatin1String("CONFIG"));
        result->newVarValues[QmlImportPathVar] = input.readerExact->absolutePathValues(
                    QLatin1String("QML_IMPORT_PATH"), input.projectDir);
        result->newVarValues[Makefile] = input.readerExact->values(QLatin1String("MAKEFILE"));
        result->newVarValues[QtVar] = input.readerExact->values(QLatin1String("QT"));
        result->newVarValues[ObjectExt] = input.readerExact->values(QLatin1String("QMAKE_EXT_OBJ"));
        result->newVarValues[ObjectsDir] = input.readerExact->values(QLatin1String("OBJECTS_DIR"));
        result->newVarValues[VersionVar] = input.readerExact->values(QLatin1String("VERSION"));
        result->newVarValues[TargetExtVar] = input.readerExact->values(QLatin1String("TARGET_EXT"));
        result->newVarValues[TargetVersionExtVar]
                = input.readerExact->values(QLatin1String("TARGET_VERSION_EXT"));
        result->newVarValues[StaticLibExtensionVar] = input.readerExact->values(QLatin1String("QMAKE_EXTENSION_STATICLIB"));
        result->newVarValues[ShLibExtensionVar] = input.readerExact->values(QLatin1String("QMAKE_EXTENSION_SHLIB"));
        result->newVarValues[AndroidArchVar] = input.readerExact->values(QLatin1String("ANDROID_TARGET_ARCH"));
        result->newVarValues[AndroidDeploySettingsFile] = input.readerExact->values(QLatin1String("ANDROID_DEPLOYMENT_SETTINGS_FILE"));
        result->newVarValues[AndroidPackageSourceDir] = input.readerExact->values(QLatin1String("ANDROID_PACKAGE_SOURCE_DIR"));
        result->newVarValues[AndroidExtraLibs] = input.readerExact->values(QLatin1String("ANDROID_EXTRA_LIBS"));
        result->newVarValues[IsoIconsVar] = input.readerExact->values(QLatin1String("ISO_ICONS"));

        result->isDeployable = false;
        if (result->projectType == ApplicationTemplate) {
            result->isDeployable = true;
        } else {
            foreach (const QString &item, input.readerExact->values(QLatin1String("DEPLOYMENT"))) {
                if (!input.readerExact->values(item + QLatin1String(".sources")).isEmpty()) {
                    result->isDeployable = true;
                    break;
                }
            }
        }


        if (readerBuildPass && readerBuildPass != input.readerExact)
            delete readerBuildPass;
    }

    if (result->state == EvalResult::EvalOk || result->state == EvalResult::EvalPartial) {

        QList<QList<VariableAndVPathInformation>> variableAndVPathInformation;
        { // Collect information on VPATHS and qmake variables
            QStringList baseVPathsExact = baseVPaths(input.readerExact, input.projectDir, input.buildDirectory);
            QStringList baseVPathsCumulative = baseVPaths(input.readerCumulative, input.projectDir, input.buildDirectory);

            const QVector<QmakeNodeStaticData::FileTypeData> &fileTypes = qmakeNodeStaticData()->fileTypeData;

            variableAndVPathInformation.reserve(fileTypes.size());
            for (int i = 0; i < fileTypes.size(); ++i) {
                FileType type = fileTypes.at(i).type;

                QList<VariableAndVPathInformation> list;
                QStringList qmakeVariables = varNames(type, input.readerExact);
                list.reserve(qmakeVariables.size());
                foreach (const QString &qmakeVariable, qmakeVariables) {
                    VariableAndVPathInformation info;
                    info.variable = qmakeVariable;
                    info.vPathsExact = fullVPaths(baseVPathsExact, input.readerExact, qmakeVariable, input.projectDir);
                    info.vPathsCumulative = fullVPaths(baseVPathsCumulative, input.readerCumulative, qmakeVariable, input.projectDir);
                    list.append(info);
                }
                variableAndVPathInformation.append(list);
            }
        }

        // extract values
        FileNameList allFiles = mergeList(result->newProjectFilesExact, result->newProjectFilesCumlative);
        foreach (const FileName &file, allFiles) {
            ProFile *fileExact = result->includeFilesExact.value(file);
            ProFile *fileCumlative = result->includeFilesCumlative.value(file);
            if (fileExact || fileCumlative) {
                result->priFileResults[file] = extractValues(input, fileExact, fileCumlative, variableAndVPathInformation);
            }
        }
        result->priFileResults[input.projectFilePath] = extractValues(input, result->fileForCurrentProjectExact,
                                                                      result->fileForCurrentProjectCumlative,
                                                                      variableAndVPathInformation);

    }

    return result;
}

void QmakeProFileNode::asyncEvaluate(QFutureInterface<EvalResult *> &fi, EvalInput input)
{
    EvalResult *evalResult = evaluate(input);
    fi.reportResult(evalResult);
}

void QmakeProFileNode::applyAsyncEvaluate()
{
    applyEvaluate(m_parseFutureWatcher.result());
    m_project->decrementPendingEvaluateFutures();
}

bool sortByNodes(Node *a, Node *b)
{
    return a->path() < b->path();
}

void QmakeProFileNode::applyEvaluate(EvalResult *evalResult)
{
    QScopedPointer<EvalResult> result(evalResult);
    if (!m_readerExact)
        return;

    if (m_project->asyncUpdateState() == QmakeProject::ShuttingDown) {
        cleanupProFileReaders();
        return;
    }

    foreach (const QString &error, evalResult->errors)
        QmakeProject::proFileParseError(error);

    // we are changing what is executed in that case
    if (result->state == EvalResult::EvalFail || m_project->wasEvaluateCanceled()) {
        m_validParse = false;
        m_project->destroyProFileReader(m_readerExact);
        m_project->destroyProFileReader(m_readerCumulative);
        m_readerExact = m_readerCumulative = 0;
        setValidParseRecursive(false);
        setParseInProgressRecursive(false);

        if (result->state == EvalResult::EvalFail) {
            QmakeProject::proFileParseError(QCoreApplication::translate("QmakeProFileNode", "Error while parsing file %1. Giving up.")
                                            .arg(m_projectFilePath.toUserOutput()));
            if (m_projectType == InvalidProject)
                return;

            // delete files && folders && projects
            removeFileNodes(fileNodes());
            removeProjectNodes(subProjectNodes());
            removeFolderNodes(subFolderNodes());

            m_projectType = InvalidProject;
        }
        return;
    }

    if (debug)
        qDebug() << "QmakeProFileNode - updating files for file " << m_projectFilePath;

    if (result->projectType != m_projectType) {
        // probably all subfiles/projects have changed anyway
        // delete files && folders && projects
        foreach (ProjectNode *projectNode, subProjectNodes()) {
            if (QmakeProFileNode *qmakeProFileNode = dynamic_cast<QmakeProFileNode *>(projectNode)) {
                qmakeProFileNode->setValidParseRecursive(false);
                qmakeProFileNode->setParseInProgressRecursive(false);
            }
        }

        removeFileNodes(fileNodes());
        removeProjectNodes(subProjectNodes());
        removeFolderNodes(subFolderNodes());

        bool changesShowInSimpleTree = showInSimpleTree() ^ showInSimpleTree(result->projectType);

        if (changesShowInSimpleTree)
            ProjectTree::instance()->emitAboutToChangeShowInSimpleTree(this);

        m_projectType = result->projectType;

        if (changesShowInSimpleTree)
            ProjectTree::instance()->emitShowInSimpleTreeChanged(this);
    }

    //
    // Add/Remove pri files, sub projects
    //

    QList<ProjectNode*> existingProjectNodes = subProjectNodes();

    QString buildDirectory = buildDir();
    SortByPath sortByPath;
    Utils::sort(existingProjectNodes, sortByPath);
    // result is already sorted

    QList<ProjectNode*> toAdd;
    QList<ProjectNode*> toRemove;
    QList<QmakePriFileNode *> toUpdate;

    QList<ProjectNode*>::const_iterator existingIt = existingProjectNodes.constBegin();
    FileNameList::const_iterator newExactIt = result->newProjectFilesExact.constBegin();
    FileNameList::const_iterator newCumlativeIt = result->newProjectFilesCumlative.constBegin();

    forever {
        bool existingAtEnd = (existingIt == existingProjectNodes.constEnd());
        bool newExactAtEnd = (newExactIt == result->newProjectFilesExact.constEnd());
        bool newCumlativeAtEnd = (newCumlativeIt == result->newProjectFilesCumlative.constEnd());

        if (existingAtEnd && newExactAtEnd && newCumlativeAtEnd)
            break; // we are done, hurray!

        // So this is one giant loop comparing 3 lists at once and sorting the comparison
        // into mainly 2 buckets: toAdd and toRemove
        // We need to distinguish between nodes that came from exact and cumalative
        // parsing, since the update call is diffrent for them
        // I believe this code to be correct, be careful in changing it

        FileName nodeToAdd;
        if (! existingAtEnd
            && (newExactAtEnd || (*existingIt)->path() < *newExactIt)
            && (newCumlativeAtEnd || (*existingIt)->path() < *newCumlativeIt)) {
            // Remove case
            toRemove << *existingIt;
            ++existingIt;
        } else if (! newExactAtEnd
                  && (existingAtEnd || *newExactIt < (*existingIt)->path())
                  && (newCumlativeAtEnd || *newExactIt < *newCumlativeIt)) {
            // Mark node from exact for adding
            nodeToAdd = *newExactIt;
            ++newExactIt;
        } else if (! newCumlativeAtEnd
                   && (existingAtEnd ||  *newCumlativeIt < (*existingIt)->path())
                   && (newExactAtEnd || *newCumlativeIt < *newExactIt)) {
            // Mark node from cumalative for adding
            nodeToAdd = *newCumlativeIt;
            ++newCumlativeIt;
        } else if (!newExactAtEnd
                   && !newCumlativeAtEnd
                   && (existingAtEnd || *newExactIt < (*existingIt)->path())
                   && (existingAtEnd || *newCumlativeIt < (*existingIt)->path())) {
            // Mark node from both for adding
            nodeToAdd = *newExactIt;
            ++newExactIt;
            ++newCumlativeIt;
        } else {
            Q_ASSERT(!newExactAtEnd || !newCumlativeAtEnd);
            // update case, figure out which case exactly
            if (newExactAtEnd) {
                ++newCumlativeIt;
            } else if (newCumlativeAtEnd) {
                ++newExactIt;
            } else if (*newExactIt < *newCumlativeIt) {
                ++newExactIt;
            } else if (*newCumlativeIt < *newExactIt) {
                ++newCumlativeIt;
            } else {
                ++newExactIt;
                ++newCumlativeIt;
            }
            // Update existingNodeIte
            ProFile *fileExact = result->includeFilesExact.value((*existingIt)->path());
            ProFile *fileCumlative = result->includeFilesCumlative.value((*existingIt)->path());
            if (fileExact || fileCumlative) {
                QmakePriFileNode *priFileNode = static_cast<QmakePriFileNode *>(*existingIt);
                priFileNode->update(result->priFileResults[(*existingIt)->path()]);
                priFileNode->setIncludedInExactParse(fileExact != 0 && includedInExactParse());
            } else {
                // We always parse exactly, because we later when async parsing don't know whether
                // the .pro file is included in this .pro file
                // So to compare that later parse with the sync one
                QmakeProFileNode *proFileNode = static_cast<QmakeProFileNode *>(*existingIt);
                proFileNode->setIncludedInExactParse(result->exactSubdirs.contains(proFileNode->path())
                                                     && includedInExactParse());
                proFileNode->asyncUpdate();
            }
            ++existingIt;
            // newCumalativeIt and newExactIt are already incremented

        }
        // If we found something to add, do it
        if (!nodeToAdd.isEmpty()) {
            ProFile *fileExact = result->includeFilesExact.value(nodeToAdd);
            ProFile *fileCumlative = result->includeFilesCumlative.value(nodeToAdd);

            // Loop preventation, make sure that exact same node is not in our parent chain
            bool loop = false;
            Node *n = this;
            while ((n = n->parentFolderNode())) {
                if (dynamic_cast<QmakePriFileNode *>(n) && n->path() == nodeToAdd) {
                    loop = true;
                    break;
                }
            }

            if (loop) {
                // Do nothing
            } else {
                if (fileExact || fileCumlative) {
                    QmakePriFileNode *qmakePriFileNode = new QmakePriFileNode(m_project, this, nodeToAdd);
                    qmakePriFileNode->setParentFolderNode(this); // Needed for loop detection
                    qmakePriFileNode->setIncludedInExactParse(fileExact != 0 && includedInExactParse());
                    toAdd << qmakePriFileNode;
                    toUpdate << qmakePriFileNode;
                } else {
                    QmakeProFileNode *qmakeProFileNode = new QmakeProFileNode(m_project, nodeToAdd);
                    qmakeProFileNode->setParentFolderNode(this); // Needed for loop detection
                    qmakeProFileNode->setIncludedInExactParse(
                                result->exactSubdirs.contains(qmakeProFileNode->path())
                                && includedInExactParse());
                    qmakeProFileNode->asyncUpdate();
                    toAdd << qmakeProFileNode;
                }
            }
        }
    } // for

    foreach (ProjectNode *node, toRemove) {
        if (QmakeProFileNode *qmakeProFileNode = dynamic_cast<QmakeProFileNode *>(node)) {
            qmakeProFileNode->setValidParseRecursive(false);
            qmakeProFileNode->setParseInProgressRecursive(false);
        }
    }

    if (!toRemove.isEmpty())
        removeProjectNodes(toRemove);
    if (!toAdd.isEmpty())
        addProjectNodes(toAdd);

    foreach (QmakePriFileNode *qmakePriFileNode, toUpdate)
        qmakePriFileNode->update(result->priFileResults[qmakePriFileNode->path()]);

    QmakePriFileNode::update(result->priFileResults[m_projectFilePath]);

    m_validParse = (result->state == EvalResult::EvalOk);
    if (m_validParse) {
        // update TargetInformation
        m_qmakeTargetInformation = result->targetInformation;

        m_subProjectsNotToDeploy = result->subProjectsNotToDeploy;
        m_installsList = result->installsList;
        m_isDeployable = result->isDeployable;

        if (m_varValues != result->newVarValues)
            m_varValues = result->newVarValues;
    } // result == EvalOk

    setParseInProgress(false);

    updateUiFiles(buildDirectory);

    cleanupProFileReaders();
}

void QmakeProFileNode::cleanupProFileReaders()
{
    m_project->destroyProFileReader(m_readerExact);
    m_project->destroyProFileReader(m_readerCumulative);

    m_readerExact = 0;
    m_readerCumulative = 0;
}

QStringList QmakeProFileNode::fileListForVar(QtSupport::ProFileReader *readerExact, QtSupport::ProFileReader *readerCumulative,
                                           const QString &varName, const QString &projectDir, const QString &buildDir)
{
    QStringList baseVPathsExact = baseVPaths(readerExact, projectDir, buildDir);
    QStringList vPathsExact = fullVPaths(baseVPathsExact, readerExact, varName, projectDir);

    QStringList result;
    result = readerExact->absoluteFileValues(varName,
                                             projectDir,
                                             vPathsExact,
                                             0);
    if (readerCumulative) {
        QStringList baseVPathsCumulative = baseVPaths(readerCumulative, projectDir, buildDir);
        QStringList vPathsCumulative = fullVPaths(baseVPathsCumulative, readerCumulative, varName, projectDir);
        result += readerCumulative->absoluteFileValues(varName,
                                                       projectDir,
                                                       vPathsCumulative,
                                                       0);
    }
    result.removeDuplicates();
    return result;
}

QString QmakeProFileNode::uiDirPath(QtSupport::ProFileReader *reader, const QString &buildDir)
{
    QString path = reader->value(QLatin1String("UI_DIR"));
    if (QFileInfo(path).isRelative())
        path = QDir::cleanPath(buildDir + QLatin1Char('/') + path);
    return path;
}

QString QmakeProFileNode::mocDirPath(QtSupport::ProFileReader *reader, const QString &buildDir)
{
    QString path = reader->value(QLatin1String("MOC_DIR"));
    if (QFileInfo(path).isRelative())
        path = QDir::cleanPath(buildDir + QLatin1Char('/') + path);
    return path;
}

QStringList QmakeProFileNode::includePaths(QtSupport::ProFileReader *reader, const QString &buildDir, const QString &projectDir)
{
    QStringList paths;
    foreach (const QString &cxxflags, reader->values(QLatin1String("QMAKE_CXXFLAGS"))) {
        if (cxxflags.startsWith(QLatin1String("-I")))
            paths.append(cxxflags.mid(2));
    }

    paths.append(reader->absolutePathValues(QLatin1String("INCLUDEPATH"), projectDir));
    // paths already contains moc dir and ui dir, due to corrrectly parsing uic.prf and moc.prf
    // except if those directories don't exist at the time of parsing
    // thus we add those directories manually (without checking for existence)
    paths << mocDirPath(reader, buildDir) << uiDirPath(reader, buildDir);
    paths.removeDuplicates();
    return paths;
}

QStringList QmakeProFileNode::libDirectories(QtSupport::ProFileReader *reader)
{
    QStringList result;
    foreach (const QString &str, reader->values(QLatin1String("LIBS"))) {
        if (str.startsWith(QLatin1String("-L")))
            result.append(str.mid(2));
    }
    return result;
}

FileNameList QmakeProFileNode::subDirsPaths(QtSupport::ProFileReader *reader,
                                            const QString &projectDir,
                                            QStringList *subProjectsNotToDeploy,
                                            QStringList *errors)
{
    FileNameList subProjectPaths;

    const QStringList subDirVars = reader->values(QLatin1String("SUBDIRS"));

    foreach (const QString &subDirVar, subDirVars) {
        // Special case were subdir is just an identifier:
        //   "SUBDIR = subid
        //    subid.subdir = realdir"
        // or
        //   "SUBDIR = subid
        //    subid.file = realdir/realfile.pro"

        QString realDir;
        const QString subDirKey = subDirVar + QLatin1String(".subdir");
        const QString subDirFileKey = subDirVar + QLatin1String(".file");
        if (reader->contains(subDirKey))
            realDir = reader->value(subDirKey);
        else if (reader->contains(subDirFileKey))
            realDir = reader->value(subDirFileKey);
        else
            realDir = subDirVar;
        QFileInfo info(realDir);
        if (!info.isAbsolute())
            info.setFile(projectDir + QLatin1Char('/') + realDir);
        realDir = info.filePath();

        QString realFile;
        if (info.isDir())
            realFile = QString::fromLatin1("%1/%2.pro").arg(realDir, info.fileName());
        else
            realFile = realDir;

        if (QFile::exists(realFile)) {
            realFile = QDir::cleanPath(realFile);
            subProjectPaths << FileName::fromString(realFile);
            if (subProjectsNotToDeploy && !subProjectsNotToDeploy->contains(realFile)
                    && reader->values(subDirVar + QLatin1String(".CONFIG"))
                        .contains(QLatin1String("no_default_target"))) {
                subProjectsNotToDeploy->append(realFile);
            }
        } else {
            if (errors)
                errors->append(QCoreApplication::translate("QmakeProFileNode", "Could not find .pro file for subdirectory \"%1\" in \"%2\".")
                               .arg(subDirVar).arg(realDir));
        }
    }

    subProjectPaths.removeDuplicates();
    return subProjectPaths;
}

TargetInformation QmakeProFileNode::targetInformation(QtSupport::ProFileReader *reader, QtSupport::ProFileReader *readerBuildPass, const QString &buildDir, const QString &projectFilePath)
{
    TargetInformation result;
    if (!reader || !readerBuildPass)
        return result;

    QStringList builds = reader->values(QLatin1String("BUILDS"));
    if (!builds.isEmpty()) {
        QString build = builds.first();
        result.buildTarget = reader->value(build + QLatin1String(".target"));
    }

    // BUILD DIR
    result.buildDir = buildDir;

    if (readerBuildPass->contains(QLatin1String("DESTDIR")))
        result.destDir = readerBuildPass->value(QLatin1String("DESTDIR"));

    // Target
    result.target = readerBuildPass->value(QLatin1String("TARGET"));
    if (result.target.isEmpty())
        result.target = QFileInfo(projectFilePath).baseName();

    result.valid = true;

    return result;
}

TargetInformation QmakeProFileNode::targetInformation() const
{
    return m_qmakeTargetInformation;
}

InstallsList QmakeProFileNode::installsList(const QtSupport::ProFileReader *reader, const QString &projectFilePath, const QString &projectDir)
{
    InstallsList result;
    if (!reader)
        return result;
    const QStringList &itemList = reader->values(QLatin1String("INSTALLS"));
    foreach (const QString &item, itemList) {
        if (reader->values(item + QLatin1String(".CONFIG")).contains(QLatin1String("no_default_install")))
            continue;
        QString itemPath;
        const QString pathVar = item + QLatin1String(".path");
        const QStringList &itemPaths = reader->values(pathVar);
        if (itemPaths.count() != 1) {
            qDebug("Invalid RHS: Variable '%s' has %d values.",
                qPrintable(pathVar), itemPaths.count());
            if (itemPaths.isEmpty()) {
                qDebug("%s: Ignoring INSTALLS item '%s', because it has no path.",
                    qPrintable(projectFilePath), qPrintable(item));
                continue;
            }
        }
        itemPath = itemPaths.last();

        const QStringList &itemFiles
            = reader->absoluteFileValues(item + QLatin1String(".files"),
                  projectDir, QStringList() << projectDir, 0);
        if (item == QLatin1String("target")) {
            result.targetPath = itemPath;
        } else {
            if (itemFiles.isEmpty()) {
                // TODO: Fix QMAKE_SUBSTITUTES handling in pro file reader, then uncomment again
//                if (!reader->values(item + QLatin1String(".CONFIG"))
//                    .contains(QLatin1String("no_check_exist"))) {
//                    qDebug("%s: Ignoring INSTALLS item '%s', because it has no files.",
//                        qPrintable(m_projectFilePath), qPrintable(item));
//                }
                continue;
            }
            result.items << InstallsItem(itemPath, itemFiles);
        }
    }
    return result;
}

InstallsList QmakeProFileNode::installsList() const
{
    return m_installsList;
}

QString QmakeProFileNode::sourceDir() const
{
    return m_projectDir;
}

QString QmakeProFileNode::buildDir(QmakeBuildConfiguration *bc) const
{
    const QDir srcDirRoot = m_project->rootQmakeProjectNode()->sourceDir();
    const QString relativeDir = srcDirRoot.relativeFilePath(m_projectDir);
    if (!bc && m_project->activeTarget())
        bc = static_cast<QmakeBuildConfiguration *>(m_project->activeTarget()->activeBuildConfiguration());
    if (!bc)
        return QString();
    return QDir::cleanPath(QDir(bc->buildDirectory().toString()).absoluteFilePath(relativeDir));
}

QString QmakeProFileNode::uiDirectory(const QString &buildDir) const
{
    const QmakeVariablesHash::const_iterator it = m_varValues.constFind(UiDirVar);
    if (it != m_varValues.constEnd() && !it.value().isEmpty())
        return it.value().front();
    return buildDir;
}

QString QmakeProFileNode::uiHeaderFile(const QString &uiDir, const FileName &formFile)
{
    QString uiHeaderFilePath = uiDir;
    uiHeaderFilePath += QLatin1String("/ui_");
    uiHeaderFilePath += formFile.toFileInfo().completeBaseName();
    uiHeaderFilePath += QLatin1String(".h");
    return QDir::cleanPath(uiHeaderFilePath);
}

void QmakeProFileNode::updateUiFiles(const QString &buildDir)
{
    m_uiFiles.clear();

    // Only those two project types can have ui files for us
    if (m_projectType == ApplicationTemplate ||
            m_projectType == SharedLibraryTemplate ||
            m_projectType == StaticLibraryTemplate) {
        // Find all ui files
        FindUiFileNodesVisitor uiFilesVisitor;
        this->accept(&uiFilesVisitor);
        const QList<FileNode*> uiFiles = uiFilesVisitor.uiFileNodes;

        // Find the UiDir, there can only ever be one
        const  QString uiDir = uiDirectory(buildDir);
        foreach (const FileNode *uiFile, uiFiles)
            m_uiFiles.insert(uiFile->path().toString(), uiHeaderFile(uiDir, uiFile->path()));
    }
}
