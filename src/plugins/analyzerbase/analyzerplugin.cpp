/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "analyzerplugin.h"
#include "analyzerconstants.h"
#include "analyzermanager.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>

#include <QtPlugin>

using namespace Analyzer;
using namespace Analyzer::Internal;

static AnalyzerPlugin *m_instance = 0;

////////////////////////////////////////////////////////////////////////
//
// AnalyzerPlugin
//
////////////////////////////////////////////////////////////////////////

AnalyzerPlugin::AnalyzerPlugin()
{
    m_instance = this;
}

AnalyzerPlugin::~AnalyzerPlugin()
{
    m_instance = 0;
}

bool AnalyzerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    (void) new AnalyzerManager(this);

    // Task integration.
    //: Category under which Analyzer tasks are listed in Issues view
    ProjectExplorer::TaskHub::addCategory(Constants::ANALYZERTASK_ID, tr("Analyzer"));

    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag AnalyzerPlugin::aboutToShutdown()
{
    AnalyzerManager::shutdown();
    return SynchronousShutdown;
}

AnalyzerPlugin *AnalyzerPlugin::instance()
{
    return m_instance;
}
