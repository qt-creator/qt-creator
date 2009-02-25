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

#include "vcsbaseplugin.h"
#include "diffhighlighter.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/mimedatabase.h>

#include <QtCore/QtPlugin>

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

bool VCSBasePlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    Core::ICore *core = Core::ICore::instance();
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
