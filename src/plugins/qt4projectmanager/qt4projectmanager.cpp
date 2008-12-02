/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "qt4projectmanager.h"

#include "qt4projectmanagerconstants.h"
#include "qt4projectmanagerplugin.h"
#include "qt4nodes.h"
#include "qt4project.h"
#include "profilecache.h"
#include "profilereader.h"
#include "qtversionmanager.h"
#include "qmakestep.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/basefilewizard.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <projectexplorer/project.h>
#include <utils/listutils.h>

#include <QtCore/QVariant>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QLinkedList>
#include <QtGui/QMenu>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

using ProjectExplorer::BuildStep;
using ProjectExplorer::FileType;
using ProjectExplorer::HeaderType;
using ProjectExplorer::SourceType;
using ProjectExplorer::FormType;
using ProjectExplorer::ResourceType;
using ProjectExplorer::UnknownFileType;

// Known file types of a Qt 4 project
static const char* qt4FileTypes[] = {"CppHeaderFiles", "CppSourceFiles", "Qt4FormFiles", "Qt4ResourceFiles" };

Qt4Manager::Qt4Manager(Qt4ProjectManagerPlugin *plugin, Core::ICore *core) :
    m_mimeType(QLatin1String(Qt4ProjectManager::Constants::PROFILE_MIMETYPE)),
    m_plugin(plugin),
    m_core(core),
    m_projectExplorer(0),
    m_contextProject(0),
    m_languageID(0),
    m_proFileCache(0)
{
    m_languageID = m_core->uniqueIDManager()->
        uniqueIdentifier(ProjectExplorer::Constants::LANG_CXX);
    m_proFileCache = new ProFileCache(this);
}

Qt4Manager::~Qt4Manager()
{
}

void Qt4Manager::init()
{
    m_projectExplorer = m_core->pluginManager()->getObject<ProjectExplorer::ProjectExplorerPlugin>();
}

int Qt4Manager::projectContext() const
{
     return m_plugin->projectContext();
}

int Qt4Manager::projectLanguage() const
{
    return m_languageID;
}

QString Qt4Manager::mimeType() const
{
    return m_mimeType;
}

ProjectExplorer::Project* Qt4Manager::openProject(const QString &fileName)
{
    typedef QMultiMap<QString, QString> DependencyMap;
    const QString dotSubDir = QLatin1String(".subdir");
    const QString dotDepends = QLatin1String(".depends");
    const QChar slash = QLatin1Char('/');

    QString errorMessage;

    m_core->messageManager()->displayStatusBarMessage(tr("Loading project %1 ...").arg(fileName), 50000);

    // TODO Make all file paths relative & remove this hack
    // We convert the path to an absolute one here because qt4project.cpp
    // && profileevaluator use absolute/canonical file paths all over the place
    // Correct fix would be to remove these calls ...
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    if (canonicalFilePath.isEmpty()) {
        m_core->messageManager()->printToOutputPane(tr("Failed opening project '%1': Project file does not exist").arg(canonicalFilePath));
        m_core->messageManager()->displayStatusBarMessage(tr("Failed opening project"), 5000);
        return 0;
    }

    foreach (ProjectExplorer::Project *pi, projectExplorer()->session()->projects()) {
        if (canonicalFilePath == pi->file()->fileName()) {
            m_core->messageManager()->printToOutputPane(tr("Failed opening project '%1': Project already open").arg(canonicalFilePath));
            m_core->messageManager()->displayStatusBarMessage(tr("Failed opening project"), 5000);
            return 0;
        }
    }

    m_core->messageManager()->displayStatusBarMessage(tr("Opening %1 ...").arg(fileName));
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    Qt4Project *pro = new Qt4Project(this, canonicalFilePath);

    m_core->messageManager()->displayStatusBarMessage(tr("Done opening project"), 5000);
    return pro;
}

ProjectExplorer::ProjectExplorerPlugin *Qt4Manager::projectExplorer() const
{
    return m_projectExplorer;
}

Core::ICore *Qt4Manager::core() const
{
    return m_core;
}

ExtensionSystem::PluginManager *Qt4Manager::pluginManager() const
{
    return m_core->pluginManager();
}

ProjectExplorer::Node *Qt4Manager::contextNode() const
{
    return m_contextNode;
}

void Qt4Manager::setContextNode(ProjectExplorer::Node *node)
{
    m_contextNode = node;
}

void Qt4Manager::setContextProject(ProjectExplorer::Project *project)
{
    m_contextProject = project;
}

ProjectExplorer::Project *Qt4Manager::contextProject() const
{
    return m_contextProject;
}

QtVersionManager *Qt4Manager::versionManager() const
{
    return m_plugin->versionManager();
}

void Qt4Manager::runQMake()
{
    runQMake(m_projectExplorer->currentProject());
}

void Qt4Manager::runQMakeContextMenu()
{
    runQMake(m_contextProject);
}

void Qt4Manager::runQMake(ProjectExplorer::Project *p)
{
    QMakeStep *qmakeStep = qobject_cast<Qt4Project *>(p)->qmakeStep();
    //found qmakeStep, now use it
    qmakeStep->setForced(true);
    const QString &config = p->activeBuildConfiguration();
    m_projectExplorer->buildManager()->appendStep(qmakeStep, config);
}

QString Qt4Manager::fileTypeId(ProjectExplorer::FileType type)
{
    switch (type) {
    case HeaderType:
        return QLatin1String(qt4FileTypes[0]);
    case SourceType:
        return QLatin1String(qt4FileTypes[1]);
    case FormType:
        return QLatin1String(qt4FileTypes[2]);
    case ResourceType:
        return QLatin1String(qt4FileTypes[3]);
    case UnknownFileType:
    default:
        break;
    }
    return QString();
}
