// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastfwd_p.h>

#include <QPointer>

namespace TextEditor { class TextEditorWidget; }

namespace QmlJS {  class ScopeChain; }

namespace QmlEditorWidgets { class ContextPaneWidget; }

namespace QmlJSEditor {

class QuickToolBar : public QObject
{
    Q_OBJECT

    QuickToolBar();
public:
    ~QuickToolBar();

    static QuickToolBar *instance();

    void apply(TextEditor::TextEditorWidget *widget, QmlJS::Document::Ptr document, const QmlJS::ScopeChain *scopeChain, QmlJS::AST::Node *node, bool update, bool force = false);
    bool isAvailable(TextEditor::TextEditorWidget *widget, QmlJS::Document::Ptr document, QmlJS::AST::Node *node);
    void setProperty(const QString &propertyName, const QVariant &value);
    void removeProperty(const QString &propertyName);
    void setEnabled(bool);
    QWidget *widget();

    void onPropertyChanged(const QString &, const QVariant &);
    void onPropertyRemoved(const QString &);
    void onPropertyRemovedAndChange(const QString &, const QString &, const QVariant &, bool removeFirst = true);
    void onPinnedChanged(bool);
    void onEnabledChanged(bool);

signals:
    void closed();

private:
    void indentLines(int startLine, int endLine);

    QmlEditorWidgets::ContextPaneWidget* contextWidget();
    QPointer<QmlEditorWidgets::ContextPaneWidget> m_widget;
    QmlJS::Document::Ptr m_doc;
    QmlJS::AST::Node *m_node = nullptr;
    TextEditor::TextEditorWidget *m_editorWidget = nullptr;
    bool m_blockWriting = false;
    QStringList m_propertyOrder;
    QStringList m_prototypes;
    QString m_oldType;
};

} // QmlJSEditor
