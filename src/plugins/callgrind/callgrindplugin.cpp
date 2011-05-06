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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "callgrindplugin.h"

#include "callgrindtool.h"
#include "callgrindsettings.h"

#include <analyzerbase/analyzermanager.h>

#include <QStringList>
#include <QtPlugin>

using namespace Analyzer;
using namespace Callgrind;
using namespace Callgrind::Internal;

CallgrindPlugin::CallgrindPlugin()
{

}

CallgrindPlugin::~CallgrindPlugin()
{

}

bool CallgrindPlugin::initialize(const QStringList &/*arguments*/, QString */*errorString*/)
{
    typedef AnalyzerSubConfigFactory<CallgrindGlobalSettings, CallgrindProjectSettings> CallgrindConfigFactory;
    AnalyzerGlobalSettings::instance()->registerSubConfigFactory(new CallgrindConfigFactory);
    AnalyzerManager::instance()->addTool(new CallgrindTool(this));

    return true;
}


void CallgrindPlugin::extensionsInitialized()
{

}

Q_EXPORT_PLUGIN(CallgrindPlugin)
