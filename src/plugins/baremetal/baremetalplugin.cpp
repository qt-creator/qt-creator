/****************************************************************************
**
** Copyright (C) 2015 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2015 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baremetalplugin.h"
#include "baremetalconstants.h"
#include "baremetaldeviceconfigurationfactory.h"
#include "baremetalruncontrolfactory.h"
#include "baremetalrunconfigurationfactory.h"

#include "gdbserverproviderssettingspage.h"
#include "gdbserverprovidermanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <QtPlugin>

namespace BareMetal {
namespace Internal {

BareMetalPlugin::BareMetalPlugin()
{
    setObjectName(QLatin1String("BareMetalPlugin"));
}

BareMetalPlugin::~BareMetalPlugin()
{
}

bool BareMetalPlugin::initialize(const QStringList &arguments, QString *errorString)
{
   Q_UNUSED(arguments)
   Q_UNUSED(errorString)

   addAutoReleasedObject(new BareMetalDeviceConfigurationFactory);
   addAutoReleasedObject(new BareMetalRunControlFactory);
   addAutoReleasedObject(new BareMetalRunConfigurationFactory);
   addAutoReleasedObject(new GdbServerProvidersSettingsPage);

   return true;
}

void BareMetalPlugin::extensionsInitialized()
{
    GdbServerProviderManager::instance()->restoreProviders();
}

} // namespace Internal
} // namespace BareMetal
