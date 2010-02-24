/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtCore/QTimer>

#include <QtGui/QPainter>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

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
        QString fullName;
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
                            val->fullName = path;
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
            static const QChar separator = QChar('/');
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
                    FolderNode *newNode = new FolderNode(newNodeIter.value()->fullName);
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
                FolderNode *newNode = new FolderNode(newNodeIter.value()->fullName);
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

void Qt4PriFileNode::update(ProFile *includeFile, ProFileReader *reader)
{
    Q_ASSERT(includeFile);
    Q_ASSERT(reader);

    // add project file node
    if (m_fileNodes.isEmpty())
        addFileNodes(QList<FileNode*>() << new FileNode(m_projectFilePath, ProjectFileType, false), this);

    const QString &projectDir = m_qt4ProFileNode->m_projectDir;

    QStringList baseVPaths;
    baseVPaths += reader->absolutePathValues("VPATH", projectDir);
    baseVPaths << projectDir; // QMAKE_ABSOLUTE_SOURCE_PATH
    baseVPaths += reader->absolutePathValues("DEPENDPATH", projectDir);
    baseVPaths.removeDuplicates();

    const QVector<Qt4NodeStaticData::FileTypeData> &fileTypes = qt4NodeStaticData()->fileTypeData;

    InternalNode contents;

    // update files
    for (int i = 0; i < fileTypes.size(); ++i) {
        FileType type = fileTypes.at(i).type;
        const QStringList qmakeVariables = varNames(type);

        QStringList newFilePaths;
        foreach (const QString &qmakeVariable, qmakeVariables) {
            QStringList vPaths;
            if (type == ProjectExplorer::SourceType)
                vPaths = reader->absolutePathValues("VPATH_" + qmakeVariable, projectDir);
            vPaths += baseVPaths;
            if (type == ProjectExplorer::HeaderType)
                vPaths += reader->absolutePathValues("INCLUDEPATH", projectDir);
            vPaths.removeDuplicates();
            newFilePaths += reader->absoluteFileValues(qmakeVariable, projectDir, vPaths, includeFile);
        }
        newFilePaths.removeDuplicates();

        if (!newFilePaths.isEmpty()) {
            InternalNode *subfolder = new InternalNode;
            subfolder->type = type;
            subfolder->icon = fileTypes.at(i).icon;
            subfolder->fullName = m_projectDir + '#' + fileTypes.at(i).typeName;
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
                while (lines.last().isEmpty())
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

/*!
  \class Qt4ProFileNode
  Implements abstract ProjectNode class
  */
Qt4ProFileNode::Qt4ProFileNode(Qt4Project *project,
                               const QString &filePath,
                               QObject *parent)
        : Qt4PriFileNode(project, this, filePath),
          // own stuff
          m_projectType(InvalidProject)
{
    if (parent)
        setParent(parent);

    m_updateTimer.setInterval(100);
    m_updateTimer.setSingleShot(true);

    connect(&m_updateTimer, SIGNAL(timeout()),
            this, SLOT(update()));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));
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
    m_updateTimer.start();
}

void Qt4ProFileNode::update()
{
    ProFileReader *reader = m_project->createProFileReader(this);
    Q_ASSERT(reader);
    if (!reader->readProFile(m_projectFilePath)) {
        m_project->proFileParseError(tr("Error while parsing file %1. Giving up.").arg(m_projectFilePath));
        m_project->destroyProFileReader(reader);
        invalidate();
        return;
    }

    if (debug)
        qDebug() << "Qt4ProFileNode - updating files for file " << m_projectFilePath;

    Qt4ProjectType projectType = InvalidProject;
    switch (reader->templateType()) {
    case ProFileEvaluator::TT_Unknown:
    case ProFileEvaluator::TT_Application: {
        projectType = ApplicationTemplate;
        break;
    }
    case ProFileEvaluator::TT_Library: {
        projectType = LibraryTemplate;
        break;
    }
    case ProFileEvaluator::TT_Script: {
        projectType = ScriptTemplate;
        break;
    }
    case ProFileEvaluator::TT_Subdirs:
        projectType = SubDirsTemplate;
        break;
    }
    if (projectType != m_projectType) {
        Qt4ProjectType oldType = m_projectType;
        // probably all subfiles/projects have changed anyway ...
        clear();
        m_projectType = projectType;
        foreach (NodesWatcher *watcher, watchers())
            if (Qt4NodesWatcher *qt4Watcher = qobject_cast<Qt4NodesWatcher*>(watcher))
                emit qt4Watcher->projectTypeChanged(this, oldType, projectType);
    }

    //
    // Add/Remove pri files, sub projects
    //

    QList<ProjectNode*> existingProjectNodes = subProjectNodes();

    QList<QString> newProjectFiles;
    QHash<QString, ProFile*> includeFiles;
    ProFile *fileForCurrentProject = 0;
    {
        if (projectType == SubDirsTemplate) {
            foreach (const QString &subDirProject, subDirsPaths(reader))
                newProjectFiles << subDirProject;
        }

        foreach (ProFile *includeFile, reader->includeFiles()) {
            if (includeFile->fileName() == m_projectFilePath) { // this file
                fileForCurrentProject = includeFile;
            } else {
                newProjectFiles << includeFile->fileName();
                includeFiles.insert(includeFile->fileName(), includeFile);
            }
        }
    }

    qSort(existingProjectNodes.begin(), existingProjectNodes.end(),
          sortNodesByPath);
    qSort(newProjectFiles.begin(), newProjectFiles.end());

    QList<ProjectNode*> toAdd;
    QList<ProjectNode*> toRemove;

    QList<ProjectNode*>::const_iterator existingNodeIter = existingProjectNodes.constBegin();
    QList<QString>::const_iterator newProjectFileIter = newProjectFiles.constBegin();
    while (existingNodeIter != existingProjectNodes.constEnd()
               && newProjectFileIter != newProjectFiles.constEnd()) {
        if ((*existingNodeIter)->path() < *newProjectFileIter) {
            toRemove << *existingNodeIter;
            ++existingNodeIter;
        } else if ((*existingNodeIter)->path() > *newProjectFileIter) {
            if (ProFile *file = includeFiles.value(*newProjectFileIter)) {
                Qt4PriFileNode *priFileNode
                    = new Qt4PriFileNode(m_project, this,
                                         *newProjectFileIter);
                priFileNode->update(file, reader);
                toAdd << priFileNode;
            } else {
                toAdd << createSubProFileNode(*newProjectFileIter);
            }
            ++newProjectFileIter;
        } else { // *existingNodeIter->path() == *newProjectFileIter
             if (ProFile *file = includeFiles.value(*newProjectFileIter)) {
                Qt4PriFileNode *priFileNode = static_cast<Qt4PriFileNode*>(*existingNodeIter);
                priFileNode->update(file, reader);
            }

            ++existingNodeIter;
            ++newProjectFileIter;
        }
    }
    while (existingNodeIter != existingProjectNodes.constEnd()) {
        toRemove << *existingNodeIter;
        ++existingNodeIter;
    }
    while (newProjectFileIter != newProjectFiles.constEnd()) {
        if (ProFile *file = includeFiles.value(*newProjectFileIter)) {
            Qt4PriFileNode *priFileNode
                    = new Qt4PriFileNode(m_project, this,
                                         *newProjectFileIter);
            priFileNode->update(file, reader);
            toAdd << priFileNode;
        } else {
            toAdd << createSubProFileNode(*newProjectFileIter);
        }
        ++newProjectFileIter;
    }

    if (!toRemove.isEmpty())
        removeProjectNodes(toRemove);
    if (!toAdd.isEmpty())
        addProjectNodes(toAdd);

    Qt4PriFileNode::update(fileForCurrentProject, reader);

    // update other variables
    QHash<Qt4Variable, QStringList> newVarValues;

    newVarValues[DefinesVar] = reader->values(QLatin1String("DEFINES"));
    newVarValues[IncludePathVar] = includePaths(reader);
    newVarValues[UiDirVar] = uiDirPaths(reader);
    newVarValues[MocDirVar] = mocDirPaths(reader);
    newVarValues[PkgConfigVar] = reader->values(QLatin1String("PKGCONFIG"));
    newVarValues[PrecompiledHeaderVar] =
            reader->absoluteFileValues(QLatin1String("PRECOMPILED_HEADER"),
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

    m_project->destroyProFileReader(reader);
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

Qt4ProFileNode *Qt4ProFileNode::createSubProFileNode(const QString &path)
{
    Qt4ProFileNode *subProFileNode = new Qt4ProFileNode(m_project, path);
    subProFileNode->update();
    return subProFileNode;
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

void Qt4ProFileNode::updateCodeModelSupportFromEditor(const QString &uiFileName, Designer::FormWindowEditor *fw)
{
    QMap<QString, Qt4UiCodeModelSupport *>::const_iterator it;
    it = m_uiCodeModelSupport.constFind(uiFileName);
    if (it != m_uiCodeModelSupport.constEnd()) {
        it.value()->updateFromEditor(fw);
    }
    foreach (ProjectExplorer::ProjectNode *pro, subProjectNodes())
        if (Qt4ProFileNode *qt4proFileNode = qobject_cast<Qt4ProFileNode *>(pro))
            qt4proFileNode->updateCodeModelSupportFromEditor(uiFileName, fw);
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

}
}
