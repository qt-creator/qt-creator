#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECT_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECT_H

#include "vcprojectmodel/vcprojectdocument_constants.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/project.h>

#include <QFuture>

class QFileSystemWatcher;

namespace ProjectExplorer {
class FolderNode;
}

namespace VcProjectManager {
namespace Internal {

class VcProjectFile;
class VcDocProjectNode;
class VcManager;

class VcProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    VcProject(VcManager *projectManager, const QString &projectFilePath, VcDocConstants::DocumentVersion docVersion);
    ~VcProject();

    QString displayName() const;
    Core::Id id() const;
    Core::IDocument *document() const;
    ProjectExplorer::IProjectManager *projectManager() const;
    ProjectExplorer::ProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;
    QString defaultBuildDirectory() const;

    bool needsConfiguration() const;
    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMessage) const;

public slots:
    void reloadProjectNodes();

protected:
    bool fromMap(const QVariantMap &map);
    bool setupTarget(ProjectExplorer::Target *t);

private:
    void addCxxModelFiles(const ProjectExplorer::FolderNode *node, QStringList &sourceFiles);
    void updateCodeModels();
    void importBuildConfigurations();

    VcManager *m_projectManager;
    VcProjectFile *m_projectFile;
    VcDocProjectNode *m_rootNode;
    QFileSystemWatcher *m_projectFileWatcher;
    QFuture<void> m_codeModelFuture;
};

class VcProjectBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
public:
    VcProjectBuildSettingsWidget();
    QString displayName() const;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECT_H
