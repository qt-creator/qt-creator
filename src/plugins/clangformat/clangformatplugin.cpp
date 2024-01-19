// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangformatconstants.h"
#include "clangformatglobalconfigwidget.h"
#include "clangformattr.h"
#include "tests/clangformat-test.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <cppeditor/cppeditorconstants.h>

#include <extensionsystem/iplugin.h>

using namespace Core;
using namespace Utils;

namespace ClangFormat {

FilePath configForFile(const FilePath &fileName);

class ClangFormatPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClangFormat.json")

    ~ClangFormatPlugin() final
    {
    }

    void initialize() final
    {
        setupClangFormatStyleFactory(this); // This overrides the default, see implementation.

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
        addTestCreator(Internal::createClangFormatTest);
#endif
    }
};

} // ClangFormat

#include "clangformatplugin.moc"
