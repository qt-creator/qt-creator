#include "vcproject.h"

#include "vcprojectfile.h"
#include "vcprojectnodes.h"
#include "vcprojectmanager.h"
#include "vcprojectmanagerconstants.h"
#include "vcprojectreader.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/filesystemwatcher.h>

#include <QFileInfo>
#include <QFileSystemWatcher>

using namespace ProjectExplorer;

namespace VcProjectManager {
namespace Internal {

VcProject::VcProject(VcManager *projectManager, const QString &projectFilePath)
    : m_projectManager(projectManager)
    , m_projectFile(new VcProjectFile(projectFilePath))
    , m_rootNode(new VcProjectNode(projectFilePath))
    , m_projectFileWatcher(new QFileSystemWatcher(this))
{
    setProjectContext(Core::Context(Constants::VC_PROJECT_CONTEXT));
    setProjectLanguage(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    reparse();

    m_projectFileWatcher->addPath(projectFilePath);
    connect(m_projectFileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(reparse()));
}

QString VcProject::displayName() const
{
    return m_rootNode->displayName();
}

Core::Id VcProject::id() const
{
    return Core::Id(Constants::VC_PROJECT_ID);
}

Core::IDocument *VcProject::document() const
{
    return m_projectFile;
}

IProjectManager *VcProject::projectManager() const
{
    return m_projectManager;
}

ProjectNode *VcProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList VcProject::files(Project::FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    // TODO: respect the mode
    return m_rootNode->files();
}

void VcProject::reparse()
{
    qDebug() << "reparse";

    QString projectFilePath = m_projectFile->filePath();
    VcProjectInfo::Project *projInfo = reader.parse(projectFilePath);

    // If file saving is done by replacing the file with the new file
    // watcher will loose it from its list
    if (m_projectFileWatcher->files().isEmpty())
        m_projectFileWatcher->addPath(projectFilePath);

    if (!projInfo) {
        m_rootNode->setDisplayName(QFileInfo(projectFilePath).fileName());
        m_rootNode->refresh(0);
    } else {
        m_rootNode->setDisplayName(projInfo->displayName);
        m_rootNode->refresh(projInfo->files);
    }

    delete projInfo;

    // TODO: can we (and is is important to) detect when the list really changed?
    emit fileListChanged();
}

} // namespace Internal
} // namespace VcProjectManager
