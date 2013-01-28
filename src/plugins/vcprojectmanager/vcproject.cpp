#include "vcproject.h"

#include "vcprojectfile.h"
#include "vcprojectnodes.h"
#include "vcmakestep.h"
#include "vcprojectmanager.h"
#include "vcprojectmanagerconstants.h"
#include "vcprojectreader.h"
#include "vcprojectbuildconfiguration.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <cpptools/ModelManagerInterface.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFormLayout>
#include <QLabel>

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

VcProject::~VcProject()
{
    m_codeModelFuture.cancel();
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

QList<BuildConfigWidget *> VcProject::subConfigWidgets()
{
    QList<ProjectExplorer::BuildConfigWidget*> list;
    list << new BuildEnvironmentWidget;
    return list;
}

QString VcProject::defaultBuildDirectory() const
{
    VcProjectFile* vcFile = static_cast<VcProjectFile *>(document());
    return vcFile->path() + QLatin1String("-build");
}

void VcProject::reparse()
{
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

    updateCodeModels();
    // TODO: can we (and is is important to) detect when the list really changed?
    emit fileListChanged();
}

bool VcProject::fromMap(const QVariantMap &map)
{
    loadBuildConfigurations();
    updateCodeModels();
    return Project::fromMap(map);
}

bool VcProject::setupTarget(ProjectExplorer::Target *t)
{
    VcProjectBuildConfigurationFactory *factory
            = ExtensionSystem::PluginManager::instance()->getObject<VcProjectBuildConfigurationFactory>();
    VcProjectBuildConfiguration *bc = factory->create(t, Constants::VC_PROJECT_BC_ID, QLatin1String("vcproj"));
    if (!bc)
        return false;

    t->addBuildConfiguration(bc);
    return true;
}

/**
 * @brief Visit folder node recursive and accumulate Source and Header files
 */
void VcProject::addCxxModelFiles(const FolderNode *node, QStringList &sourceFiles)
{
    foreach (const FileNode *file, node->fileNodes())
        if (file->fileType() == HeaderType || file->fileType() == SourceType)
            sourceFiles += file->path();
    foreach (const FolderNode *subfolder, node->subFolderNodes())
        addCxxModelFiles(subfolder, sourceFiles);
}

/**
 * @brief Update editor Code Models
 *
 * Because only language with Code Model in QtCreator and support in VS is C++,
 * this method updates C++ code model.
 * VCProj doesn't support Qt, ObjectiveC and always uses c++11.
 *
 * @note Method should pass some flags for ClangCodeModel plugin: "-fms-extensions"
 * and "-fdelayed-template-parsing", but no interface exists at this moment.
 */
void VcProject::updateCodeModels()
{
    typedef CPlusPlus::CppModelManagerInterface::ProjectPart ProjectPart;

    Kit *k = activeTarget() ? activeTarget()->kit() : KitManager::instance()->defaultKit();
    QTC_ASSERT(k, return);
    ToolChain *tc = ToolChainKitInformation::toolChain(k);
    CPlusPlus::CppModelManagerInterface *modelmanager = CPlusPlus::CppModelManagerInterface::instance();
    QTC_ASSERT(modelmanager, return);
    CPlusPlus::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);

    pinfo.clearProjectParts();
    ProjectPart::Ptr pPart(new ProjectPart());
    // VS 2005-2008 has poor c++11 support, see http://wiki.apache.org/stdcxx/C%2B%2B0xCompilerSupport
    pPart->cxx11Enabled = false;
    pPart->qtVersion = ProjectPart::NoQt;
    pPart->defines += tc->predefinedMacros(QStringList()); // TODO: extract proper CXX flags and project defines
    foreach (const HeaderPath &path, tc->systemHeaderPaths(Utils::FileName()))
        if (path.kind() != HeaderPath::FrameworkHeaderPath)
            pPart->includePaths += path.path();
    addCxxModelFiles(m_rootNode, pPart->sourceFiles);
    if (!pPart->sourceFiles.isEmpty())
        pinfo.appendProjectPart(pPart);

    modelmanager->updateProjectInfo(pinfo);
    m_codeModelFuture = modelmanager->updateSourceFiles(pPart->sourceFiles);
}

void VcProject::loadBuildConfigurations()
{
    Kit *defaultKit = KitManager::instance()->defaultKit();
    if (defaultKit) {

        // Note(Radovan): When targets are available from .vcproj or .vcproj.user
        // file create Target object for each of them

        Target *target = new Target(this, defaultKit);

        // Debug build configuration
        VcProjectBuildConfiguration *bc = new VcProjectBuildConfiguration(target);
        bc->setDefaultDisplayName(tr("Debug"));

        // build step
        ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        VcMakeStep *makeStep = new VcMakeStep(buildSteps);
        QLatin1String argument("/p:configuration=\"Debug\"");
        makeStep->addBuildArgument(argument);
        buildSteps->insertStep(0, makeStep);

        //clean step
        ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
        makeStep = new VcMakeStep(cleanSteps);
        argument = QLatin1String("/p:configuration=\"Debug\" /t:Clean");
        makeStep->addBuildArgument(argument);
        cleanSteps->insertStep(0, makeStep);

        target->addBuildConfiguration(bc);

        // Release build configuration
        bc = new VcProjectBuildConfiguration(target);
        bc->setDefaultDisplayName("Release");

        // build step
        buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        makeStep = new VcMakeStep(buildSteps);
        argument = QLatin1String("/p:configuration=\"Release\"");
        makeStep->addBuildArgument(argument);
        buildSteps->insertStep(0, makeStep);

        //clean step
        cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
        makeStep = new VcMakeStep(cleanSteps);
        argument = QLatin1String("/p:configuration=\"Release\" /t:Clean");
        makeStep->addBuildArgument(argument);
        cleanSteps->insertStep(0, makeStep);

        target->addBuildConfiguration(bc);
        addTarget(target);    }
}

VcProjectBuildSettingsWidget::VcProjectBuildSettingsWidget()
{
    QFormLayout *f1 = new QFormLayout(this);
    f1->setContentsMargins(0, 0, 0, 0);
    f1->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    QLabel *l = new QLabel(tr("Vcproj Build Configuration widget."));
    f1->addRow(tr("Vc Project"), l);
}

QString VcProjectBuildSettingsWidget::displayName() const
{
    return tr("Vc Project Settings Widget");
}

void VcProjectBuildSettingsWidget::init(BuildConfiguration *bc)
{
    Q_UNUSED(bc);
}

} // namespace Internal
} // namespace VcProjectManager
