#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECT_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECT_H

#include "vcprojectreader.h"

#include <projectexplorer/project.h>

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

    QString displayName() const;
    Core::Id id() const;
    Core::IDocument *document() const;
    ProjectExplorer::IProjectManager *projectManager() const;
    ProjectExplorer::ProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;

public slots:
    void reparse();

//    virtual QList<BuildConfigWidget*> subConfigWidgets();
//    virtual QString generatedUiHeader(const QString &formFile) const;
//    static QString makeUnique(const QString &preferedName, const QStringList &usedNames);
//    virtual QVariantMap toMap() const;
//    virtual Core::Context projectContext() const;
//    virtual Core::Context projectLanguage() const;
//    virtual bool needsConfiguration() const;
//    virtual void configureAsExampleProject(const QStringList &platforms);

private:
    VcManager *m_projectManager;
    VcProjectFile *m_projectFile;
    VcProjectNode *m_rootNode;
    VcProjectReader reader;
    QString m_name;
    QFileSystemWatcher *m_projectFileWatcher;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECT_H
