#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECT_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECT_H

#include "vcprojectreader.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/project.h>

#include <QFuture>

class QFileSystemWatcher;

namespace VcProjectManager {
namespace Internal {

class VcProjectFile;
class VcProjectNode;
class VcManager;

class VcProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    VcProject(VcManager *projectManager, const QString &projectFilePath);
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
    void reparse();

//    virtual QString generatedUiHeader(const QString &formFile) const;
//    static QString makeUnique(const QString &preferedName, const QStringList &usedNames);
//    virtual QVariantMap toMap() const;
//    virtual Core::Context projectContext() const;
//    virtual Core::Context projectLanguage() const;
//    virtual bool needsConfiguration() const;
//    virtual void configureAsExampleProject(const QStringList &platforms);

protected:
    bool fromMap(const QVariantMap &map);
    bool setupTarget(ProjectExplorer::Target *t);

private:
    void addCxxModelFiles(const ProjectExplorer::FolderNode *node, QStringList &sourceFiles);
    void updateCodeModels();
    void importBuildConfigurations();

    VcManager *m_projectManager;
    VcProjectFile *m_projectFile;
    VcProjectNode *m_rootNode;
    VcProjectReader reader;
    QString m_name;
    QFileSystemWatcher *m_projectFileWatcher;
    QFuture<void> m_codeModelFuture;
    QMap<QString, VcProjectInfo::ConfigurationInfo> m_configurations;
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
