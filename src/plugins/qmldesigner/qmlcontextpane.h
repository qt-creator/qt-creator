/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QMLCONTEXTPANE_H
#define QMLCONTEXTPANE_H

#include <QLabel>
#include <QToolBar>
#include <QPushButton>
#include <QToolButton>
#include <QGridLayout>
#include <QGroupBox>
#include <QVariant>
#include <QGraphicsDropShadowEffect>
#include <QWeakPointer>

#include <qmljs/qmljsicontextpane.h>

namespace TextEditor {
class BaseTextEditorEditable;
}

namespace QmlDesigner {

class ContextPaneWidget;

class QmlContextPane : public QmlJS::IContextPane
{
    Q_OBJECT

public:
   QmlContextPane(QObject *parent = 0);
   ~QmlContextPane();
   void apply(TextEditor::BaseTextEditorEditable *editor, QmlJS::Document::Ptr doc, const QmlJS::Snapshot &snapshot, QmlJS::AST::Node *node, bool update, bool force = 0);
   bool isAvailable(TextEditor::BaseTextEditorEditable *editor, QmlJS::Document::Ptr doc, const QmlJS::Snapshot &snapshot, QmlJS::AST::Node *node);
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
    ContextPaneWidget* contextWidget();
    QWeakPointer<ContextPaneWidget> m_widget;
    QmlJS::Document::Ptr m_doc;
    QmlJS::AST::Node *m_node;
    TextEditor::BaseTextEditorEditable *m_editor;
    bool m_blockWriting;
    QStringList m_propertyOrder;
};

} //QmlDesigner

#endif // QMLCONTEXTPANE_H
