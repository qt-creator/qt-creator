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
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cppprojectfile.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/headerpath.h>
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

class VCProjKitMatcher : public KitMatcher
{
public:
    bool matches(const Kit *k) const
    {
        ToolChain *tc = ToolChainKitInformation::toolChain(k);
        QTC_ASSERT(tc, return false);
        Abi abi = tc->targetAbi();
        switch (abi.osFlavor()) {
        case Abi::WindowsMsvc2005Flavor:
        case Abi::WindowsMsvc2008Flavor:
        case Abi::WindowsCEFlavor:
            return true;
        default:
            return false;
        }
    }
};

VcProject::VcProject(VcManager *projectManager, const QString &projectFilePath)
    : m_projectManager(projectManager)
    , m_projectFile(new VcProjectFile(projectFilePath))
    , m_rootNode(new VcProjectNode(projectFilePath))
    , m_projectFileWatcher(new QFileSystemWatcher(this))
{
    setProjectContext(Core::Context(Constants::VC_PROJECT_CONTEXT));

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

QString VcProject::defaultBuildDirectory() const
{
    VcProjectFile* vcFile = static_cast<VcProjectFile *>(document());
    return vcFile->path() + QLatin1String("-build");
}

bool VcProject::needsConfiguration() const
{
    return targets().isEmpty() || !activeTarget() || activeTarget()->buildConfigurations().isEmpty();
}

bool VcProject::supportsKit(Kit *k, QString *errorMessage) const
{
    VCProjKitMatcher matcher;
    if (!matcher.matches(k)) {
        if (errorMessage)
            *errorMessage = tr("Kit toolchain does not support MSVC 2005 or 2008 ABI");
        return false;
    }
    return true;
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

    m_configurations = projInfo->configurations;
    delete projInfo;

    updateCodeModels();
    // TODO: can we (and is is important to) detect when the list really changed?
    emit fileListChanged();
}

bool VcProject::fromMap(const QVariantMap &map)
{
    const bool ok = Project::fromMap(map);

    if (ok || needsConfiguration())
        importBuildConfigurations();

    if (!ok)
        return false;

    updateCodeModels();
    return true;
}

bool VcProject::setupTarget(ProjectExplorer::Target *t)
{
    foreach (const QString &name, m_configurations.keys()) {
        VcProjectBuildConfigurationFactory *factory
                = ExtensionSystem::PluginManager::instance()->getObject<VcProjectBuildConfigurationFactory>();
        VcProjectBuildConfiguration *bc = factory->create(t, Constants::VC_PROJECT_BC_ID, name);
        if (!bc)
            continue;
        bc->setInfo(m_configurations.value(name));

        // build step
        ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        VcMakeStep *makeStep = new VcMakeStep(buildSteps);
        QString argument(QLatin1String("/p:configuration=\"") + name + QLatin1String("\""));
        makeStep->addBuildArgument(argument);
        buildSteps->insertStep(0, makeStep);

        //clean step
        ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
        makeStep = new VcMakeStep(cleanSteps);
        argument = QLatin1String("/p:configuration=\"") + name + QLatin1String("\" /t:Clean");
        makeStep->addBuildArgument(argument);
        cleanSteps->insertStep(0, makeStep);

        t->addBuildConfiguration(bc);
    }
    return true;
}

/**
 * @brief Visit folder node recursive and accumulate Source and Header files
 */
void VcProject::addCxxModelFiles(const FolderNode *node, QStringList &projectFiles)
{
    foreach (const FileNode *file, node->fileNodes())
        if (file->fileType() == HeaderType || file->fileType() == SourceType)
            projectFiles << file->path();
    foreach (const FolderNode *subfolder, node->subFolderNodes())
        addCxxModelFiles(subfolder, projectFiles);
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
    Kit *k = activeTarget() ? activeTarget()->kit() : KitManager::instance()->defaultKit();
    QTC_ASSERT(k, return);
    ToolChain *tc = ToolChainKitInformation::toolChain(k);
    QTC_ASSERT(tc, return);
    CppTools::CppModelManagerInterface *modelmanager = CppTools::CppModelManagerInterface::instance();
    QTC_ASSERT(modelmanager, return);
    CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);

    pinfo.clearProjectParts();
    CppTools::ProjectPart::Ptr pPart(new CppTools::ProjectPart());

    BuildConfiguration *bc = activeTarget() ? activeTarget()->activeBuildConfiguration() : NULL;
    if (bc) {
        VcProjectBuildConfiguration *vbc = qobject_cast<VcProjectBuildConfiguration*>(bc);
        QTC_ASSERT(vbc, return);
        pPart->defines += vbc->info().defines.join(QLatin1String("\n")).toLatin1();
    }
    // VS 2005-2008 has poor c++11 support, see http://wiki.apache.org/stdcxx/C%2B%2B0xCompilerSupport
    pPart->cxxVersion = CppTools::ProjectPart::CXX98;
    pPart->qtVersion = CppTools::ProjectPart::NoQt;
    pPart->defines += tc->predefinedMacros(QStringList());

    QStringList cxxFlags;
    foreach (const HeaderPath &path, tc->systemHeaderPaths(cxxFlags, Utils::FileName()))
        if (path.kind() != HeaderPath::FrameworkHeaderPath)
            pPart->includePaths += path.path();

    QStringList files;
    addCxxModelFiles(m_rootNode, files);

    foreach (const QString &file, files)
        pPart->files << CppTools::ProjectFile(file, CppTools::ProjectFile::CXXSource);

    if (!pPart->files.isEmpty())
        pinfo.appendProjectPart(pPart);

    modelmanager->updateProjectInfo(pinfo);
    m_codeModelFuture = modelmanager->updateSourceFiles(files);
    setProjectLanguage(ProjectExplorer::Constants::LANG_CXX, !pPart->files.isEmpty());
}

void VcProject::importBuildConfigurations()
{
    VCProjKitMatcher matcher;
    Kit *kit = KitManager::instance()->find(&matcher);
    if (!kit)
        kit = KitManager::instance()->defaultKit();

    removeTarget(target(kit));
    addTarget(createTarget(kit));
    if (!activeTarget() && kit)
        addTarget(createTarget(kit));
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

} // namespace Internal
} // namespace VcProjectManager
