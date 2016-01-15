/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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
