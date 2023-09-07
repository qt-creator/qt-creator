// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef BINDINGEDITORWIDGET_H
#define BINDINGEDITORWIDGET_H

#include <texteditor/texteditor.h>
#include <qmljseditor/qmljseditordocument.h>
#include <qmljseditor/qmljssemantichighlighter.h>
#include <qmljseditor/qmljseditor.h>

#include <QtQml>
#include <QWidget>
#include <QDialog>

namespace QmlDesigner {

const char BINDINGEDITOR_CONTEXT_ID[] = "BindingEditor.BindingEditorContext";

class BindingEditorWidget : public QmlJSEditor::QmlJSEditorWidget
{
    Q_OBJECT

public:
    BindingEditorWidget();
    ~BindingEditorWidget() override;

    void unregisterAutoCompletion();

    bool event(QEvent *event) override;

    std::unique_ptr<TextEditor::AssistInterface> createAssistInterface(
        TextEditor::AssistKind assistKind, TextEditor::AssistReason assistReason) const override;

    void setEditorTextWithIndentation(const QString &text);

signals:
    void returnKeyClicked();

public:
    QmlJSEditor::QmlJSEditorDocument *qmljsdocument = nullptr;
    Core::IContext *m_context = nullptr;
    QAction *m_completionAction = nullptr;
    bool m_isMultiline = false;
};

class BindingDocument : public QmlJSEditor::QmlJSEditorDocument
{
public:
    BindingDocument();
    ~BindingDocument();

protected:
    void applyFontSettings() final;

    void triggerPendingUpdates() final;

private:
    QmlJSEditor::SemanticHighlighter *m_semanticHighlighter = nullptr;
};


class BindingEditorFactory : public TextEditor::TextEditorFactory
{
public:
    BindingEditorFactory();

    static void decorateEditor(TextEditor::TextEditorWidget *editor);
};

}

#endif //BINDINGEDITORWIDGET_H
