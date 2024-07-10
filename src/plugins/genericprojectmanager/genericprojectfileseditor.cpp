// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericprojectfileseditor.h"
#include "genericprojectconstants.h"

#include <coreplugin/coreplugintr.h>
#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

using namespace TextEditor;

namespace GenericProjectManager::Internal {

class ProjectFilesFactory : public TextEditorFactory
{
public:
    ProjectFilesFactory()
    {
        setId(Constants::FILES_EDITOR_ID);
        setDisplayName(::Core::Tr::tr(".files Editor"));
        addMimeType("application/vnd.qtcreator.generic.files");
        addMimeType("application/vnd.qtcreator.generic.includes");
        addMimeType("application/vnd.qtcreator.generic.config");
        addMimeType("application/vnd.qtcreator.generic.cxxflags");
        addMimeType("application/vnd.qtcreator.generic.cflags");

        setDocumentCreator([]() { return new TextDocument(Constants::FILES_EDITOR_ID); });
        setOptionalActionMask(OptionalActions::None);
    }
};

void setupGenericProjectFiles()
{
    static ProjectFilesFactory theProjectFilesFactory;
}

} // GenericProjectManager::Internal
