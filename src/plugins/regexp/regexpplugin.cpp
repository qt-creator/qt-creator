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
***************************************************************************/
#include "regexpplugin.h"
#include "settings.h"
#include "regexpwindow.h"

#include <coreplugin/baseview.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>

#include <QtCore/qplugin.h>

using namespace RegExp::Internal;

RegExpPlugin::RegExpPlugin()
{
}

RegExpPlugin::~RegExpPlugin()
{
    if (m_regexpWindow) {
        m_regexpWindow->settings().toQSettings(m_core->settings());
    }
}

void RegExpPlugin::extensionsInitialized()
{
}


bool RegExpPlugin::initialize(const QStringList & /*arguments*/, QString *error_message)
{
    Q_UNUSED(error_message)
    m_core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    m_regexpWindow = new RegExpWindow;
    Settings settings;
    settings.fromQSettings(m_core->settings());
    m_regexpWindow->setSettings(settings);
    const int plugId = m_core->uniqueIDManager()->uniqueIdentifier(QLatin1String("RegExpPlugin"));
    addAutoReleasedObject(new Core::BaseView("TextEditor.RegExpWindow",
                                             m_regexpWindow,
                                             QList<int>() << plugId,
                                             Qt::RightDockWidgetArea));
    return true;
}

Q_EXPORT_PLUGIN(RegExpPlugin)
