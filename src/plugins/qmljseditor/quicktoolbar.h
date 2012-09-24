/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
