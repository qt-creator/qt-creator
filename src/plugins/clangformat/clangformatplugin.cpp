// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatplugin.h"

#include "clangformatconfigwidget.h"
#include "clangformatglobalconfigwidget.h"
#include "clangformatconstants.h"
#include "clangformatindenter.h"
#include "clangformatsettings.h"
#include "clangformatutils.h"
#include "tests/clangformat-test.h"

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

#include <cppeditor/cppcodestylepreferencesfactory.h>
#include <cppeditor/cppcodestylesettingspage.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/textindenter.h>
#include <texteditor/texteditorsettings.h>

#include <clang/Format/Format.h>

#include <utils/algorithm.h>
#include <utils/infobar.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QMenu>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangFormat {

class ClangFormatStyleFactory : public CppEditor::CppCodeStylePreferencesFactory
{
    Q_DECLARE_TR_FUNCTIONS(ClangFormatStyleFactory)
public:
    TextEditor::Indenter *createIndenter(QTextDocument *doc) const override
    {
        return new ClangFormatForwardingIndenter(doc);
    }

    std::pair<CppEditor::CppCodeStyleWidget *, QString> additionalTab(
        TextEditor::ICodeStylePreferences *codeStyle,
        ProjectExplorer::Project *project,
        QWidget *parent) const override
    {
        return {new ClangFormatConfigWidget(codeStyle, project, parent), tr("ClangFormat")};
    }

    TextEditor::CodeStyleEditorWidget *createAdditionalGlobalSettings(
        ProjectExplorer::Project *project, QWidget *parent) override
    {
        return new ClangFormatGlobalConfigWidget(project, parent);
    }
};

static void replaceCppCodeStyle()
{
    using namespace TextEditor;
    TextEditorSettings::unregisterCodeStyleFactory(CppEditor::Constants::CPP_SETTINGS_ID);
    TextEditorSettings::registerCodeStyleFactory(new ClangFormatStyleFactory);
}

bool ClangFormatPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)
    replaceCppCodeStyle();

    ActionContainer *contextMenu = ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT);
    if (contextMenu) {
        auto openClangFormatConfigAction
            = new QAction(tr("Open Used .clang-format Configuration File"), this);
        Command *command = ActionManager::registerAction(openClangFormatConfigAction,
                                                         Constants::OPEN_CURRENT_CONFIG_ID);
        contextMenu->addSeparator();
        contextMenu->addAction(command);

        if (EditorManager::currentEditor()) {
            if (const IDocument *doc = EditorManager::currentEditor()->document())
                openClangFormatConfigAction->setData(doc->filePath().toVariant());
        }

        connect(openClangFormatConfigAction,
                &QAction::triggered,
                this,
                [openClangFormatConfigAction] {
                    const FilePath fileName = FilePath::fromVariant(openClangFormatConfigAction->data());
                    if (!fileName.isEmpty())
                        EditorManager::openEditor(FilePath::fromString(configForFile(fileName)));
                });

        connect(EditorManager::instance(),
                &EditorManager::currentEditorChanged,
                this,
                [openClangFormatConfigAction](IEditor *editor) {
                    if (!editor)
                        return;

                    if (const IDocument *doc = editor->document())
                        openClangFormatConfigAction->setData(doc->filePath().toVariant());
                });
    }
    return true;
}

QVector<QObject *> ClangFormatPlugin::createTestObjects() const
{
    return {
#ifdef WITH_TESTS
        new Internal::ClangFormatTest,
#endif
    };
}

} // namespace ClangFormat
