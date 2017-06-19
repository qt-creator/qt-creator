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

#include "cmakefilecompletionassist.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanager.h"
#include "cmakesettingspage.h"
#include "cmaketoolmanager.h"
#include "cmakekitinformation.h"

#include <texteditor/codeassist/assistinterface.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/session.h>

using namespace CMakeProjectManager::Internal;
using namespace TextEditor;
using namespace ProjectExplorer;

// -------------------------------
// CMakeFileCompletionAssistProvider
// -------------------------------

IAssistProcessor *CMakeFileCompletionAssistProvider::createProcessor() const
{
    return new CMakeFileCompletionAssist;
}

CMakeFileCompletionAssist::CMakeFileCompletionAssist() :
    KeywordsCompletionAssistProcessor(Keywords())
{
    setSnippetGroup(Constants::CMAKE_SNIPPETS_GROUP_ID);
}

IAssistProposal *CMakeFileCompletionAssist::perform(const AssistInterface *interface)
{
    Keywords kw;
    QString fileName = interface->fileName();
    if (!fileName.isEmpty() && QFileInfo(fileName).isFile()) {
        Project *p = SessionManager::projectForFile(Utils::FileName::fromString(fileName));
        if (p && p->activeTarget()) {
            CMakeTool *cmake = CMakeKitInformation::cmakeTool(p->activeTarget()->kit());
            if (cmake && cmake->isValid())
                kw = cmake->keywords();
        }
    }

    setKeywords(kw);
    return KeywordsCompletionAssistProcessor::perform(interface);
}
