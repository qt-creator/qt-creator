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

#pragma once

#include "qmljseditor.h"

#include <qmljstools/qmljsrefactoringchanges.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/quickfixassistprovider.h>
#include <texteditor/codeassist/quickfixassistprocessor.h>


namespace QmlJSEditor {
namespace Internal {

class QmlJSQuickFixAssistInterface : public TextEditor::AssistInterface
{
public:
    QmlJSQuickFixAssistInterface(QmlJSEditorWidget *editor, TextEditor::AssistReason reason);
    ~QmlJSQuickFixAssistInterface();

    const QmlJSTools::SemanticInfo &semanticInfo() const;
    QmlJSTools::QmlJSRefactoringFilePtr currentFile() const;

private:
    QmlJSTools::SemanticInfo m_semanticInfo;
    QmlJSTools::QmlJSRefactoringFilePtr m_currentFile;
};


class QmlJSQuickFixAssistProvider : public TextEditor::QuickFixAssistProvider
{
public:
    QmlJSQuickFixAssistProvider(QObject *parent = 0);
    ~QmlJSQuickFixAssistProvider();

    IAssistProvider::RunType runType() const override;
    bool supportsEditor(Core::Id editorId) const override;
    TextEditor::IAssistProcessor *createProcessor() const override;

    QList<TextEditor::QuickFixFactory *> quickFixFactories() const override;
};

} // Internal
} // QmlJSEditor
