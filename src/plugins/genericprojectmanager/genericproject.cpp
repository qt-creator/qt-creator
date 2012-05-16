/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "genericproject.h"

#include "genericbuildconfiguration.h"
#include "genericprojectconstants.h"
#include "generictarget.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <cpptools/ModelManagerInterface.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/documentmanager.h>

#include <QDir>
#include <QProcessEnvironment>

#include <QFormLayout>
#include <QMainWindow>
#include <QComboBox>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char * const TOOLCHAIN_KEY("GenericProjectManager.GenericProject.Toolchain");
} // end of anonymous namespace

////////////////////////////////////////////////////////////////////////////////////
// GenericProject
////////////////////////////////////////////////////////////////////////////////////

GenericProject::GenericProject(Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_toolChain(0)
{
    setProjectContext(Core::Context(GenericProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguage(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    QFileInfo fileInfo(m_fileName);
    QDir dir = fileInfo.dir();

    m_projectName      = fileInfo.completeBaseName();
    m_filesFileName    = QFileInfo(dir, m_projectName + QLatin1String(".files")).absoluteFilePath();
    m_includesFileName = QFileInfo(dir, m_projectName + QLatin1String(".includes")).absoluteFilePath();
    m_configFileName   = QFileInfo(dir, m_projectName + QLatin1String(".config")).absoluteFilePath();

    m_creatorIDocument  = new GenericProjectFile(this, m_fileName, GenericProject::Everything);
    m_filesIDocument    = new GenericProjectFile(this, m_filesFileName, GenericProject::Files);
    m_includesIDocument = new GenericProjectFile(this, m_includesFileName, GenericProject::Configuration);
    m_configIDocument   = new GenericProjectFile(this, m_configFileName, GenericProject::Configuration);

    Core::DocumentManager::addDocument(m_creatorIDocument);
    Core::DocumentManager::addDocument(m_filesIDocument);
    Core::DocumentManager::addDocument(m_includesIDocument);
    Core::DocumentManager::addDocument(m_configIDocument);

    m_rootNode = new GenericProjectNode(this, m_creatorIDocument);

    m_manager->registerProject(this);
}

GenericProject::~GenericProject()
{
    m_codeModelFuture.cancel();
    m_manager->unregisterProject(this);

    delete m_rootNode;
    // do not delete m_toolChain
}

GenericTarget *GenericProject::activeTarget() const
{
    return static_cast<GenericTarget *>(Project::activeTarget());
}

QString GenericProject::filesFileName() const
{
    return m_filesFileName;
}

QString GenericProject::includesFileName() const
{
    return m_includesFileName;
}

QString GenericProject::configFileName() const
{
    return m_configFileName;
}

static QStringList readLines(const QString &absoluteFileName)
{
    QStringList lines;

    QFile file(absoluteFileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        forever {
            QString line = stream.readLine();
            if (line.isNull())
                break;

            lines.append(line);
        }
    }

    return lines;
}

bool GenericProject::saveRawFileList(const QStringList &rawFileList)
{
    // Make sure we can open the file for writing
    Utils::FileSaver saver(filesFileName(), QIODevice::Text);
    if (!saver.hasError()) {
        QTextStream stream(saver.file());
        foreach (const QString &filePath, rawFileList)
            stream << filePath << QLatin1Char('\n');
        saver.setResult(&stream);
    }
    if (!saver.finalize(Core::ICore::mainWindow()))
        return false;
    refresh(GenericProject::Files);
    return true;
}

bool GenericProject::addFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    QDir baseDir(QFileInfo(m_fileName).dir());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    return saveRawFileList(newList);
}

bool GenericProject::removeFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    foreach (const QString &filePath, filePaths) {
        QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
        if (i != m_rawListEntries.end())
            newList.removeOne(i.value());
    }

    return saveRawFileList(newList);
}

bool GenericProject::setFiles(const QStringList &filePaths)
{
    QStringList newList;
    QDir baseDir(QFileInfo(m_fileName).dir());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    return saveRawFileList(newList);
}

bool GenericProject::renameFile(const QString &filePath, const QString &newFilePath)
{
    QStringList newList = m_rawFileList;

    QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
    if (i != m_rawListEntries.end()) {
        int index = newList.indexOf(i.value());
        if (index != -1) {
            QDir baseDir(QFileInfo(m_fileName).dir());
            newList.replace(index, baseDir.relativeFilePath(newFilePath));
        }
    }

    return saveRawFileList(newList);
}

void GenericProject::parseProject(RefreshOptions options)
{
    if (options & Files) {
        m_rawListEntries.clear();
        m_rawFileList = readLines(filesFileName());
        m_files = processEntries(m_rawFileList, &m_rawListEntries);
    }

    if (options & Configuration) {
        m_projectIncludePaths = processEntries(readLines(includesFileName()));

        // TODO: Possibly load some configuration from the project file
        //QSettings projectInfo(m_fileName, QSettings::IniFormat);

        m_defines.clear();

        QFile configFile(configFileName());
        if (configFile.open(QFile::ReadOnly))
            m_defines = configFile.readAll();
    }

    if (options & Files)
        emit fileListChanged();
}

void GenericProject::refresh(RefreshOptions options)
{
    QSet<QString> oldFileList;
    if (!(options & Configuration))
        oldFileList = m_files.toSet();

    parseProject(options);

    if (options & Files)
        m_rootNode->refresh();

    CPlusPlus::CppModelManagerInterface *modelManager =
        CPlusPlus::CppModelManagerInterface::instance();

    if (modelManager) {
        CPlusPlus::CppModelManagerInterface::ProjectInfo pinfo = modelManager->projectInfo(this);
        pinfo.clearProjectParts();
        CPlusPlus::CppModelManagerInterface::ProjectPart::Ptr part(
                    new CPlusPlus::CppModelManagerInterface::ProjectPart);

        if (m_toolChain) {
            part->defines = m_toolChain->predefinedMacros(QStringList());
            part->defines += '\n';

            foreach (const HeaderPath &headerPath, m_toolChain->systemHeaderPaths()) {
                if (headerPath.kind() == HeaderPath::FrameworkHeaderPath)
                    part->frameworkPaths.append(headerPath.path());
                else
                    part->includePaths.append(headerPath.path());
            }
        }

        part->includePaths += allIncludePaths();
        part->defines += m_defines;

        // ### add _defines.
        part->sourceFiles = files();
        part->sourceFiles += generated();

        QStringList filesToUpdate;

        if (options & Configuration) {
            filesToUpdate = part->sourceFiles;
            filesToUpdate.append(QLatin1String("<configuration>")); // XXX don't hardcode configuration file name
            // Full update, if there's a code model update, cancel it
            m_codeModelFuture.cancel();
        } else if (options & Files) {
            // Only update files that got added to the list
            QSet<QString> newFileList = m_files.toSet();
            newFileList.subtract(oldFileList);
            filesToUpdate.append(newFileList.toList());
        }

        pinfo.appendProjectPart(part);

        modelManager->updateProjectInfo(pinfo);
        m_codeModelFuture = modelManager->updateSourceFiles(filesToUpdate);
    }
}

/**
 * Expands environment variables in the given \a string when they are written
 * like $$(VARIABLE).
 */
static void expandEnvironmentVariables(const QProcessEnvironment &env, QString &string)
{
    static QRegExp candidate(QLatin1String("\\$\\$\\((.+)\\)"));

    int index = candidate.indexIn(string);
    while (index != -1) {
        const QString value = env.value(candidate.cap(1));

        string.replace(index, candidate.matchedLength(), value);
        index += value.length();

        index = candidate.indexIn(string, index);
    }
}

/**
 * Expands environment variables and converts the path from relative to the
 * project to an absolute path.
 *
 * The \a map variable is an optional argument that will map the returned
 * absolute paths back to their original \a entries.
 */
QStringList GenericProject::processEntries(const QStringList &paths,
                                           QHash<QString, QString> *map) const
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QDir projectDir(QFileInfo(m_fileName).dir());

    QStringList absolutePaths;
    foreach (const QString &path, paths) {
        QString trimmedPath = path.trimmed();
        if (trimmedPath.isEmpty())
            continue;

        expandEnvironmentVariables(env, trimmedPath);

        const QString absPath = QFileInfo(projectDir, trimmedPath).absoluteFilePath();
        absolutePaths.append(absPath);
        if (map)
            map->insert(absPath, trimmedPath);
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

QStringList GenericProject::allIncludePaths() const
{
    QStringList paths;
    paths += m_includePaths;
    paths += m_projectIncludePaths;
    paths.removeDuplicates();
    return paths;
}

QStringList GenericProject::projectIncludePaths() const
{
    return m_projectIncludePaths;
}

QStringList GenericProject::files() const
{
    return m_files;
}

QStringList GenericProject::generated() const
{
    return m_generated;
}

QStringList GenericProject::includePaths() const
{
    return m_includePaths;
}

void GenericProject::setIncludePaths(const QStringList &includePaths)
{
    m_includePaths = includePaths;
}

QByteArray GenericProject::defines() const
{
    return m_defines;
}

void GenericProject::setToolChain(ToolChain *tc)
{
    if (m_toolChain == tc)
        return;

    m_toolChain = tc;
    refresh(Configuration);

    foreach (Target *t, targets()) {
        foreach (BuildConfiguration *bc, t->buildConfigurations())
            bc->setToolChain(tc);
    }

    emit toolChainChanged(m_toolChain);
}

ToolChain *GenericProject::toolChain() const
{
    return m_toolChain;
}

QString GenericProject::displayName() const
{
    return m_projectName;
}

Core::Id GenericProject::id() const
{
    return Core::Id(Constants::GENERICPROJECT_ID);
}

Core::IDocument *GenericProject::document() const
{
    return m_creatorIDocument;
}

IProjectManager *GenericProject::projectManager() const
{
    return m_manager;
}

QList<BuildConfigWidget*> GenericProject::subConfigWidgets()
{
    QList<BuildConfigWidget*> list;
    list << new BuildEnvironmentWidget;
    return list;
}

GenericProjectNode *GenericProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList GenericProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode)
    return m_files; // ### TODO: handle generated files here.
}

QStringList GenericProject::buildTargets() const
{
    QStringList targets;
    targets.append(QLatin1String("all"));
    targets.append(QLatin1String("clean"));
    return targets;
}

QVariantMap GenericProject::toMap() const
{
    QVariantMap map(Project::toMap());
    map.insert(QLatin1String(TOOLCHAIN_KEY), m_toolChain ? m_toolChain->id() : QString());
    return map;
}

bool GenericProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    // Sanity check: We need both a buildconfiguration and a runconfiguration!
    QList<Target *> targetList = targets();
    foreach (Target *t, targetList) {
        if (!t->activeBuildConfiguration()) {
            removeTarget(t);
            delete t;
            continue;
        }
        if (!t->activeRunConfiguration())
            t->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(t));
    }

    // Add default setup:
    if (targets().isEmpty()) {
        GenericTargetFactory *factory =
                ExtensionSystem::PluginManager::instance()->getObject<GenericTargetFactory>();
        addTarget(factory->create(this, Core::Id(GENERIC_DESKTOP_TARGET_ID)));
    }

    QString id = map.value(QLatin1String(TOOLCHAIN_KEY)).toString();
    const ToolChainManager *toolChainManager = ToolChainManager::instance();

    if (!id.isNull()) {
        setToolChain(toolChainManager->findToolChain(id));
    } else {
        ProjectExplorer::Abi abi = ProjectExplorer::Abi::hostAbi();
        abi = ProjectExplorer::Abi(abi.architecture(), abi.os(),  ProjectExplorer::Abi::UnknownFlavor,
                                   abi.binaryFormat(), abi.wordWidth() == 32 ? 32 : 0);
        QList<ToolChain *> tcs = toolChainManager->findToolChains(abi);
        if (tcs.isEmpty())
            tcs = toolChainManager->toolChains();
        if (!tcs.isEmpty())
            setToolChain(tcs.at(0));
    }

    setIncludePaths(allIncludePaths());

    refresh(Everything);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////
// GenericBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////

GenericBuildSettingsWidget::GenericBuildSettingsWidget(GenericTarget *target)
    : m_target(target), m_toolChainChooser(0), m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // build directory
    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setEnabled(true);
    m_pathChooser->setBaseDirectory(m_target->genericProject()->projectDirectory());
    fl->addRow(tr("Build directory:"), m_pathChooser);
    connect(m_pathChooser, SIGNAL(changed(QString)), this, SLOT(buildDirectoryChanged()));

    // tool chain
    m_toolChainChooser = new QComboBox;
    m_toolChainChooser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    updateToolChainList();

    fl->addRow(tr("Tool chain:"), m_toolChainChooser);
    connect(m_toolChainChooser, SIGNAL(activated(int)), this, SLOT(toolChainSelected(int)));
    connect(m_target->genericProject(), SIGNAL(toolChainChanged(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainChanged(ProjectExplorer::ToolChain*)));
    connect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(updateToolChainList()));
    connect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(updateToolChainList()));
}

GenericBuildSettingsWidget::~GenericBuildSettingsWidget()
{ }

QString GenericBuildSettingsWidget::displayName() const
{ return tr("Generic Manager"); }

void GenericBuildSettingsWidget::init(BuildConfiguration *bc)
{
    m_buildConfiguration = static_cast<GenericBuildConfiguration *>(bc);
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory());
}

void GenericBuildSettingsWidget::buildDirectoryChanged()
{
    m_buildConfiguration->setBuildDirectory(m_pathChooser->rawPath());
}

void GenericBuildSettingsWidget::toolChainSelected(int index)
{
    using namespace ProjectExplorer;

    ToolChain *tc = static_cast<ToolChain *>(m_toolChainChooser->itemData(index).value<void *>());
    m_target->genericProject()->setToolChain(tc);
}

void GenericBuildSettingsWidget::toolChainChanged(ProjectExplorer::ToolChain *tc)
{
    for (int i = 0; i < m_toolChainChooser->count(); ++i) {
        ToolChain * currentTc = static_cast<ToolChain *>(m_toolChainChooser->itemData(i).value<void *>());
        if (currentTc != tc)
            continue;
        m_toolChainChooser->setCurrentIndex(i);
        return;
    }
}

void GenericBuildSettingsWidget::updateToolChainList()
{
    m_toolChainChooser->clear();

    QList<ToolChain *> tcs = ToolChainManager::instance()->toolChains();
    if (!m_target->genericProject()->toolChain()) {
        m_toolChainChooser->addItem(tr("<Invalid tool chain>"), qVariantFromValue(static_cast<void *>(0)));
        m_toolChainChooser->setCurrentIndex(0);
    }
    foreach (ToolChain *tc, tcs) {
        m_toolChainChooser->addItem(tc->displayName(), qVariantFromValue(static_cast<void *>(tc)));
        if (m_target->genericProject()->toolChain()
                && m_target->genericProject()->toolChain()->id() == tc->id())
            m_toolChainChooser->setCurrentIndex(m_toolChainChooser->count() - 1);
    }
}

////////////////////////////////////////////////////////////////////////////////////
// GenericProjectFile
////////////////////////////////////////////////////////////////////////////////////

GenericProjectFile::GenericProjectFile(GenericProject *parent, QString fileName, GenericProject::RefreshOptions options)
    : Core::IDocument(parent),
      m_project(parent),
      m_fileName(fileName),
      m_options(options)
{ }

GenericProjectFile::~GenericProjectFile()
{ }

bool GenericProjectFile::save(QString *, const QString &, bool)
{
    return false;
}

QString GenericProjectFile::fileName() const
{
    return m_fileName;
}

QString GenericProjectFile::defaultPath() const
{
    return QString();
}

QString GenericProjectFile::suggestedFileName() const
{
    return QString();
}

QString GenericProjectFile::mimeType() const
{
    return Constants::GENERICMIMETYPE;
}

bool GenericProjectFile::isModified() const
{
    return false;
}

bool GenericProjectFile::isSaveAsAllowed() const
{
    return false;
}

void GenericProjectFile::rename(const QString &newName)
{
    // Can't happen
    Q_UNUSED(newName);
    QTC_CHECK(false);
}

Core::IDocument::ReloadBehavior GenericProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool GenericProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    if (type == TypePermissions)
        return true;
    m_project->refresh(m_options);
    return true;
}
