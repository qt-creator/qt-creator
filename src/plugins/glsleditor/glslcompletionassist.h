/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GLSLCOMPLETIONASSIST_H
#define GLSLCOMPLETIONASSIST_H

#include "glsleditor.h"

#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/defaultassistinterface.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>

#include <QScopedPointer>
#include <QIcon>

namespace GLSL {
class Function;
}

namespace TextEditor {
class BasicProposalItem;
}

namespace GLSLEditor {
namespace Internal {

class GLSLCompletionAssistInterface;

class GLSLCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
public:
    virtual bool supportsEditor(const Core::Id &editorId) const;
    virtual TextEditor::IAssistProcessor *createProcessor() const;

    virtual int activationCharSequenceLength() const;
    virtual bool isActivationCharSequence(const QString &sequence) const;
};

class GLSLCompletionAssistProcessor : public TextEditor::IAssistProcessor
{
public:
    GLSLCompletionAssistProcessor();
    virtual ~GLSLCompletionAssistProcessor();

    virtual TextEditor::IAssistProposal *perform(const TextEditor::IAssistInterface *interface);

private:
    TextEditor::IAssistProposal *createContentProposal() const;
    TextEditor::IAssistProposal *createHintProposal(const QVector<GLSL::Function *> &symbols);
    bool acceptsIdleEditor() const;
    void addCompletion(const QString &text, const QIcon &icon, int order = 0);

    int m_startPosition;
    QScopedPointer<const GLSLCompletionAssistInterface> m_interface;
    QList<TextEditor::BasicProposalItem *> m_completions;

    QIcon m_keywordIcon;
    QIcon m_varIcon;
    QIcon m_functionIcon;
    QIcon m_typeIcon;
    QIcon m_constIcon;
    QIcon m_attributeIcon;
    QIcon m_uniformIcon;
    QIcon m_varyingIcon;
    QIcon m_otherIcon;
};

class GLSLCompletionAssistInterface : public TextEditor::DefaultAssistInterface
{
public:
    GLSLCompletionAssistInterface(QTextDocument *textDocument,
                                  int position, Core::IDocument *document,
                                  TextEditor::AssistReason reason,
                                  const QString &mimeType,
                                  const Document::Ptr &glslDoc);

    const QString &mimeType() const { return m_mimeType; }
    const Document::Ptr &glslDocument() const { return m_glslDoc; }

private:
    QString m_mimeType;
    Document::Ptr m_glslDoc;
};

} // Internal
} // GLSLEditor

#endif // GLSLCOMPLETIONASSIST_H
