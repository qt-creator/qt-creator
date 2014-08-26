/****************************************************************************
**
** Copyright (C) 2014 Tim Sander <tim@krieglstein.org>
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

#include "baremetalplugin.h"
#include "baremetalconstants.h"
#include "baremetaldeviceconfigurationfactory.h"
#include "baremetalruncontrolfactory.h"
#include "baremetalrunconfigurationfactory.h"

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

   return true;
}

} // namespace Internal
} // namespace BareMetal

