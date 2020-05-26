/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

    TextEditor::AssistInterface *createAssistInterface(TextEditor::AssistKind assistKind,
                                                       TextEditor::AssistReason assistReason) const override;

signals:
    void returnKeyClicked();

public:
    QmlJSEditor::QmlJSEditorDocument *qmljsdocument = nullptr;
    Core::IContext *m_context = nullptr;
    QAction *m_completionAction = nullptr;
};

class BindingDocument : public QmlJSEditor::QmlJSEditorDocument
{
public:
    BindingDocument();
    ~BindingDocument();

protected:
    void applyFontSettings();

    void triggerPendingUpdates();

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
