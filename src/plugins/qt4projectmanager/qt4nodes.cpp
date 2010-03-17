/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "profilereader.h"
#include "prowriter.h"
#include "qt4nodes.h"
#include "qt4project.h"
#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "qtuicodemodelsupport.h"
#include "qt4buildconfiguration.h"

#include <projectexplorer/nodesvisitor.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <cpptools/cppmodelmanagerinterface.h>
#include <cplusplus/CppDocument.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>

#include <QtGui/QPainter>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <qtconcurrent/QtConcurrentTools>

// Static cached data in struct Qt4NodeStaticData providing information and icons
// for file types and the project. Do some magic via qAddPostRoutine()
// to make sure the icons do not outlive QApplication, triggering warnings on X11.

struct FileTypeDataStorage {
    ProjectExplorer::FileType type;
    const char *typeName;
    const char *icon;
};

static const FileTypeDataStorage fileTypeDataStorage[] = {
    { ProjectExplorer::HeaderType,
      QT_TRANSLATE_NOOP("Qt4ProjectManager::Internal::Qt4PriFileNode", "Headers"),
      ":/qt4projectmanager/images/headers.png" },
    { ProjectExplorer::SourceType,
      QT_TRANSLATE_NOOP("Qt4ProjectManager::Internal::Qt4PriFileNode", "Sources"),
      ":/qt4projectmanager/images/sources.png" },
    { ProjectExplorer::FormType,
      QT_TRANSLATE_NOOP("Qt4ProjectManager::Internal::Qt4PriFileNode", "Forms"),
      ":/qt4projectmanager/images/forms.png" },
    { ProjectExplorer::ResourceType,
      QT_TRANSLATE_NOOP("Qt4ProjectManager::Internal::Qt4PriFileNode", "Resources"),
      ":/qt4projectmanager/images/qt_qrc.png" },
    { ProjectExplorer::UnknownFileType,
      QT_TRANSLATE_NOOP("Qt4ProjectManager::Internal::Qt4PriFileNode", "Other files"),
      ":/qt4projectmanager/images/unknown.png" }
};

struct Qt4NodeStaticData {
    struct FileTypeData {
        FileTypeData(ProjectExplorer::FileType t = ProjectExplorer::UnknownFileType,
                     const QString &tN = QString(),
                     const QIcon &i = QIcon()) :
        type(t), typeName(tN), icon(i) { }

        ProjectExplorer::FileType type;
        QString typeName;
        QIcon icon;
    };

    QVector<FileTypeData> fileTypeData;
    QIcon projectIcon;
};

static void clearQt4NodeStaticData();

Q_GLOBAL_STATIC_WITH_INITIALIZER(Qt4NodeStaticData, qt4NodeStaticData, {
    // File type data
    const unsigned count = sizeof(fileTypeDataStorage)/sizeof(FileTypeDataStorage);
    x->fileTypeData.reserve(count);

    // Overlay the SP_DirIcon with the custom icons
    const QSize desiredSize = QSize(16, 16);

    for (unsigned i = 0 ; i < count; i++) {
        const QIcon overlayIcon = QIcon(QLatin1String(fileTypeDataStorage[i].icon));
        const QPixmap folderPixmap =
                Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon,
                                                    overlayIcon, desiredSize);
        QIcon folderIcon;
        folderIcon.addPixmap(folderPixmap);
        const QString desc = Qt4ProjectManager::Internal::Qt4PriFileNode::tr(fileTypeDataStorage[i].typeName);
        x->fileTypeData.push_back(Qt4NodeStaticData::FileTypeData(fileTypeDataStorage[i].type,
                                                                  desc, folderIcon));
    }
    // Project icon
    const QIcon projectBaseIcon(QLatin1String(":/qt4projectmanager/images/qt_project.png"));
    const QPixmap projectPixmap = Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon,
                                                                      projectBaseIcon,
                                                                      desiredSize);
    x->projectIcon.addPixmap(projectPixmap);

    qAddPostRoutine(clearQt4NodeStaticData);
});

static void clearQt4NodeStaticData()
{
    qt4NodeStaticData()->fileTypeData.clear();
    qt4NodeStaticData()->projectIcon = QIcon();
}

enum { debug = 0 };

namespace {
    // sorting helper function
    bool sortProjectFilesByPath(ProFile *f1, ProFile *f2)
    {
        return f1->fileName() < f2->fileName();
    }
}

namespace Qt4ProjectManager {
namespace Internal {

Qt4PriFile::Qt4PriFile(Qt4PriFileNode *qt4PriFile)
    : IFile(qt4PriFile), m_priFile(qt4PriFile)
{

}

bool Qt4PriFile::save(const QString &fileName)
{
    Q_UNUSED(fileName);
    return false;
}

QString Qt4PriFile::fileName() const
{
    return m_priFile->path();
}

QString Qt4PriFile::defaultPath() const
{
    return QString();
}

QString Qt4PriFile::suggestedFileName() const
{
    return QString();
}

QString Qt4PriFile::mimeType() const
{
    return Qt4ProjectManager::Constants::PROFILE_MIMETYPE;
}

bool Qt4PriFile::isModified() const
{
    return false;
}

bool Qt4PriFile::isReadOnly() const
{
    return false;
}

bool Qt4PriFile::isSaveAsAllowed() const
{
    return false;
}

void Qt4PriFile::modified(Core::IFile::ReloadBehavior *behavior)
{
    Q_UNUSED(behavior);
    m_priFile->scheduleUpdate();
}


/*!
  \class Qt4PriFileNode
  Implements abstract ProjectNode class
  */

Qt4PriFileNode::Qt4PriFileNode(Qt4Project *project, Qt4ProFileNode* qt4ProFileNode, const QString &filePath)
        : ProjectNode(filePath),
          m_project(project),
          m_qt4ProFileNode(qt4ProFileNode),
          m_projectFilePath(QDir::fromNativeSeparators(filePath)),
          m_projectDir(QFileInfo(filePath).absolutePath())
{
    Q_ASSERT(project);
    m_qt4PriFile = new Qt4PriFile(this);
    Core::ICore::instance()->fileManager()->addFile(m_qt4PriFile);

    setDisplayName(QFileInfo(filePath).completeBaseName());

    setIcon(qt4NodeStaticData()->projectIcon);
}

void Qt4PriFileNode::scheduleUpdate()
{
    ProFileCacheManager::instance()->discardFile(m_projectFilePath);
    m_qt4ProFileNode->scheduleUpdate();
}

struct InternalNode
{
    QMap<QString, InternalNode*> subnodes;
    QStringList files;
    ProjectExplorer::FileType type;
    QString fullPath;
    QIcon icon;

    InternalNode()
    {
        type = ProjectExplorer::UnknownFileType;
    }

    ~InternalNode()
    {
        qDeleteAll(subnodes);
    }

    // Creates a tree structure from a list of absolute file paths.
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
    // The method first creates a tree that looks like the directory structure, i.e.
    //    * /
    //       * absolute
    //          * path
    // ...
    // and afterwards calls compress() which merges directory nodes with single children, i.e. to
    //    * /absolute/path
    void create(const QString &projectDir, const QStringList &newFilePaths, ProjectExplorer::FileType type)
    {
        static const QChar separator = QChar('/');
        const QString projectDirWithSeparator = projectDir + separator;
        int projectDirWithSeparatorLength = projectDirWithSeparator.length();
        foreach (const QString &file, newFilePaths) {
            QString fileWithoutPrefix;
            bool isRelative;
            if (file.startsWith(projectDirWithSeparator)) {
                isRelative = true;
                fileWithoutPrefix = file.mid(projectDirWithSeparatorLength);
            } else {
                isRelative = false;
                fileWithoutPrefix = file;
            }
            QStringList parts = fileWithoutPrefix.split(separator, QString::SkipEmptyParts);
#ifndef Q_OS_WIN
            if (!isRelative && parts.count() > 0)
                parts[0].prepend(separator);
#endif
            QStringListIterator it(parts);
            InternalNode *currentNode = this;
            QString path = (isRelative ? projectDir : "");
            while (it.hasNext()) {
                const QString &key = it.next();
                if (it.hasNext()) { // key is directory
                    path += key;
                    if (!currentNode->subnodes.contains(key)) {
                        InternalNode *val = new InternalNode;
                        val->type = type;
                        val->fullPath = path;
                        currentNode->subnodes.insert(key, val);
                        currentNode = val;
                    } else {
                        currentNode = currentNode->subnodes.value(key);
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
        static const QChar separator = QDir::separator(); // it is used for the *keys* which will become display name
        QMap<QString, InternalNode*> newSubnodes;
        QMapIterator<QString, InternalNode*> i(subnodes);
        while (i.hasNext()) {
            i.next();
            i.value()->compress();
            if (i.value()->files.isEmpty() && i.value()->subnodes.size() == 1) {
                QString key = i.value()->subnodes.begin().key();
                newSubnodes.insert(i.key()+separator+key, i.value()->subnodes.value(key));
                i.value()->subnodes.clear();
                delete i.value();
            } else {
                newSubnodes.insert(i.key(), i.value());
            }
        }
        subnodes = newSubnodes;
    }

    // Makes the projectNode's subtree below the given folder match this internal node's subtree
    void updateSubFolders(Qt4PriFileNode *projectNode, ProjectExplorer::FolderNode *folder)
    {
        updateFiles(projectNode, folder, type);

        // update folders
        QList<FolderNode *> existingFolderNodes;
        foreach (FolderNode *node, folder->subFolderNodes()) {
            if (node->nodeType() != ProjectNodeType)
                existingFolderNodes << node;
        }

        QList<FolderNode *> foldersToRemove;
        QList<FolderNode *> foldersToAdd;
        typedef QPair<InternalNode *, FolderNode *> NodePair;
        QList<NodePair> nodesToUpdate;

        // newFolders is already sorted
        qSort(existingFolderNodes.begin(), existingFolderNodes.end(), ProjectNode::sortFolderNodesByName);

        QList<FolderNode*>::const_iterator existingNodeIter = existingFolderNodes.constBegin();
        QMap<QString, InternalNode*>::const_iterator newNodeIter = subnodes.constBegin();;
        while (existingNodeIter != existingFolderNodes.constEnd()
               && newNodeIter != subnodes.constEnd()) {
            if ((*existingNodeIter)->displayName() < newNodeIter.key()) {
                foldersToRemove << *existingNodeIter;
                ++existingNodeIter;
            } else if ((*existingNodeIter)->displayName() > newNodeIter.key()) {
                FolderNode *newNode = new FolderNode(newNodeIter.value()->fullPath);
                newNode->setDisplayName(newNodeIter.key());
                if (!newNodeIter.value()->icon.isNull())
                    newNode->setIcon(newNodeIter.value()->icon);
                foldersToAdd << newNode;
                nodesToUpdate << NodePair(newNodeIter.value(), newNode);
                ++newNodeIter;
            } else { // *existingNodeIter->path() == *newPathIter
                nodesToUpdate << NodePair(newNodeIter.value(), *existingNodeIter);
                ++existingNodeIter;
                ++newNodeIter;
            }
        }

        while (existingNodeIter != existingFolderNodes.constEnd()) {
            foldersToRemove << *existingNodeIter;
            ++existingNodeIter;
        }
        while (newNodeIter != subnodes.constEnd()) {
            FolderNode *newNode = new FolderNode(newNodeIter.value()->fullPath);
            newNode->setDisplayName(newNodeIter.key());
            if (!newNodeIter.value()->icon.isNull())
                newNode->setIcon(newNodeIter.value()->icon);
            foldersToAdd << newNode;
            nodesToUpdate << NodePair(newNodeIter.value(), newNode);
            ++newNodeIter;
        }

        if (!foldersToRemove.isEmpty())
            projectNode->removeFolderNodes(foldersToRemove, folder);
        if (!foldersToAdd.isEmpty())
            projectNode->addFolderNodes(foldersToAdd, folder);

        foreach (const NodePair &np, nodesToUpdate)
            np.first->updateSubFolders(projectNode, np.second);
    }

    // Makes the folder's files match this internal node's file list
    void updateFiles(Qt4PriFileNode *projectNode, FolderNode *folder, FileType type)
    {
        QList<FileNode*> existingFileNodes;
        foreach (FileNode *fileNode, folder->fileNodes()) {
            if (fileNode->fileType() == type && !fileNode->isGenerated())
                existingFileNodes << fileNode;
        }

        QList<FileNode*> filesToRemove;
        QList<FileNode*> filesToAdd;

        qSort(files);
        qSort(existingFileNodes.begin(), existingFileNodes.end(), ProjectNode::sortNodesByPath);

        QList<FileNode*>::const_iterator existingNodeIter = existingFileNodes.constBegin();
        QList<QString>::const_iterator newPathIter = files.constBegin();
        while (existingNodeIter != existingFileNodes.constEnd()
               && newPathIter != files.constEnd()) {
            if ((*existingNodeIter)->path() < *newPathIter) {
                filesToRemove << *existingNodeIter;
                ++existingNodeIter;
            } else if ((*existingNodeIter)->path() > *newPathIter) {
                filesToAdd << new FileNode(*newPathIter, type, false);
                ++newPathIter;
            } else { // *existingNodeIter->path() == *newPathIter
                ++existingNodeIter;
                ++newPathIter;
            }
        }
        while (existingNodeIter != existingFileNodes.constEnd()) {
            filesToRemove << *existingNodeIter;
            ++existingNodeIter;
        }
        while (newPathIter != files.constEnd()) {
            filesToAdd << new FileNode(*newPathIter, type, false);
            ++newPathIter;
        }

        if (!filesToRemove.isEmpty())
            projectNode->removeFileNodes(filesToRemove, folder);
        if (!filesToAdd.isEmpty())
            projectNode->addFileNodes(filesToAdd, folder);
    }
};


QStringList Qt4PriFileNode::baseVPaths(ProFileReader *reader, const QString &projectDir)
{
    QStringList result;
    if (!reader)
        return result;
    result += reader->absolutePathValues("VPATH", projectDir);
    result << projectDir; // QMAKE_ABSOLUTE_SOURCE_PATH
    result += reader->absolutePathValues("DEPENDPATH", projectDir);
    result.removeDuplicates();
    return result;
}

QStringList Qt4PriFileNode::fullVPaths(const QStringList &baseVPaths, ProFileReader *reader, FileType type, const QString &qmakeVariable, const QString &projectDir)
{
    QStringList vPaths;
    if (!reader)
        return vPaths;
    if (type == ProjectExplorer::SourceType)
        vPaths = reader->absolutePathValues("VPATH_" + qmakeVariable, projectDir);
    vPaths += baseVPaths;
    if (type == ProjectExplorer::HeaderType)
        vPaths += reader->absolutePathValues("INCLUDEPATH", projectDir);
    vPaths.removeDuplicates();
    return vPaths;
}


void Qt4PriFileNode::update(ProFile *includeFileExact, ProFileReader *readerExact, ProFile *includeFileCumlative, ProFileReader *readerCumulative)
{
    // add project file node
    if (m_fileNodes.isEmpty())
        addFileNodes(QList<FileNode*>() << new FileNode(m_projectFilePath, ProjectFileType, false), this);

    const QString &projectDir = m_qt4ProFileNode->m_projectDir;

    QStringList baseVPathsExact = baseVPaths(readerExact, projectDir);
    QStringList baseVPathsCumulative = baseVPaths(readerCumulative, projectDir);

    const QVector<Qt4NodeStaticData::FileTypeData> &fileTypes = qt4NodeStaticData()->fileTypeData;

    InternalNode contents;

    // update files
    for (int i = 0; i < fileTypes.size(); ++i) {
        FileType type = fileTypes.at(i).type;
        const QStringList qmakeVariables = varNames(type);

        QStringList newFilePaths;
        foreach (const QString &qmakeVariable, qmakeVariables) {
            QStringList vPathsExact = fullVPaths(baseVPathsExact, readerExact, type, qmakeVariable, projectDir);
            QStringList vPathsCumulative = fullVPaths(baseVPathsCumulative, readerCumulative, type, qmakeVariable, projectDir);


            newFilePaths += readerExact->absoluteFileValues(qmakeVariable, projectDir, vPathsExact, includeFileExact);
            if (readerCumulative)
                newFilePaths += readerCumulative->absoluteFileValues(qmakeVariable, projectDir, vPathsCumulative, includeFileCumlative);

        }
        newFilePaths.removeDuplicates();

        if (!newFilePaths.isEmpty()) {
            InternalNode *subfolder = new InternalNode;
            subfolder->type = type;
            subfolder->icon = fileTypes.at(i).icon;
            subfolder->fullPath = m_projectDir + "/#" + fileTypes.at(i).typeName;
            contents.subnodes.insert(fileTypes.at(i).typeName, subfolder);
            // create the hierarchy with subdirectories
            subfolder->create(m_projectDir, newFilePaths, type);
        }
    }
    contents.updateSubFolders(this, this);
}

QList<ProjectNode::ProjectAction> Qt4PriFileNode::supportedActions() const
{
    QList<ProjectAction> actions;

    const FolderNode *folderNode = this;
    const Qt4ProFileNode *proFileNode;
    while (!(proFileNode = qobject_cast<const Qt4ProFileNode*>(folderNode)))
        folderNode = folderNode->parentFolderNode();
    Q_ASSERT(proFileNode);

    switch (proFileNode->projectType()) {
    case ApplicationTemplate:
    case LibraryTemplate:
        actions << AddFile << RemoveFile;
        break;
    case SubDirsTemplate:
        actions << AddSubProject << RemoveSubProject;
        break;
    default:
        break;
    }
    return actions;
}

bool Qt4PriFileNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false; //changeIncludes(m_includeFile, proFilePaths, AddToProFile);
}

bool Qt4PriFileNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false; //changeIncludes(m_includeFile, proFilePaths, RemoveFromProFile);
}

bool Qt4PriFileNode::addFiles(const FileType fileType, const QStringList &filePaths,
                           QStringList *notAdded)
{
    QStringList failedFiles;

    changeFiles(fileType, filePaths, &failedFiles, AddToProFile);
    if (notAdded)
        *notAdded = failedFiles;
    return failedFiles.isEmpty();
}

bool Qt4PriFileNode::removeFiles(const FileType fileType, const QStringList &filePaths,
                              QStringList *notRemoved)
{
    QStringList failedFiles;
    changeFiles(fileType, filePaths, &failedFiles, RemoveFromProFile);
    if (notRemoved)
        *notRemoved = failedFiles;
    return failedFiles.isEmpty();
}

bool Qt4PriFileNode::renameFile(const FileType fileType, const QString &filePath,
                             const QString &newFilePath)
{
    if (newFilePath.isEmpty())
        return false;

    if (!QFile::rename(filePath, newFilePath))
        return false;

    QStringList dummy;
    changeFiles(fileType, QStringList() << filePath, &dummy, RemoveFromProFile);
    if (!dummy.isEmpty())
        return false;
    changeFiles(fileType, QStringList() << newFilePath, &dummy, AddToProFile);
    if (!dummy.isEmpty())
        return false;
    return true;
}

bool Qt4PriFileNode::changeIncludes(ProFile *includeFile, const QStringList &proFilePaths,
                                    ChangeType change)
{
    Q_UNUSED(includeFile)
    Q_UNUSED(proFilePaths)
    Q_UNUSED(change)
    // TODO
    return false;
}

bool Qt4PriFileNode::priFileWritable(const QString &path)
{
    const QString dir = QFileInfo(path).dir().path();
    Core::ICore *core = Core::ICore::instance();
    Core::IVersionControl *versionControl = core->vcsManager()->findVersionControlForDirectory(dir);
    switch (Core::EditorManager::promptReadOnlyFile(path, versionControl, core->mainWindow(), false)) {
    case Core::EditorManager::RO_OpenVCS:
        if (!versionControl->vcsOpen(path)) {
            QMessageBox::warning(core->mainWindow(), tr("Failed!"), tr("Could not open the file for edit with SCC."));
            return false;
        }
        break;
    case Core::EditorManager::RO_MakeWriteable: {
        const bool permsOk = QFile::setPermissions(path, QFile::permissions(path) | QFile::WriteUser);
        if (!permsOk) {
            QMessageBox::warning(core->mainWindow(), tr("Failed!"),  tr("Could not set permissions to writable."));
            return false;
        }
        break;
    }
    case Core::EditorManager::RO_SaveAs:
    case Core::EditorManager::RO_Cancel:
        return false;
    }
    return true;
}

bool Qt4PriFileNode::saveModifiedEditors()
{
    QList<Core::IFile*> modifiedFileHandles;

    Core::ICore *core = Core::ICore::instance();

    foreach (Core::IEditor *editor, core->editorManager()->editorsForFileName(m_projectFilePath)) {
        if (Core::IFile *editorFile = editor->file()) {
            if (editorFile->isModified())
                modifiedFileHandles << editorFile;
        }
    }

    if (!modifiedFileHandles.isEmpty()) {
        bool cancelled;
        core->fileManager()->saveModifiedFiles(modifiedFileHandles, &cancelled,
                                         tr("There are unsaved changes for project file %1.").arg(m_projectFilePath));
        if (cancelled)
            return false;
        // force instant reload of ourselves
        ProFileCacheManager::instance()->discardFile(m_projectFilePath);
        m_project->qt4ProjectManager()->notifyChanged(m_projectFilePath);
    }
    return true;
}

void Qt4PriFileNode::changeFiles(const FileType fileType,
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

    QStringList lines;
    ProFile *includeFile;
    {
        QString contents;
        {
            QFile qfile(m_projectFilePath);
            if (qfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                contents = QString::fromLatin1(qfile.readAll()); // yes, really latin1
                qfile.close();
                lines = contents.split(QLatin1Char('\n'));
                while (!lines.isEmpty() && lines.last().isEmpty())
                    lines.removeLast();
            } else {
                m_project->proFileParseError(tr("Error while reading PRO file %1: %2")
                                             .arg(m_projectFilePath, qfile.errorString()));
                return;
            }
        }

        ProFileReader *reader = m_project->createProFileReader(m_qt4ProFileNode);
        includeFile = reader->parsedProFile(m_projectFilePath, contents);
        m_project->destroyProFileReader(reader);
    }

    const QStringList vars = varNames(fileType);
    QDir priFileDir = QDir(m_qt4ProFileNode->m_projectDir);

    if (change == AddToProFile) {
        ProWriter::addFiles(includeFile, &lines, priFileDir, filePaths, vars);
        notChanged->clear();
    } else { // RemoveFromProFile
        *notChanged = ProWriter::removeFiles(includeFile, &lines, priFileDir, filePaths, vars);
    }

    // save file
    save(lines);

    // This is a hack
    // We are savign twice in a very short timeframe, once the editor and once the ProFile
    // So the modification time might not change between those two saves
    // We manually tell each editor to reload it's file
    // (The .pro files are notified by the file system watcher)
    foreach (Core::IEditor *editor, Core::ICore::instance()->editorManager()->editorsForFileName(m_projectFilePath)) {
        if (Core::IFile *editorFile = editor->file()) {
            Core::IFile::ReloadBehavior b = Core::IFile::ReloadUnmodified;
            editorFile->modified(&b);
        }
    }

    includeFile->deref();
}

void Qt4PriFileNode::save(const QStringList &lines)
{
    QFile qfile(m_projectFilePath);
    if (qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        foreach (const QString &str, lines) {
            qfile.write(str.toLatin1()); // yes, really latin1
            qfile.write("\n");
        }
        qfile.close();
    }

    m_project->qt4ProjectManager()->notifyChanged(m_projectFilePath);
}

/*
  Deletes all subprojects/files/virtual folders
  */
void Qt4PriFileNode::clear()
{
    // delete files && folders && projects
    removeFileNodes(fileNodes(), this);
    removeProjectNodes(subProjectNodes());
    removeFolderNodes(subFolderNodes(), this);
}

QStringList Qt4PriFileNode::varNames(FileType type)
{
    QStringList vars;
    switch (type) {
    case ProjectExplorer::HeaderType:
        vars << QLatin1String("HEADERS");
        vars << QLatin1String("OBJECTIVE_HEADERS");
        break;
    case ProjectExplorer::SourceType:
        vars << QLatin1String("SOURCES");
        vars << QLatin1String("OBJECTIVE_SOURCES");
        vars << QLatin1String("LEXSOURCES");
        vars << QLatin1String("YACCSOURCES");
        break;
    case ProjectExplorer::ResourceType:
        vars << QLatin1String("RESOURCES");
        break;
    case ProjectExplorer::FormType:
        vars << QLatin1String("FORMS");
        break;
    default:
        vars << QLatin1String("OTHER_FILES");
        break;
    }
    return vars;
}

Qt4ProFileNode *Qt4ProFileNode::findProFileFor(const QString &fileName)
{
    if (fileName == path())
        return this;
    foreach (ProjectNode *pn, subProjectNodes())
        if (Qt4ProFileNode *qt4ProFileNode = qobject_cast<Qt4ProFileNode *>(pn))
            if (Qt4ProFileNode *result = qt4ProFileNode->findProFileFor(fileName))
                return result;
    return 0;
}

TargetInformation Qt4ProFileNode::targetInformation(const QString &fileName)
{
    TargetInformation result;
    Qt4ProFileNode *qt4ProFileNode = findProFileFor(fileName);
    if (!qt4ProFileNode)
        return result;

    return qt4ProFileNode->targetInformation();
}

/*!
  \class Qt4ProFileNode
  Implements abstract ProjectNode class
  */
Qt4ProFileNode::Qt4ProFileNode(Qt4Project *project,
                               const QString &filePath,
                               QObject *parent)
        : Qt4PriFileNode(project, this, filePath),
          m_projectType(InvalidProject),
          m_readerExact(0),
          m_readerCumulative(0)
{

    if (parent)
        setParent(parent);

    connect(ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));

    connect(&m_parseFutureWatcher, SIGNAL(finished()),
            this, SLOT(applyAsyncEvaluate()));
}

Qt4ProFileNode::~Qt4ProFileNode()
{
    CppTools::CppModelManagerInterface *modelManager
            = ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();
    QMap<QString, Qt4UiCodeModelSupport *>::const_iterator it, end;
    end = m_uiCodeModelSupport.constEnd();
    for (it = m_uiCodeModelSupport.constBegin(); it != end; ++it) {
        modelManager->removeEditorSupport(it.value());
        delete it.value();
    }
    m_parseFutureWatcher.waitForFinished();
    if (m_readerExact) {
        // Oh we need to clean up
        applyEvaluate(true, true);
        m_project->decrementPendingEvaluateFutures();
    }
}

bool Qt4ProFileNode::isParent(Qt4ProFileNode *node)
{
    while ((node = qobject_cast<Qt4ProFileNode *>(node->parentFolderNode()))) {
        if (node == this)
            return true;
    }
    return false;
}

void Qt4ProFileNode::buildStateChanged(ProjectExplorer::Project *project)
{
    if (project == m_project && !ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager()->isBuilding(m_project)) {
        QStringList filesToUpdate = updateUiFiles();
        updateCodeModelSupportFromBuild(filesToUpdate);
    }
}

bool Qt4ProFileNode::hasBuildTargets() const
{
    return (projectType() == ApplicationTemplate) || (projectType() == LibraryTemplate);
}

Qt4ProjectType Qt4ProFileNode::projectType() const
{
    return m_projectType;
}

QStringList Qt4ProFileNode::variableValue(const Qt4Variable var) const
{
    return m_varValues.value(var);
}

void Qt4ProFileNode::scheduleUpdate()
{
    m_project->scheduleAsyncUpdate(this);
}

void Qt4ProFileNode::asyncUpdate()
{
    m_project->incrementPendingEvaluateFutures();
    setupReader();
    QFuture<bool> future = QtConcurrent::run(&Qt4ProFileNode::asyncEvaluate, this);
    m_parseFutureWatcher.setFuture(future);
}

void Qt4ProFileNode::update()
{
    setupReader();
    bool parserError = evaluate();
    applyEvaluate(!parserError, false);
}

void Qt4ProFileNode::setupReader()
{
    Q_ASSERT(!m_readerExact);
    Q_ASSERT(!m_readerCumulative);

    m_readerExact = m_project->createProFileReader(this);
    m_readerExact->setCumulative(false);

    m_readerCumulative = m_project->createProFileReader(this);

    // Find out what flags we pass on to qmake
    QStringList addedUserConfigArguments;
    QStringList removedUserConfigArguments;
    m_project->activeTarget()->activeBuildConfiguration()->getConfigCommandLineArguments(&addedUserConfigArguments, &removedUserConfigArguments);

    m_readerExact->setConfigCommandLineArguments(addedUserConfigArguments, removedUserConfigArguments);
    m_readerCumulative->setConfigCommandLineArguments(addedUserConfigArguments, removedUserConfigArguments);
}

bool Qt4ProFileNode::evaluate()
{
    bool parserError = false;
    if (!m_readerExact->readProFile(m_projectFilePath)) {
        m_project->proFileParseError(tr("Error while parsing file %1. Giving up.").arg(m_projectFilePath));
        parserError = true;
    }

    if (!m_readerCumulative->readProFile(m_projectFilePath)) {
        m_project->proFileParseError(tr("Error while parsing file %1. Giving up.").arg(m_projectFilePath));
        parserError = true;
    }
    return parserError;
}

void Qt4ProFileNode::asyncEvaluate(QFutureInterface<bool> &fi)
{
    bool parserError = evaluate();
    fi.reportResult(!parserError);
}

void Qt4ProFileNode::applyAsyncEvaluate()
{
    applyEvaluate(m_parseFutureWatcher.result(), true);
    m_project->decrementPendingEvaluateFutures();
}

Qt4ProjectType proFileTemplateTypeToProjectType(ProFileEvaluator::TemplateType type)
{
    switch (type) {
    case ProFileEvaluator::TT_Unknown:
    case ProFileEvaluator::TT_Application:
        return ApplicationTemplate;
    case ProFileEvaluator::TT_Library:
        return LibraryTemplate;
    case ProFileEvaluator::TT_Script:
        return ScriptTemplate;
    case ProFileEvaluator::TT_Subdirs:
        return SubDirsTemplate;
    default:
        return InvalidProject;
    }
}

void Qt4ProFileNode::applyEvaluate(bool parseResult, bool async)
{
    if (!m_readerExact)
        return;
    if (!parseResult || m_project->wasEvaluateCanceled()) {
        m_project->destroyProFileReader(m_readerExact);
        if (m_readerCumulative)
            m_project->destroyProFileReader(m_readerCumulative);
        m_readerExact = m_readerCumulative = 0;
        if (!parseResult) // Invalidate
            invalidate();
        return;
    }

    if (debug)
        qDebug() << "Qt4ProFileNode - updating files for file " << m_projectFilePath;

    Qt4ProjectType projectType = InvalidProject;
    // Check that both are the same if we have both
    if (m_readerExact->templateType() != m_readerCumulative->templateType()) {
        // Now what. The only thing which could be reasonable is that someone
        // changes between template app and library.
        // Well, we are conservative here for now.
        // Let's wait until someone complains and look at what they are doing.
        m_project->destroyProFileReader(m_readerCumulative);
        m_readerCumulative = 0;
    }

    projectType = proFileTemplateTypeToProjectType(m_readerExact->templateType());

    if (projectType != m_projectType) {
        Qt4ProjectType oldType = m_projectType;
        // probably all subfiles/projects have changed anyway ...
        clear();
        m_projectType = projectType;
        // really emit here? or at the end? Noone is connected to this signal at the moment
        // so we kind of can ignore that question for now
        foreach (NodesWatcher *watcher, watchers())
            if (Qt4NodesWatcher *qt4Watcher = qobject_cast<Qt4NodesWatcher*>(watcher))
                emit qt4Watcher->projectTypeChanged(this, oldType, projectType);
    }

    //
    // Add/Remove pri files, sub projects
    //

    QList<ProjectNode*> existingProjectNodes = subProjectNodes();

    QStringList newProjectFilesExact;
    QHash<QString, ProFile*> includeFilesExact;
    ProFile *fileForCurrentProjectExact = 0;
    if (m_projectType == SubDirsTemplate)
        newProjectFilesExact = subDirsPaths(m_readerExact);
    foreach (ProFile *includeFile, m_readerExact->includeFiles()) {
        if (includeFile->fileName() == m_projectFilePath) { // this file
            fileForCurrentProjectExact = includeFile;
        } else {
            newProjectFilesExact << includeFile->fileName();
            includeFilesExact.insert(includeFile->fileName(), includeFile);
        }
    }


    QStringList newProjectFilesCumlative;
    QHash<QString, ProFile*> includeFilesCumlative;
    ProFile *fileForCurrentProjectCumlative = 0;
    if (m_readerCumulative) {
        if (m_projectType == SubDirsTemplate)
            newProjectFilesCumlative = subDirsPaths(m_readerCumulative);
        foreach (ProFile *includeFile, m_readerCumulative->includeFiles()) {
            if (includeFile->fileName() == m_projectFilePath) {
                fileForCurrentProjectCumlative = includeFile;
            } else {
                newProjectFilesCumlative << includeFile->fileName();
                includeFilesCumlative.insert(includeFile->fileName(), includeFile);
            }
        }
    }

    qSort(existingProjectNodes.begin(), existingProjectNodes.end(),
          sortNodesByPath);
    qSort(newProjectFilesExact);
    qSort(newProjectFilesCumlative);

    QList<ProjectNode*> toAdd;
    QList<ProjectNode*> toRemove;

    QList<ProjectNode*>::const_iterator existingIt = existingProjectNodes.constBegin();
    QStringList::const_iterator newExactIt = newProjectFilesExact.constBegin();
    QStringList::const_iterator newCumlativeIt = newProjectFilesCumlative.constBegin();

    forever {
        bool existingAtEnd = (existingIt == existingProjectNodes.constEnd());
        bool newExactAtEnd = (newExactIt == newProjectFilesExact.constEnd());
        bool newCumlativeAtEnd = (newCumlativeIt == newProjectFilesCumlative.constEnd());

        if (existingAtEnd && newExactAtEnd && newCumlativeAtEnd)
            break; // we are done, hurray!

        // So this is one giant loop comparing 3 lists at once and sorting the comparision
        // into mainly 2 buckets: toAdd and toRemove
        // We need to distinguish between nodes that came from exact and cumalative
        // parsing, since the update call is diffrent for them
        // I believe this code to be correct, be careful in changing it

        QString nodeToAdd;
        if (! existingAtEnd
            && (newExactAtEnd || (*existingIt)->path() < *newExactIt)
            && (newCumlativeAtEnd || (*existingIt)->path() < *newCumlativeIt)) {
            // Remove case
            toRemove << *existingIt;
            ++existingIt;
        } else if(! newExactAtEnd
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
            } else if(*newExactIt < *newCumlativeIt) {
                ++newExactIt;
            } else if (*newCumlativeIt < *newExactIt) {
                ++newCumlativeIt;
            } else {
                ++newExactIt;
                ++newCumlativeIt;
            }
            // Update existingNodeIte
            ProFile *fileExact = includeFilesCumlative.value((*existingIt)->path());
            ProFile *fileCumlative = includeFilesCumlative.value((*existingIt)->path());
            if (fileExact || fileCumlative) {
                static_cast<Qt4PriFileNode *>(*existingIt)->update(fileExact, m_readerExact, fileCumlative, m_readerCumulative);
            } else {
                // We always parse exactly, because we later when async parsing don't know whether
                // the .pro file is included in this .pro file
                // So to compare that later parse with the sync one
                if (async)
                    static_cast<Qt4ProFileNode *>(*existingIt)->asyncUpdate();
                else
                    static_cast<Qt4ProFileNode *>(*existingIt)->update();
            }
            ++existingIt;
            // newCumalativeIt and newExactIt are already incremented

        }
        // If we found something to add do it
        if (!nodeToAdd.isEmpty()) {
            ProFile *fileExact = includeFilesCumlative.value(nodeToAdd);
            ProFile *fileCumlative = includeFilesCumlative.value(nodeToAdd);
            if (fileExact || fileCumlative) {
                Qt4PriFileNode *qt4PriFileNode = new Qt4PriFileNode(m_project, this, nodeToAdd);
                qt4PriFileNode->update(fileExact, m_readerExact, fileCumlative, m_readerCumulative);
                toAdd << qt4PriFileNode;
            } else {
                Qt4ProFileNode *qt4ProFileNode = new Qt4ProFileNode(m_project, nodeToAdd);
                if (async)
                    qt4ProFileNode->asyncUpdate();
                else
                    qt4ProFileNode->update();
                toAdd << qt4ProFileNode;
            }
        }
    } // for

    if (!toRemove.isEmpty())
        removeProjectNodes(toRemove);
    if (!toAdd.isEmpty())
        addProjectNodes(toAdd);

    Qt4PriFileNode::update(fileForCurrentProjectExact, m_readerExact, fileForCurrentProjectCumlative, m_readerCumulative);

    // update TargetInformation
    m_qt4targetInformation = targetInformation(m_readerExact);

    // update other variables
    QHash<Qt4Variable, QStringList> newVarValues;

    newVarValues[DefinesVar] = m_readerExact->values(QLatin1String("DEFINES"));
    newVarValues[IncludePathVar] = includePaths(m_readerExact);
    newVarValues[UiDirVar] = uiDirPaths(m_readerExact);
    newVarValues[MocDirVar] = mocDirPaths(m_readerExact);
    newVarValues[PkgConfigVar] = m_readerExact->values(QLatin1String("PKGCONFIG"));
    newVarValues[PrecompiledHeaderVar] =
            m_readerExact->absoluteFileValues(QLatin1String("PRECOMPILED_HEADER"),
                                              m_projectDir,
                                              QStringList() << m_projectDir,
                                              0);

    if (m_varValues != newVarValues) {
        m_varValues = newVarValues;
        foreach (NodesWatcher *watcher, watchers())
            if (Qt4NodesWatcher *qt4Watcher = qobject_cast<Qt4NodesWatcher*>(watcher))
                emit qt4Watcher->variablesChanged(this, m_varValues, newVarValues);
    }

    createUiCodeModelSupport();
    updateUiFiles();

    foreach (NodesWatcher *watcher, watchers())
        if (Qt4NodesWatcher *qt4Watcher = qobject_cast<Qt4NodesWatcher*>(watcher))
            emit qt4Watcher->proFileUpdated(this);

    m_project->destroyProFileReader(m_readerExact);
    if (m_readerCumulative)
        m_project->destroyProFileReader(m_readerCumulative);

    m_readerExact = 0;
    m_readerCumulative = 0;
}

namespace {
    // find all ui files in project
    class FindUiFileNodesVisitor : public ProjectExplorer::NodesVisitor {
    public:
        void visitProjectNode(ProjectNode *projectNode)
        {
            visitFolderNode(projectNode);
        }
        void visitFolderNode(FolderNode *folderNode)
        {
            foreach (FileNode *fileNode, folderNode->fileNodes()) {
                if (fileNode->fileType() == ProjectExplorer::FormType)
                    uiFileNodes << fileNode;
            }
        }
        QList<FileNode*> uiFileNodes;
    };
}

// This function is triggered after a build, and updates the state ui files
// It does so by storing a modification time for each ui file we know about.

// TODO this function should also be called if the build directory is changed
QStringList Qt4ProFileNode::updateUiFiles()
{
//    qDebug()<<"Qt4ProFileNode::updateUiFiles()";
    // Only those two project types can have ui files for us
    if (m_projectType != ApplicationTemplate
        && m_projectType != LibraryTemplate)
        return QStringList();

    // Find all ui files
    FindUiFileNodesVisitor uiFilesVisitor;
    this->accept(&uiFilesVisitor);
    const QList<FileNode*> uiFiles = uiFilesVisitor.uiFileNodes;

    // Find the UiDir, there can only ever be one
    QString uiDir = buildDir();
    QStringList tmp = m_varValues[UiDirVar];
    if (tmp.size() != 0)
        uiDir = tmp.first();

    // Collect all existing generated files
    QList<FileNode*> existingFileNodes;
    foreach (FileNode *file, fileNodes()) {
        if (file->isGenerated())
            existingFileNodes << file;
    }

    // Convert uiFile to uiHeaderFilePath, find all headers that correspond
    // and try to find them in uiDir
    QStringList newFilePaths;
    foreach (FileNode *uiFile, uiFiles) {
        const QString uiHeaderFilePath
                = QString("%1/ui_%2.h").arg(uiDir, QFileInfo(uiFile->path()).completeBaseName());
        if (QFileInfo(uiHeaderFilePath).exists())
            newFilePaths << uiHeaderFilePath;
    }

    // Create a diff between those lists
    QList<FileNode*> toRemove;
    QList<FileNode*> toAdd;
    // The list of files for which we call updateSourceFile
    QStringList toUpdate;

    qSort(newFilePaths);
    qSort(existingFileNodes.begin(), existingFileNodes.end(), ProjectNode::sortNodesByPath);

    QList<FileNode*>::const_iterator existingNodeIter = existingFileNodes.constBegin();
    QList<QString>::const_iterator newPathIter = newFilePaths.constBegin();
    while (existingNodeIter != existingFileNodes.constEnd()
           && newPathIter != newFilePaths.constEnd()) {
        if ((*existingNodeIter)->path() < *newPathIter) {
            toRemove << *existingNodeIter;
            ++existingNodeIter;
        } else if ((*existingNodeIter)->path() > *newPathIter) {
            toAdd << new FileNode(*newPathIter, ProjectExplorer::HeaderType, true);
            ++newPathIter;
        } else { // *existingNodeIter->path() == *newPathIter
            QString fileName = (*existingNodeIter)->path();
            QMap<QString, QDateTime>::const_iterator it = m_uitimestamps.find(fileName);
            QDateTime lastModified = QFileInfo(fileName).lastModified();
            if (it == m_uitimestamps.constEnd() || it.value() < lastModified) {
                toUpdate << fileName;
                m_uitimestamps[fileName] = lastModified;
            }
            ++existingNodeIter;
            ++newPathIter;
        }
    }
    while (existingNodeIter != existingFileNodes.constEnd()) {
        toRemove << *existingNodeIter;
        ++existingNodeIter;
    }
    while (newPathIter != newFilePaths.constEnd()) {
        toAdd << new FileNode(*newPathIter, ProjectExplorer::HeaderType, true);
        ++newPathIter;
    }

    // Update project tree
    if (!toRemove.isEmpty()) {
        foreach (FileNode *file, toRemove)
            m_uitimestamps.remove(file->path());
        removeFileNodes(toRemove, this);
    }

    CppTools::CppModelManagerInterface *modelManager =
        ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();

    if (!toAdd.isEmpty()) {
        foreach (FileNode *file, toAdd) {
            m_uitimestamps.insert(file->path(), QFileInfo(file->path()).lastModified());
            toUpdate << file->path();

            // Also adding files depending on that
            // We only need to do that for files that were newly created
            QString fileName = QFileInfo(file->path()).fileName();
            foreach (CPlusPlus::Document::Ptr doc, modelManager->snapshot()) {
                if (doc->includedFiles().contains(fileName)) {
                    if (!toUpdate.contains(doc->fileName()))
                        toUpdate << doc->fileName();
                }
            }
        }
        addFileNodes(toAdd, this);
    }
    return toUpdate;
}

QStringList Qt4ProFileNode::uiDirPaths(ProFileReader *reader) const
{
    QStringList candidates = reader->absolutePathValues(QLatin1String("UI_DIR"),
                                                        buildDir());
    candidates.removeDuplicates();
    return candidates;
}

QStringList Qt4ProFileNode::mocDirPaths(ProFileReader *reader) const
{
    QStringList candidates = reader->absolutePathValues(QLatin1String("MOC_DIR"),
                                                        buildDir());
    candidates.removeDuplicates();
    return candidates;
}

QStringList Qt4ProFileNode::includePaths(ProFileReader *reader) const
{
    QStringList paths;
    paths = reader->absolutePathValues(QLatin1String("INCLUDEPATH"),
                                       m_projectDir);
    paths << uiDirPaths(reader) << mocDirPaths(reader);
    paths.removeDuplicates();
    return paths;
}

QStringList Qt4ProFileNode::subDirsPaths(ProFileReader *reader) const
{
    QStringList subProjectPaths;

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
            realDir = QFileInfo(reader->value(subDirKey)).filePath();
        else if (reader->contains(subDirFileKey))
            realDir = QFileInfo(reader->value(subDirFileKey)).filePath();
        else
            realDir = subDirVar;
        QFileInfo info(realDir);
        if (!info.isAbsolute()) {
            info.setFile(m_projectDir + QLatin1Char('/') + realDir);
            realDir = m_projectDir + QLatin1Char('/') + realDir;
        }

        QString realFile;
        if (info.isDir()) {
            realFile = QString::fromLatin1("%1/%2.pro").arg(realDir, info.fileName());
        } else {
            realFile = realDir;
        }

        if (QFile::exists(realFile)) {
            if (!subProjectPaths.contains(realFile))
                subProjectPaths << realFile;
        } else {
            m_project->proFileParseError(tr("Could not find .pro file for sub dir '%1' in '%2'")
                                         .arg(subDirVar).arg(realDir));
        }
    }

    return subProjectPaths;
}

TargetInformation Qt4ProFileNode::targetInformation(ProFileReader *reader) const
{
    TargetInformation result;
    if (!reader)
        return result;

    const QString baseDir = buildDir();
    // qDebug() << "base build dir is:"<<baseDir;

    // Working Directory
    if (reader->contains("DESTDIR")) {
        //qDebug() << "reader contains destdir:" << reader->value("DESTDIR");
        result.workingDir = reader->value("DESTDIR");
        if (QDir::isRelativePath(result.workingDir)) {
            result.workingDir = baseDir + QLatin1Char('/') + result.workingDir;
            //qDebug() << "was relative and expanded to" << result.workingDir;
        }
    } else {
        //qDebug() << "reader didn't contain DESTDIR, setting to " << baseDir;
        result.workingDir = baseDir;
    }

    result.target = reader->value("TARGET");
    if (result.target.isEmpty())
        result.target = QFileInfo(m_projectFilePath).baseName();

#if defined (Q_OS_MAC)
    if (reader->values("CONFIG").contains("app_bundle")) {
        result.workingDir += QLatin1Char('/')
                           + result.target
                           + QLatin1String(".app/Contents/MacOS");
    }
#endif

    result.workingDir = QDir::cleanPath(result.workingDir);

    QString wd = result.workingDir;
    if (!reader->contains("DESTDIR")
        && reader->values("CONFIG").contains("debug_and_release")
        && reader->values("CONFIG").contains("debug_and_release_target")) {
        // If we don't have a destdir and debug and release is set
        // then the executable is in a debug/release folder
        //qDebug() << "reader has debug_and_release_target";

        // Hmm can we find out whether it's debug or release in a saner way?
        // Theoretically it's in CONFIG
        QString qmakeBuildConfig = "release";
        if (m_project->activeTarget()->activeBuildConfiguration()->qmakeBuildConfiguration() & QtVersion::DebugBuild)
            qmakeBuildConfig = "debug";
        wd += QLatin1Char('/') + qmakeBuildConfig;
    }

    result.executable = QDir::cleanPath(wd + QLatin1Char('/') + result.target);
    //qDebug() << "##### updateTarget sets:" << result.workingDir << result.executable;

#if defined (Q_OS_WIN)
    result.executable += QLatin1String(".exe");
#endif
    result.valid = true;
    return result;
}

TargetInformation Qt4ProFileNode::targetInformation()
{
    return m_qt4targetInformation;
}

QString Qt4ProFileNode::buildDir() const
{
    const QDir srcDirRoot = QFileInfo(m_project->rootProjectNode()->path()).absoluteDir();
    const QString relativeDir = srcDirRoot.relativeFilePath(m_projectDir);
    return QDir(m_project->activeTarget()->activeBuildConfiguration()->buildDirectory()).absoluteFilePath(relativeDir);
}

/*
  Sets project type to InvalidProject & deletes all subprojects/files/virtual folders
  */
void Qt4ProFileNode::invalidate()
{
    if (m_projectType == InvalidProject)
        return;

    clear();

    // change project type
    Qt4ProjectType oldType = m_projectType;
    m_projectType = InvalidProject;


    foreach (NodesWatcher *watcher, watchers())
        if (Qt4NodesWatcher *qt4Watcher = qobject_cast<Qt4NodesWatcher*>(watcher))
            emit qt4Watcher->projectTypeChanged(this, oldType, InvalidProject);
}

void Qt4ProFileNode::updateCodeModelSupportFromBuild(const QStringList &files)
{
    foreach (const QString &file, files) {
        QMap<QString, Qt4UiCodeModelSupport *>::const_iterator it, end;
        end = m_uiCodeModelSupport.constEnd();
        for (it = m_uiCodeModelSupport.constBegin(); it != end; ++it) {
            if (it.value()->fileName() == file)
                it.value()->updateFromBuild();
        }
    }
}

void Qt4ProFileNode::updateCodeModelSupportFromEditor(const QString &uiFileName,
                                                      const QString &contents)
{
    const QMap<QString, Qt4UiCodeModelSupport *>::const_iterator it =
            m_uiCodeModelSupport.constFind(uiFileName);
    if (it != m_uiCodeModelSupport.constEnd())
        it.value()->updateFromEditor(contents);
    foreach (ProjectExplorer::ProjectNode *pro, subProjectNodes())
        if (Qt4ProFileNode *qt4proFileNode = qobject_cast<Qt4ProFileNode *>(pro))
            qt4proFileNode->updateCodeModelSupportFromEditor(uiFileName, contents);
}

QString Qt4ProFileNode::uiDirectory() const
{
    const Qt4VariablesHash::const_iterator it = m_varValues.constFind(UiDirVar);
    if (it != m_varValues.constEnd() && !it.value().isEmpty())
        return it.value().front();
    return buildDir();
}

QString Qt4ProFileNode::uiHeaderFile(const QString &uiDir, const QString &formFile)
{
    QString uiHeaderFilePath = uiDir;
    uiHeaderFilePath += QLatin1String("/ui_");
    uiHeaderFilePath += QFileInfo(formFile).completeBaseName();
    uiHeaderFilePath += QLatin1String(".h");
    return QDir::cleanPath(uiHeaderFilePath);
}

void Qt4ProFileNode::createUiCodeModelSupport()
{
//    qDebug()<<"creatUiCodeModelSupport()";
    CppTools::CppModelManagerInterface *modelManager
            = ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();

    // First move all to
    QMap<QString, Qt4UiCodeModelSupport *> oldCodeModelSupport;
    oldCodeModelSupport = m_uiCodeModelSupport;
    m_uiCodeModelSupport.clear();

    // Only those two project types can have ui files for us
    if (m_projectType == ApplicationTemplate || m_projectType == LibraryTemplate) {
        // Find all ui files
        FindUiFileNodesVisitor uiFilesVisitor;
        this->accept(&uiFilesVisitor);
        const QList<FileNode*> uiFiles = uiFilesVisitor.uiFileNodes;

        // Find the UiDir, there can only ever be one
        const  QString uiDir = uiDirectory();
        foreach (const FileNode *uiFile, uiFiles) {
            const QString uiHeaderFilePath = uiHeaderFile(uiDir, uiFile->path());
//            qDebug()<<"code model support for "<<uiFile->path()<<" "<<uiHeaderFilePath;
            QMap<QString, Qt4UiCodeModelSupport *>::iterator it = oldCodeModelSupport.find(uiFile->path());
            if (it != oldCodeModelSupport.end()) {
//                qDebug()<<"updated old codemodelsupport";
                Qt4UiCodeModelSupport *cms = it.value();
                cms->setFileName(uiHeaderFilePath);
                m_uiCodeModelSupport.insert(it.key(), cms);
                oldCodeModelSupport.erase(it);
            } else {
//                qDebug()<<"adding new codemodelsupport";
                Qt4UiCodeModelSupport *cms = new Qt4UiCodeModelSupport(modelManager, m_project, uiFile->path(), uiHeaderFilePath);
                m_uiCodeModelSupport.insert(uiFile->path(), cms);
                modelManager->addEditorSupport(cms);
            }
        }
    }
    // Remove old
    QMap<QString, Qt4UiCodeModelSupport *>::const_iterator it, end;
    end = oldCodeModelSupport.constEnd();
    for (it = oldCodeModelSupport.constBegin(); it!=end; ++it) {
        modelManager->removeEditorSupport(it.value());
        delete it.value();
    }
}

Qt4NodesWatcher::Qt4NodesWatcher(QObject *parent)
        : NodesWatcher(parent)
{
}

} // namespace Internal
} // namespace Qt4ProjectManager
