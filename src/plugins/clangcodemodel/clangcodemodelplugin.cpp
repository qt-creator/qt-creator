/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangcodemodelplugin.h"
#include "clangprojectsettingspropertiespage.h"
#include "fastindexer.h"
#include "pchmanager.h"
#include "utils.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/id.h>

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QtPlugin>

namespace ClangCodeModel {
namespace Internal {

bool ClangCodeModelPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    auto panelFactory = new ProjectExplorer::IProjectPanelFactory();
    panelFactory->setPriority(60);
    panelFactory->setDisplayName(ClangProjectSettingsWidget::tr("Clang Settings"));
    panelFactory->setSimpleCreatePanelFunction<ClangProjectSettingsWidget>(QIcon());

    addAutoReleasedObject(panelFactory);

    ClangCodeModel::Internal::initializeClang();

    PchManager *pchManager = new PchManager(this);
    FastIndexer *fastIndexer = 0;

#ifdef CLANG_INDEXING
    m_indexer.reset(new ClangIndexer);
    fastIndexer = m_indexer.data();
    CppTools::CppModelManagerInterface::instance()->setIndexingSupport(m_indexer->indexingSupport());
#endif // CLANG_INDEXING

    // wire up the pch manager
    QObject *session = ProjectExplorer::SessionManager::instance();
    connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            pchManager, SLOT(onAboutToRemoveProject(ProjectExplorer::Project*)));
    connect(CppTools::CppModelManagerInterface::instance(), SIGNAL(projectPartsUpdated(ProjectExplorer::Project*)),
            pchManager, SLOT(onProjectPartsUpdated(ProjectExplorer::Project*)));

    m_modelManagerSupport.reset(new ModelManagerSupport(fastIndexer));
    CppTools::CppModelManagerInterface::instance()->addModelManagerSupport(
                m_modelManagerSupport.data());

    return true;
}

void ClangCodeModelPlugin::extensionsInitialized()
{
}

} // namespace Internal
} // namespace Clang

Q_EXPORT_PLUGIN(ClangCodeModel::Internal::ClangCodeModelPlugin)
