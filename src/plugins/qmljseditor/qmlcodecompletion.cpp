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
#include "qmlexpressionundercursor.h"
#include "qmljseditor.h"
#include "qmlmodelmanagerinterface.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsscanner.h>
#include <qmljs/qmljsevaluate.h>

#include <texteditor/basetexteditor.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
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

// #define QML_WITH_SNIPPETS

namespace {

// Temporary workaround until we have proper icons for QML completion items
QIcon iconForColor(const QColor &color)
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

bool checkStartOfIdentifier(const QString &word)
{
    if (word.isEmpty())
        return false;

    const QChar ch = word.at(0);

    switch (ch.unicode()) {
    case '_': case '$':
        return true;

    default:
        return ch.isLetter();
    }
}

bool isIdentifierChar(QChar ch)
{
    switch (ch.unicode()) {
    case '_': case '$':
        return true;

    default:
        return ch.isLetterOrNumber();
    }
}

class SearchPropertyDefinitions: protected AST::Visitor
{
    QList<AST::UiPublicMember *> _properties;

public:
    QList<AST::UiPublicMember *> operator()(Document::Ptr doc)
    {
        _properties.clear();
        if (doc && doc->qmlProgram())
            doc->qmlProgram()->accept(this);
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

class EnumerateProperties: private Interpreter::MemberProcessor
{
    QSet<const Interpreter::ObjectValue *> _processed;
    QHash<QString, const Interpreter::Value *> _properties;
    bool _globalCompletion;
    Interpreter::Context *_context;
    const Interpreter::ObjectValue *_currentObject;

public:
    EnumerateProperties(Interpreter::Context *context)
        : _globalCompletion(false),
          _context(context),
          _currentObject(0)
    {
    }

    void setGlobalCompletion(bool globalCompletion)
    {
        _globalCompletion = globalCompletion;
    }

    QHash<QString, const Interpreter::Value *> operator ()(const Interpreter::Value *value)
    {
        _processed.clear();
        _properties.clear();
        _currentObject = Interpreter::value_cast<const Interpreter::ObjectValue *>(value);

        enumerateProperties(value);

        return _properties;
    }

    QHash<QString, const Interpreter::Value *> operator ()()
    {
        _processed.clear();
        _properties.clear();
        _currentObject = 0;

        foreach (const Interpreter::ObjectValue *scope, _context->scopeChain())
            enumerateProperties(scope);

        return _properties;
    }

private:
    void insertProperty(const QString &name, const Interpreter::Value *value)
    {
        if (_context->lookupMode() == Interpreter::Context::JSLookup ||
                ! dynamic_cast<const Interpreter::ASTVariableReference *>(value))
            _properties.insert(name, value);
    }

    virtual bool processProperty(const QString &name, const Interpreter::Value *value)
    {
        insertProperty(name, value);
        return true;
    }

    virtual bool processEnumerator(const QString &name, const Interpreter::Value *value)
    {
        if (! _globalCompletion)
            insertProperty(name, value);
        return true;
    }

    virtual bool processSignal(const QString &, const Interpreter::Value *)
    {
        return true;
    }

    virtual bool processSlot(const QString &name, const Interpreter::Value *value)
    {
        if (! _globalCompletion)
            insertProperty(name, value);
        return true;
    }

    virtual bool processGeneratedSlot(const QString &name, const Interpreter::Value *value)
    {
        if (_globalCompletion || (_currentObject && _currentObject->className().endsWith(QLatin1String("Keys")))) {
            // ### FIXME: add support for attached properties.
            insertProperty(name, value);
        }
        return true;
    }

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
        enumerateProperties(object->prototype(_context));

        object->processMembers(this);
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
    void showFunctionHint(const QString &functionName,
                          const QStringList &signature,
                          int startPosition);

protected:
    bool eventFilter(QObject *obj, QEvent *e);

private:
    void updateArgumentHighlight();
    void updateHintText();

    QString m_functionName;
    QStringList m_signature;
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

void FunctionArgumentWidget::showFunctionHint(const QString &functionName, const QStringList &signature, int startPosition)
{
    if (m_startpos == startPosition)
        return;

    m_functionName = functionName;
    m_signature = signature;
    m_minimumArgumentCount = signature.size();
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
    Scanner tokenize;
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
    prettyMethod += QLatin1Char('(');
    for (int i = 0; i < m_minimumArgumentCount; ++i) {
        if (i != 0)
            prettyMethod += QLatin1String(", ");

        QString arg = m_signature.at(i);
        if (arg.isEmpty()) {
            arg = QLatin1String("arg");
            arg += QString::number(i + 1);
        }

        prettyMethod += arg;
    }
    prettyMethod += QLatin1Char(')');

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

QmlCodeCompletion::QmlCodeCompletion(QmlModelManagerInterface *modelManager, QObject *parent)
    : TextEditor::ICompletionCollector(parent),
      m_modelManager(modelManager),
      m_editor(0),
      m_startPosition(0),
      m_caseSensitivity(Qt::CaseSensitive)
{
    Q_ASSERT(modelManager);
}

QmlCodeCompletion::~QmlCodeCompletion()
{ }

Qt::CaseSensitivity QmlCodeCompletion::caseSensitivity() const
{ return m_caseSensitivity; }

void QmlCodeCompletion::setCaseSensitivity(Qt::CaseSensitivity caseSensitivity)
{ m_caseSensitivity = caseSensitivity; }

TextEditor::ITextEditable *QmlCodeCompletion::editor() const
{ return m_editor; }

int QmlCodeCompletion::startPosition() const
{ return m_startPosition; }

bool QmlCodeCompletion::shouldRestartCompletion()
{ return false; }

bool QmlCodeCompletion::supportsEditor(TextEditor::ITextEditable *editor)
{
    if (qobject_cast<QmlJSTextEditor *>(editor->widget()))
        return true;

    return false;
}

bool QmlCodeCompletion::triggersCompletion(TextEditor::ITextEditable *editor)
{
    if (maybeTriggersCompletion(editor)) {
        // check the token under cursor

        if (QmlJSTextEditor *ed = qobject_cast<QmlJSTextEditor *>(editor->widget())) {

            QTextCursor tc = ed->textCursor();
            QTextBlock block = tc.block();
            const int column = tc.columnNumber();
            const int blockState = qMax(0, block.previous().userState()) & 0xff;
            const QString blockText = block.text();

            Scanner scanner;
            const QList<Token> tokens = scanner(blockText, blockState);
            foreach (const Token &tk, tokens) {
                if (column >= tk.begin() && column <= tk.end()) {
                    if (tk.is(Token::Comment) || tk.is(Token::String))
                        return false;
                    else
                        break;
                }
            }
        }
        return true;
    }

    return false;
}

bool QmlCodeCompletion::maybeTriggersCompletion(TextEditor::ITextEditable *editor)
{
    const int cursorPosition = editor->position();
    const QChar ch = editor->characterAt(cursorPosition - 1);

    if (ch == QLatin1Char('(') || ch == QLatin1Char('.'))
        return true;

    const QChar characterUnderCursor = editor->characterAt(cursorPosition);

    if (isIdentifierChar(ch) && (characterUnderCursor.isSpace() ||
                                      characterUnderCursor.isNull() ||
                                      isDelimiter(characterUnderCursor))) {
        int pos = editor->position() - 1;
        for (; pos != -1; --pos) {
            if (! isIdentifierChar(editor->characterAt(pos)))
                break;
        }
        ++pos;

        const QString word = editor->textAt(pos, cursorPosition - pos);
        if (word.length() > 2 && checkStartOfIdentifier(word)) {
            for (int i = 0; i < word.length(); ++i) {
                if (! isIdentifierChar(word.at(i)))
                    return false;
            }
            return true;
        }
    }

    return false;
}

bool QmlCodeCompletion::isDelimiter(const QChar &ch) const
{
    switch (ch.unicode()) {
    case '{':
    case '}':
    case '[':
    case ']':
    case ')':
    case '?':
    case '!':
    case ':':
    case ';':
    case ',':
        return true;

    default:
        return false;
    }
}

int QmlCodeCompletion::startCompletion(TextEditor::ITextEditable *editor)
{
    m_editor = editor;

    QmlJSTextEditor *edit = qobject_cast<QmlJSTextEditor *>(m_editor->widget());
    if (! edit)
        return -1;

    m_startPosition = editor->position();
    const QString fileName = editor->file()->fileName();

    while (editor->characterAt(m_startPosition - 1).isLetterOrNumber() ||
           editor->characterAt(m_startPosition - 1) == QLatin1Char('_'))
        --m_startPosition;

    m_completions.clear();

    const SemanticInfo semanticInfo = edit->semanticInfo();
    const QmlJS::Snapshot snapshot = semanticInfo.snapshot;
    const Document::Ptr document = semanticInfo.document;

    const QFileInfo currentFileInfo(fileName);

    bool isQmlFile = false;
    if (currentFileInfo.suffix() == QLatin1String("qml"))
        isQmlFile = true;

    const QIcon symbolIcon = iconForColor(Qt::darkCyan);

    Interpreter::Engine interp;
    Interpreter::Context context(&interp);

    // Set up the current scope chain.
    AST::Node *declaringMember = semanticInfo.declaringMember(editor->position());
    context.build(declaringMember, document, snapshot);

    // Search for the operator that triggered the completion.
    QChar completionOperator;
    if (m_startPosition > 0)
        completionOperator = editor->characterAt(m_startPosition - 1);

    if (completionOperator.isSpace() || completionOperator.isNull() || isDelimiter(completionOperator) ||
            (completionOperator == QLatin1Char('(') && m_startPosition != editor->position())) {
        // It's a global completion.
        EnumerateProperties enumerateProperties(&context);
        enumerateProperties.setGlobalCompletion(true);
        QHashIterator<QString, const Interpreter::Value *> it(enumerateProperties());
        while (it.hasNext()) {
            it.next();

            TextEditor::CompletionItem item(this);
            item.text = it.key();
            item.icon = symbolIcon;
            m_completions.append(item);
        }
    }

    else if (completionOperator == QLatin1Char('.') || completionOperator == QLatin1Char('(')) {
        // Look at the expression under cursor.
        QTextCursor tc = edit->textCursor();
        tc.setPosition(m_startPosition - 1);

        QmlExpressionUnderCursor expressionUnderCursor;
        QmlJS::AST::ExpressionNode *expression = expressionUnderCursor(tc);
        //qDebug() << "expression:" << expression;

        if (expression  != 0) {
            Evaluate evaluate(&context);

            // Evaluate the expression under cursor.
            const Interpreter::Value *value = interp.convertToObject(evaluate(expression));
            //qDebug() << "type:" << interp.typeId(value);

            if (value && completionOperator == QLatin1Char('.')) { // member completion
                EnumerateProperties enumerateProperties(&context);
                QHashIterator<QString, const Interpreter::Value *> it(enumerateProperties(value));
                while (it.hasNext()) {
                    it.next();

                    TextEditor::CompletionItem item(this);
                    item.text = it.key();
                    item.icon = symbolIcon;
                    m_completions.append(item);
                }
            } else if (value && completionOperator == QLatin1Char('(') && m_startPosition == editor->position()) {
                // function completion
                if (const Interpreter::FunctionValue *f = value->asFunctionValue()) {
                    QString functionName = expressionUnderCursor.text();
                    int indexOfDot = functionName.lastIndexOf(QLatin1Char('.'));
                    if (indexOfDot != -1)
                        functionName = functionName.mid(indexOfDot + 1);

                    // Recreate if necessary
                    if (!m_functionArgumentWidget)
                        m_functionArgumentWidget = new QmlJSEditor::Internal::FunctionArgumentWidget;

                    QStringList signature;
                    for (int i = 0; i < f->argumentCount(); ++i)
                        signature.append(f->argumentName(i));

                    m_functionArgumentWidget->showFunctionHint(functionName.trimmed(),
                                                               signature,
                                                               m_startPosition);
                }

                return -1; // We always return -1 when completing function prototypes.
            }
        }

        if (! m_completions.isEmpty())
            return m_startPosition;

        return -1;
    }

#ifdef QML_WITH_SNIPPETS
    if (isQmlFile && (completionOperator.isNull() || completionOperator.isSpace() || isDelimiter(completionOperator))) {
        updateSnippets();
        m_completions.append(m_snippets);
    }
#endif

    if (! m_completions.isEmpty())
        return m_startPosition;

    return -1;
}

void QmlCodeCompletion::completions(QList<TextEditor::CompletionItem> *completions)
{
    const int length = m_editor->position() - m_startPosition;

    if (length == 0)
        *completions = m_completions;
    else if (length > 0) {
        const QString key = m_editor->textAt(m_startPosition, length);

        filter(m_completions, completions, key, FirstLetterCaseSensitive);
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

    return TextEditor::ICompletionCollector::partiallyComplete(completionItems);
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

static bool qmlCompletionItemLessThan(const TextEditor::CompletionItem &l, const TextEditor::CompletionItem &r)
{
    if (l.text.at(0).isUpper() && r.text.at(0).isLower())
        return false;
    else if (l.text.at(0).isLower() && r.text.at(0).isUpper())
        return true;

    return l.text < r.text;
}

QList<TextEditor::CompletionItem> QmlCodeCompletion::getCompletions()
{
    QList<TextEditor::CompletionItem> completionItems;

    completions(&completionItems);

    qStableSort(completionItems.begin(), completionItems.end(), qmlCompletionItemLessThan);

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
