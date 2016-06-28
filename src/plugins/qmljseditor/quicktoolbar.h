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

#include <qmljs/qmljsicontextpane.h>

#include <QPointer>

namespace QmlEditorWidgets { class ContextPaneWidget; }

namespace QmlJSEditor {

class QuickToolBar : public QmlJS::IContextPane
{
    Q_OBJECT

public:
   QuickToolBar(QObject *parent = 0);
   ~QuickToolBar();
   void apply(TextEditor::TextEditorWidget *widget, QmlJS::Document::Ptr document, const QmlJS::ScopeChain *scopeChain, QmlJS::AST::Node *node, bool update, bool force = false);
   bool isAvailable(TextEditor::TextEditorWidget *widget, QmlJS::Document::Ptr document, QmlJS::AST::Node *node);
   void setProperty(const QString &propertyName, const QVariant &value);
   void removeProperty(const QString &propertyName);
   void setEnabled(bool);
   QWidget* widget();

   void onPropertyChanged(const QString &, const QVariant &);
   void onPropertyRemoved(const QString &);
   void onPropertyRemovedAndChange(const QString &, const QString &, const QVariant &, bool removeFirst = true);
   void onPinnedChanged(bool);
   void onEnabledChanged(bool);

private:
   void indentLines(int startLine, int endLine);

    QmlEditorWidgets::ContextPaneWidget* contextWidget();
    QPointer<QmlEditorWidgets::ContextPaneWidget> m_widget;
    QmlJS::Document::Ptr m_doc;
    QmlJS::AST::Node *m_node;
    TextEditor::TextEditorWidget *m_editorWidget;
    bool m_blockWriting;
    QStringList m_propertyOrder;
    QStringList m_prototypes;
    QString m_oldType;
};

} //QmlDesigner
