/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "analyzerplugin.h"
#include "analyzerconstants.h"
#include "analyzermanager.h"
#include "ianalyzertool.h"
#include "analyzerruncontrolfactory.h"

#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

#include <QtPlugin>
#include <QMessageBox>
#include <QApplication>

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

    addAutoReleasedObject(new AnalyzerRunControlFactory());

    // Task integration.
    //: Category under which Analyzer tasks are listed in Issues view
    ProjectExplorer::ProjectExplorerPlugin::instance()->taskHub()
            ->addCategory(Core::Id(Constants::ANALYZERTASK_ID), tr("Analyzer"));

    return true;
}

void AnalyzerPlugin::extensionsInitialized()
{
    AnalyzerManager::extensionsInitialized();
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

Q_EXPORT_PLUGIN(AnalyzerPlugin)
