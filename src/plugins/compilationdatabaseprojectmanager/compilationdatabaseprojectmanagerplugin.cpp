/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "compilationdatabaseprojectmanagerplugin.h"

#include "compilationdatabaseconstants.h"
#include "compilationdatabaseproject.h"
#include "compilationdatabasetests.h"

#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectmanager.h>
#include <utils/utilsicons.h>

namespace CompilationDatabaseProjectManager {
namespace Internal {

bool CompilationDatabaseProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);
    Core::FileIconProvider::registerIconOverlayForFilename(Utils::Icons::PROJECT.imageFileName(),
                                                           "compile_commands.json");

    ProjectExplorer::ProjectManager::registerProjectType<CompilationDatabaseProject>(
                Constants::COMPILATIONDATABASEMIMETYPE);

    return true;
}

void CompilationDatabaseProjectManagerPlugin::extensionsInitialized()
{
}

QList<QObject *> CompilationDatabaseProjectManagerPlugin::createTestObjects() const
{
    QList<QObject *> tests;
#ifdef WITH_TESTS
    tests << new CompilationDatabaseTests;
#endif
    return tests;
}

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
