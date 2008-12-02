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

#include "vcsbaseplugin.h"
#include "diffhighlighter.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/mimedatabase.h>

#include <QtCore/qplugin.h>

namespace VCSBase {
namespace Internal {

VCSBasePlugin *VCSBasePlugin::m_instance = 0;

VCSBasePlugin::VCSBasePlugin()
{
    m_instance = this;
}

VCSBasePlugin::~VCSBasePlugin()
{
    m_instance = 0;
}

bool VCSBasePlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Core::ICore *core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();

    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/vcsbase/VCSBase.mimetypes.xml"), errorMessage))
        return false;

    return true;
}

void VCSBasePlugin::extensionsInitialized()
{
}

VCSBasePlugin *VCSBasePlugin::instance()
{
    return m_instance;
}

} // namespace Internal
} // namespace VCSBase

Q_EXPORT_PLUGIN(VCSBase::Internal::VCSBasePlugin)
