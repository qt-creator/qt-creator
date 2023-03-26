// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljstools/qmljsrefactoringchanges.h>
#include <qmljstools/qmljssemanticinfo.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprovider.h>

namespace QmlJSEditor {

class QmlJSEditorWidget;

namespace Internal {

class QmlJSQuickFixAssistInterface : public TextEditor::AssistInterface
{
public:
    QmlJSQuickFixAssistInterface(QmlJSEditorWidget *editor, TextEditor::AssistReason reason);
    ~QmlJSQuickFixAssistInterface() override;

    const QmlJSTools::SemanticInfo &semanticInfo() const;
    QmlJSTools::QmlJSRefactoringFilePtr currentFile() const;
    bool isBaseObject() const override { return false; }

private:
    QmlJSTools::SemanticInfo m_semanticInfo;
    QmlJSTools::QmlJSRefactoringFilePtr m_currentFile;
};


class QmlJSQuickFixAssistProvider : public TextEditor::IAssistProvider
{
public:
    QmlJSQuickFixAssistProvider() = default;
    ~QmlJSQuickFixAssistProvider() override = default;

    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;
};

} // Internal
} // QmlJSEditor
