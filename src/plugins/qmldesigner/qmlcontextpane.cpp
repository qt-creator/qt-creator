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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlcontextpane.h"
#include <contextpanewidget.h>
#include <qmldesignerplugin.h>
#include <quicktoolbarsettingspage.h>

#include <utils/changeset.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljspropertyreader.h>
#include <qmljs/qmljsrewriter.h>
#include <qmljs/qmljsindenter.h>
#include <qmljs/qmljslookupcontext.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsscopebuilder.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/tabsettings.h>
#include <coreplugin/icore.h>
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
    contextWidget();

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

void QmlContextPane::apply(TextEditor::BaseTextEditorEditable *editor, Document::Ptr doc, const QmlJS::Snapshot &snapshot, AST::Node *node, bool update, bool force)
{
    if (!QuickToolBarSettings::get().enableContextPane && !force && !update) {
        contextWidget()->hide();
        return;
    }

    if (doc.isNull())
        return;

    if (update && editor != m_editor)
        return; //do not update for different editor

    m_blockWriting = true;

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

        QRect rect;
        rect.setHeight(m_widget->height() + 10);
        rect.setWidth(reg.boundingRect().width() - reg.boundingRect().left());
        rect.moveTo(reg.boundingRect().topLeft());
        reg = reg.intersect(rect);

        if (contextWidget()->acceptsType(prototypes)) {
            m_node = 0;
            PropertyReader propertyReader(doc, initializer);
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
            contextWidget()->setType(prototypes);
            if (!update)
                contextWidget()->activate(p3 , p1, p2, QuickToolBarSettings::get().pinContextPane);
            else
                contextWidget()->rePosition(p3 , p1, p2, QuickToolBarSettings::get().pinContextPane);
            contextWidget()->setOptions(QuickToolBarSettings::get().enableContextPane, QuickToolBarSettings::get().pinContextPane);
            contextWidget()->setPath(doc->path());
            contextWidget()->setProperties(&propertyReader); 
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

    m_blockWriting = false;

}

bool QmlContextPane::isAvailable(TextEditor::BaseTextEditorEditable *, Document::Ptr doc, const QmlJS::Snapshot &snapshot, AST::Node *node)
{
    if (doc.isNull())
        return false;

    if (!node)
        return false;

        LookupContext::Ptr lookupContext = LookupContext::create(doc, snapshot, QList<Node*>());
        const Interpreter::ObjectValue *scopeObject = doc->bind()->findQmlObject(node);

        QStringList prototypes;

        while (scopeObject) {
            prototypes.append(scopeObject->className());
            scopeObject =  scopeObject->prototype(lookupContext->context());
        }

        if (prototypes.contains("Rectangle") ||
            prototypes.contains("Image") ||
            prototypes.contains("BorderImage") ||
            prototypes.contains("TextEdit") ||
            prototypes.contains("TextInput") ||
            prototypes.contains("PropertyAnimation") ||
            prototypes.contains("Text"))
            return true;

        return false;
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

        int line = -1;
        int endLine;

        Rewriter::BindingType bindingType = Rewriter::ScriptBinding;

        if (stringValue.contains("{") && stringValue.contains("}"))
            bindingType = Rewriter::ObjectBinding;

        PropertyReader propertyReader(m_doc, initializer);
        if (propertyReader.hasProperty(propertyName)) {
            rewriter.changeBinding(initializer, propertyName, stringValue, bindingType);
        } else {
            rewriter.addBinding(initializer, propertyName, stringValue, bindingType);
        }

        int column;

        int changeSetPos = changeSet.operationList().last().pos1;
        int changeSetLength = changeSet.operationList().last().text.length();
        QTextCursor tc = m_editor->editor()->textCursor();
        tc.beginEditBlock();
        changeSet.apply(&tc);

        m_editor->convertPosition(changeSetPos, &line, &column); //get line
        m_editor->convertPosition(changeSetPos + changeSetLength, &endLine, &column); //get line

        if (line > 0) {
            TextEditor::TabSettings ts = m_editor->editor()->tabSettings();
            QmlJSIndenter indenter;
            indenter.setTabSize(ts.m_tabSize);
            indenter.setIndentSize(ts.m_indentSize);

            for (int i=line;i<=endLine;i++) {
                QTextBlock start = m_editor->editor()->document()->findBlockByNumber(i);
                QTextBlock end = m_editor->editor()->document()->findBlockByNumber(i);

                if (end.isValid()) {
                    const int indent = indenter.indentForBottomLine(m_editor->editor()->document()->begin(), end.next(), QChar::Null);
                    ts.indentLine(start, indent);
                }
            }
        }
        tc.endEditBlock();
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

        PropertyReader propertyReader(m_doc, initializer);
        if (propertyReader.hasProperty(propertyName)) {
            Utils::ChangeSet changeSet;
            Rewriter rewriter(m_doc->source(), &changeSet, m_propertyOrder);
            rewriter.removeBindingByName(initializer, propertyName);
            QTextCursor tc(m_editor->editor()->document());
            changeSet.apply(&tc);
        }
    }
}

void QmlContextPane::setEnabled(bool b)
{
    if (m_widget)
        m_widget->currentWidget()->setEnabled(b);
    if (!b)
        m_widget->hide();
}


QWidget* QmlContextPane::widget()
{
    return contextWidget();
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

void QmlContextPane::onPropertyRemovedAndChange(const QString &remove, const QString &change, const QVariant &value, bool removeFirst)
{
    if (m_blockWriting)
        return;

    if (!m_doc)
        return;

    QTextCursor tc(m_editor->editor()->document());
    tc.beginEditBlock();

    if (removeFirst) {
        removeProperty(remove);
        setProperty(change, value);
    } else {
        setProperty(change, value);
        removeProperty(remove);
    }


    tc.endEditBlock();

    m_doc.clear(); //the document is outdated

}

void QmlContextPane::onPinnedChanged(bool b)
{
    QuickToolBarSettings settings = QuickToolBarSettings::get();
    settings.pinContextPane = b;
    settings.set();
}

void QmlContextPane::onEnabledChanged(bool b)
{
    QuickToolBarSettings settings = QuickToolBarSettings::get();
    settings.pinContextPane = b;
    settings.enableContextPane = b;
    settings.set();
}

ContextPaneWidget* QmlContextPane::contextWidget()
{
    if (m_widget.isNull()) { //lazily recreate widget
        m_widget = new ContextPaneWidget;
        connect(m_widget.data(), SIGNAL(propertyChanged(QString,QVariant)), this, SLOT(onPropertyChanged(QString,QVariant)));
        connect(m_widget.data(), SIGNAL(removeProperty(QString)), this, SLOT(onPropertyRemoved(QString)));
        connect(m_widget.data(), SIGNAL(removeAndChangeProperty(QString,QString,QVariant, bool)), this, SLOT(onPropertyRemovedAndChange(QString,QString,QVariant, bool)));
        connect(m_widget.data(), SIGNAL(enabledChanged(bool)), this, SLOT(onEnabledChanged(bool)));
        connect(m_widget.data(), SIGNAL(pinnedChanged(bool)), this, SLOT(onPinnedChanged(bool)));
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
