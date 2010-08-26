#ifndef QUICKTOOLBAR_H
#define QUICKTOOLBAR_H

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
   void apply(TextEditor::BaseTextEditorEditable *editor, QmlJS::LookupContext::Ptr lookupContext, QmlJS::AST::Node *node, bool update, bool force = 0);
   bool isAvailable(TextEditor::BaseTextEditorEditable *editor, QmlJS::LookupContext::Ptr lookupContext, QmlJS::AST::Node *node);
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
    QWeakPointer<QmlEditorWidgets::ContextPaneWidget> m_widget;
    QmlJS::Document::Ptr m_doc;
    QmlJS::AST::Node *m_node;
    TextEditor::BaseTextEditorEditable *m_editor;
    bool m_blockWriting;
    QStringList m_propertyOrder;
};

} //QmlDesigner

#endif // QUICKTOOLBAR_H
