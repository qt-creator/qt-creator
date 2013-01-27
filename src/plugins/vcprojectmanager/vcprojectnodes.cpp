#include "vcprojectnodes.h"

#include "vcprojectreader.h"
#include "vcprojectmanagerconstants.h"

// TODO: temp - just for VcProjectReader::typeForFileName()
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QFileInfo>

using namespace ProjectExplorer;

namespace VcProjectManager {
namespace Internal {

VcProjectNode::VcProjectNode(const QString &projectFilePath)
    : ProjectNode(projectFilePath)
{
}

bool VcProjectNode::hasBuildTargets() const
{
    return true;
}

QList<ProjectNode::ProjectAction> VcProjectNode::supportedActions(ProjectExplorer::Node *node) const
{
    Q_UNUSED(node);
    // TODO: add actions
    return QList<ProjectNode::ProjectAction>();
}

bool VcProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool VcProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}

bool VcProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}

bool VcProjectNode::addFiles(const FileType fileType,
                             const QStringList &filePaths,
                             QStringList *notAdded)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notAdded);
    // TODO: obvious
    return false;
}

bool VcProjectNode::removeFiles(const FileType fileType,
                                const QStringList &filePaths,
                                QStringList *notRemoved)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notRemoved);
    // TODO: obvious
    return false;
}

bool VcProjectNode::deleteFiles(const FileType fileType,
                                const QStringList &filePaths)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    // TODO: obvious
    return false;
}

bool VcProjectNode::renameFile(const FileType fileType,
                               const QString &filePath,
                               const QString &newFilePath)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePath);
    Q_UNUSED(newFilePath);
    // TODO: obvious
    return false;
}

QList<RunConfiguration *> VcProjectNode::runConfigurationsFor(Node *node)
{
    Q_UNUSED(node);
    // TODO: what's this for
    return QList<RunConfiguration *>();
}

void VcProjectNode::refresh(VcProjectInfo::Filter *files)
{
    removeFileNodes(fileNodes(), this);
    removeFolderNodes(subFolderNodes(), this);
    m_files.clear();

    QList<FileNode *> projectFileNodeWrapped;
    projectFileNodeWrapped.append(createFileNode(path()));
    this->addFileNodes(projectFileNodeWrapped, this);

    if (!files)
        return;

    QString projectPath = QFileInfo(path()).path();
    QList<VcProjectInfo::Filter *> filterQueue;
    QList<FolderNode *> parentQueue;
    filterQueue.prepend(files);
    parentQueue.prepend(this);

    QList<FileNode *> fileNodes;
    QList<FolderNode *> filterNodes;
    while (!filterQueue.empty()) {
        VcProjectInfo::Filter *filter = filterQueue.takeLast();
        FolderNode *parent = parentQueue.takeLast();

        fileNodes.clear();
        filterNodes.clear();
        foreach (VcProjectInfo::File *file, filter->files) {
            FileNode *fileNode = createFileNode(projectPath + file->relativePath);
            m_files.append(fileNode->path());
            fileNodes.append(fileNode);
        }
        addFileNodes(fileNodes, parent);

        foreach (VcProjectInfo::Filter *childFilter, filter->filters) {
            FolderNode *filterNode = new FolderNode(parent->path() + childFilter->name);
            filterNode->setDisplayName(childFilter->name);
            filterQueue.prepend(childFilter);
            parentQueue.prepend(filterNode);
            filterNodes.append(filterNode);
        }
        addFolderNodes(filterNodes, parent);
    }
}

QStringList VcProjectNode::files()
{
    return m_files;
}

FileNode *VcProjectNode::createFileNode(const QString &filePath)
{
    QFileInfo fileInfo = QFileInfo(filePath);
    FileType fileType = typeForFileName(Core::ICore::mimeDatabase(), fileInfo);

    return new FileNode(fileInfo.canonicalFilePath(), fileType, false);
}

FileType VcProjectNode::typeForFileName(const Core::MimeDatabase *db, const QFileInfo &file)
{
    const Core::MimeType mt = db->findByFile(file);
    if (!mt)
        return UnknownFileType;

    const QString typeName = mt.type();
    if (typeName == QLatin1String(Constants::VCPROJ_MIMETYPE))
        return ProjectFileType;
    if (typeName == QLatin1String(ProjectExplorer::Constants::CPP_SOURCE_MIMETYPE)
        || typeName == QLatin1String(ProjectExplorer::Constants::C_SOURCE_MIMETYPE))
        return SourceType;
    if (typeName == QLatin1String(ProjectExplorer::Constants::CPP_HEADER_MIMETYPE)
        || typeName == QLatin1String(ProjectExplorer::Constants::C_HEADER_MIMETYPE))
        return HeaderType;
    if (typeName == QLatin1String(ProjectExplorer::Constants::RESOURCE_MIMETYPE))
        return ResourceType;
    if (typeName == QLatin1String(ProjectExplorer::Constants::FORM_MIMETYPE))
        return FormType;
    if (typeName == QLatin1String(ProjectExplorer::Constants::QML_MIMETYPE))
        return QMLType;
    return UnknownFileType;
}

} // namespace Internal
} // namespace VcProjectManager
