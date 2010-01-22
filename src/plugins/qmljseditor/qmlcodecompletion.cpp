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
#include "qmllookupcontext.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljssymbol.h>
#include <qmljs/qmljsscanner.h>

#include <texteditor/basetexteditor.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QDebug>

#include <QtGui/QPainter>
#include <QtGui/QLabel>
#include <QtGui/QStylePainter>
#include <QtGui/QStyleOption>
#include <QtGui/QToolButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJS;


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

namespace {

class ExpressionUnderCursor
{
    QTextCursor _cursor;
    QmlJSScanner scanner;

public:
    QString operator()(const QTextCursor &cursor)
    {
        _cursor = cursor;

        QTextBlock block = _cursor.block();
        const QString blockText = block.text().left(cursor.columnNumber());
        //qDebug() << "block text:" << blockText;

        int startState = block.previous().userState();
        if (startState == -1)
            startState = 0;
        else
            startState = startState & 0xff;

        const QList<Token> originalTokens = scanner(blockText, startState);
        QList<Token> tokens;
        int skipping = 0;
        for (int index = originalTokens.size() - 1; index != -1; --index) {
            const Token &tk = originalTokens.at(index);

            if (tk.is(Token::Comment) || tk.is(Token::String) || tk.is(Token::Number))
                continue;

            if (! skipping) {
                tokens.append(tk);

                if (tk.is(Token::Identifier)) {
                    if (index > 0 && originalTokens.at(index - 1).isNot(Token::Dot))
                        break;
                }
            } else {
                //qDebug() << "skip:" << blockText.mid(tk.offset, tk.length);
            }

            if (tk.is(Token::RightParenthesis))
                ++skipping;

            else if (tk.is(Token::LeftParenthesis)) {
                --skipping;

                if (! skipping)
                    tokens.append(tk);
            }
        }

        if (! tokens.isEmpty()) {
            QString expr;
            for (int index = tokens.size() - 1; index >= 0; --index) {
                Token tk = tokens.at(index);
                expr.append(QLatin1Char(' '));
                expr.append(blockText.midRef(tk.offset, tk.length));
            }

            //qDebug() << "expression under cursor:" << expr;
            return expr;
        }

        //qDebug() << "no expression";
        return QString();
    }
};

class SearchPropertyDefinitions: protected AST::Visitor
{
    QList<AST::UiPublicMember *> _properties;

public:
    QList<AST::UiPublicMember *> operator()(AST::Node *node)
    {
        _properties.clear();
        if (node)
            node->accept(this);
        return _properties;
    }


protected:
    using AST::Visitor::visit;

    virtual bool visit(AST::UiPublicMember *member)
    {
        if (member->propertyToken.isValid()) {
            _properties.append(member);
        }

        return true;
    }
};

class Evaluate: protected QmlJS::AST::Visitor
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

    virtual bool preVisit(QmlJS::AST::Node *ast) // ### remove me
    {
        using namespace QmlJS::AST;

        if (cast<NumericLiteral *>(ast))
            return true;

        else if (cast<StringLiteral *>(ast))
            return true;

        else if (cast<IdentifierExpression *>(ast))
            return true;

        else if (cast<NestedExpression *>(ast))
            return true;

        else if (cast<FieldMemberExpression *>(ast))
            return true;

        else if (cast<CallExpression *>(ast))
            return true;

        return false;
    }

    virtual bool visit(QmlJS::AST::NestedExpression *)
    {
        return true;
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
        if (! ast->name)
            return false;

        _value = _interp->globalObject()->property(ast->name->asString());
        return false;
    }

    virtual bool visit(QmlJS::AST::FieldMemberExpression *ast)
    {
        if (! ast->name)
            return false;

        if (const QmlJS::Interpreter::Value *base = _interp->convertToObject(evaluate(ast->base))) {
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

class EnumerateProperties
{
    QSet<const Interpreter::ObjectValue *> _processed;
    QHash<QString, const Interpreter::Value *> _properties;

public:
    QHash<QString, const Interpreter::Value *> operator()(const Interpreter::Value *value)
    {
        _processed.clear();
        _properties.clear();
        enumerateProperties(value);
        return _properties;
    }

private:
    void enumerateProperties(const Interpreter::Value *value)
    {
        if (! value)
            return;
        else if (const Interpreter::ObjectValue *object = value->asObjectValue()) {
            enumerateProperties(object);
        }
    }

    void enumerateProperties(const Interpreter::ObjectValue *object)
    {
        if (! object || _processed.contains(object))
            return;

        _processed.insert(object);
        enumerateProperties(object->prototype());

        for (Interpreter::ObjectValue::MemberIterator it = object->firstMember(); it != object->lastMember(); ++it) {
            _properties.insert(it.key(), it.value());
        }
    }
};

} // end of anonymous namespace

namespace QmlJSEditor {
namespace Internal {

class FakeToolTipFrame : public QWidget
{
public:
    FakeToolTipFrame(QWidget *parent = 0) :
        QWidget(parent, Qt::ToolTip | Qt::WindowStaysOnTopHint)
    {
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_DeleteOnClose);

        // Set the window and button text to the tooltip text color, since this
        // widget draws the background as a tooltip.
        QPalette p = palette();
        const QColor toolTipTextColor = p.color(QPalette::Inactive, QPalette::ToolTipText);
        p.setColor(QPalette::Inactive, QPalette::WindowText, toolTipTextColor);
        p.setColor(QPalette::Inactive, QPalette::ButtonText, toolTipTextColor);
        setPalette(p);
    }

protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *e);
};

class FunctionArgumentWidget : public QLabel
{
public:
    FunctionArgumentWidget();
    void showFunctionHint(const QString &functionName, int minimumArgumentCount, int startPosition);

protected:
    bool eventFilter(QObject *obj, QEvent *e);

private:
    void updateArgumentHighlight();
    void updateHintText();

    QString m_functionName;
    int m_minimumArgumentCount;
    int m_startpos;
    int m_currentarg;
    int m_current;
    bool m_escapePressed;

    TextEditor::ITextEditor *m_editor;

    QWidget *m_pager;
    QLabel *m_numberLabel;
    FakeToolTipFrame *m_popupFrame;
};

void FakeToolTipFrame::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();
}

void FakeToolTipFrame::resizeEvent(QResizeEvent *)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);
}


FunctionArgumentWidget::FunctionArgumentWidget():
    m_minimumArgumentCount(0),
    m_startpos(-1),
    m_current(0),
    m_escapePressed(false)
{
    QObject *editorObject = Core::EditorManager::instance()->currentEditor();
    m_editor = qobject_cast<TextEditor::ITextEditor *>(editorObject);

    m_popupFrame = new FakeToolTipFrame(m_editor->widget());

    setParent(m_popupFrame);
    setFocusPolicy(Qt::NoFocus);

    m_pager = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(m_pager);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    m_numberLabel = new QLabel;
    hbox->addWidget(m_numberLabel);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_pager);
    layout->addWidget(this);
    m_popupFrame->setLayout(layout);

    setTextFormat(Qt::RichText);
    setMargin(1);

    qApp->installEventFilter(this);
}

void FunctionArgumentWidget::showFunctionHint(const QString &functionName, int mininumArgumentCount, int startPosition)
{
    if (m_startpos == startPosition)
        return;

    m_functionName = functionName;
    m_minimumArgumentCount = mininumArgumentCount;
    m_startpos = startPosition;
    m_current = 0;
    m_escapePressed = false;

    // update the text
    m_currentarg = -1;
    updateArgumentHighlight();

    m_popupFrame->show();
}

void FunctionArgumentWidget::updateArgumentHighlight()
{
    int curpos = m_editor->position();
    if (curpos < m_startpos) {
        m_popupFrame->close();
        return;
    }

    updateHintText();


    QString str = m_editor->textAt(m_startpos, curpos - m_startpos);
    int argnr = 0;
    int parcount = 0;
    QmlJSScanner tokenize;
    const QList<Token> tokens = tokenize(str);
    for (int i = 0; i < tokens.count(); ++i) {
        const Token &tk = tokens.at(i);
        if (tk.is(Token::LeftParenthesis))
            ++parcount;
        else if (tk.is(Token::RightParenthesis))
            --parcount;
        else if (! parcount && tk.is(Token::Colon))
            ++argnr;
    }

    if (m_currentarg != argnr) {
        // m_currentarg = argnr;
        updateHintText();
    }

    if (parcount < 0)
        m_popupFrame->close();
}

bool FunctionArgumentWidget::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            m_escapePressed = true;
        }
        break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            m_escapePressed = true;
        }
        break;
    case QEvent::KeyRelease:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && m_escapePressed) {
            m_popupFrame->close();
            return false;
        }
        updateArgumentHighlight();
        break;
    case QEvent::WindowDeactivate:
    case QEvent::FocusOut:
        if (obj != m_editor->widget())
            break;
        m_popupFrame->close();
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel: {
            QWidget *widget = qobject_cast<QWidget *>(obj);
            if (! (widget == this || m_popupFrame->isAncestorOf(widget))) {
                m_popupFrame->close();
            }
        }
        break;
    default:
        break;
    }
    return false;
}

void FunctionArgumentWidget::updateHintText()
{
    QString prettyMethod;
    prettyMethod += QString::fromLatin1("function ");
    prettyMethod += m_functionName;
    prettyMethod += QLatin1String("(arguments...)");

    m_numberLabel->setText(prettyMethod);

    m_popupFrame->setFixedWidth(m_popupFrame->minimumSizeHint().width());

    const QDesktopWidget *desktop = QApplication::desktop();
#ifdef Q_WS_MAC
    const QRect screen = desktop->availableGeometry(desktop->screenNumber(m_editor->widget()));
#else
    const QRect screen = desktop->screenGeometry(desktop->screenNumber(m_editor->widget()));
#endif

    const QSize sz = m_popupFrame->sizeHint();
    QPoint pos = m_editor->cursorRect(m_startpos).topLeft();
    pos.setY(pos.y() - sz.height() - 1);

    if (pos.x() + sz.width() > screen.right())
        pos.setX(screen.right() - sz.width());

    m_popupFrame->move(pos);
}

} } // end of namespace QmlJSEditor::Internal

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

bool QmlCodeCompletion::triggersCompletion(TextEditor::ITextEditable *editor)
{
    const QChar ch = editor->characterAt(editor->position() - 1);

    if (ch == QLatin1Char('(') || ch == QLatin1Char('.'))
        return true;

    return false;
}

int QmlCodeCompletion::startCompletion(TextEditor::ITextEditable *editor)
{
    m_editor = editor;

    QmlJSTextEditor *edit = qobject_cast<QmlJSTextEditor *>(m_editor->widget());
    if (! edit)
        return -1;

    m_startPosition = editor->position();

    while (editor->characterAt(m_startPosition - 1).isLetterOrNumber() || editor->characterAt(m_startPosition - 1) == QLatin1Char('_'))
        --m_startPosition;

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

    const QFileInfo currentFileInfo(qmlDocument->fileName());
    const QString currentFilePath = currentFileInfo.absolutePath();

    const QIcon typeIcon = iconForColor(Qt::yellow);

    QChar completionOperator;
    if (m_startPosition > 0)
        completionOperator = editor->characterAt(m_startPosition - 1);

    if (completionOperator.isSpace() || completionOperator.isNull()) {
        // Add the visible components to the completion box.
        foreach (QmlJS::Document::Ptr doc, snapshot) {
            const QFileInfo fileInfo(doc->fileName());

            if (fileInfo.suffix() != QLatin1String("qml"))
                continue;
            else if (fileInfo.absolutePath() != currentFilePath) // ### FIXME includ `imported' components
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

        // Add the visible IDs to the completion box
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

    if (completionOperator == QLatin1Char('.') || completionOperator == QLatin1Char('(')) {
        ExpressionUnderCursor expressionUnderCursor;

        QTextCursor tc = edit->textCursor();
        tc.setPosition(m_startPosition - 1);

        const QString expression = expressionUnderCursor(tc);
        //qDebug() << "expression:" << expression;

        QmlJS::Document::Ptr exprDoc = QmlJS::Document::create(QLatin1String("<expression>"));
        exprDoc->setSource(expression);
        exprDoc->parseExpression();

        if (exprDoc->ast()) {
            Interpreter::Engine interp;
            Evaluate evaluate(&interp);

            SearchPropertyDefinitions searchPropertyDefinitions;

            const QList<AST::UiPublicMember *> properties = searchPropertyDefinitions(qmlDocument->ast());
            foreach (AST::UiPublicMember *prop, properties) {
                if (prop->name && prop->memberType && prop->memberType->asString() == QLatin1String("string")) {
                    interp.globalObject()->setProperty(prop->name->asString(), interp.stringValue());
                }
            }

            const Interpreter::Value *value = interp.convertToObject(evaluate(exprDoc->ast()));
            //qDebug() << "type:" << interp.typeId(value);

            const QIcon symbolIcon = iconForColor(Qt::darkCyan);

            if (value && completionOperator == QLatin1Char('.')) { // member completion
                EnumerateProperties enumerateProperties;
                QHashIterator<QString, const Interpreter::Value *> it(enumerateProperties(value));
                while (it.hasNext()) {
                    it.next();

                    TextEditor::CompletionItem item(this);
                    item.text = it.key();
                    item.icon = symbolIcon;
                    m_completions.append(item);
                }
            } else if (value && completionOperator == QLatin1Char('(')) {
                if (const Interpreter::FunctionValue *f = value->asFunctionValue()) {
                    QString functionName = expression;
                    int indexOfDot = expression.lastIndexOf(QLatin1Char('.'));
                    if (indexOfDot != -1)
                        functionName = expression.mid(indexOfDot + 1);

                    // Recreate if necessary
                    if (!m_functionArgumentWidget)
                        m_functionArgumentWidget = new QmlJSEditor::Internal::FunctionArgumentWidget;

                    m_functionArgumentWidget->showFunctionHint(functionName.trimmed(), f->argumentCount(), m_startPosition);
                }

                return -1;
            }
        }

        if (! m_completions.isEmpty())
            return m_startPosition;

        return -1;
    }

    if (completionOperator.isNull()
            || completionOperator.isSpace()
            || completionOperator == QLatin1Char('{')
            || completionOperator == QLatin1Char('}')
            || completionOperator == QLatin1Char(':')
            || completionOperator == QLatin1Char(';')) {
        updateSnippets();
        m_completions.append(m_snippets);
    }

    if (! m_completions.isEmpty())
        return m_startPosition;

    return -1;
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
    if (xml.readNextStartElement()) {
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
                } else {
                    xml.skipCurrentElement();
                }
            }
        } else {
            xml.skipCurrentElement();
        }
    }
    if (xml.hasError())
        qWarning() << qmlsnippets << xml.errorString() << xml.lineNumber() << xml.columnNumber();
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
