// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cmakefilecompletionassist.h"

#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmaketool.h"

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <texteditor/codeassist/assistinterface.h>

#include <QFileInfo>

using namespace CMakeProjectManager::Internal;
using namespace TextEditor;
using namespace ProjectExplorer;

// -------------------------------
// CMakeFileCompletionAssistProvider
// -------------------------------

IAssistProcessor *CMakeFileCompletionAssistProvider::createProcessor(const AssistInterface *) const
{
    return new CMakeFileCompletionAssist;
}

CMakeFileCompletionAssist::CMakeFileCompletionAssist() :
    KeywordsCompletionAssistProcessor(Keywords())
{
    setSnippetGroup(Constants::CMAKE_SNIPPETS_GROUP_ID);
    setDynamicCompletionFunction(&TextEditor::pathComplete);
}

IAssistProposal *CMakeFileCompletionAssist::perform(const AssistInterface *interface)
{
    Keywords kw;
    const Utils::FilePath &filePath = interface->filePath();
    if (!filePath.isEmpty() && filePath.toFileInfo().isFile()) {
        Project *p = SessionManager::projectForFile(filePath);
        if (p && p->activeTarget()) {
            CMakeTool *cmake = CMakeKitAspect::cmakeTool(p->activeTarget()->kit());
            if (cmake && cmake->isValid())
                kw = cmake->keywords();
        }
    }

    setKeywords(kw);
    return KeywordsCompletionAssistProcessor::perform(interface);
}
