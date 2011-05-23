/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindplugin.h"
#include "valgrindsettings.h"
#include "callgrindtool.h"
#include "callgrindsettings.h"
#include "memchecktool.h"
#include "memchecksettings.h"

#include <analyzerbase/analyzersettings.h>
#include <analyzerbase/analyzermanager.h>

#include <QtCore/QStringList>
#include <QtCore/QtPlugin>

using namespace Analyzer;
using namespace Valgrind::Internal;


static AbstractAnalyzerSubConfig *valgrindConfigFactory()
{
    return new ValgrindSettings();
}

static AbstractAnalyzerSubConfig *globalCallgrindFactory()
{
    return new CallgrindGlobalSettings();
}

static AbstractAnalyzerSubConfig *projectCallgrindFactory()
{
    return new CallgrindProjectSettings();
}

static AbstractAnalyzerSubConfig *globalMemcheckFactory()
{
    return new MemcheckGlobalSettings();
}

static AbstractAnalyzerSubConfig *projectMemcheckFactory()
{
    return new MemcheckProjectSettings();
}

bool ValgrindPlugin::initialize(const QStringList &, QString *)
{
    AnalyzerGlobalSettings::instance()->registerSubConfigs(&valgrindConfigFactory, &valgrindConfigFactory);
    AnalyzerGlobalSettings::instance()->registerSubConfigs(&globalCallgrindFactory, &projectCallgrindFactory);
    AnalyzerGlobalSettings::instance()->registerSubConfigs(&globalMemcheckFactory, &projectMemcheckFactory);

    AnalyzerManager::instance()->addTool(new MemcheckTool(this));
    AnalyzerManager::instance()->addTool(new CallgrindTool(this));
    return true;
}


void ValgrindPlugin::extensionsInitialized()
{
}

Q_EXPORT_PLUGIN(Valgrind::Internal::ValgrindPlugin)
