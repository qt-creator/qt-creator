#include "qmlcontextpane.h"
#include <contextpanewidget.h>
#include <qmldesignerplugin.h>

#include <utils/changeset.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljspropertyreader.h>
#include <qmljs/qmljsrewriter.h>
#include <qmljs/qmljsindenter.h>
#include <qmljs/qmljslookupcontext.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsbind.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/tabsettings.h>
#include <colorwidget.h>
#include <QDebug>

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

void QmlContextPane::apply(TextEditor::BaseTextEditorEditable *editor, Document::Ptr doc, const QmlJS::Snapshot &snapshot, AST::Node *node, bool update)
{
    if (!Internal::BauhausPlugin::pluginInstance()->settings().enableContextPane)
        return;

    if (doc.isNull())
        return;

    if (update && editor != m_editor)
        return; //do not update for different editor

    LookupContext::Ptr lookupContext = LookupContext::create(doc, snapshot, QList<Node*>());
    const Interpreter::ObjectValue *scopeObject = doc->bind()->findQmlObject(node);

    QStringList prototypes;
    while (scopeObject) {
        prototypes.append(scopeObject->className());
        scopeObject =  scopeObject->prototype(lookupContext->context());
    }

    setEnabled(doc->isParsedCorrectly());
    m_editor = editor;
    contextWidget()->setParent(editor->widget()->parentWidget());
    contextWidget()->colorDialog()->setParent(editor->widget()->parentWidget());

    if (cast<UiObjectDefinition*>(node) || cast<UiObjectBinding*>(node)) {
        UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(node);
        UiObjectBinding *objectBinding = cast<UiObjectBinding*>(node);

        QString name;
        quint32 offset;
        quint32 end;
        UiObjectInitializer *initializer;
        if (objectDefinition) {
            name = objectDefinition->qualifiedTypeNameId->name->asString();
            initializer = objectDefinition->initializer;
            offset = objectDefinition->firstSourceLocation().offset;
            end = objectDefinition->lastSourceLocation().end();
        } else if (objectBinding) {
            name = objectBinding->qualifiedTypeNameId->name->asString();
            initializer = objectBinding->initializer;
            offset = objectBinding->firstSourceLocation().offset;
            end = objectBinding->lastSourceLocation().end();
        }

        int line1;
        int column1;
        int line2;
        int column2;
        m_editor->convertPosition(offset, &line1, &column1); //get line
        m_editor->convertPosition(end, &line2, &column2); //get line

        QRegion reg;
        if (line1 > -1 && line2 > -1)
            reg = m_editor->editor()->translatedLineRegion(line1 - 1, line2);

        if (name.contains("Text")) {
            m_node = 0;
            PropertyReader propertyReader(doc.data(), initializer);
            QTextCursor tc(editor->editor()->document());
            tc.setPosition(offset); 
            QPoint p1 = editor->editor()->mapToParent(editor->editor()->viewport()->mapToParent(editor->editor()->cursorRect(tc).topLeft()) - QPoint(0, contextWidget()->height() + 10));
            tc.setPosition(end);
            QPoint p2 = editor->editor()->mapToParent(editor->editor()->viewport()->mapToParent(editor->editor()->cursorRect(tc).bottomLeft()) + QPoint(0, 10));
            QPoint offset = QPoint(10, 0);
            if (reg.boundingRect().width() < 400)
                offset = QPoint(400 - reg.boundingRect().width() + 10 ,0);
            QPoint p3 = editor->editor()->mapToParent(editor->editor()->viewport()->mapToParent(reg.boundingRect().topRight()) + offset);
            p2.setX(p1.x());
            if (!update)
                contextWidget()->activate(p3 , p1, p2);
            else
                contextWidget()->rePosition(p3 , p1, p2);
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

    if (cast<UiObjectDefinition*>(m_node) || cast<UiObjectBinding*>(m_node)) {
        UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(m_node);
        UiObjectBinding *objectBinding = cast<UiObjectBinding*>(m_node);

        UiObjectInitializer *initializer;
        if (objectDefinition)
            initializer = objectDefinition->initializer;
        else if (objectBinding)
            initializer = objectBinding->initializer;

        Utils::ChangeSet changeSet;
        Rewriter rewriter(m_doc->source(), &changeSet, m_propertyOrder);

        int line = 1;

        PropertyReader propertyReader(m_doc.data(), initializer);
        if (propertyReader.hasProperty(propertyName)) {
            rewriter.changeProperty(initializer, propertyName, stringValue, Rewriter::ScriptBinding);
        } else {
            rewriter.addBinding(initializer, propertyName, stringValue, Rewriter::ScriptBinding);
            int column;
            m_editor->convertPosition(changeSet.operationList().first().pos1, &line, &column); //get line
        }

        QTextCursor tc(m_editor->editor()->document());
        tc.beginEditBlock();
        int cursorPostion = tc.position();
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
    if (cast<UiObjectDefinition*>(m_node) || cast<UiObjectBinding*>(m_node)) {
        UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(m_node);
        UiObjectBinding *objectBinding = cast<UiObjectBinding*>(m_node);

        UiObjectInitializer *initializer;
        if (objectDefinition)
            initializer = objectDefinition->initializer;
        else if (objectBinding)
            initializer = objectBinding->initializer;

        PropertyReader propertyReader(m_doc.data(), initializer);
        if (propertyReader.hasProperty(propertyName)) {
            Utils::ChangeSet changeSet;
            Rewriter rewriter(m_doc->source(), &changeSet, m_propertyOrder);
            rewriter.removeProperty(initializer, propertyName);
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

    QTextCursor tc(m_editor->editor()->document());
    tc.beginEditBlock();

    removeProperty(remove);
    setProperty(change, value);

    tc.endEditBlock();

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
