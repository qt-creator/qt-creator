#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECTNODES_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECTNODES_H

#include <projectexplorer/projectnodes.h>

#include <QList>

class QString;
class QStringList;

namespace VcProjectManager {
namespace Internal {

namespace VcProjectInfo {
struct Filter;
}

class VcProjectNode : public ProjectExplorer::ProjectNode
{
public:
    VcProjectNode(const QString &projectFilePath);

    bool hasBuildTargets() const;
    QList<ProjectAction> supportedActions(Node *node) const;
    bool canAddSubProject(const QString &proFilePath) const;
    bool addSubProjects(const QStringList &proFilePaths);
    bool removeSubProjects(const QStringList &proFilePaths);
    bool addFiles(const ProjectExplorer::FileType fileType,
                  const QStringList &filePaths,
                  QStringList *notAdded);
    bool removeFiles(const ProjectExplorer::FileType fileType,
                     const QStringList &filePaths,
                     QStringList *notRemoved);
    bool deleteFiles(const ProjectExplorer::FileType fileType,
                     const QStringList &filePaths);
    bool renameFile(const ProjectExplorer::FileType fileType,
                     const QString &filePath,
                     const QString &newFilePath);
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node);

    void refresh(VcProjectInfo::Filter *files);
    QStringList files();

private:
    static ProjectExplorer::FileNode *createFileNode(const QString &filePath);
    // TODO: use the one from projectnodes.h
    static ProjectExplorer::FileType typeForFileName(const Core::MimeDatabase *db, const QFileInfo &file);

private:
    QStringList m_files;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECTNODES_H
