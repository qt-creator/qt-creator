/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "regexpplugin.h"

#include "regexpwindow.h"
#include "settings.h"

#include <coreplugin/baseview.h>
#include <coreplugin/icore.h>

#include <QtCore/QtPlugin>

using namespace RegExp::Internal;

RegExpPlugin::RegExpPlugin()
{
}

RegExpPlugin::~RegExpPlugin()
{
    if (m_regexpWindow)
        m_regexpWindow->settings().toQSettings(Core::ICore::instance()->settings());
}

void RegExpPlugin::extensionsInitialized()
{
}

bool RegExpPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    Core::ICore *core = Core::ICore::instance();
    m_regexpWindow = new RegExpWindow;
    Settings settings;
    settings.fromQSettings(core->settings());
    m_regexpWindow->setSettings(settings);
    addAutoReleasedObject(new Core::BaseView("TextEditor.RegExpWindow",
                                             m_regexpWindow,
                                             Core::Contect("RegExpPlugin"),
                                             plugId,
                                             Qt::RightDockWidgetArea));
    return true;
}

Q_EXPORT_PLUGIN(RegExpPlugin)
