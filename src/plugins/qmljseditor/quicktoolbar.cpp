/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "quicktoolbar.h"
#include "quicktoolbarsettingspage.h"

#include <utils/changeset.h>
#include <qmleditorwidgets/contextpanewidget.h>
#include <qmleditorwidgets/customcolordialog.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljspropertyreader.h>
#include <qmljs/qmljsrewriter.h>
#include <qmljstools/qmljsindenter.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljsutils.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/tabsettings.h>
#include <coreplugin/icore.h>

#include <QDebug>

using namespace QmlJS;
using namespace AST;
using namespace QmlEditorWidgets;

namespace QmlJSEditor {

static inline const ObjectValue * getPropertyChangesTarget(Node *node, const ScopeChain &scopeChain)
{
    UiObjectInitializer *initializer = initializerOfObject(node);
    if (initializer) {
        for (UiObjectMemberList *members = initializer->members; members; members = members->next) {
            if (UiScriptBinding *scriptBinding = cast<UiScriptBinding *>(members->member)) {
                if (scriptBinding->qualifiedId
                        && scriptBinding->qualifiedId->name == QLatin1String("target")
                        && ! scriptBinding->qualifiedId->next) {
                    Evaluate evaluator(&scopeChain);
                    const Value *targetValue = evaluator(scriptBinding->statement);
                    if (const ObjectValue *targetObject = value_cast<ObjectValue>(targetValue))
                        return targetObject;
                    else
                        return 0;
                }
            }
        }
    }
    return 0;
}

QuickToolBar::QuickToolBar(QObject *parent)
    : ::IContextPane(parent)
    , m_editorWidget(0)
    , m_blockWriting(false)
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

QuickToolBar::~QuickToolBar()
{
    delete m_widget.data();
    m_widget = 0;
}

void QuickToolBar::apply(TextEditor::TextEditorWidget *editorWidget, Document::Ptr document, const ScopeChain *scopeChain, Node *node, bool update, bool force)
{
    if (!QuickToolBarSettings::get().enableContextPane && !force && !update) {
        contextWidget()->hide();
        return;
    }

    if (document.isNull())
        return;

    if (update && editorWidget != m_editorWidget)
        return; //do not update for different editor

    m_blockWriting = true;

    const ObjectValue *scopeObject = document->bind()->findQmlObject(node);

    bool isPropertyChanges = false;

    if (scopeChain && scopeObject) {
        m_prototypes.clear();
        foreach (const ObjectValue *object,
                 PrototypeIterator(scopeObject, scopeChain->context()).all()) {
            m_prototypes.append(object->className());
        }

        if (m_prototypes.contains(QLatin1String("PropertyChanges"))) {
            isPropertyChanges = true;
            const ObjectValue *targetObject = getPropertyChangesTarget(node, *scopeChain);
            m_prototypes.clear();
            if (targetObject) {
                foreach (const ObjectValue *object,
                         PrototypeIterator(targetObject, scopeChain->context()).all()) {
                    m_prototypes.append(object->className());
                }
            }
        }
    }

    setEnabled(document->isParsedCorrectly());
    m_editorWidget = editorWidget;
    contextWidget()->setParent(editorWidget->parentWidget());
    contextWidget()->colorDialog()->setParent(editorWidget->parentWidget());

    if (cast<UiObjectDefinition*>(node) || cast<UiObjectBinding*>(node)) {
        UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(node);
        UiObjectBinding *objectBinding = cast<UiObjectBinding*>(node);

        QString name;
        quint32 offset = 0;
        quint32 end = 0;
        UiObjectInitializer *initializer = 0;
        if (objectDefinition) {
            name = objectDefinition->qualifiedTypeNameId->name.toString();
            initializer = objectDefinition->initializer;
            offset = objectDefinition->firstSourceLocation().offset;
            end = objectDefinition->lastSourceLocation().end();
        } else if (objectBinding) {
            name = objectBinding->qualifiedTypeNameId->name.toString();
            initializer = objectBinding->initializer;
            offset = objectBinding->firstSourceLocation().offset;
            end = objectBinding->lastSourceLocation().end();
        }

        if (!scopeChain) {
            if (name != m_oldType)
                m_prototypes.clear();
        }

        m_oldType = name;

        m_prototypes.append(name);

        int line1;
        int column1;
        int line2;
        int column2;
        m_editorWidget->convertPosition(offset, &line1, &column1); //get line
        m_editorWidget->convertPosition(end, &line2, &column2); //get line

        QRegion reg;
        if (line1 > -1 && line2 > -1)
            reg = m_editorWidget->translatedLineRegion(line1 - 1, line2);

        QRect rect;
        rect.setHeight(widget()->height() + 10);
        rect.setWidth(reg.boundingRect().width() - reg.boundingRect().left());
        rect.moveTo(reg.boundingRect().topLeft());
        reg = reg.intersected(rect);

        if (contextWidget()->acceptsType(m_prototypes)) {
            m_node = 0;
            PropertyReader propertyReader(document, initializer);
            QTextCursor tc = m_editorWidget->textCursor();
            tc.setPosition(offset);
            QPoint p1 = m_editorWidget->mapToParent(m_editorWidget->viewport()->mapToParent(m_editorWidget->cursorRect(tc).topLeft()) - QPoint(0, contextWidget()->height() + 10));
            tc.setPosition(end);
            QPoint p2 = m_editorWidget->mapToParent(m_editorWidget->viewport()->mapToParent(m_editorWidget->cursorRect(tc).bottomLeft()) + QPoint(0, 10));
            QPoint offset = QPoint(10, 0);
            if (reg.boundingRect().width() < 400)
                offset = QPoint(400 - reg.boundingRect().width() + 10 ,0);
            QPoint p3 = m_editorWidget->mapToParent(m_editorWidget->viewport()->mapToParent(reg.boundingRect().topRight()) + offset);
            p2.setX(p1.x());
            contextWidget()->setIsPropertyChanges(isPropertyChanges);
            if (!update)
                contextWidget()->setType(m_prototypes);
            if (!update)
                contextWidget()->activate(p3 , p1, p2, QuickToolBarSettings::get().pinContextPane);
            else
                contextWidget()->rePosition(p3 , p1, p2, QuickToolBarSettings::get().pinContextPane);
            contextWidget()->setOptions(QuickToolBarSettings::get().enableContextPane, QuickToolBarSettings::get().pinContextPane);
            contextWidget()->setPath(document->path());
            contextWidget()->setProperties(&propertyReader);
            m_doc = document;
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

bool QuickToolBar::isAvailable(TextEditor::TextEditorWidget *, Document::Ptr document, Node *node)
{
    if (document.isNull())
        return false;

    if (!node)
        return false;

    QString name;

    UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(node);
    UiObjectBinding *objectBinding = cast<UiObjectBinding*>(node);
    if (objectDefinition)
        name = objectDefinition->qualifiedTypeNameId->name.toString();

    else if (objectBinding)
        name = objectBinding->qualifiedTypeNameId->name.toString();

    QStringList prototypes;
    prototypes.append(name);

    if (prototypes.contains(QLatin1String("Rectangle")) ||
            prototypes.contains(QLatin1String("Image")) ||
            prototypes.contains(QLatin1String("BorderImage")) ||
            prototypes.contains(QLatin1String("TextEdit")) ||
            prototypes.contains(QLatin1String("TextInput")) ||
            prototypes.contains(QLatin1String("PropertyAnimation")) ||
            prototypes.contains(QLatin1String("NumberAnimation")) ||
            prototypes.contains(QLatin1String("Text")) ||
            prototypes.contains(QLatin1String("PropertyChanges")))
        return true;

    return false;
}

void QuickToolBar::setProperty(const QString &propertyName, const QVariant &value)
{

    QString stringValue = value.toString();
    if (value.type() == QVariant::Color)
        stringValue = QLatin1Char('\"') + value.toString() + QLatin1Char('\"');

    if (cast<UiObjectDefinition*>(m_node) || cast<UiObjectBinding*>(m_node)) {
        UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(m_node);
        UiObjectBinding *objectBinding = cast<UiObjectBinding*>(m_node);

        UiObjectInitializer *initializer = 0;
        if (objectDefinition)
            initializer = objectDefinition->initializer;
        else if (objectBinding)
            initializer = objectBinding->initializer;

        Utils::ChangeSet changeSet;
        Rewriter rewriter(m_doc->source(), &changeSet, m_propertyOrder);

        int line = -1;
        int endLine;

        Rewriter::BindingType bindingType = Rewriter::ScriptBinding;

        if (stringValue.contains(QLatin1Char('{')) && stringValue.contains(QLatin1Char('}')))
            bindingType = Rewriter::ObjectBinding;

        PropertyReader propertyReader(m_doc, initializer);
        if (propertyReader.hasProperty(propertyName))
            rewriter.changeBinding(initializer, propertyName, stringValue, bindingType);
        else
            rewriter.addBinding(initializer, propertyName, stringValue, bindingType);

        int column;

        int changeSetPos = changeSet.operationList().last().pos1;
        int changeSetLength = changeSet.operationList().last().text.length();
        QTextCursor tc = m_editorWidget->textCursor();
        tc.beginEditBlock();
        changeSet.apply(&tc);

        m_editorWidget->convertPosition(changeSetPos, &line, &column); //get line
        m_editorWidget->convertPosition(changeSetPos + changeSetLength, &endLine, &column); //get line

        indentLines(line, endLine);
        tc.endEditBlock();
    }
}

void QuickToolBar::removeProperty(const QString &propertyName)
{
    if (cast<UiObjectDefinition*>(m_node) || cast<UiObjectBinding*>(m_node)) {
        UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(m_node);
        UiObjectBinding *objectBinding = cast<UiObjectBinding*>(m_node);

        UiObjectInitializer *initializer = 0;
        if (objectDefinition)
            initializer = objectDefinition->initializer;
        else if (objectBinding)
            initializer = objectBinding->initializer;

        PropertyReader propertyReader(m_doc, initializer);
        if (propertyReader.hasProperty(propertyName)) {
            Utils::ChangeSet changeSet;
            Rewriter rewriter(m_doc->source(), &changeSet, m_propertyOrder);
            rewriter.removeBindingByName(initializer, propertyName);
            QTextCursor tc(m_editorWidget->document());
            changeSet.apply(&tc);
        }
    }
}

void QuickToolBar::setEnabled(bool b)
{
    if (m_widget)
        contextWidget()->currentWidget()->setEnabled(b);
    if (!b)
        widget()->hide();
}


QWidget* QuickToolBar::widget()
{
    return contextWidget();
}


void QuickToolBar::onPropertyChanged(const QString &name, const QVariant &value)
{
    if (m_blockWriting)
        return;
    if (!m_doc)
        return;

    setProperty(name, value);
    m_doc.clear(); //the document is outdated
}

void QuickToolBar::onPropertyRemovedAndChange(const QString &remove, const QString &change, const QVariant &value, bool removeFirst)
{
    if (m_blockWriting)
        return;

    if (!m_doc)
        return;

    QTextCursor tc = m_editorWidget->textCursor();
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

void QuickToolBar::onPinnedChanged(bool b)
{
    QuickToolBarSettings settings = QuickToolBarSettings::get();
    settings.pinContextPane = b;
    settings.set();
}

void QuickToolBar::onEnabledChanged(bool b)
{
    QuickToolBarSettings settings = QuickToolBarSettings::get();
    settings.pinContextPane = b;
    settings.enableContextPane = b;
    settings.set();
}

void QuickToolBar::indentLines(int startLine, int endLine)
{
    if (startLine > 0) {
        TextEditor::TabSettings tabSettings = m_editorWidget->textDocument()->tabSettings();
        for (int i = startLine; i <= endLine; i++) {
            QTextBlock start = m_editorWidget->document()->findBlockByNumber(i);

            if (start.isValid()) {
                QmlJSEditor::Internal::Indenter indenterMy;
                indenterMy.indentBlock(m_editorWidget->document(), start, QChar::Null, tabSettings);
            }
        }
    }
}

ContextPaneWidget* QuickToolBar::contextWidget()
{
    if (m_widget.isNull()) { //lazily recreate widget
        m_widget = new ContextPaneWidget;
        connect(m_widget.data(), SIGNAL(propertyChanged(QString,QVariant)), this, SLOT(onPropertyChanged(QString,QVariant)));
        connect(m_widget.data(), SIGNAL(removeProperty(QString)), this, SLOT(onPropertyRemoved(QString)));
        connect(m_widget.data(), SIGNAL(removeAndChangeProperty(QString,QString,QVariant,bool)), this, SLOT(onPropertyRemovedAndChange(QString,QString,QVariant,bool)));
        connect(m_widget.data(), SIGNAL(enabledChanged(bool)), this, SLOT(onEnabledChanged(bool)));
        connect(m_widget.data(), SIGNAL(pinnedChanged(bool)), this, SLOT(onPinnedChanged(bool)));
        connect(m_widget.data(), SIGNAL(closed()), this, SIGNAL(closed()));
    }
    return m_widget.data();
}

void QuickToolBar::onPropertyRemoved(const QString &propertyName)
{
    if (m_blockWriting)
        return;

    if (!m_doc)
        return;

    removeProperty(propertyName);
    m_doc.clear(); //the document is outdated
}

} //QmlDesigner
