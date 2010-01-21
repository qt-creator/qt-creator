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

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljssymbol.h>
#include <texteditor/basetexteditor.h>

#include <coreplugin/icore.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QXmlStreamReader>
#include <QtGui/QPainter>

#include <QtDebug>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;


// Temporary workaround until we have proper icons for QML completion items
static QIcon iconForColor(const QColor &color)
{
    QPixmap pix(6, 6);

    int pixSize = 20;
    QBrush br(color);

    QPixmap pm(2 * pixSize, 2 * pixSize);
    QPainter pmp(&pm);
    pmp.fillRect(0, 0, pixSize, pixSize, Qt::lightGray);
    pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::lightGray);
    pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::darkGray);
    pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::darkGray);
    pmp.fillRect(0, 0, 2 * pixSize, 2 * pixSize, color);
    br = QBrush(pm);

    QPainter p(&pix);
    int corr = 1;
    QRect r = pix.rect().adjusted(corr, corr, -corr, -corr);
    p.setBrushOrigin((r.width() % pixSize + pixSize) / 2 + corr, (r.height() % pixSize + pixSize) / 2 + corr);
    p.fillRect(r, br);

    p.fillRect(r.width() / 4 + corr, r.height() / 4 + corr,
               r.width() / 2, r.height() / 2,
               QColor(color.rgb()));
    p.drawRect(pix.rect().adjusted(0, 0, -1, -1));

    return pix;
}


class Evaluate: public QmlJS::AST::Visitor
{
    QmlJS::Interpreter::Engine *_interp;
    const QmlJS::Interpreter::Value *_value;

public:
    Evaluate(QmlJS::Interpreter::Engine *interp)
        : _interp(interp), _value(0)
    {}

    const QmlJS::Interpreter::Value *operator()(QmlJS::AST::Node *node)
    { return evaluate(node); }

    const QmlJS::Interpreter::Value *evaluate(QmlJS::AST::Node *node)
    {
        const QmlJS::Interpreter::Value *previousValue = switchValue(0);

        if (node)
            node->accept(this);

        return switchValue(previousValue);
    }

protected:
    using QmlJS::AST::Visitor::visit;

    const QmlJS::Interpreter::Value *switchValue(const QmlJS::Interpreter::Value *value)
    {
        const QmlJS::Interpreter::Value *previousValue = _value;
        _value = value;
        return previousValue;
    }

    virtual bool preVisit(QmlJS::AST::Node *ast)
    {
        using namespace QmlJS::AST;

        if (cast<NumericLiteral *>(ast))
            return true;

        if (cast<StringLiteral *>(ast))
            return true;

        if (cast<IdentifierExpression *>(ast))
            return true;

        else if (cast<FieldMemberExpression *>(ast))
            return true;

        else if (cast<CallExpression *>(ast))
            return true;

        return false;
    }

    virtual bool visit(QmlJS::AST::StringLiteral *)
    {
        _value = _interp->convertToObject(_interp->stringValue());
        return false;
    }

    virtual bool visit(QmlJS::AST::NumericLiteral *)
    {
        _value = _interp->convertToObject(_interp->numberValue());
        return false;
    }

    virtual bool visit(QmlJS::AST::IdentifierExpression *ast)
    {
        _value = _interp->globalObject()->property(ast->name->asString());
        return false;
    }

    virtual bool visit(QmlJS::AST::FieldMemberExpression *ast)
    {
        if (const QmlJS::Interpreter::Value *base = evaluate(ast->base)) {
            if (const QmlJS::Interpreter::ObjectValue *obj = base->asObjectValue()) {
                _value = obj->property(ast->name->asString());
            }
        }

        return false;
    }

    virtual bool visit(QmlJS::AST::CallExpression *ast)
    {
        if (const QmlJS::Interpreter::Value *base = evaluate(ast->base)) {
            if (const QmlJS::Interpreter::FunctionValue *obj = base->asFunctionValue()) {
                _value = obj->returnValue();
            }
        }

        return false;
    }

};

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
        return -1;

    const QmlJS::Snapshot &snapshot = m_modelManager->snapshot();

    if (! qmlDocument->qmlProgram()) {
        qmlDocument = snapshot.value(qmlDocument->fileName());

        if (! qmlDocument)
            return -1;
    }

    const QIcon typeIcon = iconForColor(Qt::yellow);

    foreach (QmlJS::Document::Ptr doc, snapshot) {
        const QFileInfo fileInfo(doc->fileName());
        if (fileInfo.suffix() != QLatin1String("qml"))
            continue;

        const QString typeName = fileInfo.baseName();
        if (typeName.isEmpty())
            continue;

        if (typeName.at(0).isUpper()) {
            TextEditor::CompletionItem item(this);
            item.text = typeName;
            item.icon = typeIcon;
            m_completions.append(item);
        }
    }

    QChar previousChar;
    if (m_startPosition > 0)
        previousChar = editor->characterAt(m_startPosition - 1);

    if (previousChar.isSpace() || previousChar.isNull()) {
        // ### FIXME
        const QIcon idIcon = iconForColor(Qt::darkGray);
        QStringList ids = qmlDocument->ids().keys();
        foreach (const QString &id, ids) {
            if (id.isEmpty())
                continue;

            TextEditor::CompletionItem item(this);
            item.text = id;
            item.icon = idIcon;
            m_completions.append(item);
        }
    }

    // FIXME: this completion strategy is not going to work when the document was never parsed correctly.
    if (qmlDocument->qmlProgram() != 0) {
        const QIcon otherIcon = iconForColor(Qt::darkCyan);

        // qDebug() << "*** program:" << program;
        QmlExpressionUnderCursor expressionUnderCursor;
        QTextCursor cursor(edit->document());
        cursor.setPosition(pos);
        expressionUnderCursor(cursor, qmlDocument);

        QmlLookupContext context(expressionUnderCursor.expressionScopes(), qmlDocument, m_modelManager->snapshot(), m_typeSystem);
        QmlResolveExpression resolver(context);

        QmlJS::AST::Node *expr = expressionUnderCursor.expressionNode();

        QmlJS::Interpreter::Engine engine;
        Evaluate evaluate(&engine);
        if (const QmlJS::Interpreter::Value *value = evaluate(expr)) {
            if (const QmlJS::Interpreter::ObjectValue *object = value->asObjectValue()) {
                for (QmlJS::Interpreter::ObjectValue::MemberIterator it = object->firstMember(); it != object->lastMember(); ++it) {
                    TextEditor::CompletionItem item(this);
                    item.text = it.key();
                    item.icon = otherIcon;
                    m_completions.append(item);
                }
                return pos;
            }
        }

        // qDebug()<<"*** expression under cursor:"<<expressionUnderCursor.expressionNode();
        const QList<QmlJS::Symbol*> symbols = resolver.visibleSymbols(expressionUnderCursor.expressionNode());
        // qDebug()<<"***"<<symbols.size()<<"visible symbols";

        foreach (QmlJS::Symbol *symbol, symbols) {
            if (symbol->isIdSymbol())
                continue; // nothing to do here.

            const QString word = symbol->name();
            if (! word.isEmpty()) {
                TextEditor::CompletionItem item(this);
                item.text = word;
                item.icon = otherIcon;
                m_completions.append(item);
            }
        }
    }

    if (previousChar.isNull()
            || previousChar.isSpace()
            || previousChar == QLatin1Char('{')
            || previousChar == QLatin1Char('}')
            || previousChar == QLatin1Char(':')
            || previousChar == QLatin1Char(';')) {
        updateSnippets();
        m_completions.append(m_snippets);
    }

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
        const TextEditor::CompletionItem item = completionItems.first();

        if (!item.data.canConvert<QString>()) {
            complete(item);
            return true;
        }
    }

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

    return false;
}

void QmlCodeCompletion::cleanup()
{
    m_editor = 0;
    m_startPosition = 0;
    m_completions.clear();
}


void QmlCodeCompletion::updateSnippets()
{
    QString qmlsnippets = Core::ICore::instance()->resourcePath() + QLatin1String("/snippets/qml.xml");
    if (!QFile::exists(qmlsnippets))
        return;

    QDateTime lastModified = QFileInfo(qmlsnippets).lastModified();
    if (!m_snippetFileLastModified.isNull() &&  lastModified == m_snippetFileLastModified)
        return;

    const QIcon icon = iconForColor(Qt::red);

    m_snippetFileLastModified = lastModified;
    QFile file(qmlsnippets);
    file.open(QIODevice::ReadOnly);
    QXmlStreamReader xml(&file);
    while (!xml.atEnd() && xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("snippets")) {
            while (xml.readNextStartElement()) {
                if (xml.name() == QLatin1String("snippet")) {
                    TextEditor::CompletionItem item(this);
                    QString title, data;
                    QString description = xml.attributes().value("description").toString();

                    while (!xml.atEnd()) {
                        xml.readNext();
                        if (xml.isEndElement()) {
                            int i = 0;
                            while (i < data.size() && data.at(i).isLetterOrNumber())
                                ++i;
                            title = data.left(i);
                            item.text = title;
                            if (!description.isEmpty()) {
                                item.text +=  QLatin1Char(' ');
                                item.text += description;
                            }
                            item.data = QVariant::fromValue(data);
                            item.icon = icon;
                            m_snippets.append(item);
                            break;

                        }

                        if (xml.isCharacters())
                            data += xml.text();
                        else if (xml.isStartElement()) {
                            if (xml.name() != QLatin1String("tab"))
                                xml.raiseError(QLatin1String("invalid snippets file"));
                            else {
                                data += QChar::ObjectReplacementCharacter;
                                data += xml.readElementText();
                                data += QChar::ObjectReplacementCharacter;
                            }
                        }

                    }
                }
            }
        }
    }
    if (xml.hasError())
        qWarning() << qmlsnippets << xml.errorString();
    file.close();
}

QList<TextEditor::CompletionItem> QmlCodeCompletion::getCompletions()
{
    QList<TextEditor::CompletionItem> completionItems;

    completions(&completionItems);

    qStableSort(completionItems.begin(), completionItems.end(), completionItemLessThan);

    // Remove duplicates
    QString lastKey;
    QList<TextEditor::CompletionItem> uniquelist;

    foreach (const TextEditor::CompletionItem &item, completionItems) {
        if (item.text != lastKey) {
            uniquelist.append(item);
            lastKey = item.text;
        } else {
            if (item.data.canConvert<QString>())
                uniquelist.append(item);
        }
    }

    return uniquelist;
}
