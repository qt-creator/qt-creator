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

#include "clangformatplugin.h"

#include "clangformatconfigwidget.h"
#include "clangformatconstants.h"
#include "clangformatindenter.h"
#include "clangformatutils.h"

#include <utils/qtcassert.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <cppeditor/cppeditorconstants.h>

#include <cpptools/cppcodestylepreferencesfactory.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/texteditorsettings.h>

#include <clang/Format/Format.h>

#include <utils/algorithm.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QMenu>

#include <QtPlugin>

using namespace ProjectExplorer;

namespace ClangFormat {

ClangFormatPlugin::ClangFormatPlugin() = default;
ClangFormatPlugin::~ClangFormatPlugin() = default;

#ifdef KEEP_LINE_BREAKS_FOR_NON_EMPTY_LINES_BACKPORTED
class ClangFormatStyleFactory : public CppTools::CppCodeStylePreferencesFactory
{
public:
    TextEditor::CodeStyleEditorWidget *createCodeStyleEditor(
        TextEditor::ICodeStylePreferences *preferences, QWidget *parent = nullptr) override
    {
        if (!parent)
            return new ClangFormatConfigWidget;
        return new ClangFormatConfigWidget(SessionManager::startupProject());
    }

    QWidget *createEditor(TextEditor::ICodeStylePreferences *, QWidget *) const override
    {
        return nullptr;
    }

    TextEditor::Indenter *createIndenter(QTextDocument *doc) const override
    {
        return new ClangFormatIndenter(doc);
    }
};

static void replaceCppCodeStyle()
{
    using namespace TextEditor;
    TextEditorSettings::unregisterCodeStyleFactory(CppTools::Constants::CPP_SETTINGS_ID);
    ICodeStylePreferencesFactory *factory = new ClangFormatStyleFactory();
    TextEditorSettings::registerCodeStyleFactory(factory);
}
#endif

bool ClangFormatPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);
#ifdef KEEP_LINE_BREAKS_FOR_NON_EMPTY_LINES_BACKPORTED
    replaceCppCodeStyle();

    Core::ActionContainer *contextMenu = Core::ActionManager::actionContainer(
        CppEditor::Constants::M_CONTEXT);
    if (contextMenu) {
        auto openClangFormatConfigAction
            = new QAction(tr("Open Used .clang-format Configuration File"), this);
        Core::Command *command
            = Core::ActionManager::registerAction(openClangFormatConfigAction,
                                                  Constants::OPEN_CURRENT_CONFIG_ID);
        contextMenu->addSeparator();
        contextMenu->addAction(command);

        if (Core::EditorManager::currentEditor()) {
            const Core::IDocument *doc = Core::EditorManager::currentEditor()->document();
            if (doc)
                openClangFormatConfigAction->setData(doc->filePath().toString());
        }

        connect(openClangFormatConfigAction,
                &QAction::triggered,
                this,
                [openClangFormatConfigAction]() {
                    const QString fileName = openClangFormatConfigAction->data().toString();
                    if (!fileName.isEmpty()) {
                        const QString clangFormatConfigPath = configForFile(
                            Utils::FileName::fromString(fileName));
                        Core::EditorManager::openEditor(clangFormatConfigPath);
                    }
                });

        connect(Core::EditorManager::instance(),
                &Core::EditorManager::currentEditorChanged,
                this,
                [openClangFormatConfigAction](Core::IEditor *editor) {
                    if (!editor)
                        return;

                    const Core::IDocument *doc = editor->document();
                    if (doc)
                        openClangFormatConfigAction->setData(doc->filePath().toString());
                });
    }
#endif
    return true;
}

} // namespace ClangFormat
