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

#ifndef QUICKTOOLBAR_H
#define QUICKTOOLBAR_H

#include <qmljs/qmljsicontextpane.h>

#include <QPointer>

namespace TextEditor {
class BaseTextEditor;
}

namespace QmlEditorWidgets {
class ContextPaneWidget;
}

namespace QmlJSEditor {

class QuickToolBar : public QmlJS::IContextPane
{
    Q_OBJECT

public:
   QuickToolBar(QObject *parent = 0);
   ~QuickToolBar();
   void apply(TextEditor::BaseTextEditor *editor, QmlJS::Document::Ptr document, const QmlJS::ScopeChain *scopeChain, QmlJS::AST::Node *node, bool update, bool force = false);
   bool isAvailable(TextEditor::BaseTextEditor *editor, QmlJS::Document::Ptr document, QmlJS::AST::Node *node);
   void setProperty(const QString &propertyName, const QVariant &value);
   void removeProperty(const QString &propertyName);
   void setEnabled(bool);
   QWidget* widget();

public slots:
   void onPropertyChanged(const QString &, const QVariant &);
   void onPropertyRemoved(const QString &);
   void onPropertyRemovedAndChange(const QString &, const QString &, const QVariant &, bool removeFirst = true);
   void onPinnedChanged(bool);
   void onEnabledChanged(bool);

private:
    QmlEditorWidgets::ContextPaneWidget* contextWidget();
    QPointer<QmlEditorWidgets::ContextPaneWidget> m_widget;
    QmlJS::Document::Ptr m_doc;
    QmlJS::AST::Node *m_node;
    TextEditor::BaseTextEditor *m_editor;
    bool m_blockWriting;
    QStringList m_propertyOrder;
    QStringList m_prototypes;
    QString m_oldType;
};

} //QmlDesigner

#endif // QUICKTOOLBAR_H
