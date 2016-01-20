/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cmakeprojectplugin.h"

#include "cmakeeditor.h"
#include "cmakebuildstep.h"
#include "cmakeprojectmanager.h"
#include "cmakebuildconfiguration.h"
#include "cmakerunconfiguration.h"
#include "cmakeprojectconstants.h"
#include "cmakelocatorfilter.h"
#include "cmakesettingspage.h"
#include "cmaketoolmanager.h"
#include "cmakekitinformation.h"

#include <utils/mimetypes/mimedatabase.h>
#include <projectexplorer/kitmanager.h>

using namespace CMakeProjectManager::Internal;

bool CMakeProjectPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    Utils::MimeDatabase::addMimeTypes(QLatin1String(":cmakeproject/CMakeProjectManager.mimetypes.xml"));

    addAutoReleasedObject(new CMakeSettingsPage);
    addAutoReleasedObject(new CMakeManager);
    addAutoReleasedObject(new CMakeBuildStepFactory);
    addAutoReleasedObject(new CMakeRunConfigurationFactory);
    addAutoReleasedObject(new CMakeBuildConfigurationFactory);
    addAutoReleasedObject(new CMakeEditorFactory);
    addAutoReleasedObject(new CMakeLocatorFilter);

    new CMakeToolManager(this);

    ProjectExplorer::KitManager::registerKitInformation(new CMakeKitInformation);
    ProjectExplorer::KitManager::registerKitInformation(new CMakeGeneratorKitInformation);
    ProjectExplorer::KitManager::registerKitInformation(new CMakeConfigurationKitInformation);

    return true;
}

void CMakeProjectPlugin::extensionsInitialized()
{
    //restore the cmake tools before loading the kits
    CMakeToolManager::restoreCMakeTools();
}
