#ifndef VCPROJECTMANAGER_H
#define VCPROJECTMANAGER_H

#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>

class QAction;

namespace VcProjectManager {
namespace Internal {

class VcManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    VcManager();

    QString mimeType() const;
    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);

private slots:
    void reparseActiveProject();
    void reparseContextProject();
    void updateContextMenu(ProjectExplorer::Project *project, ProjectExplorer::Node *node);

private:
    ProjectExplorer::Project *m_contextProject;
    QAction *m_reparseAction;
    QAction *m_reparseContextMenuAction;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_H
