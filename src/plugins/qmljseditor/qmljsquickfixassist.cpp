// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsquickfixassist.h"
#include "qmljseditor.h"
#include "qmljseditorconstants.h"
#include "qmljseditordocument.h"

//temp
#include "qmljsquickfix.h"

#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>

#include <utils/algorithm.h>

using namespace QmlJSTools;
using namespace TextEditor;

namespace QmlJSEditor {

using namespace Internal;

// -----------------------
// QmlJSQuickFixAssistInterface
// -----------------------
QmlJSQuickFixAssistInterface::QmlJSQuickFixAssistInterface(QmlJSEditorWidget *editor,
                                                           AssistReason reason)
    : AssistInterface(editor->textCursor(), editor->textDocument()->filePath(), reason)
    , m_semanticInfo(editor->qmlJsEditorDocument()->semanticInfo())
    , m_currentFile(QmlJSRefactoringChanges::file(editor, m_semanticInfo.document))
{}

QmlJSQuickFixAssistInterface::~QmlJSQuickFixAssistInterface() = default;

const SemanticInfo &QmlJSQuickFixAssistInterface::semanticInfo() const
{
    return m_semanticInfo;
}

QmlJSRefactoringFilePtr QmlJSQuickFixAssistInterface::currentFile() const
{
    return m_currentFile;
}

// ---------------------------
// QmlJSQuickFixAssistProcessor
// ---------------------------
class QmlJSQuickFixAssistProcessor : public IAssistProcessor
{
    IAssistProposal *perform() override
    {
        return GenericProposal::createProposal(interface(), findQmlJSQuickFixes(interface()));
    }
};

// ---------------------------
// QmlJSQuickFixAssistProvider
// ---------------------------

IAssistProcessor *QmlJSQuickFixAssistProvider::createProcessor(const AssistInterface *) const
{
    return new QmlJSQuickFixAssistProcessor;
}

} // namespace QmlJSEditor
