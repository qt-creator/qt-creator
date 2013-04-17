#ifndef VCPROJECTMANAGER_H
#define VCPROJECTMANAGER_H

#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>

class QAction;

namespace VcProjectManager {
namespace Internal {

class VcProjectBuildOptionsPage;
struct MsBuildInformation;

class VcManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    VcManager(VcProjectBuildOptionsPage *configPage);

    QString mimeType() const;
    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);
    QVector<MsBuildInformation *> msBuilds() const;
    VcProjectBuildOptionsPage *buildOptionsPage();

private slots:
    void updateContextMenu(ProjectExplorer::Project *project, ProjectExplorer::Node *node);
    void onOptionsPageUpdate();

private:
    bool checkIfVersion2003(const QString &filePath) const;
    bool checkIfVersion2005(const QString &filePath) const;
    bool checkIfVersion2008(const QString &filePath) const;
    void readSchemaPath();

private:
    ProjectExplorer::Project *m_contextProject;
    VcProjectBuildOptionsPage *m_configPage;

    QString m_vc2003Schema;
    QString m_vc2005Schema;
    QString m_vc2008Schema;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_H
