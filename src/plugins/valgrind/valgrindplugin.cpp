/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindplugin.h"

#include "callgrindtool.h"
#include "memchecktool.h"
#include "valgrindruncontrolfactory.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzersettings.h>

#include <coreplugin/dialogs/ioptionspage.h>

#include <valgrind/valgrindsettings.h>

#include <utils/hostosinfo.h>

#include <QtPlugin>
#include <QCoreApplication>

using namespace Analyzer;

namespace Valgrind {
namespace Internal {

static ValgrindGlobalSettings *theGlobalSettings = 0;

class ValgrindOptionsPage : public Core::IOptionsPage
{
public:
    explicit ValgrindOptionsPage()
    {
        setId(ANALYZER_VALGRIND_SETTINGS);
        setDisplayName(tr("Valgrind"));
        setCategory("T.Analyzer");
        setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
        setCategoryIcon(QLatin1String(":/images/analyzer_category.png"));
    }

    QWidget *createPage(QWidget *parent) {
        return theGlobalSettings->createConfigWidget(parent);
    }

    void apply() {
        theGlobalSettings->writeSettings();
    }
    void finish() {}
};

class ValgrindAction : public AnalyzerAction
{
public:
    ValgrindAction() {}
};


ValgrindPlugin::~ValgrindPlugin()
{
    delete theGlobalSettings;
    theGlobalSettings = 0;
}

bool ValgrindPlugin::initialize(const QStringList &, QString *)
{
    theGlobalSettings = new ValgrindGlobalSettings();
    theGlobalSettings->readSettings();

    addAutoReleasedObject(new ValgrindOptionsPage());

    IAnalyzerTool *memcheckTool = new MemcheckTool(this);
    IAnalyzerTool *callgrindTool = new CallgrindTool(this);
    ValgrindAction *action = 0;

    QString callgrindToolTip = tr("Valgrind Function Profile uses the "
        "\"callgrind\" tool to record function calls when a program runs.");

    QString memcheckToolTip = tr("Valgrind Analyze Memory uses the "
         "\"memcheck\" tool to find memory leaks");

    if (!Utils::HostOsInfo::isWindowsHost()) {
        action = new ValgrindAction;
        action->setId("Memcheck.Local");
        action->setTool(memcheckTool);
        action->setText(tr("Valgrind Memory Analyzer"));
        action->setToolTip(memcheckToolTip);
        action->setMenuGroup(Constants::G_ANALYZER_TOOLS);
        action->setStartMode(StartLocal);
        action->setEnabled(false);
        AnalyzerManager::addAction(action);

        action = new ValgrindAction;
        action->setId("Callgrind.Local");
        action->setTool(callgrindTool);
        action->setText(tr("Valgrind Function Profiler"));
        action->setToolTip(callgrindToolTip);
        action->setMenuGroup(Constants::G_ANALYZER_TOOLS);
        action->setStartMode(StartLocal);
        action->setEnabled(false);
        AnalyzerManager::addAction(action);
    }

    action = new ValgrindAction;
    action->setId("Memcheck.Remote");
    action->setTool(memcheckTool);
    action->setText(tr("Valgrind Memory Analyzer (Remote)"));
    action->setToolTip(memcheckToolTip);
    action->setMenuGroup(Constants::G_ANALYZER_REMOTE_TOOLS);
    action->setStartMode(StartRemote);
    AnalyzerManager::addAction(action);

    action = new ValgrindAction;
    action->setId("Callgrind.Remote");
    action->setTool(callgrindTool);
    action->setText(tr("Valgrind Function Profiler (Remote)"));
    action->setToolTip(callgrindToolTip);
    action->setMenuGroup(Constants::G_ANALYZER_REMOTE_TOOLS);
    action->setStartMode(StartRemote);
    AnalyzerManager::addAction(action);

    addAutoReleasedObject(new ValgrindRunControlFactory());

    return true;
}

ValgrindGlobalSettings *ValgrindPlugin::globalSettings()
{
   return theGlobalSettings;
}

} // namespace Internal
} // namespace Valgrind


Q_EXPORT_PLUGIN(Valgrind::Internal::ValgrindPlugin)
