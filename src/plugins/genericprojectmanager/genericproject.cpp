/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "genericproject.h"
#include "genericprojectconstants.h"
#include "genericmakestep.h"

#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QtDebug>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QProcess>
#include <QtCore/QCoreApplication>

#include <QtGui/QFormLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QComboBox>
#include <QtGui/QStringListModel>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

namespace {

/**
 * An editable string list model. New strings can be added by editing the entry
 * called "<new>", displayed at the end.
 */
class ListModel: public QStringListModel
{
public:
    ListModel(QObject *parent)
        : QStringListModel(parent) {}

    virtual ~ListModel() {}

    virtual int rowCount(const QModelIndex &parent) const
    { return 1 + QStringListModel::rowCount(parent); }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const
    { return QStringListModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled; }

    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const
    {
        if (row == stringList().size())
            return createIndex(row, column);

        return QStringListModel::index(row, column, parent);
    }

    virtual QVariant data(const QModelIndex &index, int role) const
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (index.row() == stringList().size())
                return QCoreApplication::translate("GenericProject", "<new>");
        }

        return QStringListModel::data(index, role);
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (role == Qt::EditRole && index.row() == stringList().size())
            insertRow(index.row(), QModelIndex());

        return QStringListModel::setData(index, value, role);
    }
};

} // end of anonymous namespace

////////////////////////////////////////////////////////////////////////////////////
// GenericProject
////////////////////////////////////////////////////////////////////////////////////

GenericProject::GenericProject(Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_toolChain(0)
{
    QFileInfo fileInfo(m_fileName);
    QDir dir = fileInfo.dir();

    m_projectName      = fileInfo.completeBaseName();
    m_filesFileName    = QFileInfo(dir, m_projectName + QLatin1String(".files")).absoluteFilePath();
    m_includesFileName = QFileInfo(dir, m_projectName + QLatin1String(".includes")).absoluteFilePath();
    m_configFileName   = QFileInfo(dir, m_projectName + QLatin1String(".config")).absoluteFilePath();

    m_file = new GenericProjectFile(this, fileName);
    m_rootNode = new GenericProjectNode(this, m_file);

    m_manager->registerProject(this);
}

GenericProject::~GenericProject()
{
    m_manager->unregisterProject(this);

    delete m_rootNode;
    delete m_toolChain;
}

QString GenericProject::filesFileName() const
{ return m_filesFileName; }

QString GenericProject::includesFileName() const
{ return m_includesFileName; }

QString GenericProject::configFileName() const
{ return m_configFileName; }

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

            line = line.trimmed();
            if (line.isEmpty())
                continue;

            lines.append(line);
        }
    }

    return lines;
}


void GenericProject::parseProject(RefreshOptions options)
{
    if (options & Files)
        m_files = convertToAbsoluteFiles(readLines(filesFileName()));

    if (options & Configuration) {
        m_projectIncludePaths = convertToAbsoluteFiles(readLines(includesFileName()));

        QSettings projectInfo(m_fileName, QSettings::IniFormat);
        m_generated = convertToAbsoluteFiles(projectInfo.value(QLatin1String("generated")).toStringList());

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

    CppTools::CppModelManagerInterface *modelManager =
        ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();

    if (m_toolChain && modelManager) {
        const QByteArray predefinedMacros = m_toolChain->predefinedMacros();
        const QList<ProjectExplorer::HeaderPath> systemHeaderPaths = m_toolChain->systemHeaderPaths();

        CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelManager->projectInfo(this);
        pinfo.defines = predefinedMacros;
        pinfo.defines += '\n';
        pinfo.defines += m_defines;

        QStringList allIncludePaths, allFrameworkPaths;

        foreach (const ProjectExplorer::HeaderPath &headerPath, m_toolChain->systemHeaderPaths()) {
            if (headerPath.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath)
                allFrameworkPaths.append(headerPath.path());
            else
                allIncludePaths.append(headerPath.path());
        }

        allIncludePaths += this->allIncludePaths();

        pinfo.frameworkPaths = allFrameworkPaths;
        pinfo.includePaths = allIncludePaths;

        // ### add _defines.
        pinfo.sourceFiles = files();
        pinfo.sourceFiles += generated();

        QStringList filesToUpdate;

        if (options & Configuration) {
            filesToUpdate = pinfo.sourceFiles;
            filesToUpdate.append(QLatin1String("<configuration>")); // XXX don't hardcode configuration file name
        } else if (options & Files) {
            // Only update files that got added to the list
            QSet<QString> newFileList = m_files.toSet();
            newFileList.subtract(oldFileList);
            filesToUpdate.append(newFileList.toList());
        }

        modelManager->updateProjectInfo(pinfo);
        modelManager->updateSourceFiles(filesToUpdate);
    }
}

QStringList GenericProject::convertToAbsoluteFiles(const QStringList &paths) const
{
    const QDir projectDir(QFileInfo(m_fileName).dir());
    QStringList absolutePaths;
    foreach (const QString &file, paths) {
        QFileInfo fileInfo(projectDir, file);
        absolutePaths.append(fileInfo.absoluteFilePath());
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
{ return m_projectIncludePaths; }

QStringList GenericProject::files() const
{ return m_files; }

QStringList GenericProject::generated() const
{ return m_generated; }

QStringList GenericProject::includePaths() const
{ return m_includePaths; }

void GenericProject::setIncludePaths(const QStringList &includePaths)
{ m_includePaths = includePaths; }

QByteArray GenericProject::defines() const
{ return m_defines; }

void GenericProject::setToolChainId(const QString &toolChainId)
{
    using namespace ProjectExplorer;

    m_toolChainId = toolChainId;

    delete m_toolChain;
    m_toolChain = 0;

    if (toolChainId == QLatin1String("mingw")) {
        const QLatin1String qmake_cxx("g++"); // ### FIXME
        const QString mingwDirectory; // ### FIXME

        m_toolChain = ToolChain::createMinGWToolChain(qmake_cxx, mingwDirectory);

    } else if (toolChainId == QLatin1String("msvc")) {
        const QString msvcVersion; // ### FIXME
        m_toolChain = ToolChain::createMSVCToolChain(msvcVersion, false);

    } else if (toolChainId == QLatin1String("wince")) {
        const QString msvcVersion, wincePlatform; // ### FIXME
        m_toolChain = ToolChain::createWinCEToolChain(msvcVersion, wincePlatform);

    } else if (toolChainId == QLatin1String("gcc") || toolChainId == QLatin1String("icc")) {
        const QLatin1String qmake_cxx("g++"); // ### FIXME
        m_toolChain = ToolChain::createGccToolChain(qmake_cxx);
    }
}

QString GenericProject::buildParser(const QString &buildConfiguration) const
{
    if (m_toolChain) {
        switch (m_toolChain->type()) {
        case ProjectExplorer::ToolChain::GCC:
        case ProjectExplorer::ToolChain::LinuxICC:
        case ProjectExplorer::ToolChain::MinGW:
            return QLatin1String(ProjectExplorer::Constants::BUILD_PARSER_GCC);

        case ProjectExplorer::ToolChain::MSVC:
        case ProjectExplorer::ToolChain::WINCE:
            return ProjectExplorer::Constants::BUILD_PARSER_MSVC;

        default:
            break;
        } // switch
    }

    return QString();
}

ProjectExplorer::ToolChain *GenericProject::toolChain() const
{
    return m_toolChain;
}

QString GenericProject::toolChainId() const
{ return m_toolChainId; }

QString GenericProject::name() const
{
    return m_projectName;
}

Core::IFile *GenericProject::file() const
{
    return m_file;
}

ProjectExplorer::IProjectManager *GenericProject::projectManager() const
{
    return m_manager;
}

QList<ProjectExplorer::Project *> GenericProject::dependsOn()
{
    return QList<Project *>();
}

bool GenericProject::isApplication() const
{
    return true;
}

ProjectExplorer::Environment GenericProject::environment(const QString &) const
{
    return ProjectExplorer::Environment::systemEnvironment();
}

QString GenericProject::buildDirectory(const QString &buildConfiguration) const
{
    QString buildDirectory = value(buildConfiguration, "buildDirectory").toString();

    if (buildDirectory.isEmpty()) {
        QFileInfo fileInfo(m_fileName);

        buildDirectory = fileInfo.absolutePath();
    }

    return buildDirectory;
}

ProjectExplorer::BuildStepConfigWidget *GenericProject::createConfigWidget()
{
    return new GenericBuildSettingsWidget(this);
}

QList<ProjectExplorer::BuildStepConfigWidget*> GenericProject::subConfigWidgets()
{
    return QList<ProjectExplorer::BuildStepConfigWidget*>();
}

 void GenericProject::newBuildConfiguration(const QString &buildConfiguration)
 {
     makeStep()->setBuildTarget(buildConfiguration, "all", true);
 }

GenericProjectNode *GenericProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList GenericProject::files(FilesMode fileMode) const
{
    return m_files; // ### TODO: handle generated files here.
}

QStringList GenericProject::targets() const
{
    QStringList targets;
    targets.append(QLatin1String("all"));
    targets.append(QLatin1String("clean"));
    return targets;
}

GenericMakeStep *GenericProject::makeStep() const
{
    foreach (ProjectExplorer::BuildStep *bs, buildSteps()) {
        if (GenericMakeStep *ms = qobject_cast<GenericMakeStep *>(bs))
            return ms;
    }
    return 0;
}

void GenericProject::restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader)
{
    Project::restoreSettingsImpl(reader);

    if (buildConfigurations().isEmpty()) {
        GenericMakeStep *makeStep = new GenericMakeStep(this);
        insertBuildStep(0, makeStep);

        const QLatin1String all("all");

        addBuildConfiguration(all);
        setActiveBuildConfiguration(all);
        makeStep->setBuildTarget(all, all, /* on = */ true);

        const QLatin1String buildDirectory("buildDirectory");

        const QFileInfo fileInfo(file()->fileName());
        setValue(all, buildDirectory, fileInfo.absolutePath());
    }

    QString toolChainId = reader.restoreValue(QLatin1String("toolChain")).toString();
    if (toolChainId.isEmpty())
        toolChainId = QLatin1String("gcc");

    setToolChainId(toolChainId.toLower()); // ### move

    const QStringList userIncludePaths =
            reader.restoreValue(QLatin1String("includePaths")).toStringList();

    setIncludePaths(allIncludePaths());

    refresh(Everything);
}

void GenericProject::saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer)
{
    Project::saveSettingsImpl(writer);

    writer.saveValue(QLatin1String("toolChain"), m_toolChainId);
    writer.saveValue(QLatin1String("includePaths"), m_includePaths);
}

////////////////////////////////////////////////////////////////////////////////////
// GenericBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////

GenericBuildSettingsWidget::GenericBuildSettingsWidget(GenericProject *project)
    : m_project(project)
{
    QFormLayout *fl = new QFormLayout(this);

    // build directory
    m_pathChooser = new Core::Utils::PathChooser(this);
    m_pathChooser->setEnabled(true);
    fl->addRow(tr("Build directory:"), m_pathChooser);
    connect(m_pathChooser, SIGNAL(changed(QString)), this, SLOT(buildDirectoryChanged()));

    // tool chain
    QComboBox *toolChainChooser = new QComboBox;
    toolChainChooser->addItems(ProjectExplorer::ToolChain::supportedToolChains());
    toolChainChooser->setCurrentIndex(toolChainChooser->findText(m_project->toolChainId()));
    fl->addRow(tr("Toolchain:"), toolChainChooser);
    connect(toolChainChooser, SIGNAL(activated(QString)), m_project, SLOT(setToolChainId(QString)));
}

GenericBuildSettingsWidget::~GenericBuildSettingsWidget()
{ }

QString GenericBuildSettingsWidget::displayName() const
{ return tr("Generic Manager"); }

void GenericBuildSettingsWidget::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;
    m_pathChooser->setPath(m_project->buildDirectory(buildConfiguration));
}

void GenericBuildSettingsWidget::buildDirectoryChanged()
{
    m_project->setValue(m_buildConfiguration, "buildDirectory", m_pathChooser->path());
}

////////////////////////////////////////////////////////////////////////////////////
// GenericProjectFile
////////////////////////////////////////////////////////////////////////////////////

GenericProjectFile::GenericProjectFile(GenericProject *parent, QString fileName)
    : Core::IFile(parent),
      m_project(parent),
      m_fileName(fileName)
{ }

GenericProjectFile::~GenericProjectFile()
{ }

bool GenericProjectFile::save(const QString &)
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

bool GenericProjectFile::isReadOnly() const
{
    return true;
}

bool GenericProjectFile::isSaveAsAllowed() const
{
    return false;
}

void GenericProjectFile::modified(ReloadBehavior *)
{
}
