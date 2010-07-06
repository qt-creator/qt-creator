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
   void apply(TextEditor::BaseTextEditorEditable *editor, QmlJS::Document::Ptr doc, QmlJS::AST::Node *node, bool update);
   void setProperty(const QString &propertyName, const QVariant &value);
   void removeProperty(const QString &propertyName);
   void setEnabled(bool);

public slots:
       void onPropertyChanged(const QString &, const QVariant &);
       void onPropertyRemoved(const QString &);
       void onPropertyRemovedAndChange(const QString &, const QString &, const QVariant &);
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
