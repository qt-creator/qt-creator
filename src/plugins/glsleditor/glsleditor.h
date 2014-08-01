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

#ifndef GLSLEDITOR_H
#define GLSLEDITOR_H

#include <texteditor/basetexteditor.h>

#include <QSharedPointer>
#include <QSet>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTimer;
QT_END_NAMESPACE

namespace GLSL {
class Engine;
class TranslationUnitAST;
class Scope;
} // namespace GLSL

namespace GLSLEditor {
namespace Internal {

class GlslEditor;
class GlslEditorWidget;

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

class GlslEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT

public:
    GlslEditorWidget(TextEditor::BaseTextDocument *doc, QWidget *parent);
    GlslEditorWidget(GlslEditorWidget *other);

    int editorRevision() const;
    bool isOutdated() const;

    QSet<QString> identifiers() const;

    static int languageVariant(const QString &mimeType);

    TextEditor::IAssistInterface *createAssistInterface(TextEditor::AssistKind assistKind,
                                                        TextEditor::AssistReason reason) const;

private slots:
    void updateDocument();
    void updateDocumentNow();

protected:
    TextEditor::BaseTextEditor *createEditor();

private:
    GlslEditorWidget(TextEditor::BaseTextEditorWidget *); // avoid stupidity
    void ctor();
    void setSelectedElements();
    QString wordUnderCursor() const;

    QTimer *m_updateDocumentTimer;
    QComboBox *m_outlineCombo;
    Document::Ptr m_glslDocument;
};

} // namespace Internal
} // namespace GLSLEditor

#endif // GLSLEDITOR_H
