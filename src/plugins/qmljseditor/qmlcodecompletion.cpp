/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlcodecompletion.h"
#include "qmljseditor.h"
#include "qmlmodelmanagerinterface.h"
#include "qmlexpressionundercursor.h"
#include "qmllookupcontext.h"
#include "qmlresolveexpression.h"

#include <qmljs/qmljssymbol.h>
#include <texteditor/basetexteditor.h>

#include <QtDebug>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

QmlCodeCompletion::QmlCodeCompletion(QmlModelManagerInterface *modelManager, QmlJS::TypeSystem *typeSystem, QObject *parent)
    : TextEditor::ICompletionCollector(parent),
      m_modelManager(modelManager),
      m_editor(0),
      m_startPosition(0),
      m_caseSensitivity(Qt::CaseSensitive),
      m_typeSystem(typeSystem)
{
    Q_ASSERT(modelManager);
    Q_ASSERT(typeSystem);
}

QmlCodeCompletion::~QmlCodeCompletion()
{ }

Qt::CaseSensitivity QmlCodeCompletion::caseSensitivity() const
{ return m_caseSensitivity; }

void QmlCodeCompletion::setCaseSensitivity(Qt::CaseSensitivity caseSensitivity)
{ m_caseSensitivity = caseSensitivity; }

bool QmlCodeCompletion::supportsEditor(TextEditor::ITextEditable *editor)
{
    if (qobject_cast<QmlJSTextEditor *>(editor->widget()))
        return true;

    return false;
}

bool QmlCodeCompletion::triggersCompletion(TextEditor::ITextEditable *)
{ return false; }

int QmlCodeCompletion::startCompletion(TextEditor::ITextEditable *editor)
{
    m_editor = editor;

    QmlJSTextEditor *edit = qobject_cast<QmlJSTextEditor *>(m_editor->widget());
    if (! edit)
        return -1;

    int pos = editor->position();

    while (editor->characterAt(pos - 1).isLetterOrNumber() || editor->characterAt(pos - 1) == QLatin1Char('_'))
        --pos;

    m_startPosition = pos;
    m_completions.clear();

    QmlJS::Document::Ptr qmlDocument = edit->qmlDocument();
//    qDebug() << "*** document:" << qmlDocument;
    if (qmlDocument.isNull())
        return pos;

    if (!qmlDocument->qmlProgram())
        qmlDocument = m_modelManager->snapshot().value(qmlDocument->fileName());

    // FIXME: this completion strategy is not going to work when the document was never parsed correctly.
    if (qmlDocument && qmlDocument->qmlProgram()) {
         QmlJS::AST::UiProgram *program = qmlDocument->qmlProgram();
//         qDebug() << "*** program:" << program;
 
         if (program) {
            QmlExpressionUnderCursor expressionUnderCursor;
            QTextCursor cursor(edit->document());
            cursor.setPosition(pos);
            expressionUnderCursor(cursor, qmlDocument);

            QmlLookupContext context(expressionUnderCursor.expressionScopes(), qmlDocument, m_modelManager->snapshot(), m_typeSystem);
            QmlResolveExpression resolver(context);
//            qDebug()<<"*** expression under cursor:"<<expressionUnderCursor.expressionNode();
            QList<QmlJS::Symbol*> symbols = resolver.visibleSymbols(expressionUnderCursor.expressionNode());
//            qDebug()<<"***"<<symbols.size()<<"visible symbols";

            foreach (QmlJS::Symbol *symbol, symbols) {
                QString word;

                if (symbol->isIdSymbol()) {
                    word = symbol->asIdSymbol()->id();
                } else {
                    word = symbol->name();
                }

                if (word.isEmpty())
                    continue;
 
                TextEditor::CompletionItem item(this);
                item.text = word;
                m_completions.append(item);
            }
        }
    }


    // snippets completion
    TextEditor::CompletionItem item(this);
    item.text = QLatin1String("Rectangle - declaration");
    item.data = QVariant::fromValue(QString("Rectangle {\nwidth: $100$;\nheight: 100;\n$$\n}"));
    m_completions.append(item);

    item.text = QLatin1String("Item - declaration");
    item.data = QVariant::fromValue(QString("Item {\nwidth: $100$;\nheight: 100;\n$$\n}"));
    m_completions.append(item);

    item.text = QLatin1String("BorderImage - declaration");
    item.data = QLatin1String("BorderImage {\n"
                              "id: $name$;\n"
                              "width: $100$; height: $100$;\n"
                              "border.left: $2$; border.top: $2$;\n"
                              "border.right: $2$; border.bottom: $2$;\n"
                              "source: \"$name$\";\n"
                              "}\n");
    m_completions.append(item);

    item.text = QLatin1String("State - declaration");
    item.data = QLatin1String("State {\n"
                              "name: \"$state$;\"\n"
                              "PropertyChanges {\n"
                              "target: $target$;\n"
                              "$$\n"
                              "}\n"
                              "}\n");
    m_completions.append(item);

    item.text = QLatin1String("states - declaration");
    item.data = QLatin1String("states: [\n"
                              "State {\n"
                              "name: \"$state$\";\n"
                              "PropertyChanges {\n"
                              "target: $target$;\n"
                              "$$\n"
                              "}\n"
                              "}\n"
                              "]\n");
    m_completions.append(item);

    item.text = QLatin1String("property - declaration");
    item.data = QLatin1String("property $var$ $name$: $value$;\n");
    m_completions.append(item);

    item.text = QLatin1String("readonly property - declaration");
    item.data = QLatin1String("readonly property $var$ $name$: $value$;\n");
    m_completions.append(item);

    item.text = QLatin1String("NumericAnimation - declaration");
    item.data = QLatin1String("NumberAnimation { matchTargets: \"$target$\"; matchProperties: \"$properties$\"; duration: $1000$ }\n");
    m_completions.append(item);

    return pos;
}

void QmlCodeCompletion::completions(QList<TextEditor::CompletionItem> *completions)
{
    // ### FIXME: this code needs to be generalized.

    const int length = m_editor->position() - m_startPosition;

    if (length == 0)
        *completions = m_completions;
    else if (length > 0) {
        const QString key = m_editor->textAt(m_startPosition, length);

        /*
         * This code builds a regular expression in order to more intelligently match
         * camel-case style. This means upper-case characters will be rewritten as follows:
         *
         *   A => [a-z0-9_]*A (for any but the first capital letter)
         *
         * Meaning it allows any sequence of lower-case characters to preceed an
         * upper-case character. So for example gAC matches getActionController.
         */
        QString keyRegExp;
        keyRegExp += QLatin1Char('^');
        bool first = true;
        foreach (const QChar &c, key) {
            if (c.isUpper() && !first) {
                keyRegExp += QLatin1String("[a-z0-9_]*");
                keyRegExp += c;
            } else if (m_caseSensitivity == Qt::CaseInsensitive && c.isLower()) {
                keyRegExp += QLatin1Char('[');
                keyRegExp += c;
                keyRegExp += c.toUpper();
                keyRegExp += QLatin1Char(']');
            } else {
                keyRegExp += QRegExp::escape(c);
            }
            first = false;
        }
        const QRegExp regExp(keyRegExp, Qt::CaseSensitive);

        foreach (TextEditor::CompletionItem item, m_completions) {
            if (regExp.indexIn(item.text) == 0) {
                item.relevance = (key.length() > 0 &&
                                    item.text.startsWith(key, Qt::CaseInsensitive)) ? 1 : 0;
                (*completions) << item;
            }
        }
    }
}

void QmlCodeCompletion::complete(const TextEditor::CompletionItem &item)
{
    QString toInsert = item.text;

    if (QmlJSTextEditor *edit = qobject_cast<QmlJSTextEditor *>(m_editor->widget())) {
        if (item.data.isValid()) {
            QTextCursor tc = edit->textCursor();
            tc.beginEditBlock();
            tc.setPosition(m_startPosition);
            tc.setPosition(m_editor->position(), QTextCursor::KeepAnchor);
            tc.removeSelectedText();

            toInsert = item.data.toString();
            edit->insertCodeSnippet(toInsert);
            tc.endEditBlock();
            return;
        }
    }

    const int length = m_editor->position() - m_startPosition;
    m_editor->setCurPos(m_startPosition);
    m_editor->replace(length, toInsert);
}

bool QmlCodeCompletion::partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems)
{
    if (completionItems.count() == 1) {
        complete(completionItems.first());
        return true;
    } else {
        // Compute common prefix
        QString firstKey = completionItems.first().text;
        QString lastKey = completionItems.last().text;
        const int length = qMin(firstKey.length(), lastKey.length());
        firstKey.truncate(length);
        lastKey.truncate(length);

        while (firstKey != lastKey) {
            firstKey.chop(1);
            lastKey.chop(1);
        }

        int typedLength = m_editor->position() - m_startPosition;
        if (!firstKey.isEmpty() && firstKey.length() > typedLength) {
            m_editor->setCurPos(m_startPosition);
            m_editor->replace(typedLength, firstKey);
        }
    }
    return false;
}

void QmlCodeCompletion::cleanup()
{
    m_editor = 0;
    m_startPosition = 0;
    m_completions.clear();
}

