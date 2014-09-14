/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>

#include <utils/qtcoverride.h>

#include <QIcon>
#include <QScopedPointer>
#include <QSharedPointer>

namespace GLSL {
class Engine;
class Function;
class TranslationUnitAST;
class Scope;
} // namespace GLSL

namespace TextEditor { class AssistProposalItem; }

namespace GlslEditor {
namespace Internal {

class Document
{
public:
    typedef QSharedPointer<Document> Ptr;

    Document();
    ~Document();

    GLSL::Engine *engine() const { return _engine; }
    GLSL::TranslationUnitAST *ast() const { return _ast; }
    GLSL::Scope *globalScope() const { return _globalScope; }

    GLSL::Scope *scopeAt(int position) const;
    void addRange(const QTextCursor &cursor, GLSL::Scope *scope);

private:
    struct Range {
        QTextCursor cursor;
        GLSL::Scope *scope;
    };

    GLSL::Engine *_engine;
    GLSL::TranslationUnitAST *_ast;
    GLSL::Scope *_globalScope;
    QList<Range> _cursors;

    friend class GlslEditorWidget;
};

class GlslCompletionAssistInterface;

class GlslCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    bool supportsEditor(Core::Id editorId) const QTC_OVERRIDE;
    TextEditor::IAssistProcessor *createProcessor() const QTC_OVERRIDE;

    int activationCharSequenceLength() const QTC_OVERRIDE;
    bool isActivationCharSequence(const QString &sequence) const QTC_OVERRIDE;
};

class GlslCompletionAssistProcessor : public TextEditor::IAssistProcessor
{
public:
    GlslCompletionAssistProcessor();
    ~GlslCompletionAssistProcessor();

    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) QTC_OVERRIDE;

private:
    TextEditor::IAssistProposal *createHintProposal(const QVector<GLSL::Function *> &symbols);
    bool acceptsIdleEditor() const;

    int m_startPosition;
    QScopedPointer<const GlslCompletionAssistInterface> m_interface;

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

class GlslCompletionAssistInterface : public TextEditor::AssistInterface
{
public:
    GlslCompletionAssistInterface(QTextDocument *textDocument,
                                  int position, const QString &fileName,
                                  TextEditor::AssistReason reason,
                                  const QString &mimeType,
                                  const Document::Ptr &glslDoc);

    const QString &mimeType() const { return m_mimeType; }
    const Document::Ptr &glslDocument() const { return m_glslDoc; }

private:
    QString m_mimeType;
    Document::Ptr m_glslDoc;
};

} // namespace Internal
} // namespace GlslEditor

#endif // GLSLCOMPLETIONASSIST_H
