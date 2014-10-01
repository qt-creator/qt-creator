/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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
#include "valgrindsettings.h"
#include "valgrindconfigwidget.h"

#include <analyzerbase/analyzermanager.h>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <cppeditor/cppeditorconstants.h>

#include <utils/hostosinfo.h>

#include <QtPlugin>
#include <QCoreApplication>
#include <QPointer>

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
        setDisplayName(QCoreApplication::translate("Valgrind::Internal::ValgrindOptionsPage", "Valgrind"));
        setCategory("T.Analyzer");
        setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
        setCategoryIcon(QLatin1String(":/images/analyzer_category.png"));
    }

    QWidget *widget()
    {
        if (!m_widget)
            m_widget = new ValgrindConfigWidget(theGlobalSettings, 0, true);
        return m_widget;
    }

    void apply()
    {
        theGlobalSettings->writeSettings();
    }

    void finish()
    {
        delete m_widget;
    }

private:
    QPointer<QWidget> m_widget;
};

class ValgrindAction : public AnalyzerAction
{
public:
    explicit ValgrindAction(QObject *parent = 0) : AnalyzerAction(parent) { }
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

    m_memcheckTool = new MemcheckTool(this);
    m_callgrindTool = new CallgrindTool(this);

    ValgrindAction *action = 0;

    QString callgrindToolTip = tr("Valgrind Function Profile uses the "
        "\"callgrind\" tool to record function calls when a program runs.");

    QString memcheckToolTip = tr("Valgrind Analyze Memory uses the "
         "\"memcheck\" tool to find memory leaks.");

    if (!Utils::HostOsInfo::isWindowsHost()) {
        action = new ValgrindAction(this);
        action->setId("Memcheck.Local");
        action->setTool(m_memcheckTool);
        action->setText(tr("Valgrind Memory Analyzer"));
        action->setToolTip(memcheckToolTip);
        action->setMenuGroup(Constants::G_ANALYZER_TOOLS);
        action->setStartMode(StartLocal);
        action->setEnabled(false);
        AnalyzerManager::addAction(action);

        action = new ValgrindAction(this);
        action->setId("Callgrind.Local");
        action->setTool(m_callgrindTool);
        action->setText(tr("Valgrind Function Profiler"));
        action->setToolTip(callgrindToolTip);
        action->setMenuGroup(Constants::G_ANALYZER_TOOLS);
        action->setStartMode(StartLocal);
        action->setEnabled(false);
        AnalyzerManager::addAction(action);
    }

    action = new ValgrindAction(this);
    action->setId("Memcheck.Remote");
    action->setTool(m_memcheckTool);
    action->setText(tr("Valgrind Memory Analyzer (External Remote Application)"));
    action->setToolTip(memcheckToolTip);
    action->setMenuGroup(Constants::G_ANALYZER_REMOTE_TOOLS);
    action->setStartMode(StartRemote);
    AnalyzerManager::addAction(action);

    action = new ValgrindAction(this);
    action->setId("Callgrind.Remote");
    action->setTool(m_callgrindTool);
    action->setText(tr("Valgrind Function Profiler (External Remote Application)"));
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

void ValgrindPlugin::extensionsInitialized()
{
    using namespace Core;

    // If there is a CppEditor context menu add our own context menu actions.
    if (ActionContainer *editorContextMenu =
            ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT)) {
        Context analyzerContext = Context(Analyzer::Constants::C_ANALYZEMODE);
        editorContextMenu->addSeparator(analyzerContext);

        QAction *action = new QAction(tr("Profile Costs of This Function and Its Callees"), this);
        action->setIcon(QIcon(QLatin1String(Analyzer::Constants::ANALYZER_CONTROL_START_ICON)));
        connect(action, SIGNAL(triggered()), m_callgrindTool, SLOT(handleShowCostsOfFunction()));
        Command *cmd = ActionManager::registerAction(action, "Analyzer.Callgrind.ShowCostsOfFunction",
            analyzerContext);
        editorContextMenu->addAction(cmd);
        cmd->setAttribute(Command::CA_Hide);
        cmd->setAttribute(Command::CA_NonConfigurable);
    }
}

} // namespace Internal
} // namespace Valgrind


Q_EXPORT_PLUGIN(Valgrind::Internal::ValgrindPlugin)
