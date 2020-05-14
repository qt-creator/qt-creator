/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangtoolsplugin.h"

#include "clangtool.h"
#include "clangtoolsconstants.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolsprojectsettingswidget.h"
#include "settingswidget.h"

#ifdef WITH_TESTS
#include "clangtoolspreconfiguredsessiontests.h"
#include "clangtoolsunittests.h"
#endif

#include <utils/mimetypes/mimedatabase.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/qtcassert.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>

#include <texteditor/texteditor.h>

#include <cppeditor/cppeditorconstants.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/target.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QToolBar>

using namespace Core;
using namespace ProjectExplorer;

namespace ClangTools {
namespace Internal {

static ProjectPanelFactory *m_projectPanelFactoryInstance = nullptr;

ProjectPanelFactory *projectPanelFactory()
{
    return m_projectPanelFactoryInstance;
}

class ClangToolsPluginPrivate
{
public:
    ClangTool clangTool;
    ClangToolsOptionsPage optionsPage;
};

ClangToolsPlugin::~ClangToolsPlugin()
{
    delete d;
}

bool ClangToolsPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // Import tidy/clazy diagnostic configs from CppTools now
    // instead of at opening time of the settings page
    ClangToolsSettings::instance();

    d = new ClangToolsPluginPrivate;

    registerAnalyzeActions();

    auto panelFactory = m_projectPanelFactoryInstance = new ProjectPanelFactory;
    panelFactory->setPriority(100);
    panelFactory->setId(Constants::PROJECT_PANEL_ID);
    panelFactory->setDisplayName(tr("Clang Tools"));
    panelFactory->setCreateWidgetFunction([](Project *project) { return new ProjectSettingsWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    return true;
}

void ClangToolsPlugin::registerAnalyzeActions()
{
    ActionManager::registerAction(d->clangTool.startAction(), Constants::RUN_ON_PROJECT);
    Command *cmd = ActionManager::registerAction(d->clangTool.startOnCurrentFileAction(),
                                                 Constants::RUN_ON_CURRENT_FILE);
    ActionContainer *mtoolscpp = ActionManager::actionContainer(CppTools::Constants::M_TOOLS_CPP);
    if (mtoolscpp)
        mtoolscpp->addAction(cmd);

    Core::ActionContainer *mcontext = Core::ActionManager::actionContainer(
        CppEditor::Constants::M_CONTEXT);
    if (mcontext)
        mcontext->addAction(cmd, CppEditor::Constants::G_CONTEXT_FIRST);

    // add button to tool bar of C++ source files
    connect(EditorManager::instance(), &EditorManager::editorOpened, this, [this, cmd](IEditor *editor) {
        if (editor->document()->filePath().isEmpty()
            || !Utils::mimeTypeForName(editor->document()->mimeType()).inherits("text/x-c++src"))
            return;
        auto *textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
        if (!textEditor)
            return;
        TextEditor::TextEditorWidget *widget = textEditor->editorWidget();
        if (!widget)
            return;
        const QIcon icon = Utils::Icon({{":/debugger/images/debugger_singleinstructionmode.png",
                                         Utils::Theme::IconsBaseColor}})
                               .icon();
        QAction *action = widget->toolBar()->addAction(icon, tr("Analyze File"), [this, editor]() {
            d->clangTool.startTool(editor->document()->filePath());
        });
        cmd->augmentActionWithShortcutToolTip(action);
    });
}

QVector<QObject *> ClangToolsPlugin::createTestObjects() const
{
    QVector<QObject *> tests;
#ifdef WITH_TESTS
    tests << new PreconfiguredSessionTests;
    tests << new ClangToolsUnitTests;
#endif
    return tests;
}

} // namespace Internal
} // namespace ClangTools
