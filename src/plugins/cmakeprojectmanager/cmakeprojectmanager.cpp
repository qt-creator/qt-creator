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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#include "cmakeprojectmanager.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace CMakeProjectManager::Internal;

CMakeManager::CMakeManager()
{
    Core::ICore *core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    m_projectContext = core->uniqueIDManager()->uniqueIdentifier(CMakeProjectManager::Constants::PROJECTCONTEXT);
    m_projectLanguage = core->uniqueIDManager()->uniqueIdentifier(ProjectExplorer::Constants::LANG_CXX);
}

int CMakeManager::projectContext() const
{
    return m_projectContext;
}

int CMakeManager::projectLanguage() const
{
    return m_projectLanguage;
}

ProjectExplorer::Project *CMakeManager::openProject(const QString &fileName)
{
    // TODO check wheter this project is already opened
    return new CMakeProject(this, fileName);
}

QString CMakeManager::mimeType() const
{
    return Constants::CMAKEMIMETYPE;
}
