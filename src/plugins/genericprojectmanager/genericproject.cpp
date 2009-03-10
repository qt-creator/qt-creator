/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "genericproject.h"
#include "genericprojectconstants.h"
#include "genericprojectnodes.h"
#include "makestep.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <cpptools/cppmodelmanagerinterface.h>

#include <QtDebug>
#include <QDir>
#include <QSettings>

#include <QProcess>
#include <QFormLayout>
#include <QMainWindow>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

////////////////////////////////////////////////////////////////////////////////////
// GenericProject
////////////////////////////////////////////////////////////////////////////////////
GenericProject::GenericProject(Manager *manager, const QString &fileName)
    : _manager(manager),
      _fileName(fileName),
      _toolChain(0)
{
    qDebug() << Q_FUNC_INFO;

    _file = new GenericProjectFile(this, fileName);
    _rootNode = new GenericProjectNode(_file);

    refresh();
}

GenericProject::~GenericProject()
{
    qDebug() << Q_FUNC_INFO;

    delete _rootNode;
    delete _toolChain;
}

void GenericProject::refresh()
{
    qDebug() << Q_FUNC_INFO;

    _rootNode->refresh();

    setToolChain(_rootNode->toolChainId());

    CppTools::CppModelManagerInterface *modelManager =
        ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();

    if (_toolChain && modelManager) {
        const QByteArray predefinedMacros = _toolChain->predefinedMacros();
        const QList<ProjectExplorer::HeaderPath> systemHeaderPaths = _toolChain->systemHeaderPaths();

        CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelManager->projectInfo(this);
        pinfo.defines = predefinedMacros;

        QStringList allIncludePaths, allFrameworkPaths;

        foreach (const ProjectExplorer::HeaderPath &headerPath, _toolChain->systemHeaderPaths()) {
            if (headerPath.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath)
                allFrameworkPaths.append(headerPath.path());

            else
                allIncludePaths.append(headerPath.path());
        }

        allIncludePaths += _rootNode->includePaths();

        pinfo.frameworkPaths = allFrameworkPaths;
        pinfo.includePaths = allIncludePaths;

        // ### add _defines.
        pinfo.sourceFiles = _rootNode->files();
        pinfo.sourceFiles += _rootNode->generated();

        modelManager->updateProjectInfo(pinfo);
        modelManager->updateSourceFiles(pinfo.sourceFiles);
    }
}

void GenericProject::setToolChain(const QString &toolChainId)
{
    using namespace ProjectExplorer;

    qDebug() << Q_FUNC_INFO;

    delete _toolChain;
    _toolChain = 0;

    if (toolChainId == QLatin1String("mingw")) {
        const QLatin1String qmake_cxx("g++"); // ### FIXME
        const QString mingwDirectory; // ### FIXME

        _toolChain = ToolChain::createMinGWToolChain(qmake_cxx, mingwDirectory);

    } else if (toolChainId == QLatin1String("msvc")) {
        const QString msvcVersion; // ### FIXME
        _toolChain = ToolChain::createMSVCToolChain(msvcVersion);

    } else if (toolChainId == QLatin1String("wince")) {
        const QString msvcVersion, wincePlatform; // ### FIXME
        _toolChain = ToolChain::createWinCEToolChain(msvcVersion, wincePlatform);

    } else if (toolChainId == QLatin1String("gcc") || toolChainId == QLatin1String("icc")) {
        const QLatin1String qmake_cxx("g++"); // ### FIXME
        _toolChain = ToolChain::createGccToolChain(qmake_cxx);

    }
}

QString GenericProject::buildParser(const QString &buildConfiguration) const
{
    qDebug() << Q_FUNC_INFO;

    if (_toolChain) {
        switch (_toolChain->type()) {
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

QString GenericProject::name() const
{
    qDebug() << Q_FUNC_INFO;

    return _projectName;
}

Core::IFile *GenericProject::file() const
{
    qDebug() << Q_FUNC_INFO;

    return _file;
}

ProjectExplorer::IProjectManager *GenericProject::projectManager() const
{
    qDebug() << Q_FUNC_INFO;

    return _manager;
}

QList<ProjectExplorer::Project *> GenericProject::dependsOn()
{
    qDebug() << Q_FUNC_INFO;

    return QList<Project *>();
}

bool GenericProject::isApplication() const
{
    qDebug() << Q_FUNC_INFO;

    return true;
}

ProjectExplorer::Environment GenericProject::environment(const QString &) const
{
    qDebug() << Q_FUNC_INFO;

    return ProjectExplorer::Environment::systemEnvironment();
}

QString GenericProject::buildDirectory(const QString &buildConfiguration) const
{
    qDebug() << Q_FUNC_INFO;

    QString buildDirectory = value(buildConfiguration, "buildDirectory").toString();

    if (buildDirectory.isEmpty()) {
        QFileInfo fileInfo(_fileName);

        buildDirectory = fileInfo.absolutePath();
    }

    return buildDirectory;
}

ProjectExplorer::BuildStepConfigWidget *GenericProject::createConfigWidget()
{
    qDebug() << Q_FUNC_INFO;

    return new GenericBuildSettingsWidget(this);
}

QList<ProjectExplorer::BuildStepConfigWidget*> GenericProject::subConfigWidgets()
{
    qDebug() << Q_FUNC_INFO;

    return QList<ProjectExplorer::BuildStepConfigWidget*>();
}

 void GenericProject::newBuildConfiguration(const QString &buildConfiguration)
 {
     qDebug() << Q_FUNC_INFO;

     makeStep()->setBuildTarget(buildConfiguration, "all", true);
 }

ProjectExplorer::ProjectNode *GenericProject::rootProjectNode() const
{
    qDebug() << Q_FUNC_INFO;

    return _rootNode;
}

QStringList GenericProject::files(FilesMode fileMode) const
{
    qDebug() << Q_FUNC_INFO;

    return _rootNode->files();
}

void GenericProject::saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer)
{
    qDebug() << Q_FUNC_INFO;

    Project::saveSettingsImpl(writer);
}

QStringList GenericProject::targets() const
{
    QStringList targets;
    targets.append(QLatin1String("all"));
    targets.append(QLatin1String("clean"));
    return targets;
}

MakeStep *GenericProject::makeStep() const
{
    qDebug() << Q_FUNC_INFO;

    foreach (ProjectExplorer::BuildStep *bs, buildSteps()) {
        if (MakeStep *ms = qobject_cast<MakeStep *>(bs))
            return ms;
    }

    return 0;
}

void GenericProject::restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader)
{
    Project::restoreSettingsImpl(reader);

    if (buildConfigurations().isEmpty()) {
        MakeStep *makeStep = new MakeStep(this);
        insertBuildStep(0, makeStep);

        const QLatin1String all("all");

        addBuildConfiguration(all);
        setActiveBuildConfiguration(all);
        makeStep->setBuildTarget(all, all, /* on = */ true);

        const QLatin1String buildDirectory("buildDirectory");

        const QFileInfo fileInfo(file()->fileName());
        setValue(all, buildDirectory, fileInfo.absolutePath());
    }
}

////////////////////////////////////////////////////////////////////////////////////
// GenericBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////
GenericBuildSettingsWidget::GenericBuildSettingsWidget(GenericProject *project)
    : _project(project)
{
    qDebug() << Q_FUNC_INFO;

    QFormLayout *fl = new QFormLayout(this);
    
    _pathChooser = new Core::Utils::PathChooser(this);
    _pathChooser->setEnabled(true);

    fl->addRow("Build directory:", _pathChooser);

    connect(_pathChooser, SIGNAL(changed()), this, SLOT(buildDirectoryChanged()));
}

GenericBuildSettingsWidget::~GenericBuildSettingsWidget()
{ }

QString GenericBuildSettingsWidget::displayName() const
{ return tr("Generic Manager"); }

void GenericBuildSettingsWidget::init(const QString &buildConfiguration)
{
    qDebug() << Q_FUNC_INFO;

    _buildConfiguration = buildConfiguration;
    _pathChooser->setPath(_project->buildDirectory(buildConfiguration));
}

void GenericBuildSettingsWidget::buildDirectoryChanged()
{
    qDebug() << Q_FUNC_INFO;

    _project->setValue(_buildConfiguration, "buildDirectory", _pathChooser->path());
}


////////////////////////////////////////////////////////////////////////////////////
// GenericProjectFile
////////////////////////////////////////////////////////////////////////////////////
GenericProjectFile::GenericProjectFile(GenericProject *parent, QString fileName)
    : Core::IFile(parent),
      _project(parent),
      _fileName(fileName)
{ }

GenericProjectFile::~GenericProjectFile()
{ }

bool GenericProjectFile::save(const QString &)
{
    qDebug() << Q_FUNC_INFO;
    return false;
}

QString GenericProjectFile::fileName() const
{
    qDebug() << Q_FUNC_INFO;
    return _fileName;
}

QString GenericProjectFile::defaultPath() const
{
    qDebug() << Q_FUNC_INFO;
    return QString();
}

QString GenericProjectFile::suggestedFileName() const
{
    qDebug() << Q_FUNC_INFO;
    return QString();
}

QString GenericProjectFile::mimeType() const
{
    qDebug() << Q_FUNC_INFO;
    return Constants::GENERICMIMETYPE;
}

bool GenericProjectFile::isModified() const
{
    qDebug() << Q_FUNC_INFO;
    return false;
}

bool GenericProjectFile::isReadOnly() const
{
    qDebug() << Q_FUNC_INFO;
    return true;
}

bool GenericProjectFile::isSaveAsAllowed() const
{
    qDebug() << Q_FUNC_INFO;
    return false;
}

void GenericProjectFile::modified(ReloadBehavior *)
{
    qDebug() << Q_FUNC_INFO;
}

