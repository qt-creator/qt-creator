// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatplugin.h"

#include "clangformatconfigwidget.h"
#include "clangformatconstants.h"
#include "clangformatglobalconfigwidget.h"
#include "clangformatindenter.h"
#include "clangformattr.h"
#include "clangformatutils.h"
#include "tests/clangformat-test.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <cppeditor/cppcodestylepreferencesfactory.h>
#include <cppeditor/cppeditorconstants.h>

#include <projectexplorer/project.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/texteditorsettings.h>

#include <QAction>

using namespace Core;
using namespace CppEditor;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ClangFormat {

class ClangFormatStyleFactory : public CppCodeStylePreferencesFactory
{
public:
    Indenter *createIndenter(QTextDocument *doc) const override
    {
        return new ClangFormatForwardingIndenter(doc);
    }

    std::pair<TextEditor::CodeStyleEditorWidget *, QString> additionalTab(
        ICodeStylePreferences *codeStyle, Project *project, QWidget *parent) const override
    {
        return {createClangFormatConfigWidget(codeStyle, project, parent), Tr::tr("ClangFormat")};
    }

    CodeStyleEditorWidget *createAdditionalGlobalSettings(
        ICodeStylePreferences *codeStyle, Project *project, QWidget *parent) override
    {
        return new ClangFormatGlobalConfigWidget(codeStyle, project, parent);
    }
};

ClangFormatPlugin::~ClangFormatPlugin()
{
    TextEditorSettings::unregisterCodeStyleFactory(CppEditor::Constants::CPP_SETTINGS_ID);
    delete m_factory;
}

void ClangFormatPlugin::initialize()
{
    TextEditorSettings::unregisterCodeStyleFactory(CppEditor::Constants::CPP_SETTINGS_ID);
    m_factory = new ClangFormatStyleFactory;
    TextEditorSettings::registerCodeStyleFactory(m_factory);

    ActionContainer *contextMenu = ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT);
    if (contextMenu) {
        contextMenu->addSeparator();

        ActionBuilder openConfig(this,  Constants::OPEN_CURRENT_CONFIG_ID);
        openConfig.setText(Tr::tr("Open Used .clang-format Configuration File"));
        openConfig.addToContainer(CppEditor::Constants::M_CONTEXT);
        openConfig.addOnTriggered(this, [] {
            if (const IDocument *doc = EditorManager::currentDocument()) {
                const FilePath filePath = doc->filePath();
                if (!filePath.isEmpty())
                    EditorManager::openEditor(configForFile(filePath));
            }
        });
    }

#ifdef WITH_TESTS
    addTest<Internal::ClangFormatTest>();
#endif
}

} // ClangFormat
