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

#include "cmakeprojectplugin.h"
#include "cmakeprojectmanager.h"
#include "cmakerunconfiguration.h"
#include "cmakestep.h"
#include "makestep.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>


using namespace CMakeProjectManager::Internal;

CMakeProjectPlugin::CMakeProjectPlugin()
{
}

CMakeProjectPlugin::~CMakeProjectPlugin()
{
}

bool CMakeProjectPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":cmakeproject/CMakeProject.mimetypes.xml"), errorMessage))
        return false;
    CMakeSettingsPage *cmp = new CMakeSettingsPage();
    addAutoReleasedObject(cmp);
    addAutoReleasedObject(new CMakeManager(cmp));
    addAutoReleasedObject(new CMakeBuildStepFactory());
    addAutoReleasedObject(new MakeBuildStepFactory());
    addAutoReleasedObject(new CMakeRunConfigurationFactory());
    return true;
}

void CMakeProjectPlugin::extensionsInitialized()
{
}

Q_EXPORT_PLUGIN(CMakeProjectPlugin)
