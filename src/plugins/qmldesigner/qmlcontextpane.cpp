#include "qmlcontextpane.h"
#include <contextpanewidget.h>
#include <qmldesignerplugin.h>

#include <utils/changeset.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljspropertyreader.h>
#include <qmljs/qmljsrewriter.h>
#include <qmljs/qmljsindenter.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/tabsettings.h>
#include <colorwidget.h>

using namespace QmlJS;
using namespace AST;

namespace QmlDesigner {

static inline QString textAt(const Document* doc,
                                      const SourceLocation &from,
                                      const SourceLocation &to)
{
    return doc->source().mid(from.offset, to.end() - from.begin());
}

QmlContextPane::QmlContextPane(QObject *parent) : ::QmlJS::IContextPane(parent), m_blockWriting(false)
{
    m_node = 0;
    ContextPaneWidget();

    m_propertyOrder
               << QLatin1String("id")
               << QLatin1String("name")
               << QLatin1String("target")
               << QLatin1String("property")
               << QLatin1String("x")
               << QLatin1String("y")
               << QLatin1String("width")
               << QLatin1String("height")
               << QLatin1String("position")
               << QLatin1String("color")
               << QLatin1String("radius")
               << QLatin1String("text")
               << QLatin1String("font.family")
               << QLatin1String("font.bold")
               << QLatin1String("font.italic")
               << QLatin1String("font.underline")
               << QLatin1String("font.strikeout")
               << QString::null
               << QLatin1String("states")
               << QLatin1String("transitions")
               ;
}

QmlContextPane::~QmlContextPane()
{
    //if the pane was never activated the widget is not in a widget tree
    if (!m_widget.isNull())
        delete m_widget.data();
        m_widget.clear();
}

void QmlContextPane::apply(TextEditor::BaseTextEditorEditable *editor, Document::Ptr doc,  Node *node, bool update)
{
    if (!Internal::BauhausPlugin::pluginInstance()->settings().enableContextPane)
        return;

    setEnabled(doc->isParsedCorrectly());
    m_editor = editor;
    contextWidget()->setParent(editor->widget()->parentWidget());
    contextWidget()->colorDialog()->setParent(editor->widget()->parentWidget());

    if (UiObjectDefinition *objectMember = cast<UiObjectDefinition*>(node)) {

        QString name = objectMember->qualifiedTypeNameId->name->asString();
        if (name.contains("Text")) {

            m_node = 0;
            PropertyReader propertyReader(doc.data(), objectMember->initializer);
            QTextCursor tc(editor->editor()->document());
            const quint32 offset = objectMember->firstSourceLocation().offset;
            const quint32 end = objectMember->lastSourceLocation().end();
            QString name = objectMember->qualifiedTypeNameId->name->asString();
            tc.setPosition(offset); 
            QPoint p1 = editor->editor()->mapToParent(editor->editor()->viewport()->mapToParent(editor->editor()->cursorRect(tc).topLeft()) - QPoint(0, contextWidget()->height() + 10));
            tc.setPosition(end);
            QPoint p2 = editor->editor()->mapToParent(editor->editor()->viewport()->mapToParent(editor->editor()->cursorRect(tc).bottomLeft()) + QPoint(0, 10));
            p2.setX(p1.x());
            if (!update)
                contextWidget()->activate(p1 , p2);
            else
                contextWidget()->rePosition(p1 , p2);
            m_blockWriting = true;
            contextWidget()->setType(name);
            contextWidget()->setProperties(&propertyReader);
            m_blockWriting = false;
            m_doc = doc;
            m_node = node;
        } else {
            contextWidget()->setParent(0);
            contextWidget()->hide();
            contextWidget()->colorDialog()->hide();
        }
    } else {
        contextWidget()->setParent(0);
        contextWidget()->hide();
        contextWidget()->colorDialog()->hide();
    }

}

void QmlContextPane::setProperty(const QString &propertyName, const QVariant &value)
{
    QString stringValue = value.toString();
    if (value.type() == QVariant::Color)
        stringValue = QChar('\"') + value.toString() + QChar('\"');    

    if (UiObjectDefinition *objectMember = cast<UiObjectDefinition*>(m_node)) {


        Utils::ChangeSet changeSet;
        Rewriter rewriter(m_doc->source(), &changeSet, m_propertyOrder);

        int line = 1;

        PropertyReader propertyReader(m_doc.data(), objectMember->initializer);
        if (propertyReader.hasProperty(propertyName)) {
            rewriter.changeProperty(objectMember->initializer, propertyName, stringValue, Rewriter::ScriptBinding);
        } else {
            rewriter.addBinding(objectMember->initializer, propertyName, stringValue, Rewriter::ScriptBinding);
            int column;
            m_editor->convertPosition(changeSet.operationList().first().pos1, &line, &column); //get line
        }

        QTextCursor tc(m_editor->editor()->document());
        int cursorPostion = tc.position();
        tc.beginEditBlock();
        changeSet.apply(&tc);
        if (line > 0) {
            TextEditor::TabSettings ts = m_editor->editor()->tabSettings();
            QmlJSIndenter indenter;
            indenter.setTabSize(ts.m_tabSize);
            indenter.setIndentSize(ts.m_indentSize);

            QTextBlock start = m_editor->editor()->document()->findBlockByLineNumber(line);
            QTextBlock end = m_editor->editor()->document()->findBlockByLineNumber(line);

            const int indent = indenter.indentForBottomLine(m_editor->editor()->document()->begin(), end.next(), QChar::Null);
            ts.indentLine(start, indent);
        }
        tc.endEditBlock();
        tc.setPosition(cursorPostion);
    }
}

void QmlContextPane::removeProperty(const QString &propertyName)
{
    if (UiObjectDefinition *objectMember = cast<UiObjectDefinition*>(m_node)) {
        PropertyReader propertyReader(m_doc.data(), objectMember->initializer);
        if (propertyReader.hasProperty(propertyName)) {
            Utils::ChangeSet changeSet;
            Rewriter rewriter(m_doc->source(), &changeSet, m_propertyOrder);
            rewriter.removeProperty(objectMember->initializer, propertyName);
            QTextCursor tc(m_editor->editor()->document());
            changeSet.apply(&tc);
        }
    }
}

void QmlContextPane::setEnabled(bool b)
{
    if (m_widget)
        m_widget->currentWidget()->setEnabled(b);
}


void QmlContextPane::onPropertyChanged(const QString &name, const QVariant &value)
{
    if (m_blockWriting)
        return;
    if (!m_doc)
        return;
    setProperty(name, value);
    m_doc.clear(); //the document is outdated
}

void QmlContextPane::onPropertyRemovedAndChange(const QString &remove, const QString &change, const QVariant &value)
{
    if (m_blockWriting)
        return;

    if (!m_doc)
        return;

    setProperty(change, value);
    removeProperty(remove);
    m_doc.clear(); //the document is outdated

}

ContextPaneWidget* QmlContextPane::contextWidget()
{
    if (m_widget.isNull()) { //lazily recreate widget
        m_widget = new ContextPaneWidget;
        connect(m_widget.data(), SIGNAL(propertyChanged(QString,QVariant)), this, SLOT(onPropertyChanged(QString,QVariant)));
        connect(m_widget.data(), SIGNAL(removeProperty(QString)), this, SLOT(onPropertyRemoved(QString)));
        connect(m_widget.data(), SIGNAL(removeAndChangeProperty(QString,QString,QVariant)), this, SLOT(onPropertyRemovedAndChange(QString,QString,QVariant)));
    }
    return m_widget.data();
}

void QmlContextPane::onPropertyRemoved(const QString &propertyName)
{
    if (m_blockWriting)
        return;

    if (!m_doc)
        return;

    removeProperty(propertyName);
    m_doc.clear(); //the document is outdated
}

} //QmlDesigner
